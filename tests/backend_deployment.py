#!/usr/bin/env python3
"""Validate namespaced backend deployment ownership, secrets and autoscaling."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from run import ROOT,SCRATCH

with tempfile.TemporaryDirectory(prefix='aftershock-backend-deployment-',dir=SCRATCH) as temporary:
    output=Path(temporary)/'resources.json'
    subprocess.run([sys.executable,ROOT/'tools/match/backend_kubernetes.py','--namespace','backend-test',
                    '--image','aftershock-match:issue29','--output',output],check=True)
    objects=json.loads(output.read_text())['items']
    by_kind={item['kind']:item for item in objects}
    assert len(objects)==len(by_kind)==7
    assert all(item['metadata']['namespace']=='backend-test' for item in objects)
    deployment=by_kind['Deployment']['spec']
    assert deployment['replicas']==2 and deployment['strategy']['rollingUpdate']['maxUnavailable']==0
    pod=deployment['template']['spec']
    assert pod['serviceAccountName']=='backend' and pod['securityContext']['fsGroup']==65532
    container,=pod['containers']
    assert container['args']==['backend'] and container['securityContext']['runAsNonRoot']
    assert container['securityContext']['readOnlyRootFilesystem'] and not container['securityContext']['allowPrivilegeEscalation']
    env={row['name']:row for row in container['env']}
    assert env['BACKEND_DATABASE']['valueFrom']['secretKeyRef']['key']=='database'
    assert env['BACKEND_APP_ID']['valueFrom']['secretKeyRef']['key']=='app-id'
    assert env['BACKEND_TLS_KEY']['value']=='/config/tls.key'
    assert env['BACKEND_READER_KEY_FILE']['value']=='/config/reader.key'
    assert 'BACKEND_RESULTS_DEV_INSECURE' not in env
    assert container['readinessProbe']['httpGet']['scheme']=='HTTPS'
    assert container['resources']['requests']['cpu'] and container['resources']['limits']['memory']
    rules=by_kind['Role']['rules']
    assert rules==[dict(apiGroups=['agones.dev'],resources=['gameservers'],verbs=['get','list']),
                   dict(apiGroups=['allocation.agones.dev'],resources=['gameserverallocations'],verbs=['create'])]
    assert by_kind['Service']['spec']['type']=='ClusterIP'
    hpa=by_kind['HorizontalPodAutoscaler']['spec']
    assert hpa['minReplicas']==2 and hpa['maxReplicas']==10 and hpa['scaleTargetRef']['name']=='backend'
    assert by_kind['PodDisruptionBudget']['spec']['minAvailable']==1
    assert 'Secret' not in by_kind and 'ClusterRole' not in by_kind
print('PASS: backend namespaced deployment, secret references, TLS probes and HPA')
