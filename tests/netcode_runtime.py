#!/usr/bin/env python3
"""Check real engine hit validation through a private delayed/lossy UDP loopback."""
import argparse
import bisect
import heapq
import os
from pathlib import Path
import random
import re
import selectors
import signal
import socket
import subprocess
import tempfile
import threading
import time

from cook import cook
from run import ROOT, content_maps, content_settings
from window import wait_for


class LoopbackDelay:
    def __init__(self, server_port):
        self.front = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.front.bind(('127.0.0.1', 0))
        self.back = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.back.bind(('127.0.0.1', 0))
        self.back.connect(('127.0.0.1', server_port))
        self.port = self.front.getsockname()[1]
        self.stop = threading.Event()
        self.count = self.dropped = 0
        self.error = None
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
                        if self.count % 20 == 0:  # 5% deterministic datagram loss
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
    parser.add_argument('--snapshot-budget', type=int, default=0)
    parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
    parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-netcode-runtime'))
    args = parser.parse_args()
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
        cook(ROOT / 'tests/assets/animation/rigs.json', base)
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reservation:
            reservation.bind(('127.0.0.1', 0))
            port = reservation.getsockname()[1]
        common = ['+set', 'fs_basepath', str(home), *content_settings(args.content),
                  '+set', 'net_enabled', '1', '+set', 'net_ip', '127.0.0.1', '+set', 'sv_pure', '0']
        client = proxy = None
        with server_log.open('wb') as server_stream, client_log.open('wb') as client_stream:
            server = subprocess.Popen([str(args.server.resolve()), *common,
                '+set', 'fs_homepath', str(home / 'server'), '+set', 'net_port', str(port),
                '+set', 'dedicated', '1', '+set', 'bot_enable', '0', '+set', 'sv_fps', '50',
                '+set', 'g_rewind', '1', '+set', 'g_rewindTrace', '1',
                '+set', 'sv_snapshotBudget', str(args.snapshot_budget),
                '+devmap', content_maps(args.content)[0]], cwd=ROOT, env=env,
                stdin=subprocess.PIPE, stdout=server_stream, stderr=subprocess.STDOUT)
            try:
                wait_for(lambda: '-----------------------------------' in server_log.read_text(), server)
                proxy = LoopbackDelay(port)
                (base / 'netcode.cfg').write_text('\n'.join([
                    f'connect 127.0.0.1:{proxy.port}', 'wait 400', 'cmd give all',
                    'wait 100', 'weapon 6', 'wait 100', 'say netcode_ready', 'wait 100', '+attack', 'wait 800', 'cmd give ammo',
                    'wait 800', '-attack', 'say netcode_done', 'wait 100', 'quit']) + '\n')
                client = subprocess.Popen(['xvfb-run', '-a', str(args.client.resolve()), *common,
                    '+set', 'fs_homepath', str(home / 'client'), '+set', 'net_port', '0',
                    '+set', 'r_mode', '3', '+set', 'r_fullscreen', '0', '+set', 's_initsound', '0',
                    '+set', 'cl_allowDownload', '0', '+set', 'cl_autoRecordDemo', '0',
                    '+set', 'cg_showmiss', '1', '+set', 'cl_shownet', '-1' if args.snapshot_budget else '0',
                    '+set', 'com_maxfps', '100', '+exec', 'netcode.cfg'],
                    cwd=ROOT, env=env, stdout=client_stream, stderr=subprocess.STDOUT, start_new_session=True)
                wait_for(lambda: 'ClientBegin: 0' in server_log.read_text(), client)
                wait_for(lambda: 'netcode_ready' in server_log.read_text(), client)
                server.stdin.write(b'rewind_target 0\n')
                server.stdin.flush()
                wait_for(lambda: 'Rewind target created:' in server_log.read_text() or 'server: rewind_target 0' in server_log.read_text(), server)
                assert 'Rewind target created:' in server_log.read_text(), 'missing developer rewind target'
                if args.snapshot_budget:
                    wait_for(lambda: 'netcode_done' in server_log.read_text(), client, seconds=45)
                    server.stdin.write(b'status\n')
                    server.stdin.flush()
                    wait_for(lambda: 'Replication client 0:' in server_log.read_text(), server)
                assert client.wait(timeout=45) == 0
            finally:
                if client is not None and client.poll() is None:
                    os.killpg(client.pid, signal.SIGTERM)
                    client.wait(timeout=5)
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
    assert 70 <= median_age <= 180, median_age
    reports = re.findall(r'Rewind report: age=(\d+) limit=(\d+) clamped=[01] hit=[01]', client_log.read_text())
    assert len(reports) >= 10 and all(int(age) <= int(limit) <= 1000 for age, limit in reports), reports
    errors = [float(value) for value in re.findall(r'Prediction miss: ([0-9.]+)', client_log.read_text())]
    assert max(errors, default=0) <= 32, errors
    assert 'ERROR:' not in text and 'ERROR:' not in client_log.read_text()
    print(f'PASS: {matches}/{checked} network shots agree ({positive} hits); {uncompensated_wrong} wrong without rewind; '
          f'100 ms RTT, 5% loss, median view age {median_age} ms, prediction error <= {max(errors, default=0):.3f} units')


if __name__ == '__main__':
    main()
