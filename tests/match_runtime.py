#!/usr/bin/env python3
"""Real match process, native client, separate gRPC shipper and durable ingest stub."""
import argparse
import json
import os
from pathlib import Path
from run import SCRATCH
import shutil
import signal
import socket
import subprocess
import tempfile
import time

ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--controller',type=Path,required=True)
parser.add_argument('--server',type=Path,required=True)
parser.add_argument('--client',type=Path)
parser.add_argument('--data',type=Path,default=(SCRATCH / 'aftershock-openarena-baseoa'))
parser.add_argument('--output',type=Path,default=(SCRATCH / 'aftershock-match-runtime'))
args=parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
def free_port(kind):
    with socket.socket(socket.AF_INET,kind) as sock:
        sock.bind(('127.0.0.1',0))
        return sock.getsockname()[1]
def stop(process):
    if process.poll() is None:
        os.killpg(process.pid,signal.SIGTERM)
        try: process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid,signal.SIGKILL)
            process.wait(timeout=5)
with tempfile.TemporaryDirectory(prefix='aftershock-match-live-') as temporary:
    root=Path(temporary)
    content=root/'content'
    subprocess.run(['python3',str(ROOT/'tools/match/content.py'),str(content)],check=True)
    game='baseoa' if args.client else 'aftershock'
    if args.client:
        paks=sorted(args.data.resolve().glob('*.pk3'))
        assert paks,'installed OpenArena content is required for the native client'
        (content/game).mkdir()
        for pak in paks:(content/game/pak.name).symlink_to(pak)
        (content/game/'zz-aftershock-level.pk3').symlink_to(content/'aftershock/pak0.pk3')
    home=root/'home';home.mkdir()
    state=root/'state';state.mkdir()
    spec=dict(id='local-1',map='two_lane',mode=0,frag_limit=0,time_limit=1,players=2,password='local-secret',token='allocation-secret')
    port,ingest=free_port(socket.SOCK_DGRAM),free_port(socket.SOCK_STREAM)
    env=dict(os.environ,MATCH_HOME=str(home),MATCH_CONTENT=str(content),MATCH_GAME=game,
             MATCH_SERVER=str(args.server.resolve()),MATCH_SPEC=json.dumps(spec),MATCH_PORT=str(port),
             MATCH_INGEST=f'127.0.0.1:{ingest}',MATCH_LISTEN=f'127.0.0.1:{ingest}',MATCH_DEV_INSECURE='1',
             MATCH_TOKENS=json.dumps({'local-1':'allocation-secret'}),MATCH_STATE=str(state),LC_ALL='C')
    processes=[];streams=[]
    def launch(name,command,environment=env):
        stream=(args.output/(name+'.log')).open('w');streams.append(stream)
        p=subprocess.Popen(command,env=environment,stdout=stream,stderr=subprocess.STDOUT,start_new_session=True)
        processes.append(p);return p
    try:
        stub=launch('ingest',[str(args.controller.resolve()),'stub'])
        ship=launch('ship',[str(args.controller.resolve()),'ship'])
        server=launch('server',[str(args.controller.resolve()),'run'])
        deadline=time.monotonic()+20
        while 'ready match=' not in (args.output/'server.log').read_text():
            assert server.poll() is None, (args.output/'server.log').read_text()
            assert time.monotonic()<deadline,'server never became ready'
            time.sleep(.1)
        if args.client:
            icds=list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'));assert len(icds)==1
            client_env=dict(os.environ,LC_ALL='C',LP_NUM_THREADS='1',VK_DRIVER_FILES=str(icds[0]),VK_ICD_FILENAMES=str(icds[0]))
            client=launch('client',['xvfb-run','-a',str(args.client.resolve()),
                '+set','fs_basepath',str(content),'+set','fs_homepath',str(root/'client'),'+set','fs_basegame',game,
                '+set','net_port','0','+set','r_mode','3','+set','r_fullscreen','0','+set','s_initsound','0',
                '+set','cl_allowDownload','0','+set','cl_autoRecordDemo','0','+set','com_maxfps','20',
                '+set','password',spec['password'],'+connect',f'127.0.0.1:{port}'],client_env)
            deadline=time.monotonic()+25
            while 'ClientBegin: 0' not in (args.output/'server.log').read_text():
                assert client.poll() is None,'native client exited'
                assert time.monotonic()<deadline,'native client never joined'
                time.sleep(.1)
        assert server.wait(timeout=90)==0,(args.output/'server.log').read_text()
        assert ship.wait(timeout=10)==0,(args.output/'ship.log').read_text()
        batches=[json.loads(line) for line in (state/'events.jsonl').read_text().splitlines()]
        assert batches and batches[-1]['final'] and batches[-1]['checkpoint']['completed']
        assert any('Exit: Timelimit hit.' in line for b in batches for line in b['events'])
        if args.client:
            assert batches[-1]['checkpoint']['joins']>=1
            assert 'Static cgame loaded.' in (args.output/'client.log').read_text()
        for name in ('ack.json','results.done','engine.done'):
            shutil.copyfile(home/name,args.output/name)
        shutil.copyfile(state/'events.jsonl',args.output/'events.jsonl')
    finally:
        for p in reversed(processes):stop(p)
        for stream in streams:stream.close()
print('PASS: match-spec launch, loaded-map readiness, native match exit and acknowledged gRPC events/checkpoint')
