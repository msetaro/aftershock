#!/usr/bin/env python3
"""Check ingest isolation, bounded resources, TLS, namespaced identity and autoscaling."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from run import ROOT, SCRATCH

with tempfile.TemporaryDirectory(prefix='aftershock-ingest-deployment-', dir=SCRATCH) as temporary:
    output = Path(temporary)/'resources.json'
    subprocess.run([sys.executable, ROOT/'tools/match/ingest_kubernetes.py',
                    '--namespace', 'ingest-test', '--image', 'aftershock-match:issue30',
                    '--output', output], check=True)
    objects = json.loads(output.read_text())['items']
    kinds = {item['kind']: item for item in objects}
    assert len(kinds) == len(objects) == 7
    assert all(item['metadata']['namespace'] == 'ingest-test' for item in objects)
    assert all(item['metadata']['name'] == 'ingest' for item in objects)
    deployment = kinds['Deployment']['spec']
    assert deployment['replicas'] == 2
    assert deployment['strategy']['rollingUpdate']['maxUnavailable'] == 0
    pod = deployment['template']['spec']
    assert pod['serviceAccountName'] == 'ingest'
    container, = pod['containers']
    assert container['args'] == ['ingest']
    assert container['securityContext']['runAsNonRoot']
    assert container['securityContext']['readOnlyRootFilesystem']
    assert not container['securityContext']['allowPrivilegeEscalation']
    env = {entry['name']: entry for entry in container['env']}
    assert env['INGEST_DATABASE']['valueFrom']['secretKeyRef'] == dict(name='ingest-config', key='database')
    assert env['INGEST_TLS_CERT']['value'] == '/config/tls.crt'
    assert env['INGEST_TLS_KEY']['value'] == '/config/tls.key'
    assert env['INGEST_READER_KEY_FILE']['value'] == '/config/reader.key'
    assert 'MATCH_DEV_INSECURE' not in env and 'BACKEND_DATABASE' not in env
    assert container['readinessProbe']['httpGet']['scheme'] == 'HTTPS'
    assert container['resources']['requests']['cpu'] and container['resources']['limits']['memory']
    assert kinds['Role']['rules'] == [dict(apiGroups=['agones.dev'], resources=['gameservers'], verbs=['get', 'list'])]
    assert kinds['Service']['spec']['type'] == 'ClusterIP'
    hpa = kinds['HorizontalPodAutoscaler']['spec']
    assert hpa['scaleTargetRef']['name'] == 'ingest'
    assert hpa['minReplicas'] == 2 and hpa['maxReplicas'] >= hpa['minReplicas']
    assert hpa['metrics'][0]['resource']['target']['averageUtilization'] == 65
    assert kinds['PodDisruptionBudget']['spec']['minAvailable'] == 1
    assert not any(item['kind'] in ('Secret', 'PersistentVolumeClaim', 'ClusterRole') for item in objects)
    fleet_output = Path(temporary)/'fleet'
    subprocess.run([sys.executable, ROOT/'tools/match/kubernetes.py', '--namespace', 'ingest-test',
                    '--image', 'aftershock-match:issue30', '--output', fleet_output,
                    '--production-ingest', '--ingest-ca-secret', 'ingest-trust'], check=True)
    fleet_objects = json.loads((fleet_output/'resources.json').read_text())['items']
    assert {row['kind'] for row in fleet_objects} == {'Namespace', 'Fleet'}
    fleet = next(row for row in fleet_objects if row['kind'] == 'Fleet')
    pod = fleet['spec']['template']['spec']['template']['spec']
    containers = {row['name']: row for row in pod['containers']}
    for name in ('server', 'results'):
        assert containers[name]['lifecycle']['preStop']['exec']['command'] == ['/app/match', 'drain']
    env = {row['name']: row for row in containers['results']['env']}
    assert 'MATCH_DEV_INSECURE' not in env
    assert env['MATCH_INGEST_CA_FILE']['value'] == '/ingest-trust/ca.crt'
    trust = next(row for row in pod['volumes'] if row['name'] == 'ingest-trust')
    assert trust['secret']['secretName'] == 'ingest-trust'
    assert trust['secret']['items'] == [dict(key='ca.crt', path='ca.crt')]
print('PASS: isolated stateless ingest deployment, TLS, read-only Agones RBAC and HPA')
