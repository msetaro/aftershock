#!/usr/bin/env python3
"""Send owned synthesized voice between two real clients through a dedicated server."""
import argparse
import os
from pathlib import Path
import re
import signal
import socket
import subprocess
import tempfile
import time
from run import ROOT, SCRATCH, content_maps, content_settings

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--client', type=Path, required=True, help='AFTERSHOCK_DEVTOOLS client')
parser.add_argument('--server', type=Path, required=True)
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--output', type=Path, default=SCRATCH / 'aftershock-voice-runtime')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
paks = sorted(args.data.resolve().glob('*.pk3'))
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
assert paks and len(icds) == 1
processes, logs = [], []


def wait_for_log(path, token, process, seconds=30):
    deadline = time.monotonic() + seconds
    while token not in path.read_text(errors='replace'):
        assert process.poll() is None and time.monotonic() < deadline, f'missing {token}: {path}'
        time.sleep(.1)


with tempfile.TemporaryDirectory(prefix='aftershock-voice-') as temporary:
    home = Path(temporary)
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    base.mkdir()
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reservation:
        reservation.bind(('127.0.0.1', 0))
        port = reservation.getsockname()[1]
    common = ['+set', 'fs_basepath', str(home), *content_settings(args.content),
              '+set', 'net_enabled', '1', '+set', 'net_ip', '127.0.0.1']
    env = dict(os.environ, LC_ALL='C', LP_NUM_THREADS='1', SDL_AUDIODRIVER='dummy',
               VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]))
    server_log = args.output / 'server.log'
    try:
        logs.append(server_log.open('w'))
        server = subprocess.Popen([str(args.server.resolve()), *common, '+set', 'fs_homepath', str(home/'server'),
            '+set', 'net_port', str(port), '+set', 'dedicated', '1', '+set', 'sv_pure', '0',
            '+set', 'sv_voip', '1', '+set', 'bot_enable', '0', '+devmap', content_maps(args.content)[0]],
            cwd=ROOT, env=env, stdout=logs[-1], stderr=subprocess.STDOUT, start_new_session=True)
        processes.append(server)
        wait_for_log(server_log, 'Static game loaded.', server)
        clients = []
        for role in ['receiver', 'sender']:
            commands = [f'connect 127.0.0.1:{port}']
            commands += ['wait 100', 'voip_test 100', 'wait 200'] if role == 'sender' else ['wait 600']
            commands += ['s_voiceInfo', 's_audioInfo', 'quit']
            (base / f'{role}.cfg').write_text('\n'.join(commands) + '\n')
            logs.append((args.output / f'{role}.log').open('w'))
            client = subprocess.Popen(['xvfb-run', '-a', str(args.client.resolve()), *common,
                '+set', 'fs_homepath', str(home/role), '+set', 'net_port', '0', '+set', 'name', role,
                '+set', 'r_mode', '3', '+set', 'r_fullscreen', '0', '+set', 's_initsound', '1',
                '+set', 'cl_voip', '1', '+set', 'cl_voipSend', '0', '+set', 'cl_autoRecordDemo', '0',
                '+set', 'com_maxfps', '20', '+set', 'com_maxfpsUnfocused', '20', '+exec', f'{role}.cfg'],
                cwd=ROOT, env=env, stdout=logs[-1], stderr=subprocess.STDOUT, start_new_session=True)
            processes.append(client)
            clients.append(client)
            wait_for_log(server_log, f'ClientBegin: {len(clients)-1}', client)
        for client in reversed(clients):
            assert client.wait(timeout=90) == 0
    finally:
        for process in reversed(processes):
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait(timeout=5)
        for log in logs:
            log.close()
receiver = (args.output / 'receiver.log').read_text(errors='replace')
sender = (args.output / 'sender.log').read_text(errors='replace')
pattern = r'Audio voice: encoded=(\d+) decoded=(\d+) rejected=(\d+) concealed=(\d+) overruns=(\d+) queued=(\d+)'
sent = re.findall(pattern, sender)
heard = re.findall(pattern, receiver)
assert len(sent) == len(heard) == 1, (sent, heard)
assert int(sent[0][0]) == 100 and int(sent[0][1]) == 0, sent
assert 95 <= int(heard[0][1]) <= 100 and int(heard[0][5]) == 0, heard
peak = re.search(r'Audio events:.* peak=([\d.]+)', receiver)
assert peak and float(peak[1]) > 1000, 'received speech must reach the real mixer'
assert 'capture device opened' not in receiver + sender, 'synthetic test must not open a microphone'
print('PASS: Opus voice crosses real client/server UDP, reaches the voice bus and drains without microphone capture')
