"""Timed authenticated HTTPS profile probe for the ingest acceptance workload."""
import http.client
import json
import time


class ProfileProbe:
    def __init__(self, host, port, trust, token, player, timeout=5):
        self.host, self.port, self.trust = host, port, trust
        self.token, self.player, self.timeout = token, player, timeout
        self.connection = http.client.HTTPSConnection(host, port, context=trust, timeout=timeout)
        self.used = False
        self.last_milliseconds = 0
        self.last_reconnects = 0

    def sample(self):
        started = time.monotonic()
        self.last_reconnects = 0
        try:
            self.connection.request('GET', '/v1/profile', headers={'Authorization': 'Bearer '+self.token})
            response = self.connection.getresponse()
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
