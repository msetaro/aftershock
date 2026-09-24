"""Timed authenticated HTTPS profile probe for the ingest acceptance workload."""
import http.client
import json
import io
import ssl
import time


class _ReceivedBytes(io.RawIOBase):
    """Count decrypted response bytes before HTTP buffering can hide a partial read."""
    def __init__(self, raw, response):
        self.raw, self.response = raw, response

    def readable(self):
        return True

    def readinto(self, buffer):
        count = self.raw.readinto(buffer)
        if count:
            self.response.received += count
        return count

    def close(self):
        try:
            self.raw.close()
        finally:
            super().close()


class _Response(http.client.HTTPResponse):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.received = 0
        self.fp = io.BufferedReader(_ReceivedBytes(self.fp.detach(), self))


class ProfileProbe:
    def __init__(self, host, port, trust, token, player, timeout=5):
        self.host, self.port, self.trust = host, port, trust
        self.token, self.player, self.timeout = token, player, timeout
        self.connection = http.client.HTTPSConnection(host, port, context=trust, timeout=timeout)
        self.connection.response_class = self.response
        self.used = False
        self.last_milliseconds = 0
        self.last_reconnects = 0

    def response(self, *args, **kwargs):
        self.current_response = _Response(*args, **kwargs)
        return self.current_response

    def sample(self):
        started = time.monotonic()
        self.last_reconnects = 0
        try:
            for attempt in range(2):
                remaining = self.timeout-(time.monotonic()-started)
                if remaining <= 0:
                    raise TimeoutError('profile deadline exceeded')
                reused = self.used and self.connection.sock is not None
                self.connection.timeout = remaining
                if self.connection.sock is not None:
                    self.connection.sock.settimeout(remaining)
                self.current_response = None
                try:
                    self.connection.request('GET', '/v1/profile', headers={'Authorization': 'Bearer '+self.token})
                    response = self.connection.getresponse()
                    break
                except (BrokenPipeError, ConnectionResetError, ssl.SSLEOFError, ssl.SSLZeroReturnError,
                        http.client.RemoteDisconnected):
                    # RFC 9110 9.2.2: retry this bodyless GET once only on an
                    # already-successful connection closed before a response.
                    # No status/header/body bytes may have arrived, even if the
                    # HTTP reader raised while buffering a partial status line.
                    received = self.current_response and self.current_response.received
                    if attempt or not reused or received:
                        raise
                    self.connection.close()
                    self.connection = http.client.HTTPSConnection(self.host, self.port, context=self.trust,
                                                                  timeout=remaining)
                    self.connection.response_class = self.response
                    self.used = False
                    self.last_reconnects = 1
            # Status, body, timeout and TLS verification/protocol errors never retry.
            body = response.read(65537)
            if response.status != 200 or len(body) > 65536 or response.length not in (None, 0):
                raise ValueError('invalid profile HTTP response')
            if json.loads(body)['player_id'] != self.player:
                raise ValueError('profile ownership mismatch')
            self.used = True
        finally:
            self.last_milliseconds = (time.monotonic()-started)*1000
        return self.last_milliseconds

    def close(self):
        self.connection.close()
