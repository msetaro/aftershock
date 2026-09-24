"""Production ingest scenarios used by the owned backend kind acceptance driver."""
import base64
import concurrent.futures
import copy
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import sys
import threading
import time
import urllib.error
import urllib.parse
import urllib.request
from ingest_probe import ProfileProbe


def retirement_held(pods):
    producers = [pod for pod in pods if pod['name'].startswith('ingest-burst-')]
    return len(producers) == 100 and all(
        {row['name'] for row in pod['containers']} == {'server', 'results'} and
        all('running' in row['state'] and row['restartCount'] == 0
            for row in pod['containers']) for pod in producers)


class IngestAcceptance:
    """Shares the driver's owned cluster, cleanup scope and native client only."""
    def __init__(self, context):
        self.c = context
        self.native = None
        self.evidence = {}

    def sql(self, query):
        c = self.c
        return c['command']([*c['ctl'], 'exec', 'deployment/ingest-db', '--', 'psql', '-U', 'postgres',
                             '-d', 'results', '-Atc', query], stdout=subprocess.PIPE, text=True).stdout.strip()

    def snapshot(self, name):
        # Outside measured windows: retain CPU pressure/throttling and container
        # completion evidence without serializing pod specifications or secrets.
        c = self.c
        # Cgroups can disappear during collection; skip removed files.
        c['log_run']('ingest-resources-'+name+'.log', ['docker', 'exec', c['node'], 'sh', '-c',
            'cat /proc/pressure/cpu; find /sys/fs/cgroup -name cpu.stat '
            + "-exec awk 'BEGIN { for (i=1; i<ARGC; i++) { file=ARGV[i]; "
            + "if ((getline value < file)>0) { print file; do { print value } "
            + "while ((getline value < file)>0) } close(file) } exit }' {} +"])
        pods = c['document']([*c['ctl'], 'get', 'pods', '-o', 'json'])['items']
        safe = [dict(name=p['metadata']['name'], uid=p['metadata']['uid'],
                     containers=p.get('status', {}).get('containerStatuses', [])) for p in pods]
        (c['output']/('ingest-containers-'+name+'.json')).write_text(json.dumps(safe, indent=2)+'\n')
        return safe

    def prepare(self, resources):
        c = self.c
        apply, secret, resource = c['apply'], c['secret'], c['resource']
        password = c['secrets'].token_hex(32)
        apply('ingest-db-secret.json', secret('ingest-db', dict(password=password)))
        database = dict(name='postgres', image=c['POSTGRES'], env=[dict(name='POSTGRES_DB', value='results'),
            dict(name='POSTGRES_PASSWORD', valueFrom=dict(secretKeyRef=dict(name='ingest-db', key='password')))],
            readinessProbe=dict(exec=dict(command=['pg_isready', '-U', 'postgres', '-d', 'results']), periodSeconds=2),
            resources=dict(requests=dict(cpu='100m', memory='128Mi'), limits=dict(cpu='1', memory='512Mi')))
        apply('ingest-db.json', dict(apiVersion='apps/v1', kind='Deployment', metadata=dict(name='ingest-db', namespace=c['namespace']),
            spec=dict(replicas=1, selector=dict(matchLabels=dict(app='ingest-db')),
                template=dict(metadata=dict(labels=dict(app='ingest-db')), spec=dict(containers=[database])))))
        apply('ingest-db-service.json', resource('Service', 'ingest-db', spec=dict(selector=dict(app='ingest-db'), ports=[dict(port=5432)])))
        c['log_run']('ingest-db-ready.log', [*c['ctl'], 'rollout', 'status', 'deployment/ingest-db', '--timeout=120s'], timeout=130)
        apply('ingest-config-secret.json', secret('ingest-config', {
            'database': f'postgres://postgres:{password}@ingest-db:5432/results?sslmode=disable',
            'tls.crt': c['cert'].read_bytes(), 'tls.key': c['key'].read_bytes(), 'reader.key': c['reader']}))
        apply('ingest-trust-secret.json', secret('ingest-trust', {'ca.crt': c['cert'].read_bytes()}))
        generated = c['output']/'ingest-generated.json'
        c['command']([sys.executable, c['ROOT']/'tools/match/ingest_kubernetes.py', '--namespace', c['namespace'],
                      '--image', c['args'].image, '--output', generated])
        deployment = json.loads(generated.read_text())
        self.hpa = next(row for row in deployment['items'] if row['kind'] == 'HorizontalPodAutoscaler')
        apply('ingest.json', deployment)
        c['log_run']('ingest-ready.log', [*c['ctl'], 'rollout', 'status', 'deployment/ingest', '--timeout=120s'], timeout=130)
        fixture = c['root']/'ingest-fixture'
        fixture.mkdir()
        c['command'](['go', 'build', '-o', fixture/'fixture', c['ROOT']/'tests/assets/ingest/burst.go'],
                      env=dict(os.environ, CGO_ENABLED='0'), timeout=60)
        c['command'](['docker', 'cp', fixture, c['node']+':/aftershock-ingest-fixture'])
        c['command'](['docker', 'exec', c['node'], 'chown', '-R', '65532:65532', '/aftershock-ingest-fixture'])
        for row in resources['items']:
            if row['kind'] == 'Fleet':
                pod = row['spec']['template']['spec']['template']['spec']
                pod['volumes'].append(dict(name='ingest-fixture', hostPath=dict(path='/aftershock-ingest-fixture', type='Directory')))
                next(container for container in pod['containers'] if container['name'] == 'server')['volumeMounts'].append(
                    dict(name='ingest-fixture', mountPath='/fixture', readOnly=True))
                self.fleet = copy.deepcopy(row)
        return resources

    def inspect(self, server):
        c = self.c
        result = c['document']([*c['ctl'], 'exec', server, '-c', 'server', '--', '/fixture/fixture', 'inspect'])
        return {name: base64.b64decode(data) for name, data in result.items()}

    def outage(self, match, server):
        c = self.c
        c['wait_for'](lambda: int(self.sql(f"SELECT count(*) FROM results.events WHERE match_id='{match}'")) > 0,
                      30, 'initial committed native events')
        # Fault injection into owned resources only. Restore the generated HPA
        # unchanged after recovery; no lowered scaling target can mask the test.
        c['log_run']('ingest-stop-hpa.log', [*c['ctl'], 'delete', 'hpa', 'ingest'])
        c['log_run']('ingest-stop.log', [*c['ctl'], 'scale', 'deployment/ingest', '--replicas=0'])
        c['wait_for'](lambda: not c['document']([*c['ctl'], 'get', 'pods', '-l', 'app=ingest', '-o', 'json'])['items'],
                      60, 'all ingest pods stopped')
        ended = c['wait_for'](lambda: (files if 'engine.done' in (files := self.inspect(server)) else None),
                              100, 'native engine completion during ingest outage')
        assert 'results.done' not in ended and 'pending.json' in ended
        original_ack = ended.get('ack.json')
        stopped_at = time.monotonic()
        while time.monotonic()-stopped_at < 70:
            time.sleep(1)
        pod = c['document']([*c['ctl'], 'get', 'pod', server, '-o', 'json'])
        assert all(row['restartCount'] == 0 for row in pod['status']['containerStatuses']), \
            'controller exited while final data was unacknowledged; old one-minute timeout must not retire the pod'
        retained = self.inspect(server)
        gs = c['document']([*c['ctl'], 'get', 'gameserver', server, '-o', 'json'])
        assert gs['status']['state'] == 'Allocated', 'unacknowledged match was retired'
        assert retained.get('ack.json') == original_ack and 'results.done' not in retained
        assert 'pending.json' in retained and retained['baseoa/games.log'] == ended['baseoa/games.log']
        log = retained['baseoa/games.log']
        assert log.endswith(b'\n')
        ack = json.loads(original_ack) if original_ack else dict(end=0)
        assert ack['end'] < len(log), 'test did not leave native events unacknowledged'
        self.native = log
        (c['output']/'outage-native.log').write_bytes(log)
        self.evidence['outage'] = dict(post_engine_outage_seconds=round(time.monotonic()-stopped_at, 3),
                                      retained_ack=ack['end'], native_bytes=len(log), restarts=0)
        c['log_run']('ingest-recover.log', [*c['ctl'], 'scale', 'deployment/ingest', '--replicas=2'])
        c['log_run']('ingest-recovered.log', [*c['ctl'], 'rollout', 'status', 'deployment/ingest', '--timeout=120s'], timeout=130)
        c['apply']('ingest-hpa-restored.json', self.hpa)

    def final_record(self, match):
        data = self.sql(f"SELECT json_build_object('final',final,'end',offset_bytes,'checkpoint',checkpoint) FROM results.matches WHERE match_id='{match}'")
        if not data:
            return None
        final = json.loads(data)
        if not final['final']:
            return None
        rows = json.loads(self.sql(f"SELECT json_agg(json_build_object('sequence',sequence,'raw',encode(raw,'hex')) ORDER BY sequence) FROM results.events WHERE match_id='{match}'"))
        expected = []
        offset = 0
        for raw in self.native.split(b'\n')[:-1]:
            expected.append(dict(sequence=offset, raw=raw.hex()))
            offset += len(raw)+1
        assert rows == expected and final['end'] == offset, 'native events missing, changed or duplicated after outage'
        self.evidence['outage'].update(events=len(rows), stored_bytes=offset, exact_native_match=True)
        (self.c['output']/'ingest-events.json').write_text(json.dumps(rows, indent=2)+'\n')
        return final

    def drain_match(self):
        c = self.c
        c['wait_for'](c['ready'], 120, 'replacement native server before preStop check')
        match = 'drain-'+c['secrets'].token_hex(12)
        spec = dict(id=match, map='two_lane', mode=0, frag_limit=0, time_limit=1, players=1,
                    password=c['secrets'].token_hex(16), token=c['secrets'].token_hex(32))
        allocation = dict(apiVersion='allocation.agones.dev/v1', kind='GameServerAllocation',
            metadata=dict(namespace=c['namespace']), spec=dict(selectors=[dict(matchLabels={'agones.dev/fleet': 'aftershock'})],
                metadata=dict(labels={'aftershock.dev/match': match}, annotations={'aftershock.dev/match': json.dumps(spec)})))
        path = c['root']/'drain-allocation.json'
        path.write_text(json.dumps(allocation))
        path.chmod(0o600)
        response = c['document']([*c['ctl'], 'create', '-f', path, '-o', 'json'])
        assert response['status']['state'] == 'Allocated'
        server = response['status']['gameServerName']
        c['follow']('drain-server.log', [*c['ctl'], 'logs', '-f', server, '-c', 'server'])
        first = c['wait_for'](lambda: self.sql(f"SELECT offset_bytes FROM results.matches WHERE match_id='{match}' AND offset_bytes>0"),
                             30, 'initial native drain stream')
        c['log_run']('drain-delete.log', [*c['ctl'], 'delete', 'pod', server, '--grace-period=75', '--wait=false'])
        deadline = time.monotonic()+60
        while self.sql(f"SELECT final FROM results.matches WHERE match_id='{match}'") != 't':
            pod = subprocess.run([*c['ctl'], 'get', 'pod', server, '-o', 'name'], stdout=subprocess.DEVNULL,
                                 stderr=subprocess.DEVNULL, timeout=10)
            assert pod.returncode == 0, 'pod disappeared before final durable acknowledgement'
            assert time.monotonic() < deadline, 'native preStop flush timed out'
            time.sleep(.2)
        assert self.sql(f"SELECT checkpoint->>'completed' FROM results.matches WHERE match_id='{match}'") == 'false'
        assert self.sql(f"SELECT count(*) FROM results.player_results WHERE match_id='{match}'") == '0'
        assert int(self.sql(f"SELECT count(*) FROM results.events WHERE match_id='{match}' AND sequence>={int(first)} AND type='ShutdownGame'")) >= 1
        self.evidence['prestop'] = dict(native_engine=True, kubernetes_pod_deletion=True,
                                       final_committed=True, aborted_scores_not_published=True)

    def finish(self):
        c = self.c
        self.drain_match()
        (c['output']/'ingest-native.json').write_text(json.dumps(self.evidence, indent=2)+'\n')
        # Native UI acceptance is complete; remove rendering work from both
        # latency windows equally. The real backend request path remains active.
        c['execute']('quit')
        assert c['client'].wait(timeout=10) == 0
        c['client'] = None
        ticket = c['secrets'].token_bytes(32)
        config = c['root']/'provider/config.json'
        identity = json.loads(config.read_text())
        identity['ticket'] = ticket.hex()
        config.write_text(json.dumps(identity))
        c['command'](['docker', 'cp', config, c['node']+':/aftershock-provider/config.json'])
        c['command'](['docker', 'exec', c['node'], 'chown', '65532:65532', '/aftershock-provider/config.json'])
        c['log_run']('benchmark-provider-restart.log', [*c['ctl'], 'rollout', 'restart', 'deployment/auth-fixture'])
        c['log_run']('benchmark-provider-ready.log', [*c['ctl'], 'rollout', 'status', 'deployment/auth-fixture', '--timeout=60s'], timeout=70)
        # Measure ordinary HTTPS service traffic. kubectl port-forward traverses
        # the API server/kubelet and would mix control-plane load into this latency.
        c['apply']('burst-backend-service.json', c['resource']('Service', 'backend-benchmark',
            spec=dict(type='NodePort', selector=dict(app='backend'),
                      ports=[dict(port=8443, targetPort='https', nodePort=30443)])))
        benchmark_endpoint = c['benchmark_endpoint']
        def service_ready():
            try:
                with urllib.request.urlopen(benchmark_endpoint+'/healthz', context=c['trust'], timeout=5) as response:
                    return json.load(response).get('ready') is True
            except (urllib.error.URLError, OSError):
                return False
        c['wait_for'](service_ready, 30, 'private HTTPS benchmark service')
        request = urllib.request.Request(benchmark_endpoint+'/v1/login', data=json.dumps(dict(ticket=ticket.hex())).encode(),
                                         headers={'Content-Type': 'application/json'}, method='POST')
        def login_ready():
            try:
                with urllib.request.urlopen(request, context=c['trust'], timeout=10) as response:
                    return json.load(response)
            except urllib.error.HTTPError as error:
                # A restarted fixture's Service endpoints can lag rollout readiness.
                # Only a pre-authentication provider-unavailable response is retryable;
                # benchmark requests below must all succeed without retries.
                if error.code != 503 or json.load(error).get('error') != 'authentication_unavailable':
                    raise
                return None
        session = c['wait_for'](login_ready, 30, 'restarted authentication fixture through backend')
        assert session['player_id'] == c['player']
        token = session['token']
        coordinator = dict(name='coordinator', image=c['args'].image, command=['/fixture/fixture'],
            env=[dict(name='FIXTURE_MODE', value='coordinator')],
            resources=dict(requests=dict(cpu='10m', memory='16Mi'), limits=dict(cpu='250m', memory='64Mi')),
            volumeMounts=[dict(name='fixture', mountPath='/fixture', readOnly=True)],
            readinessProbe=dict(httpGet=dict(path='/status', port=8080), periodSeconds=1))
        c['apply']('burst-coordinator.json', dict(apiVersion='apps/v1', kind='Deployment',
            metadata=dict(name='burst-coordinator', namespace=c['namespace']), spec=dict(replicas=1,
                selector=dict(matchLabels=dict(app='burst-coordinator')),
                template=dict(metadata=dict(labels=dict(app='burst-coordinator')), spec=dict(containers=[coordinator],
                    volumes=[dict(name='fixture', hostPath=dict(path='/aftershock-ingest-fixture', type='Directory'))])))))
        c['apply']('burst-service.json', c['resource']('Service', 'burst-coordinator',
            spec=dict(selector=dict(app='burst-coordinator'), ports=[dict(port=8080)])))
        c['log_run']('burst-coordinator-ready.log', [*c['ctl'], 'rollout', 'status', 'deployment/burst-coordinator', '--timeout=60s'], timeout=70)
        fleet = copy.deepcopy(self.fleet)
        fleet['metadata']['name'] = 'ingest-burst'
        fleet['spec']['replicas'] = 100
        game = fleet['spec']['template']['spec']
        game['ports'] = []
        pod = game['template']['spec']
        pod['volumes'] = [row for row in pod['volumes'] if row['name'] != 'test-content']
        for row in pod['volumes']:
            if row['name'] == 'home':
                row['emptyDir']['sizeLimit'] = '8Mi'
        containers = {row['name']: row for row in pod['containers']}
        producer = containers['server']
        producer.update(command=['/fixture/fixture'], args=[], env=[], ports=[],
                        resources=dict(requests=dict(cpu='5m', memory='8Mi'), limits=dict(cpu='100m', memory='32Mi')))
        producer['volumeMounts'] = [row for row in producer['volumeMounts'] if row['name'] != 'test-content']
        shipper = containers['results']
        shipper.update(command=['/fixture/fixture'], args=[])
        shipper['volumeMounts'].append(dict(name='ingest-fixture', mountPath='/fixture', readOnly=True))
        shipper['env'].append(dict(name='FIXTURE_MODE', value='shipper'))
        shipper['resources'] = dict(requests=dict(cpu='5m', memory='16Mi'), limits=dict(cpu='250m', memory='64Mi'))
        shipper['env'] = [row for row in shipper['env'] if row['name'] != 'MATCH_GAME']
        shipper['env'].append(dict(name='MATCH_GAME', value='aftershock'))
        c['apply']('burst-fleet.json', fleet)
        c['wait_for'](lambda: sum(g.get('status', {}).get('state') == 'Ready' for g in
            c['document']([*c['ctl'], 'get', 'gameservers', '-l', 'agones.dev/fleet=ingest-burst', '-o', 'json'])['items']) == 100,
            240, '100 actual Agones Ready producers')
        allocation_dir = c['root']/'burst-allocations'
        allocation_dir.mkdir(mode=0o700)
        def allocate(index):
            match = 'burst-'+c['secrets'].token_hex(8)+'-'+str(index)
            spec = dict(id=match, map='two_lane', mode=0, frag_limit=0, time_limit=1, players=1,
                        password='', token=c['secrets'].token_hex(32), join_key=c['secrets'].token_hex(32),
                        expected_players=[str(1000000+index)])
            allocation = dict(apiVersion='allocation.agones.dev/v1', kind='GameServerAllocation',
                metadata=dict(namespace=c['namespace']), spec=dict(selectors=[dict(matchLabels={'agones.dev/fleet': 'ingest-burst'})],
                    metadata=dict(labels={'aftershock.dev/match': match}, annotations={'aftershock.dev/match': json.dumps(spec)})))
            path = allocation_dir/(str(index)+'.json')
            path.write_text(json.dumps(allocation))
            path.chmod(0o600)
            response = c['document']([*c['ctl'], 'create', '-f', path, '-o', 'json'])
            assert response['status']['state'] == 'Allocated'
            return match
        with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
            matches = list(pool.map(allocate, range(100)))
        assert len(set(matches)) == 100
        with c['socket'].socket() as reservation:
            reservation.bind(('127.0.0.1', 0))
            port = reservation.getsockname()[1]
        c['follow']('burst-forward.log', [*c['ctl'], 'port-forward', 'service/burst-coordinator', f'{port}:8080'])
        c['wait_for'](lambda: 'Forwarding from' in (c['output']/'burst-forward.log').read_text(), 20, 'burst coordinator forward')
        coordinator_url = f'http://127.0.0.1:{port}'
        def control(path, post=False):
            request = urllib.request.Request(coordinator_url+path, method='POST' if post else 'GET')
            with urllib.request.urlopen(request, timeout=5) as response:
                data = response.read()
            return json.loads(data) if path == '/status' else None
        c['wait_for'](lambda: control('/status')['ready'] == 100, 60, '100 producers at the same ending barrier')
        c['wait_for'](lambda: self.sql("SELECT count(*) FROM results.matches WHERE match_id LIKE 'burst-%' AND NOT final") == '100',
                      60, '100 authenticated initial streams committed')
        stop = threading.Event()
        lock = threading.Lock()
        samples, failures = [], []
        endpoint = urllib.parse.urlsplit(benchmark_endpoint)
        def sample():
            probe = ProfileProbe(endpoint.hostname, endpoint.port, c['trust'], token, c['player'])
            try:
                while not stop.is_set():
                    started = time.monotonic()
                    elapsed = probe.sample()
                    with lock:
                        samples.append(dict(start=started, milliseconds=elapsed))
                    time.sleep(.01)
            except Exception as error:
                with lock:
                    failures.append(type(error).__name__)
                raise
            finally:
                probe.close()
        try:
            with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
                workers = [pool.submit(sample) for _ in range(4)]
                try:
                    time.sleep(1)
                    self.snapshot('baseline-start')
                    baseline_start = time.monotonic()
                    time.sleep(5)
                    baseline_end = time.monotonic()
                    assert retirement_held(self.snapshot('burst-start')), 'producer fleet was not running before release'
                    burst_start = time.monotonic()
                    control('/release', True)
                    deadline = time.monotonic()+60
                    while len((status := control('/status'))['acked']) != 100 or status['shippers_held'] != 100:
                        assert time.monotonic() < deadline, '100 ending acknowledgements timed out'
                        time.sleep(.1)
                    burst_end = time.monotonic()
                    held = retirement_held(self.snapshot('burst-end'))
                finally:
                    stop.set()
                for worker in workers:
                    worker.result()
            def distribution(start, end):
                values = sorted(row['milliseconds'] for row in samples if start <= row['start'] < end)
                assert len(values) >= 100, 'insufficient authenticated profile samples'
                return dict(samples=len(values), p50=values[math.ceil(len(values)*.50)-1],
                            p95=values[math.ceil(len(values)*.95)-1], p99=values[math.ceil(len(values)*.99)-1], maximum=max(values))
            baseline, during = distribution(baseline_start, baseline_end), distribution(burst_start, burst_end)
            limit = max(baseline['p95']*1.25, baseline['p95']+5.0)
            report = dict(producers=100, actual_agones_allocations=True, simulated_endings=True,
                baseline_ms=baseline, burst_ms=during, p95_limit_ms=limit, errors=failures,
                ending_spread_ms=max(status['ended'].values())-min(status['ended'].values()),
                final_acknowledgements=len(status['acked']), completed_shippers_held=status['shippers_held'], burst_seconds=burst_end-burst_start,
                retirement_held_during_measurement=held, transport='verified HTTPS through private NodePort')
            (c['output']/'ingest-burst.json').write_text(json.dumps(report, indent=2)+'\n')
            (c['output']/'ingest-latency-samples.json').write_text(json.dumps(samples)+'\n')
            assert held, 'fixture containers retired during the ingest measurement'
            assert len(status['ended']) == 100 and report['ending_spread_ms'] <= 5000
            assert not failures and during['p95'] <= limit, 'player-facing profile latency regressed beyond declared noise allowance'
            assert self.sql("SELECT count(*) FROM results.matches WHERE match_id LIKE 'burst-%' AND final") == '100'
            assert self.sql("SELECT count(*) FROM results.events WHERE match_id LIKE 'burst-%'") == '600'
            assert self.sql("SELECT count(*)||':'||sum(score)||':'||sum(kills)||':'||sum(deaths) FROM results.player_results WHERE match_id LIKE 'burst-%'") == '100:100:0:0'
            assert self.sql("SELECT count(*) FROM results.matches m WHERE match_id LIKE 'burst-%' AND offset_bytes<>(SELECT COALESCE(sum(octet_length(raw)+1),0) FROM results.events e WHERE e.match_id=m.match_id)") == '0'
            report['exact_events_and_totals'] = True
            (c['output']/'ingest-burst.json').write_text(json.dumps(report, indent=2)+'\n')
            self.evidence['burst'] = report
            control('/retire', True)
        finally:
            c['log_run']('ingest-benchmark-backend.log', [*c['ctl'], 'logs', 'deployment/backend',
                '--all-pods=true', '--prefix', '--since=2m'])
            request = urllib.request.Request(benchmark_endpoint+'/v1/logout', data=b'{}',
                headers={'Authorization': 'Bearer '+token, 'Content-Type': 'application/json'}, method='POST')
            with urllib.request.urlopen(request, context=c['trust'], timeout=5) as response:
                assert response.status == 200
            shutil.rmtree(allocation_dir, ignore_errors=True)
        return self.evidence

    def cleanup(self):
        for name in ('ingest-db-secret.json', 'ingest-config-secret.json', 'ingest-trust-secret.json'):
            (self.c['output']/name).unlink(missing_ok=True)
