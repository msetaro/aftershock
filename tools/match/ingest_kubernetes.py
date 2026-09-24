#!/usr/bin/env python3
"""Generate stateless ingest resources; operators own the separate results database and TLS secret."""
import argparse
import json
from pathlib import Path
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--namespace', required=True)
parser.add_argument('--image', required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--secret', default='ingest-config')
args = parser.parse_args()
for name in (args.namespace, args.secret):
    if not re.fullmatch(r'[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?', name):
        parser.error('namespace/secret must be DNS labels')
if args.output.exists():
    parser.error('choose a new output file')
meta = dict(name='ingest', namespace=args.namespace)
labels = dict(app='ingest')
def resource(api, kind, **fields):
    return dict(apiVersion=api, kind=kind, metadata=meta, **fields)
environment = [dict(name='INGEST_DATABASE', valueFrom=dict(secretKeyRef=dict(name=args.secret, key='database'))),
               dict(name='INGEST_NAMESPACE', valueFrom=dict(fieldRef=dict(fieldPath='metadata.namespace')))]
for name, value in dict(INGEST_LISTEN=':50051', INGEST_HTTP_LISTEN=':8444',
                       INGEST_TLS_CERT='/config/tls.crt', INGEST_TLS_KEY='/config/tls.key',
                       INGEST_READER_KEY_FILE='/config/reader.key').items():
    environment.append(dict(name=name, value=value))
security = dict(runAsNonRoot=True, runAsUser=65532, runAsGroup=65532, readOnlyRootFilesystem=True,
                allowPrivilegeEscalation=False, capabilities=dict(drop=['ALL']),
                seccompProfile=dict(type='RuntimeDefault'))
container = dict(name='ingest', image=args.image, imagePullPolicy='IfNotPresent', args=['ingest'],
                 env=environment, securityContext=security,
                 ports=[dict(name='grpc', containerPort=50051), dict(name='https', containerPort=8444)],
                 resources=dict(requests=dict(cpu='100m', memory='64Mi'), limits=dict(cpu='500m', memory='256Mi')),
                 readinessProbe=dict(httpGet=dict(path='/healthz', port='https', scheme='HTTPS'),
                                     periodSeconds=5, timeoutSeconds=2, failureThreshold=6),
                 volumeMounts=[dict(name='config', mountPath='/config', readOnly=True)])
objects = [
    resource('v1', 'ServiceAccount'),
    resource('rbac.authorization.k8s.io/v1', 'Role', rules=[
        dict(apiGroups=['agones.dev'], resources=['gameservers'], verbs=['get', 'list'])]),
    resource('rbac.authorization.k8s.io/v1', 'RoleBinding',
             subjects=[dict(kind='ServiceAccount', name='ingest', namespace=args.namespace)],
             roleRef=dict(apiGroup='rbac.authorization.k8s.io', kind='Role', name='ingest')),
    resource('apps/v1', 'Deployment', spec=dict(replicas=2, selector=dict(matchLabels=labels),
        strategy=dict(type='RollingUpdate', rollingUpdate=dict(maxUnavailable=0, maxSurge=1)),
        template=dict(metadata=dict(labels=labels), spec=dict(serviceAccountName='ingest',
            securityContext=dict(fsGroup=65532), terminationGracePeriodSeconds=30,
            containers=[container], volumes=[dict(name='config', secret=dict(secretName=args.secret, defaultMode=0o440))])))),
    resource('v1', 'Service', spec=dict(type='ClusterIP', selector=labels,
        ports=[dict(name='grpc', port=50051, targetPort='grpc'), dict(name='https', port=8444, targetPort='https')])),
    resource('autoscaling/v2', 'HorizontalPodAutoscaler', spec=dict(
        scaleTargetRef=dict(apiVersion='apps/v1', kind='Deployment', name='ingest'),
        minReplicas=2, maxReplicas=10, metrics=[dict(type='Resource', resource=dict(name='cpu',
            target=dict(type='Utilization', averageUtilization=65)))])),
    resource('policy/v1', 'PodDisruptionBudget', spec=dict(minAvailable=1, selector=dict(matchLabels=labels))),
]
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(dict(apiVersion='v1', kind='List', items=objects), indent=2)+'\n')
print(args.output)
