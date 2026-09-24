"""Real-kind backend endpoint withdrawal and fresh TLS availability during retirement."""
import http.client
import json
import socket
import time


def check_retirement(c):
    pods = c['document']([*c['ctl'], 'get', 'pods', '-l', 'app=backend', '-o', 'json'])['items']
    victim = next(p for p in pods if not p['metadata'].get('deletionTimestamp') and
                  any(s.get('ready') for s in p['status'].get('containerStatuses', [])))
    name, uid = victim['metadata']['name'], victim['metadata']['uid']
    with socket.socket() as reservation:
        reservation.bind(('127.0.0.1', 0))
        port = reservation.getsockname()[1]
    forward = c['follow']('backend-retirement-forward.log', [*c['ctl'], 'port-forward', 'pod/'+name, f'{port}:8443'])
    report = dict(endpoint_withdrawn=False, fresh_tls_samples=0, terminated=False)

    def fresh_health():
        connection = http.client.HTTPSConnection('127.0.0.1', port, context=c['trust'], timeout=2)
        try:
            connection.request('GET', '/healthz')
            response = connection.getresponse()
            assert response.status == 200 and json.loads(response.read())['ready'] is True
        finally:
            connection.close()

    try:
        c['wait_for'](lambda: 'Forwarding from' in (c['output']/'backend-retirement-forward.log').read_text(),
                      20, 'direct backend TLS forward')
        fresh_health()
        started = time.monotonic()
        c['log_run']('backend-retirement-delete.log', [*c['ctl'], 'delete', 'pod', name, '--wait=false'])

        def withdrawn():
            slices = c['document']([*c['ctl'], 'get', 'endpointslices', '-l',
                                   'kubernetes.io/service-name=backend', '-o', 'json'])['items']
            endpoints = [e for s in slices for e in s.get('endpoints', []) if e.get('targetRef', {}).get('uid') == uid]
            return not endpoints or all(e.get('conditions', {}).get('ready') is False for e in endpoints)

        c['wait_for'](withdrawn, 5, 'retiring backend endpoint withdrawn')
        report['endpoint_withdrawn'] = True
        # Requests deliberately target the withdrawn pod, modeling stale routing.
        # Every call creates a new verified TLS connection, without any retry.
        while time.monotonic()-started < 5:
            fresh_health()
            report['fresh_tls_samples'] += 1
            time.sleep(.1)
        report['observed_seconds'] = time.monotonic()-started
        assert report['fresh_tls_samples'] >= 10, 'insufficient fresh TLS retirement samples'

        def gone():
            return all(p['metadata']['uid'] != uid for p in
                       c['document']([*c['ctl'], 'get', 'pods', '-l', 'app=backend', '-o', 'json'])['items'])

        c['wait_for'](gone, 35, 'retiring backend bounded exit')
        report['terminated'], report['termination_seconds'] = True, time.monotonic()-started
        assert report['termination_seconds'] < 40
        c['log_run']('backend-retirement-replacement.log', [*c['ctl'], 'rollout', 'status',
            'deployment/backend', '--timeout=60s'], timeout=70)
        print('PASS: withdrawn backend endpoint serves fresh TLS before bounded termination', flush=True)
        return report
    except Exception as error:
        report['error'] = type(error).__name__
        raise
    finally:
        (c['output']/'backend-retirement.json').write_text(json.dumps(report, indent=2)+'\n')
        if forward.poll() is None:
            forward.terminate()
            forward.wait(timeout=5)
