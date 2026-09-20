#!/usr/bin/env python3
"""Private kind Fleet: Ready, allocate, native player, match exit, durable ingest, replacement."""
import argparse
import json
import os
from pathlib import Path
import shutil
import signal
import socket
import subprocess
import sys
import tarfile
import tempfile
import time
import uuid

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/match'))
from toolchain import NODE,tool
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--image',default='aftershock-match:issue28')
parser.add_argument('--client',type=Path,required=True)
parser.add_argument('--data',type=Path,default=Path('/tmp/aftershock-openarena-baseoa'))
parser.add_argument('--output',type=Path,default=Path('/tmp/aftershock-match-kind'))
args=parser.parse_args()
if args.output.exists():parser.error('choose a new output directory')
args.output.mkdir(parents=True,mode=0o700)
output=args.output.resolve()
docker=shutil.which('docker');assert docker,'Docker is required'
kind,kubectl,helm,chart=[tool(name) for name in ('kind','kubectl','helm','agones-1.60.0.tgz')]
cluster='aftershock-'+uuid.uuid4().hex[:10]
kubeconfig=output/'kubeconfig'
ctl=[kubectl,'--kubeconfig',str(kubeconfig),'-n','aftershock-match']
processes=[];streams=[];created=False

def command(arguments,**kwargs):
    kwargs.setdefault('timeout',30)
    return subprocess.run([str(x) for x in arguments],check=True,**kwargs)
def document(arguments):
    return json.loads(command(arguments,stdout=subprocess.PIPE,text=True).stdout)
def log_run(name,arguments,**kwargs):
    with (output/name).open('w') as stream:
        return command(arguments,stdout=stream,stderr=subprocess.STDOUT,**kwargs)
def wait_for(check,seconds,label):
    deadline=time.monotonic()+seconds
    while time.monotonic()<deadline:
        value=check()
        if value:return value
        time.sleep(1)
    raise AssertionError(label+' timed out')
def follow(name,arguments,environment=None):
    stream=(output/name).open('w');streams.append(stream)
    process=subprocess.Popen([str(x) for x in arguments],env=environment,stdout=stream,stderr=subprocess.STDOUT,start_new_session=True)
    processes.append(process);return process

try:
    # These ports are exposed only on loopback, including Docker Desktop's VM boundary.
    reservations=[]
    for _ in range(2):
        sock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);sock.bind(('127.0.0.1',0));reservations.append(sock)
    ports=[sock.getsockname()[1] for sock in reservations]
    config=dict(kind='Cluster',apiVersion='kind.x-k8s.io/v1alpha4',nodes=[dict(role='control-plane',
                extraPortMappings=[dict(containerPort=7000+i,hostPort=port,listenAddress='127.0.0.1',protocol='UDP') for i,port in enumerate(ports)])])
    (output/'kind.json').write_text(json.dumps(config))
    for sock in reservations:sock.close()
    created=True
    log_run('create.log',[kind,'create','cluster','--name',cluster,'--image',NODE,'--config',output/'kind.json','--kubeconfig',kubeconfig,'--wait','60s'],timeout=240)
    log_run('image.log',[kind,'load','docker-image',args.image,'--name',cluster],timeout=120)
    # Generate manifest metadata before Helm so namespace-specific SDK RBAC exists.
    manifests=output/'manifests'
    command([sys.executable,ROOT/'tools/match/kubernetes.py','--image',args.image,'--output',manifests,'--ci-content','/aftershock-ci'])
    log_run('namespace.log',[*ctl,'create','namespace','aftershock-match'])
    log_run('agones.log',[helm,'install','agones',chart,'--namespace','agones-system','--create-namespace','--kubeconfig',kubeconfig,
        '--server-side=false','--set','agones.allocator.install=false','--set','agones.ping.install=false','--set','agones.controller.replicas=1',
        '--set','agones.controller.numWorkers=2','--set','agones.image.sdk.memoryRequest=32Mi','--set','agones.image.sdk.memoryLimit=128Mi',
        '--set','gameservers.namespaces[0]=aftershock-match','--set','gameservers.minPort=7000',
        '--set','gameservers.maxPort=7001','--wait','--timeout','180s'],timeout=240)
    node=cluster+'-control-plane'
    with tempfile.TemporaryDirectory(prefix='aftershock-match-client-') as temporary:
        content=Path(temporary)
        command([sys.executable,ROOT/'tools/match/content.py',content])
        base=content/'baseoa';base.mkdir()
        paks=sorted(args.data.resolve().glob('*.pk3'));assert len(paks)>=8,'complete OpenArena paks are required'
        for pak in paks:(base/pak.name).symlink_to(pak)
        (base/'zz-aftershock-level.pk3').symlink_to(content/'aftershock/pak0.pk3')
        command([docker,'exec',node,'mkdir','-p','/aftershock-ci'])
        # Resolve every public OA pak explicitly. Docker cp of a directory retains
        # nested symlinks, whose host paths do not exist inside a kind node.
        with tempfile.TemporaryFile() as archive:
            with tarfile.open(fileobj=archive,mode='w') as tar:
                for pak in sorted(base.glob('*.pk3')):tar.add(pak.resolve(),arcname='baseoa/'+pak.name)
            archive.seek(0)
            command([docker,'exec','-i',node,'tar','-xf','-','-C','/aftershock-ci'],stdin=archive,timeout=120)
        log_run('resources.log',[*ctl,'apply','-f',manifests/'resources.json'])
        def ready():
            servers=document([*ctl,'get','gameservers','-o','json'])['items']
            return next((g['metadata']['name'] for g in servers if g.get('status',{}).get('state')=='Ready'),None)
        original=wait_for(ready,120,'warm Fleet Ready')
        follow('server.log',[*ctl,'logs','-f',original,'-c','server'])
        follow('ship.log',[*ctl,'logs','-f',original,'-c','results'])
        allocation=document([*ctl,'create','-f',manifests/'allocation.json','-o','json'])['status']
        assert allocation['state']=='Allocated' and allocation['gameServerName']==original
        allocated_port=allocation['ports'][0]['port'];assert allocated_port in (7000,7001)
        spec=json.loads((manifests/'match.json').read_text())
        wait_for(lambda:'allocated match=' in (output/'server.log').read_text(),20,'match spec application')
        icds=list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'));assert len(icds)==1
        environment=dict(os.environ,LC_ALL='C',LP_NUM_THREADS='1',VK_DRIVER_FILES=str(icds[0]),VK_ICD_FILENAMES=str(icds[0]))
        client=follow('client.log',['xvfb-run','-a',args.client.resolve(),'+set','fs_basepath',content,'+set','fs_homepath',content/'client',
            '+set','fs_basegame','baseoa','+set','net_port','0','+set','r_mode','3','+set','r_fullscreen','0','+set','s_initsound','0',
            '+set','cl_allowDownload','0','+set','cl_autoRecordDemo','0','+set','com_maxfps','20',
            '+set','password',spec['password'],'+connect','127.0.0.1:'+str(ports[allocated_port-7000])],environment)
        wait_for(lambda:'ClientBegin: 0' in (output/'server.log').read_text(),30,'native player acceptance')
        def sample():
            rows=document([docker,'exec',node,'crictl','stats','--output','json'])['stats']
            return {row['attributes']['metadata']['name']:row for row in rows
                    if row['attributes']['labels'].get('io.kubernetes.pod.name')==original}
        pod=document([*ctl,'get','pod',original,'-o','json'])
        containers=pod['spec']['containers']+[c for c in pod['spec'].get('initContainers',[]) if c.get('restartPolicy')=='Always']
        assert {c['name'] for c in containers}=={'server','results','agones-gameserver-sidecar'}
        requests=[c['resources']['requests'] for c in containers]
        requested_cores=sum(float(r['cpu'][:-1])/1000 if r['cpu'].endswith('m') else float(r['cpu']) for r in requests)
        def memory_bytes(value):
            for suffix,scale in [('Ki',1024),('Mi',1024**2),('Gi',1024**3)]:
                if value.endswith(suffix):return float(value[:-2])*scale
            return float(value)
        requested_memory=sum(memory_bytes(r.get('memory','0')) for r in requests)
        first=sample()
        time.sleep(20)
        second=sample()
        assert set(first)==set(second)=={'server','results','agones-gameserver-sidecar'}
        cores=sum((int(second[name]['cpu']['usageCoreNanoSeconds']['value'])-int(first[name]['cpu']['usageCoreNanoSeconds']['value'])) /
                  (int(second[name]['cpu']['timestamp'])-int(first[name]['cpu']['timestamp'])) for name in first)
        memory=sum(int(row['memory']['workingSetBytes']['value']) for row in second.values())
        assert cores>0 and memory>0
        density=dict(scenario='one connected native player; includes engine wrapper, results and Agones SDK; excludes shared ingest/control plane',
                     seconds=20,measured_vcpus=cores,working_set_bytes=memory,
                     resource_equivalent_matches_per_vcpu=1/cores,resource_equivalent_matches_per_gb=1_000_000_000/memory,
                     limitation='single-match resource measurement, not a saturation or worst-case capacity guarantee')
        density.update(requested_vcpus=requested_cores,requested_memory_bytes=requested_memory,
                       request_budget_matches_per_vcpu=1/requested_cores,request_budget_matches_per_gb=1_000_000_000/requested_memory)
        (output/'density.json').write_text(json.dumps(density,indent=2)+'\n')
        (output/'density-samples.json').write_text(json.dumps(dict(first=first,second=second),indent=2)+'\n')
        def results():
            data=command([*ctl,'exec','deployment/ingest','-c','ingest','--','/app/match','records'],stdout=subprocess.PIPE,text=True).stdout
            rows=[json.loads(line) for line in data.splitlines()]
            final=next((row for row in rows if row['match']==spec['id'] and row['final']),None)
            if final:
                (output/'events.jsonl').write_text(data)
                assert final['checkpoint']['completed'] and final['checkpoint']['joins']>=1
                return final
        final=wait_for(results,100,'final durable result')
        replacement=wait_for(lambda:(name if (name:=ready()) and name!=original else None),90,'warm replacement after SDK Shutdown')
        assert 'Static cgame loaded.' in (output/'client.log').read_text()
        assert 'Unpure client' not in (output/'server.log').read_text()
        report=dict(match=spec['id'],original=original,replacement=replacement,final_offset=final['end'],checkpoint=final['checkpoint'])
        (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        print('PASS: kind warm Fleet -> allocation -> native player -> match completion -> durable gRPC result -> replacement')
finally:
    for p in reversed(processes):
        if p.poll() is None:
            os.killpg(p.pid,signal.SIGTERM)
            try:p.wait(timeout=5)
            except subprocess.TimeoutExpired:os.killpg(p.pid,signal.SIGKILL);p.wait(timeout=5)
    for stream in streams:stream.close()
    if created:
        # This invocation owns the random cluster and its private kubeconfig only.
        log_run('delete.log',[kind,'delete','cluster','--name',cluster,'--kubeconfig',kubeconfig],timeout=90)
