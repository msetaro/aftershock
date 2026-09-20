#!/usr/bin/env python3
"""Generate a local Compose project with one ingest stub and N independent matches."""
import argparse
import json
from pathlib import Path
import secrets

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--matches',type=int,default=2)
parser.add_argument('--image',default='aftershock-match:issue28')
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--port',type=int,default=27960)
args=parser.parse_args()
if not 1<=args.matches<=32 or not 1024<=args.port<=65535-args.matches:
    parser.error('require 1..32 matches and an unprivileged port range')
if args.output.exists():parser.error('choose a new output directory; match homes must not be reused')
args.output.mkdir(parents=True,mode=0o700)
base=dict(image=args.image,read_only=True,user='65532:65532',cap_drop=['ALL'],
          security_opt=['no-new-privileges:true'],pids_limit=64,mem_limit='192m',cpus='0.5')
services={};volumes={'results':{}};tokens={};specs=[]
for i in range(args.matches):
    name=f'match-{i+1}'
    spec=dict(id=name,map='two_lane',mode=0,frag_limit=10,time_limit=1,players=8,
              password=secrets.token_hex(16),token=secrets.token_hex(16))
    specs.append(spec);tokens[name]=spec['token'];volumes[name]={}
    services[name]=dict(base,command=['run'],environment=dict(MATCH_SPEC=json.dumps(spec)),
                       volumes=[name+':/home/match'],ports=[f'127.0.0.1:{args.port+i}:27960/udp'],
                       depends_on=['ingest',name+'-ship'])
    services[name+'-ship']=dict(base,command=['ship'],environment=dict(MATCH_INGEST='ingest:50051',MATCH_DEV_INSECURE='1'),
                              volumes=[name+':/home/match'],depends_on=['ingest'])
services['ingest']=dict(base,command=['stub'],environment=dict(MATCH_DEV_INSECURE='1',MATCH_TOKENS=json.dumps(tokens)),
                        volumes=['results:/state'])
compose=dict(services=services,volumes=volumes)
(args.output/'compose.json').write_text(json.dumps(compose,indent=2)+'\n')
(args.output/'matches.json').write_text(json.dumps(specs,indent=2)+'\n')
print(args.output/'compose.json')
