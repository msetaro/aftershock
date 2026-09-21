#!/usr/bin/env python3
"""Generate a private kind acceptance namespace: warm Fleet, ingest stub and allocation."""
import argparse
import base64
import json
from pathlib import Path
import secrets
import re

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--image',default='aftershock-match:issue28')
parser.add_argument('--namespace',default='aftershock-'+secrets.token_hex(6))
parser.add_argument('--ci-content',help='optional node-local OpenArena test root (never added to the image)')
args=parser.parse_args()
namespace=args.namespace
if not re.fullmatch(r'[a-z][a-z0-9-]{0,61}[a-z0-9]',namespace):parser.error('namespace must be a DNS label of 2..63 characters')
if args.output.exists():parser.error('choose a new output directory')
args.output.mkdir(parents=True,mode=0o700)
spec=dict(id='kind-'+secrets.token_hex(8),map='two_lane',mode=0,frag_limit=0,time_limit=1,players=2,
          password=secrets.token_hex(16),token=secrets.token_hex(16))
security=dict(runAsNonRoot=True,runAsUser=65532,runAsGroup=65532,readOnlyRootFilesystem=True,
              allowPrivilegeEscalation=False,capabilities=dict(drop=['ALL']),seccompProfile=dict(type='RuntimeDefault'))
resources=dict(requests=dict(cpu='50m',memory='32Mi'),limits=dict(cpu='500m',memory='192Mi'))
def container(name,mode):
    return dict(name=name,image=args.image,imagePullPolicy='Never',args=[mode],securityContext=security,resources=resources)
backend=container('ingest','stub')
backend.update(env=[dict(name='MATCH_DEV_INSECURE',value='1'),dict(name='MATCH_TOKENS',valueFrom=dict(secretKeyRef=dict(name='ingest-tokens',key='tokens')))],
               ports=[dict(name='grpc',containerPort=50051)],volumeMounts=[dict(name='results',mountPath='/state')],
               readinessProbe=dict(tcpSocket=dict(port='grpc'),periodSeconds=2))
server=container('server','run');shipper=container('results','ship')
shared=[dict(name='home',mountPath='/home/match')]
server.update(ports=[dict(name='game',containerPort=27960,protocol='UDP')],volumeMounts=shared)
shipper.update(volumeMounts=shared,env=[dict(name='MATCH_DEV_INSECURE',value='1'),dict(name='MATCH_INGEST',value='ingest:50051')])
volumes=[dict(name='home',emptyDir=dict(sizeLimit='256Mi'))]
if args.ci_content:
    path=Path(args.ci_content)
    if not path.is_absolute() or '..' in path.parts:parser.error('--ci-content must be an absolute node path')
    volumes.append(dict(name='test-content',hostPath=dict(path=str(path),type='Directory')))
    server['volumeMounts']=shared+[dict(name='test-content',mountPath='/data',readOnly=True)]
    server['env']=[dict(name='MATCH_CONTENT',value='/data'),dict(name='MATCH_GAME',value='baseoa')]
    shipper['env'].append(dict(name='MATCH_GAME',value='baseoa'))
objects=[
 dict(apiVersion='v1',kind='Namespace',metadata=dict(name=namespace)),
 dict(apiVersion='v1',kind='Secret',metadata=dict(name='ingest-tokens',namespace=namespace),type='Opaque',
      data=dict(tokens=base64.b64encode(json.dumps({spec['id']:spec['token']}).encode()).decode())),
 dict(apiVersion='apps/v1',kind='Deployment',metadata=dict(name='ingest',namespace=namespace),spec=dict(replicas=1,
      selector=dict(matchLabels=dict(app='ingest')),template=dict(metadata=dict(labels=dict(app='ingest')),
      spec=dict(automountServiceAccountToken=False,securityContext=dict(fsGroup=65532),containers=[backend],volumes=[dict(name='results',emptyDir=dict(sizeLimit='256Mi'))])))),
 dict(apiVersion='v1',kind='Service',metadata=dict(name='ingest',namespace=namespace),spec=dict(selector=dict(app='ingest'),ports=[dict(port=50051,targetPort='grpc')])),
 dict(apiVersion='agones.dev/v1',kind='Fleet',metadata=dict(name='aftershock',namespace=namespace),spec=dict(replicas=1,scheduling='Packed',
      template=dict(spec=dict(container='server',ports=[dict(name='game',containerPort=27960,protocol='UDP',portPolicy='Dynamic')],
      health=dict(initialDelaySeconds=60,periodSeconds=5,failureThreshold=6),template=dict(spec=dict(securityContext=dict(fsGroup=65532),
      terminationGracePeriodSeconds=75,containers=[server,shipper],volumes=volumes))))))]
allocation=dict(apiVersion='allocation.agones.dev/v1',kind='GameServerAllocation',metadata=dict(namespace=namespace),
                spec=dict(selectors=[dict(matchLabels={'agones.dev/fleet':'aftershock'})],
                          metadata=dict(annotations={'aftershock.dev/match':json.dumps(spec)})))
for name,value in [('resources.json',dict(apiVersion='v1',kind='List',items=objects)),('allocation.json',allocation),('match.json',spec)]:
    (args.output/name).write_text(json.dumps(value,indent=2)+'\n')
print(args.output/'resources.json')
