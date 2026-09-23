#!/usr/bin/env python3
"""Owned kind acceptance: native HTTPS login, allocation, signed play and profile results."""
import argparse
import base64
import concurrent.futures
import hashlib
import http.server
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import signal
import socket
import ssl
import subprocess
import sys
import tarfile
import tempfile
import threading
import time
import urllib.parse
import urllib.request
import uuid
import yaml
from cook import cook
from run import ROOT, SCRATCH

sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT/'tools/match'))
from tools.agent import Engine
from toolchain import NODE, tool

POSTGRES = 'postgres:18-bookworm@sha256:3725f4e2499eef5134592b3b4ab79a543ed7f8e533b05b5b637af926630f6650'
METRICS_URL = 'https://github.com/kubernetes-sigs/metrics-server/releases/download/v0.8.1/components.yaml'
METRICS_SHA = '4a672c4891902573a3ff753cece5de1bf1f55dd053403dfec39df9d1636b7ff1'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--image', required=True)
parser.add_argument('--deployment-only', action='store_true', help='check deployment/metrics/HPA only; never full native acceptance')
parser.add_argument('--client', type=Path)
parser.add_argument('--data', type=Path, required=True)
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-backend-kind')
args = parser.parse_args()
if not args.deployment_only and args.client is None:
    parser.error('--client is required for full native acceptance')
if args.output.exists():
    parser.error('choose a new output directory')
args.output.mkdir(parents=True, mode=0o700)
output = args.output.resolve()
kind, kubectl, helm, chart = [tool(name) for name in ('kind', 'kubectl', 'helm', 'agones-1.60.0.tgz')]
cluster = 'aftershock-backend-'+uuid.uuid4().hex[:8]
namespace = cluster
kubeconfig = output/'kubeconfig'
ctl = [kubectl, '--kubeconfig', str(kubeconfig), '-n', namespace]
processes, streams = [], []
created = False
provider = worker = engine = None

def command(arguments, **kwargs):
    kwargs.setdefault('timeout', 30)
    return subprocess.run([str(x) for x in arguments], check=True, **kwargs)

def document(arguments):
    return json.loads(command(arguments, stdout=subprocess.PIPE, text=True).stdout)

def log_run(name, arguments, **kwargs):
    with (output/name).open('w') as stream:
        return command(arguments, stdout=stream, stderr=subprocess.STDOUT, **kwargs)

def follow(name, arguments):
    stream = (output/name).open('w')
    streams.append(stream)
    process = subprocess.Popen([str(x) for x in arguments], stdout=stream, stderr=subprocess.STDOUT, start_new_session=True)
    processes.append(process)
    return process

def wait_for(check, seconds, label, pump=False):
    deadline = time.monotonic()+seconds
    while time.monotonic() < deadline:
        if pump:
            engine.step(2)
        value = check()
        if value:
            return value
        time.sleep(.1 if pump else 1)
    raise AssertionError(label+' timed out; see '+str(output))

def apply(name, value, cluster_scope=False):
    path = output/name
    path.write_text(json.dumps(value))
    path.chmod(0o600)
    log_run(name+'.log', [*(ctl[:3] if cluster_scope else ctl), 'apply', '-f', path])

def resource(kind, name, **fields):
    return dict(apiVersion='v1', kind=kind, metadata=dict(name=name, namespace=namespace), **fields)

def secret(name, values):
    return resource('Secret', name, data={k: base64.b64encode(v if isinstance(v, bytes) else v.encode()).decode() for k, v in values.items()})

def sql(query):
    # Captured privately: callers must never put credentials into public reports/logs.
    return command([*ctl, 'exec', 'deployment/backend-db', '--', 'psql', '-U', 'postgres', '-d', 'backend', '-Atc', query],
                   stdout=subprocess.PIPE, text=True).stdout.strip()

try:
    created = True
    log_run('create.log', [kind, 'create', 'cluster', '--name', cluster, '--image', NODE,
                          '--kubeconfig', kubeconfig, '--wait', '60s'], timeout=240)
    log_run('image.log', [kind, 'load', 'docker-image', args.image, '--name', cluster], timeout=180)
    node = cluster+'-control-plane'
    # Pull the pinned manifest through the node runtime. Importing a Docker
    # multi-platform index can reference platforms absent from its local store.
    log_run('postgres-pull.log', ['docker', 'exec', node, 'crictl', 'pull', POSTGRES], timeout=240)
    network = document(['docker', 'inspect', node])[0]['NetworkSettings']['Networks']['kind']
    gateway = network['Gateway']
    log_run('namespace.log', [*ctl, 'create', 'namespace', namespace])
    manifests = output/'match-resources'
    command([sys.executable, ROOT/'tools/match/kubernetes.py', '--namespace', namespace, '--image', args.image,
             '--output', manifests, '--ci-content', '/aftershock-ci'])
    log_run('agones.log', [helm, 'install', 'agones', chart, '--namespace', 'agones-system', '--create-namespace',
        '--kubeconfig', kubeconfig, '--server-side=false', '--set', 'agones.allocator.install=false',
        '--set', 'agones.ping.install=false', '--set', 'agones.controller.replicas=1', '--set', 'agones.controller.numWorkers=2',
        '--set', 'agones.image.sdk.memoryRequest=32Mi', '--set', 'agones.image.sdk.memoryLimit=128Mi',
        '--set', 'gameservers.namespaces[0]='+namespace, '--set', 'gameservers.minPort=7000',
        '--set', 'gameservers.maxPort=7003', '--wait', '--timeout', '180s'], timeout=240)
    metrics = urllib.request.urlopen(METRICS_URL, timeout=60).read()
    assert hashlib.sha256(metrics).hexdigest() == METRICS_SHA
    metrics_objects = list(yaml.safe_load_all(metrics))
    for obj in metrics_objects:
        if obj['kind'] == 'Deployment':
            # Private kind kubelets have self-signed serving certificates. This
            # exception is isolated to metrics collection, never backend TLS.
            obj['spec']['template']['spec']['containers'][0]['args'].append('--kubelet-insecure-tls')
    apply('metrics.json', dict(apiVersion='v1', kind='List', items=metrics_objects), cluster_scope=True)
    with tempfile.TemporaryDirectory(prefix='backend-kind-content-', dir=SCRATCH) as temporary:
        root = Path(temporary)
        content = root/'content'
        command([sys.executable, ROOT/'tools/match/content.py', content])
        paks = sorted(args.data.resolve().glob('*.pk3'))
        assert len(paks) >= 8, 'complete public OpenArena data required'
        command(['docker', 'exec', node, 'mkdir', '-p', '/aftershock-ci/baseoa'])
        with tempfile.TemporaryFile() as archive:
            with tarfile.open(fileobj=archive, mode='w') as tar:
                for pak in paks:
                    tar.add(pak.resolve(), arcname='baseoa/'+pak.name)
                tar.add(content/'aftershock/pak0.pk3', arcname='baseoa/zz-aftershock-level.pk3')
            archive.seek(0)
            command(['docker', 'exec', '-i', node, 'tar', '-xf', '-', '-C', '/aftershock-ci'], stdin=archive, timeout=120)
        cert, key = root/'tls.crt', root/'tls.key'
        log_run('certificate.log', ['openssl', 'req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-days', '1',
            '-keyout', key, '-out', cert, '-subj', '/CN=aftershock-owned-fixture', '-addext',
            f'subjectAltName=IP:127.0.0.1,IP:{gateway},DNS:backend,DNS:backend.{namespace}.svc'])
        trust = ssl.create_default_context(cafile=str(cert))
        ticket = secrets.token_bytes(32)
        publisher, reader, password = [secrets.token_hex(32) for _ in range(3)]
        player = '18446744073709551615'
        class Provider(http.server.BaseHTTPRequestHandler):
            def do_GET(self):
                query = urllib.parse.parse_qs(urllib.parse.urlsplit(self.path).query)
                valid = query == dict(key=[publisher], appid=['12345'], ticket=[ticket.hex()], identity=['aftershock'])
                body = json.dumps(dict(response=dict(params=dict(result='OK', steamid=player, publisherbanned=False)))).encode()
                self.send_response(200 if valid else 401)
                self.send_header('Content-Length', str(len(body)))
                self.end_headers()
                self.wfile.write(body)
            def log_message(self, *_):
                pass
        provider = http.server.ThreadingHTTPServer((gateway, 0), Provider)
        server_tls = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        server_tls.load_cert_chain(cert, key)
        provider.socket = server_tls.wrap_socket(provider.socket, server_side=True)
        worker = threading.Thread(target=provider.serve_forever)
        worker.start()
        apply('db-secret.json', secret('backend-db', dict(password=password)))
        db_container = dict(name='postgres', image=POSTGRES, env=[dict(name='POSTGRES_DB', value='backend'),
            dict(name='POSTGRES_PASSWORD', valueFrom=dict(secretKeyRef=dict(name='backend-db', key='password')))],
            readinessProbe=dict(exec=dict(command=['pg_isready', '-U', 'postgres', '-d', 'backend']), periodSeconds=2),
            resources=dict(requests=dict(cpu='100m', memory='128Mi'), limits=dict(cpu='1', memory='512Mi')))
        apply('db.json', dict(apiVersion='apps/v1', kind='Deployment', metadata=dict(name='backend-db', namespace=namespace),
            spec=dict(replicas=1, selector=dict(matchLabels=dict(app='backend-db')),
            template=dict(metadata=dict(labels=dict(app='backend-db')), spec=dict(containers=[db_container])))))
        apply('db-service.json', resource('Service', 'backend-db', spec=dict(selector=dict(app='backend-db'), ports=[dict(port=5432)])))
        log_run('db-ready.log', [*ctl, 'rollout', 'status', 'deployment/backend-db', '--timeout=120s'], timeout=130)
        apply('backend-secret.json', secret('backend-config', {
            'database': f'postgres://postgres:{password}@backend-db:5432/backend?sslmode=disable', 'app-id': '12345',
            'tls.crt': cert.read_bytes(), 'tls.key': key.read_bytes(), 'ca.crt': cert.read_bytes(),
            'publisher.key': publisher, 'reader.key': reader, 'catalog.json': '["weapons/range_rifle.asweapon"]'}))
        resources = json.loads((manifests/'resources.json').read_text())
        for obj in resources['items']:
            if obj['kind'] == 'Deployment':
                obj['spec']['template']['spec']['containers'][0]['env'].append(dict(name='MATCH_READ_TOKEN',
                    valueFrom=dict(secretKeyRef=dict(name='backend-config', key='reader.key'))))
        apply('match.json', resources)
        command([sys.executable, ROOT/'tools/match/backend_kubernetes.py', '--namespace', namespace, '--image', args.image,
                 '--output', output/'backend-generated.json', '--extra-ca', '--development-results'])
        resources = json.loads((output/'backend-generated.json').read_text())
        for obj in resources['items']:
            if obj['kind'] == 'Deployment':
                obj['spec']['template']['spec']['containers'][0]['env'] += [
                    dict(name='BACKEND_STEAM_URL', value=f'https://{gateway}:{provider.server_port}/authenticate'),
                    dict(name='BACKEND_MATCH_MINUTES', value='1')]
        apply('backend.json', resources)
        log_run('backend-ready.log', [*ctl, 'rollout', 'status', 'deployment/backend', '--timeout=120s'], timeout=130)
        def ready():
            return next((g['metadata']['name'] for g in document([*ctl, 'get', 'gameservers', '-o', 'json'])['items']
                         if g.get('status', {}).get('state') == 'Ready'), None)
        original = wait_for(ready, 120, 'warm Fleet Ready')
        follow('server.log', [*ctl, 'logs', '-f', original, '-c', 'server'])
        follow('ship.log', [*ctl, 'logs', '-f', original, '-c', 'results'])
        with socket.socket() as reservation:
            reservation.bind(('127.0.0.1', 0))
            port = reservation.getsockname()[1]
        follow('forward.log', [*ctl, 'port-forward', 'service/backend', f'{port}:8443'])
        wait_for(lambda: 'Forwarding from' in (output/'forward.log').read_text(), 20, 'HTTPS forward')
        endpoint = f'https://127.0.0.1:{port}'
        def get(path):
            with urllib.request.urlopen(endpoint+path, context=trust, timeout=5) as response:
                return response.read().decode()
        assert json.loads(get('/healthz'))['ready']
        def check_scaling():
            # Real resource metrics must drive a scale-out. Use a low test target,
            # preserving the generated production 65% target in its source manifest.
            log_run('hpa-target.log', [*ctl, 'patch', 'hpa', 'backend', '--type=json', '-p',
                '[{"op":"replace","path":"/spec/metrics/0/resource/target/averageUtilization","value":1}]'])
            stopping = threading.Event()
            def load():
                while not stopping.is_set():
                    get('/healthz')
            with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
                futures = [pool.submit(load) for _ in range(4)]
                try:
                    def scaled():
                        hpa = document([*ctl, 'get', 'hpa', 'backend', '-o', 'json'])
                        deployment = document([*ctl, 'get', 'deployment', 'backend', '-o', 'json'])
                        return hpa if hpa.get('status', {}).get('desiredReplicas', 0) > 2 and deployment.get('status', {}).get('readyReplicas', 0) > 2 else None
                    hpa = wait_for(scaled, 180, 'resource metrics and HPA scale-out')
                    (output/'hpa.json').write_text(json.dumps(hpa, indent=2))
                finally:
                    stopping.set()
                for future in futures:
                    future.result()
            assert json.loads(get('/healthz'))['ready']
            return hpa
        hpa = check_scaling()
        if args.deployment_only:
            (output/'report.json').write_text(json.dumps(dict(full=False, deployment=True,
                scaled_replicas=hpa['status']['desiredReplicas']), indent=2)+'\n')
            print('PASS: deployment-only health, real resource metrics and HPA; native acceptance NOT run')
            sys.exit(0)
        home = root/'client'
        base = home/'baseoa'
        base.mkdir(parents=True)
        (base/'zz-aftershock-level.pk3').symlink_to(content/'aftershock/pak0.pk3')
        (home/'backend-ticket.bin').write_bytes(ticket)
        (home/'backend-ticket.bin').chmod(0o600)
        source = root/'source'
        source.mkdir()
        shutil.copyfile('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', source/'font.ttf')
        shutil.copyfile(ROOT/'tests/assets/backend/shell.json', source/'shell.json')
        project = source/'assets.json'
        project.write_text(json.dumps(dict(version=1, assets=[dict(name='ui/backend', kind='ui', source='shell.json')])))
        cook(project, base)
        engine = Engine(args.client, args.data, 'openarena', home=home, arguments=[
            '+set', 'net_enabled', '1', '+set', 'net_port', '0', '+set', 'backend_url', endpoint,
            '+set', 'backend_ca', cert, '+set', 'ui_document', 'ui/backend.asui', '+set', 'cl_allowDownload', '0'])
        engine.request('session', dt=50, seed=17)
        engine.step(4)
        def execute(text):
            engine.request('exec', command=text)
            engine.step(2)
        def info():
            execute('backend_info')
            rows = re.findall(r'Backend: status=(.*?) name=(.*?) match=(\S*) score=(\S*) kills=(\S*) deaths=(\S*)', engine.log_path.read_text(errors='replace'))
            assert rows, 'native public backend status must be inspectable without credentials'
            return rows[-1]
        def keypress(name):
            engine.request('key', name=name, down=True)
            engine.step()
            engine.request('key', name=name, down=False)
            engine.step()
        def activate(name):
            for _ in range(6):
                execute('ui_info')
                focus = re.findall(r'UI document: loaded=1 page=main focus=(\w+)', engine.log_path.read_text())
                assert focus, 'authored main menu must be visible'
                if focus[-1] == name:
                    keypress('PAD0_A')
                    return
                keypress('PAD0_DPAD_DOWN')
            raise AssertionError('cannot focus '+name)
        execute('backend_dev_login')
        wait_for(lambda: info()[0] == 'Signed in', 20, 'verified fixture login', True)
        activate('profile')
        wait_for(lambda: info()[:2] == ('Profile loaded', 'Player'), 20, 'owned native profile', True)
        activate('queue')
        assignment = wait_for(lambda: sql("SELECT id||':'||ingest_token FROM backend_matches LIMIT 1"), 20, 'persistent assignment', True)
        match, ingest_token = assignment.split(':')
        # #28's development ingest has static allocation-token configuration.
        # Supply the real generated token before match end; never bypass validation.
        apply('ingest-token.json', secret('ingest-tokens', dict(tokens=json.dumps({match: ingest_token}))))
        log_run('ingest-restart.log', [*ctl, 'rollout', 'restart', 'deployment/ingest'])
        log_run('ingest-ready.log', [*ctl, 'rollout', 'status', 'deployment/ingest', '--timeout=60s'], timeout=70)
        wait_for(lambda: 'ClientBegin: 0' in (output/'server.log').read_text(), 30, 'native signed join', True)
        assert player in (output/'server.log').read_text(), 'server must log verified account attribution'
        wait_for(lambda: bool(engine.request('state')['player']), 20, 'active native player', True)
        execute('kill')
        def match_metrics():
            reply = subprocess.run([*ctl, 'get', '--raw',
                f'/apis/metrics.k8s.io/v1beta1/namespaces/{namespace}/pods/{original}'],
                stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, timeout=10)
            if reply.returncode:
                return None
            data = json.loads(reply.stdout)
            containers = {row['name']: row['usage'] for row in data['containers']}
            if not {'server', 'results', 'agones-gameserver-sidecar'} <= containers.keys():
                return None
            assert all(re.fullmatch(r'[0-9]+[a-zA-Z]*', usage['cpu']) and
                       re.fullmatch(r'[1-9][0-9]*[a-zA-Z]*', usage['memory']) for usage in containers.values())
            return data
        pod_metrics = wait_for(match_metrics, 45, 'real per-match container resource metrics', True)
        (output/'match-metrics.json').write_text(json.dumps(pod_metrics, indent=2))
        def final_record():
            data = command([*ctl, 'exec', 'deployment/ingest', '--', '/app/match', 'records'], stdout=subprocess.PIPE, text=True).stdout
            rows = [json.loads(line) for line in data.splitlines()]
            final = next((row for row in rows if row['match'] == match and row['final']), None)
            if final:
                (output/'events.jsonl').write_text(data)
                return final
        final = wait_for(final_record, 110, 'acknowledged match completion', True)
        execute('disconnect')
        activate('results')
        values = wait_for(lambda: (row if (row := info())[0] == 'Results loaded' else None), 20, 'native results UI', True)
        assert values[2] == match and values[3:] == ('-1', '0', '1'), values
        capture = engine.request('capture', name='backend-results')
        engine.step(2)
        shutil.copyfile(base/capture['path'], output/'profile-results.png')
        metrics = get('/metrics')
        assert 'aftershock_backend_requests_total{service="login"}' in metrics
        (output/'backend-metrics.txt').write_text(metrics)
        activate('logout')
        wait_for(lambda: info()[0] == 'Signed out', 20, 'native logout', True)
        report = dict(full=True, match=match, account=player, gameserver=original, profile_values=values,
                      checkpoint=final['checkpoint'], scaled_replicas=hpa['status']['desiredReplicas'])
        (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        text = engine.log_path.read_text(errors='replace')
        assert ticket.hex() not in text and ingest_token not in text and publisher not in text
        print('PASS: native HTTPS login -> kind backend/Agones -> signed match -> verified stats -> profile results; health/metrics/HPA')
finally:
    if engine:
        shutil.copyfile(engine.log_path, output/'client.log')
        engine.close()
    if provider:
        provider.shutdown()
        provider.server_close()
        worker.join(timeout=5)
    for process in reversed(processes):
        if process.poll() is None:
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
    for stream in streams:
        stream.close()
    if created:
        for name, arguments in (
            ('pods.log', [*ctl, 'get', 'pods', '-o', 'wide']),
            ('events.log', [*ctl, 'get', 'events', '--sort-by=.lastTimestamp']),
            ('backend.log', [*ctl, 'logs', '-l', 'app=backend', '--all-containers', '--tail=100']),
        ):
            with (output/name).open('w') as stream:
                try:
                    subprocess.run(arguments, stdout=stream, stderr=subprocess.STDOUT, timeout=20)
                except subprocess.TimeoutExpired:
                    stream.write('Diagnostic collection timed out; continuing owned cluster cleanup.\n')
        log_run('delete.log', [kind, 'delete', 'cluster', '--name', cluster, '--kubeconfig', kubeconfig], timeout=90)
    # Generated manifests include ephemeral credentials. Never upload them.
    for name in ('db-secret.json', 'backend-secret.json', 'ingest-token.json', 'match.json'):
        (output/name).unlink(missing_ok=True)
    shutil.rmtree(output/'match-resources', ignore_errors=True)
    kubeconfig.unlink(missing_ok=True)
