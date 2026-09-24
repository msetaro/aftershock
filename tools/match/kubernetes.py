#!/usr/bin/env python3
"""Generate a namespace and warm Fleet, with an optional local-development ingest stub."""
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
parser.add_argument('--production-ingest', action='store_true', help='use TLS ingest; omit the development stub and static tokens')
parser.add_argument('--ingest-ca-secret', help='CA-only secret containing ca.crt for private ingest trust')
args=parser.parse_args()
namespace=args.namespace
if not re.fullmatch(r'[a-z][a-z0-9-]{0,61}[a-z0-9]',namespace):parser.error('namespace must be a DNS label of 2..63 characters')
if args.ingest_ca_secret and (not args.production_ingest or not re.fullmatch(r'[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?', args.ingest_ca_secret)):
    parser.error('--ingest-ca-secret requires production ingest and a DNS label')
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
if args.production_ingest:
    shipper['env'] = [row for row in shipper['env'] if row['name'] != 'MATCH_DEV_INSECURE']
    for workload in (server, shipper):
        workload['lifecycle'] = dict(preStop=dict(exec=dict(command=['/app/match', 'drain'])))
    if args.ingest_ca_secret:
        volumes.append(dict(name='ingest-trust', secret=dict(secretName=args.ingest_ca_secret,
            items=[dict(key='ca.crt', path='ca.crt')], defaultMode=0o444)))
        shipper['volumeMounts'] = shared+[dict(name='ingest-trust', mountPath='/ingest-trust', readOnly=True)]
        shipper['env'].append(dict(name='MATCH_INGEST_CA_FILE', value='/ingest-trust/ca.crt'))
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
if args.production_ingest:
    objects = [obj for obj in objects if obj['kind'] in ('Namespace', 'Fleet')]
for name,value in [('resources.json',dict(apiVersion='v1',kind='List',items=objects)),('allocation.json',allocation),('match.json',spec)]:
    (args.output/name).write_text(json.dumps(value,indent=2)+'\n')
print(args.output/'resources.json')
