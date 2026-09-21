#!/usr/bin/env python3
"""Verify real loopback handshake acceptance and refusal between engine builds."""
import argparse
import os
from pathlib import Path
from run import SCRATCH
import socket
import subprocess
import tempfile

from run import ROOT, content_maps, content_settings
from window import wait_for

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--client', type=Path, required=True)
parser.add_argument('--server', type=Path, required=True)
parser.add_argument('--other-server', type=Path, required=True, help='built with AFTERSHOCK_NET_VERSION=3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-protocol-runtime'))
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
paks = sorted(args.data.resolve().glob('*.pk3'))
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if not paks or len(icds) != 1:
    parser.error('installed game content and one lavapipe ICD are required')
env = dict(os.environ, LP_NUM_THREADS='1', VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]))
for name, server_binary, accepted in [('same', args.server, True), ('different', args.other_server, False)]:
    with tempfile.TemporaryDirectory(prefix='aftershock-protocol-') as temporary:
        home = Path(temporary)
        base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
        base.mkdir()
        for pak in paks:
            (base / pak.name).symlink_to(pak)
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reservation:
            reservation.bind(('127.0.0.1', 0))
            port = reservation.getsockname()[1]
        (base / 'protocol.cfg').write_text(f'connect 127.0.0.1:{port}\nwait 600\nquit\n')
        common = ['+set', 'fs_basepath', str(home), *content_settings(args.content),
                  '+set', 'sv_pure', '0', '+set', 'net_enabled', '1', '+set', 'net_ip', '127.0.0.1']
        server_log = args.output / (name + '-server.log')
        client_log = args.output / (name + '-client.log')
        with server_log.open('wb') as stream:
            server = subprocess.Popen([str(server_binary.resolve()), *common,
                '+set', 'fs_homepath', str(home / 'server'), '+set', 'net_port', str(port),
                '+set', 'dedicated', '1', '+set', 'bot_enable', '0',
                '+map', content_maps(args.content)[0]], cwd=ROOT, env=env,
                stdin=subprocess.PIPE, stdout=stream, stderr=subprocess.STDOUT)
            try:
                wait_for(lambda: '-----------------------------------' in server_log.read_text(), server)
                with client_log.open('wb') as client_stream:
                    subprocess.run(['timeout', '45', 'xvfb-run', '-a', str(args.client.resolve()), *common,
                        '+set', 'fs_homepath', str(home / 'client'), '+set', 'net_port', '0',
                        '+set', 'r_mode', '3', '+set', 'r_fullscreen', '0', '+set', 's_initsound', '0',
                        '+set', 'cl_allowDownload', '0', '+set', 'cl_autoRecordDemo', '0',
                        '+set', 'com_maxfps', '100', '+exec', 'protocol.cfg'], cwd=ROOT, env=env,
                        stdout=client_stream, stderr=subprocess.STDOUT, check=True)
                text = client_log.read_text()
                negotiated = 'Aftershock protocol negotiated: version=2 schema=' in text
                refused = 'Incompatible Aftershock protocol/schema' in text
                assert negotiated == accepted and refused != accepted, (name, client_log)
                joined = 'ClientBegin: 0' in server_log.read_text()
                assert joined == accepted, (name, server_log)
            finally:
                if server.poll() is None:
                    server.stdin.write(b'quit\n')
                    server.stdin.flush()
                    try:
                        server.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        server.kill()
                        server.wait()
print('PASS: real client joins matching loopback server and refuses the other protocol build before joining')
