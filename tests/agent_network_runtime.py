#!/usr/bin/env python3
"""Explicit native server/client steps dispatch external UDP without advancing idle clocks."""
import argparse
import json
from pathlib import Path
import shutil
import socket
import sys
import time
from run import ROOT, SCRATCH, content_maps
sys.path.insert(0, str(ROOT))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--client', type=Path, required=True)
parser.add_argument('--server', type=Path, required=True)
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-agent-network')
args = parser.parse_args()
assert not args.output.exists(), 'choose a new output directory'
args.output.mkdir(parents=True)
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reservation:
    reservation.bind(('127.0.0.1', 0))
    port = reservation.getsockname()[1]
common = ['+set', 'net_enabled', '1', '+set', 'net_ip', '127.0.0.1', '+set', 'sv_pure', '1']
report = dict(ok=False, content=args.content, dt=20, udp_query=False, native_join=False, chat=False)


def advance(engine):
    before = engine.request('state')
    after = engine.step()
    assert after['frame'] == before['frame']+1 and after['time'] == before['time']+20, (before, after)
    return after


with Engine(args.server, args.data, args.content, headless=False,
            arguments=[*common, '+set', 'net_port', str(port), '+set', 'dedicated', '1',
                       '+set', 'bot_enable', '0', '+set', 'g_password', 'agent-network-owned']) as server:
    try:
        server.request('hello')
        server.request('session', dt=20, seed=123)
        server.request('exec', command='map '+content_maps(args.content)[0])
        advance(server)
        # This is an OS UDP socket, not the engine's internal loopback queue.
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
            probe.bind(('127.0.0.1', 0))
            probe.settimeout(.1)
            probe.sendto(b'\xff\xff\xff\xffgetinfo agent_step_probe', ('127.0.0.1', port))
            before = server.request('state')
            time.sleep(.1)
            assert server.request('state') == before, 'idle wall time advanced dedicated simulation'
            advance(server)
            try:
                packet, peer = probe.recvfrom(8192)
                report['udp_query'] = peer == ('127.0.0.1', port) and packet.startswith(b'\xff\xff\xff\xffinfoResponse\n') and b'agent_step_probe' in packet
            except socket.timeout:
                pass
        with Engine(args.client, args.data, args.content,
                    arguments=[*common, '+set', 'net_port', '0', '+set', 'password', 'agent-network-owned',
                               '+set', 'cl_allowDownload', '0']) as client:
            try:
                client.request('hello')
                client.request('session', dt=20, seed=456)
                client.request('exec', command=f'connect 127.0.0.1:{port}')
                before = client.request('state')
                time.sleep(.1)
                assert client.request('state') == before, 'idle wall time advanced client simulation'
                for _ in range(400):
                    advance(server)
                    advance(client)
                    if client.request('state')['player'] is not None:
                        report['native_join'] = True
                        break
                if report['native_join']:
                    client.request('exec', command='say agent_external_udp_ready')
                    for _ in range(20):
                        advance(server)
                        advance(client)
                    report['chat'] = 'agent_external_udp_ready' in server.log_path.read_text(errors='replace')
                report['server'] = server.request('state')
                report['client'] = client.request('state')
                report['network'] = client.request('profile')['network']
                report['ok'] = report['udp_query'] and report['native_join'] and report['chat'] and report['network']['incomingBytes'] > 0 and report['network']['snapshots'] > 0
            finally:
                shutil.copyfile(client.log_path, args.output/'client.log')
    finally:
        shutil.copyfile(server.log_path, args.output/'server.log')
        (args.output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
assert report['ok'], 'explicit steps failed external UDP query/join/snapshot/chat; see '+str(args.output/'report.json')
print('PASS: explicit server/client UDP dispatch, native snapshots/chat, fixed dt and idle clocks')
