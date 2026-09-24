#!/usr/bin/env python3
"""Generate backend resources; operators provision the referenced credentials/TLS secret."""
import argparse
import json
from pathlib import Path
import re

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--namespace',required=True)
parser.add_argument('--image',required=True)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--secret',default='backend-config')
parser.add_argument('--extra-ca',action='store_true',help='load /config/ca.crt in addition to system trust')
parser.add_argument('--development-results',action='store_true',help='local #28 stub only: plaintext results transport')
args=parser.parse_args()
for name in (args.namespace,args.secret):
    if not re.fullmatch(r'[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?',name):parser.error('namespace/secret must be DNS labels')
if args.output.exists():parser.error('choose a new output file')
meta=dict(name='backend',namespace=args.namespace)
labels=dict(app='backend')
def resource(api,kind,**fields):
    return dict(apiVersion=api,kind=kind,metadata=meta,**fields)
def credential(key):
    return dict(secretKeyRef=dict(name=args.secret,key=key))
environment=[dict(name='BACKEND_DATABASE',valueFrom=credential('database')),
             dict(name='BACKEND_APP_ID',valueFrom=credential('app-id')),
             dict(name='BACKEND_NAMESPACE',valueFrom=dict(fieldRef=dict(fieldPath='metadata.namespace')))]
for name,value in dict(BACKEND_LISTEN=':8443',BACKEND_TLS_CERT='/config/tls.crt',BACKEND_TLS_KEY='/config/tls.key',
                       BACKEND_STEAM_KEY_FILE='/config/publisher.key',BACKEND_CATALOG='/config/catalog.json',
                       BACKEND_READER_KEY_FILE='/config/reader.key',BACKEND_RESULTS='ingest:50051').items():
    environment.append(dict(name=name,value=value))
if args.extra_ca:environment.append(dict(name='BACKEND_CA_FILE',value='/config/ca.crt'))
if args.development_results:environment.append(dict(name='BACKEND_RESULTS_DEV_INSECURE',value='1'))
security=dict(runAsNonRoot=True,runAsUser=65532,runAsGroup=65532,readOnlyRootFilesystem=True,
              allowPrivilegeEscalation=False,capabilities=dict(drop=['ALL']),seccompProfile=dict(type='RuntimeDefault'))
probe=dict(httpGet=dict(path='/healthz',port='https',scheme='HTTPS'),periodSeconds=5,timeoutSeconds=2,failureThreshold=6)
# Keep TLS available while terminating endpoints propagate; reserve another
# five seconds for backend.go's HTTP shutdown inside the total grace budget.
container=dict(name='backend',image=args.image,imagePullPolicy='IfNotPresent',args=['backend'],env=environment,
               securityContext=security,ports=[dict(name='https',containerPort=8443)],
               resources=dict(requests=dict(cpu='100m',memory='64Mi'),limits=dict(cpu='500m',memory='256Mi')),
               lifecycle=dict(preStop=dict(sleep=dict(seconds=10))),
               readinessProbe=probe,volumeMounts=[dict(name='config',mountPath='/config',readOnly=True)])
objects=[
 resource('v1','ServiceAccount'),
 resource('rbac.authorization.k8s.io/v1','Role',rules=[
     dict(apiGroups=['agones.dev'],resources=['gameservers'],verbs=['get','list']),
     dict(apiGroups=['allocation.agones.dev'],resources=['gameserverallocations'],verbs=['create'])]),
 resource('rbac.authorization.k8s.io/v1','RoleBinding',subjects=[dict(kind='ServiceAccount',name='backend',namespace=args.namespace)],
          roleRef=dict(apiGroup='rbac.authorization.k8s.io',kind='Role',name='backend')),
 resource('apps/v1','Deployment',spec=dict(replicas=2,selector=dict(matchLabels=labels),
     strategy=dict(type='RollingUpdate',rollingUpdate=dict(maxUnavailable=0,maxSurge=1)),
     template=dict(metadata=dict(labels=labels),spec=dict(serviceAccountName='backend',securityContext=dict(fsGroup=65532),
         terminationGracePeriodSeconds=20,containers=[container],volumes=[dict(name='config',secret=dict(secretName=args.secret,defaultMode=0o440))])))),
 resource('v1','Service',spec=dict(type='ClusterIP',selector=labels,ports=[dict(name='https',port=8443,targetPort='https')])),
 resource('autoscaling/v2','HorizontalPodAutoscaler',spec=dict(scaleTargetRef=dict(apiVersion='apps/v1',kind='Deployment',name='backend'),
     minReplicas=2,maxReplicas=10,metrics=[dict(type='Resource',resource=dict(name='cpu',target=dict(type='Utilization',averageUtilization=65)))])),
 resource('policy/v1','PodDisruptionBudget',spec=dict(minAvailable=1,selector=dict(matchLabels=labels)))]
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_text(json.dumps(dict(apiVersion='v1',kind='List',items=objects),indent=2)+'\n')
print(args.output)
