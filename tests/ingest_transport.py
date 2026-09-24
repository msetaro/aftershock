#!/usr/bin/env python3
"""Real TLS fault checks for timed profile probes across idle connection retirement."""
import http.client
import http.server
import json
from pathlib import Path
import ssl
import subprocess
import tempfile
import threading
import time
from ingest_probe import ProfileProbe
from run import SCRATCH


class Server(http.server.ThreadingHTTPServer):
    daemon_threads = True

    def handle_error(self, request, address):
        pass  # Fault cases intentionally close sockets during requests.


class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def log_message(self, *args):
        pass

    def do_GET(self):
        with self.server.lock:
            self.server.requests += 1
            number = self.server.requests
        assert self.path == '/v1/profile' and self.headers.get('Authorization') == 'Bearer fixture'
        mode = self.server.mode
        if mode == 'fresh-close' or (mode == 'twice-close' and number >= 2):
            self.close_connection = True
            return
        if mode == 'retire' and number == 2:
            time.sleep(.06)
            self.close_connection = True
            return
        if mode == 'retire' and number == 3:
            time.sleep(.06)
        if mode == 'timeout' and number == 2:
            time.sleep(.2)
        body = json.dumps(dict(player_id='42')).encode()
        status = 503 if mode == 'status' and number == 2 else 200
        if mode == 'wrong-player' and number == 2:
            body = b'{"player_id":"43"}'
        self.send_response(status)
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        try:
            if mode == 'partial' and number == 2:
                self.wfile.write(body[:3])
                self.wfile.flush()
                self.close_connection = True
            else:
                self.wfile.write(body)
                self.wfile.flush()
            if mode == 'twice-close' and number == 1:
                self.close_connection = True
        except (BrokenPipeError, ConnectionResetError, ssl.SSLError):
            pass


with tempfile.TemporaryDirectory(prefix='ingest-transport-', dir=SCRATCH) as temporary:
    root = Path(temporary)
    cert, key = root/'cert.pem', root/'key.pem'
    subprocess.run(['openssl', 'req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-days', '1',
                    '-subj', '/CN=127.0.0.1', '-addext', 'subjectAltName=IP:127.0.0.1',
                    '-keyout', key, '-out', cert], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    key.chmod(0o600)
    trust = ssl.create_default_context(cafile=str(cert))
    server_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    server_context.load_cert_chain(cert, key)
    for mode in ('retire', 'fresh-close', 'twice-close', 'status', 'partial', 'wrong-player', 'timeout', 'untrusted'):
        server = Server(('127.0.0.1', 0), Handler)
        server.socket = server_context.wrap_socket(server.socket, server_side=True)
        server.mode, server.requests, server.lock = mode, 0, threading.Lock()
        thread = threading.Thread(target=server.serve_forever, kwargs=dict(poll_interval=.01), daemon=True)
        thread.start()
        probe = ProfileProbe('127.0.0.1', server.server_port,
                             ssl.create_default_context() if mode == 'untrusted' else trust,
                             'fixture', '42', timeout=.05 if mode == 'timeout' else 2)
        try:
            if mode not in ('fresh-close', 'untrusted'):
                probe.sample()
                assert probe.last_reconnects == 0
            if mode == 'retire':
                elapsed = probe.sample()
                assert probe.last_reconnects == 1 and server.requests == 3
                assert elapsed >= 110, 'timing discarded an attempt or reconnect'
                probe.sample()
                assert probe.last_reconnects == 0 and server.requests == 4
            else:
                try:
                    probe.sample()
                except (http.client.HTTPException, OSError, ValueError, json.JSONDecodeError):
                    pass
                else:
                    raise AssertionError('fault was accepted: '+mode)
                assert probe.last_reconnects == (1 if mode == 'twice-close' else 0), mode
                assert server.requests <= (2 if mode not in ('fresh-close', 'untrusted') else 1), mode
            print('PASS:', mode, flush=True)
        finally:
            probe.close()
            server.shutdown()
            server.server_close()
            thread.join()
