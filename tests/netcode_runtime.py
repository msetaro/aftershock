#!/usr/bin/env python3
"""Check real engine hit validation through a private delayed/lossy UDP loopback."""
import argparse
import bisect
import heapq
import json
import os
from pathlib import Path
from run import SCRATCH
import random
import re
import selectors
import signal
import socket
import subprocess
import sys
import tempfile
import threading
import time

from cook import cook
from run import ROOT, content_maps, content_settings
from window import XInput, wait_for


class LoopbackDelay:
    def __init__(self, server_port, loss=True):
        self.front = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.front.bind(('127.0.0.1', 0))
        self.back = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.back.bind(('127.0.0.1', 0))
        self.back.connect(('127.0.0.1', server_port))
        self.port = self.front.getsockname()[1]
        self.stop = threading.Event()
        self.count = self.dropped = 0
        self.error = None
        self.loss = loss
        self.thread = threading.Thread(target=self.run)
        self.thread.start()

    def run(self):
        client, sequence, pending = None, 0, []
        randomizer = random.Random(12)
        try:
            with selectors.DefaultSelector() as selector:
                for direction, endpoint in enumerate((self.front, self.back)):
                    endpoint.setblocking(False)
                    selector.register(endpoint, selectors.EVENT_READ, direction)
                while not self.stop.is_set():
                    for key, _ in selector.select(0.002):
                        packet, address = key.fileobj.recvfrom(65535)
                        if key.data == 0:
                            if client is not None and address != client:
                                continue
                            client = address
                        self.count += 1
                        if self.loss and self.count % 20 == 0:  # 5% deterministic datagram loss
                            self.dropped += 1
                            continue
                        assert len(pending) < 4096
                        sequence += 1
                        # 50 ms each way: 100 ms RTT, +/-15 ms combined jitter.
                        deadline = time.monotonic() + 0.05 + randomizer.uniform(-0.0075, 0.0075)
                        heapq.heappush(pending, (deadline, sequence, key.data, packet))
                    while pending and pending[0][0] <= time.monotonic():
                        _, _, direction, packet = heapq.heappop(pending)
                        if direction == 0:
                            self.back.send(packet)
                        elif client is not None:
                            self.front.sendto(packet, client)
        except Exception as error:
            self.error = error

    def close(self):
        self.stop.set()
        self.thread.join(timeout=2)
        self.front.close()
        self.back.close()
        assert not self.thread.is_alive()
        if self.error:
            raise self.error


def stop_client(client, grace=5):
    if client is None or client.poll() is not None:
        return
    try:
        os.killpg(client.pid, signal.SIGTERM)
        try:
            client.wait(timeout=grace)
        except subprocess.TimeoutExpired:
            os.killpg(client.pid, signal.SIGKILL)
            client.wait(timeout=5)
    except ProcessLookupError:
        client.wait(timeout=5)


def intersects(start, end, minimum, maximum):
    enter, leave = 0.0, 1.0
    for a, b, low, high in zip(start, end, minimum, maximum):
        if a == b:
            if not low <= a <= high:
                return False
        else:
            near, far = sorted(((low - a) / (b - a), (high - a) / (b - a)))
            enter, leave = max(enter, near), min(leave, far)
    return enter <= leave


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', type=Path, required=True)
    parser.add_argument('--server', type=Path, required=True, help='AFTERSHOCK_DEVTOOLS build')
    parser.add_argument('--weapons', action='store_true', help='exercise the data-driven weapon and grenade path')
    parser.add_argument('--inside-xvfb', action='store_true', help=argparse.SUPPRESS)
    parser.add_argument('--client-fps', type=int, default=100, help='cap rendered frames; 20 exercises slow CI rendering')
    parser.add_argument('--snapshot-budget', type=int, default=0)
    parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
    parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
    parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-netcode-runtime'))
    args = parser.parse_args()
    if not 20 <= args.client_fps <= 200:
        parser.error('client FPS must be in 20..200')
    scenario_timeout = 120 if args.weapons else 45
    if args.weapons and not args.inside_xvfb:
        subprocess.run(['timeout', '240', 'xvfb-run', '-a', sys.executable, str(Path(__file__).resolve()),
                        *sys.argv[1:], '--inside-xvfb'], cwd=ROOT, check=True)
        return
    if not 0 <= args.snapshot_budget <= 16384:
        parser.error('snapshot budget must be in 0..16384')
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    paks = sorted(args.data.resolve().glob('*.pk3'))
    icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
    if not paks or len(icds) != 1:
        parser.error('installed content and one lavapipe ICD are required')
    env = dict(os.environ, LP_NUM_THREADS='1', VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]))
    server_log, client_log = args.output / 'server.log', args.output / 'client.log'
    with tempfile.TemporaryDirectory(prefix='aftershock-netcode-') as temporary:
        home = Path(temporary)
        base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
        base.mkdir()
        for pak in paks:
            (base / pak.name).symlink_to(pak)
        weapon_options = []
        if args.weapons:
            cook(ROOT / 'tests/assets/range.json', base)
            source = home / 'source'
            source.mkdir()
            rifle = json.loads((ROOT / 'tests/assets/weapons/rifle.weapon.json').read_text())
            rifle.update(name='range_network', interval_ms=20, magazine=300, reserve=0, spread_degrees=0,
                         damage=1, minimum_damage=1)
            grenade = dict(rifle, name='range_network_grenade', fire_mode='semi', ballistics='projectile',
                           damage=0, minimum_damage=0, magazine=4,
                           projectile=dict(rifle['projectile'], speed=80, gravity=100, fuse_ms=2000, radius=0))
            for name, definition in [('network', rifle), ('grenade', grenade)]:
                (source / (name + '.json')).write_text(json.dumps(definition))
            recipe = source / 'assets.json'
            recipe.write_text(json.dumps({'version': 1, 'assets': [
                {'name': 'weapons/' + name, 'kind': 'weapon', 'source': name + '.json'} for name in ('network', 'grenade')]}))
            cook(recipe, base)
            weapon_options = ['+set', 'g_weapons', 'weapons/network.asweapon weapons/grenade.asweapon', '+set', 'g_weaponTrace', '1']
        else:
            cook(ROOT / 'tests/assets/animation/rigs.json', base)
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reservation:
            reservation.bind(('127.0.0.1', 0))
            port = reservation.getsockname()[1]
        common = ['+set', 'fs_basepath', str(home), *content_settings(args.content),
                  '+set', 'net_enabled', '1', '+set', 'net_ip', '127.0.0.1', '+set', 'sv_pure', '0']
        client = proxy = device = None
        with server_log.open('wb') as server_stream, client_log.open('wb') as client_stream:
            server = subprocess.Popen([str(args.server.resolve()), *common,
                '+set', 'fs_homepath', str(home / 'server'), '+set', 'net_port', str(port),
                '+set', 'dedicated', '1', '+set', 'bot_enable', '0', '+set', 'sv_fps', '50',
                '+set', 'g_rewind', '1', '+set', 'g_rewindTrace', '1',
                '+set', 'sv_snapshotBudget', str(args.snapshot_budget), *weapon_options,
                '+devmap', content_maps(args.content)[0]], cwd=ROOT, env=env,
                stdin=subprocess.PIPE, stdout=server_stream, stderr=subprocess.STDOUT)
            try:
                wait_for(lambda: '-----------------------------------' in server_log.read_text(), server)
                # Large initial gamestate must complete before measuring lossy gameplay.
                proxy = LoopbackDelay(port, loss=not args.weapons)
                commands = [
                    f'connect 127.0.0.1:{proxy.port}', 'wait 400', 'cmd give all',
                    'wait 100', 'weapon 6', 'wait 100', 'say netcode_ready', 'wait 100', '+attack', 'wait 800', 'cmd give ammo',
                    'wait 800', '-attack', 'say netcode_done', 'wait 100', 'quit']
                if args.weapons:
                    weapon_commands = ['weapon 1', 'wait 100', 'say netcode_ready',
                        'wait 100', '+attack', 'wait 350', '+button12', 'wait 100', '-attack', '-button12',
                        'weapon 2', 'wait 60', '+attack', 'wait 10', '-attack', 'wait 200',
                        'weapon 1', 'wait 60', '+attack', 'wait 100', '-attack', 'wait 100',
                        'say netcode_done', 'wait 100', 'quit']
                    (base / 'netcode-ready.cfg').write_text('\n'.join(weapon_commands) + '\n')
                    commands = ['bind F10 \"exec netcode-ready.cfg\"', f'connect 127.0.0.1:{proxy.port}']
                (base / 'netcode.cfg').write_text('\n'.join(commands) + '\n')
                display = [] if args.weapons else ['xvfb-run', '-a']
                client = subprocess.Popen([*display, str(args.client.resolve()), *common,
                    '+set', 'fs_homepath', str(home / 'client'), '+set', 'net_port', '0',
                    '+set', 'r_mode', '3', '+set', 'r_fullscreen', '0', '+set', 's_initsound', '0',
                    '+set', 'cl_allowDownload', '0', '+set', 'cl_autoRecordDemo', '0',
                    '+set', 'cg_weaponTrace', '1' if args.weapons else '0',
                    '+set', 'cg_showmiss', '1', '+set', 'cl_shownet', '-1' if args.snapshot_budget else '0',
                    '+set', 'com_maxfps', str(args.client_fps), '+exec', 'netcode.cfg'],
                    cwd=ROOT, env=env, stdout=client_stream, stderr=subprocess.STDOUT, start_new_session=True)
                wait_for(lambda: 'ClientBegin: 0' in server_log.read_text(), client)
                if args.weapons:
                    wait_for(lambda: 'CL_InitCGame:' in client_log.read_text() and
                             'Weapon server state: owner=0 hand=0' in server_log.read_text(), client)
                    proxy.loss = True
                    device = XInput()
                    device.verify_window(client)
                    device.key('F10')
                wait_for(lambda: 'netcode_ready' in server_log.read_text(), client)
                scenario_started = time.monotonic()
                server.stdin.write(b'rewind_target 0\n')
                server.stdin.flush()
                wait_for(lambda: 'Rewind target created:' in server_log.read_text() or 'server: rewind_target 0' in server_log.read_text(), server)
                assert 'Rewind target created:' in server_log.read_text(), 'missing developer rewind target'
                if args.snapshot_budget:
                    wait_for(lambda: 'netcode_done' in server_log.read_text(), client, seconds=scenario_timeout)
                    scenario_seconds = time.monotonic() - scenario_started
                    if args.weapons and args.client_fps == 20:
                        assert scenario_seconds > 45, 'slow-render control did not exercise the old deadline'
                    print(f'Loopback scenario completed in {scenario_seconds:.1f}s at <= {args.client_fps} client FPS')
                    server.stdin.write(b'status\n')
                    server.stdin.flush()
                    wait_for(lambda: 'Replication client 0:' in server_log.read_text(), server)
                assert client.wait(timeout=scenario_timeout) == 0
            finally:
                stop_client(client)
                if device is not None:
                    device.close()
                if proxy is not None:
                    proxy.close()
                if server.poll() is None:
                    server.stdin.write(b'quit\n')
                    server.stdin.flush()
                    try:
                        server.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        server.kill()
                        server.wait()
    text = server_log.read_text()
    if args.snapshot_budget:
        policy = re.search(r'Replication client 0: budget=(\d+) deferred=\d+ total_deferred=(\d+)', text)
        assert policy and int(policy[1]) == args.snapshot_budget and int(policy[2]) > 0, policy
    target = int(re.search(r'Rewind target created: entity=(\d+)', text)[1])
    if args.snapshot_budget:
        assert re.search(r'#' + str(target) + r'\s+.*pos\.trBase', client_log.read_text()), 'budget never delivered target state'
    rows = re.findall(r'Rewind target boxes: time=(\d+) entity=\d+ ([-0-9. ]+)\n', text)
    frames = {int(t): list(map(float, values.split())) for t, values in rows}
    times = sorted(frames)
    assert len(times) > 100 and proxy.count > 100 and proxy.dropped > 0

    def sample(time):
        index = bisect.bisect_right(times, time) - 1
        if index < 0:
            return None
        before = times[index]
        after = times[min(index + 1, len(times) - 1)]
        fraction = (time - before) / (after - before) if after != before else 0
        return [a + fraction * (b - a) for a, b in zip(frames[before], frames[after])]

    rows = re.findall(r'Rewind trace: shooter=0 view=(\d+) age=(\d+) clamped=\d+ hit=(\d+) fraction=[0-9.]+ ray=([-0-9. ]+)\n', text)
    checked = matches = positive = uncompensated_wrong = 0
    ages = []
    for timestamp, age, actual, ray in rows:
        bounds = sample(int(timestamp))
        if bounds is None:
            continue
        ray = list(map(float, ray.split()))
        start, end = ray[:3], ray[3:]
        low, high = bounds[:3], bounds[3:]
        expected = intersects(start, end, [x + 0.25 for x in low], [x - 0.25 for x in high])
        # Exclude only sub-unit boundary ambiguity from printed trace coordinates.
        if expected != intersects(start, end, [x - 0.25 for x in low], [x + 0.25 for x in high]):
            continue
        checked += 1
        positive += expected
        matches += expected == (int(actual) == target)
        ages.append(int(age))
        current = sample(int(timestamp) + int(age))
        uncompensated_wrong += expected != intersects(start, end, current[:3], current[3:])
    assert checked >= 100 and positive >= 5 and matches / checked >= 0.99, (checked, matches, positive)
    assert uncompensated_wrong >= 5, uncompensated_wrong
    median_age = sorted(ages)[len(ages) // 2]
    # A deliberately slower client adds one input/render interval. Keep the
    # original 100-FPS bound and never exceed the configured 200-ms rewind window.
    frame_allowance = max(0, (1000 + args.client_fps - 1) // args.client_fps - 10)
    assert 70 <= median_age <= min(200, 180 + frame_allowance), median_age
    reports = re.findall(r'Rewind report: age=(\d+) limit=(\d+) clamped=[01] hit=[01]', client_log.read_text())
    assert len(reports) >= 10 and all(int(age) <= int(limit) <= 1000 for age, limit in reports), reports
    errors = [float(value) for value in re.findall(r'Prediction miss: ([0-9.]+)', client_log.read_text())]
    assert max(errors, default=0) <= 32, errors
    if args.weapons:
        received = client_log.read_text()
        state_pattern = r'Weapon %s state: owner=0 hand=0 tick=(\d+) sequence=(\d+) magazine=(\d+) reserve=(\d+) chamber=(\d+) ads=(\d+)'
        authoritative = {row[0]: row[1:] for row in re.findall(state_pattern % 'server', text)}
        states = {row[0]: row[1:] for row in re.findall(state_pattern % 'client', received)}
        assert len(states) >= 100 and all(authoritative.get(tick) == state for tick, state in states.items()), 'network weapon snapshot differs'
        for label in ('Weapon prediction', 'Weapon animation prediction'):
            comparisons = re.findall(label + r': hand=0 tick=\d+ equal=(\d)', received)
            assert len(comparisons) >= 50 and comparisons.count('1') / len(comparisons) >= 0.95, (label, len(comparisons), comparisons.count('0'))
            assert set(comparisons[-20:]) == {'1'}, 'prediction did not settle: ' + label
            print(f'PASS: {label}: {comparisons.count("1")}/{len(comparisons)} under delayed/lossy loopback')
        assert 'Weapon projectile server: owner=0 hand=0 sequence=1 ' in text
        assert 'Weapon projectile client: owner=0 hand=0 sequence=1 ' in received
        assert received.count('Weapon projectile predicted: hand=0 sequence=1 ') == 1
        corrections = re.findall(r'Weapon projectile correction: hand=0 sequence=1 error=([0-9.]+)', received)
        assert corrections and max(map(float, corrections)) <= 1, corrections
        assert 'Weapon projectile exploded: owner=0 sequence=1 ' in text
        assert 'Weapon rejected' not in text and 'Weapon rejected' not in received
    assert 'ERROR:' not in text and 'ERROR:' not in client_log.read_text()
    print(f'PASS: {matches}/{checked} network shots agree ({positive} hits); {uncompensated_wrong} wrong without rewind; '
          f'100 ms RTT, 5% loss, median view age {median_age} ms, prediction error <= {max(errors, default=0):.3f} units')


if __name__ == '__main__':
    main()
