#!/usr/bin/env python3
"""Exercise backend ticket admission through our dedicated server's real UDP handshake."""
import argparse
import hashlib
import hmac
import json
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time
from run import ROOT, SCRATCH, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--server', type=Path, required=True)
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-backend-join')
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
packet = args.output/'packet'
run(['g++', '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     'tests/probes/join_packet.cpp', 'engine/qcommon/huffman.cpp', '-o', packet])
key = bytes([0x42])*32
issued = int(time.time())
def ticket(player='18446744073709551615', match='match-1', start=issued, nonce='ab'*16):
    payload = f'1.{player}.{match}.{start}.{start+120}.{nonce}'
    return payload+'.'+hmac.new(key, ('aftershock/join/v1\n'+payload).encode(), hashlib.sha256).hexdigest()

with tempfile.TemporaryDirectory(prefix='aftershock-backend-join-', dir=SCRATCH) as temporary:
    root = Path(temporary)
    subprocess.run([sys.executable, str(ROOT/'tools/match/content.py'), str(root/'content')], check=True)
    home = root/'home'
    home.mkdir()
    config = home/'match-join.json'
    config.write_text('{}')
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reservation:
        reservation.bind(('127.0.0.1', 0))
        address = ('127.0.0.1', reservation.getsockname()[1])
    log = args.output/'server.log'
    with log.open('w') as out:
        server = subprocess.Popen([str(args.server.resolve()), '+set', 'fs_basepath', str(root/'content'),
            '+set', 'fs_homepath', str(home), '+set', 'fs_basegame', 'aftershock',
            '+set', 'dedicated', '1', '+set', 'net_enabled', '1', '+set', 'net_ip', address[0],
            '+set', 'net_port', str(address[1]), '+set', 'bot_enable', '0', '+set', 'sv_pure', '0',
            '+set', 'sv_reconnectlimit', '0', '+set', 'sv_zombietime', '0', '+set', 'sv_maxclients', '4',
            '+joinconfig', '+map', 'two_lane'], stdin=subprocess.PIPE, stdout=out, stderr=subprocess.STDOUT,
            text=True, cwd=ROOT)
        def wait_log(marker):
            deadline = time.monotonic()+20
            while marker not in log.read_text(errors='replace'):
                assert server.poll() is None and time.monotonic() < deadline, f'missing {marker}; see {log}'
                time.sleep(.05)
        def command(text):
            server.stdin.write(text+'\n')
            server.stdin.flush()
        def response(sock, prefixes):
            deadline = time.monotonic()+5
            while time.monotonic() < deadline:
                sock.settimeout(max(.01, deadline-time.monotonic()))
                data, peer = sock.recvfrom(4096)
                if peer == address and data.startswith(b'\xff'*4):
                    text = data[4:].decode()
                    if text.startswith(prefixes):
                        return text
            raise TimeoutError('no matching handshake response')
        def connect(sock, token='', qport=1234, handshake=None, delay=2.1):
            time.sleep(delay)  # The shared leaky bucket drains one query per second; a connect uses two.
            if handshake is None:
                sock.sendto(b'\xff'*4+b'getchallenge 123', address)
                handshake = response(sock, ('challengeResponse ',)).split()
                assert handshake[0] == 'challengeResponse' and handshake[4] == 'aftershock'
            info = dict(challenge=handshake[1], protocol=handshake[3], as_protocol=handshake[5],
                        as_schema=handshake[6], qport=str(qport), client='aftershock', name='backend-test')
            if token:
                info['as_ticket'] = token
            text = 'connect "'+''.join('\\'+k+'\\'+v for k,v in info.items())+'"'
            encoded = subprocess.check_output([packet], input=b'\xff'*4+text.encode())
            sock.sendto(encoded, address)
            return response(sock, ('connectResponse ', 'print\n')), handshake
        try:
            wait_log('Static game loaded.')
            wait_log('Join configuration rejected;')
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as first, socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as other:
                first.settimeout(5)
                other.settimeout(5)
                first.bind(('127.0.0.1', 0)); other.bind(('127.0.0.1', 0))
                reply, _ = connect(first)
                assert 'Join ticket rejected' in reply, 'invalid initial config admitted an anonymous connection: '+reply
                config.write_text(json.dumps(dict(version=1, match_id='match-1', join_key=key.hex(),
                                                  expected_players=['18446744073709551615'])))
                os.chmod(config, 0o600)
                command('joinconfig')
                wait_log('Join configuration ready: match-1')
                for token in ('', ticket(player='123'), ticket(match='other'), ticket(start=issued-121), ticket()[:-1]+'!'):
                    reply, _ = connect(first, token)
                    assert 'Join ticket rejected' in reply, reply
                valid = ticket()
                reply, handshake = connect(first, valid)
                assert reply.startswith('connectResponse '), reply
                # Discard the first response, then retransmit the exact handshake.
                retry, _ = connect(first, valid, handshake=handshake, delay=.15)
                assert retry == reply, retry
                # The same token from a different endpoint cannot replace the owner.
                denied, _ = connect(other, valid, delay=.15)
                assert 'Join ticket rejected' in denied, denied
                retry, _ = connect(first, valid, handshake=handshake, delay=.15)
                assert retry == reply, 'rejected alternate peer disturbed the admitted session'
                command('dumpuser 0')
                command('kickall')
                wait_log('was kicked')
                command('joinconfig')
                time.sleep(.25)
                denied, _ = connect(first, valid)
                assert 'Join ticket rejected' in denied, 'disconnect/config reload reopened a used ticket'
                fresh, _ = connect(first, ticket(nonce='ac'*16))
                assert fresh.startswith('connectResponse '), fresh
                output = log.read_text(errors='replace')
                assert valid not in output and key.hex() not in output and 'as_ticket' not in output
        finally:
            if server.poll() is None:
                command('quit')
                try:
                    server.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    server.kill(); server.wait(timeout=5)
print('PASS: actual UDP admission, expected players, lost-response retry, endpoint binding and replay rejection')
