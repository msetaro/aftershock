"""Production ingest scenarios used by the owned backend kind acceptance driver."""
import base64
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time


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
        retained = self.inspect(server)
        pod = c['document']([*c['ctl'], 'get', 'pod', server, '-o', 'json'])
        assert all(row['restartCount'] == 0 for row in pod['status']['containerStatuses']), \
            'controller exited while final data was unacknowledged; old one-minute timeout must not retire the pod'
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

    def finish(self):
        # Filled by the separately measured 100-producer burst acceptance.
        raise AssertionError('100 simultaneous endings and backend latency acceptance still required')

    def cleanup(self):
        for name in ('ingest-db-secret.json', 'ingest-config-secret.json', 'ingest-trust-secret.json'):
            (self.c['output']/name).unlink(missing_ok=True)
