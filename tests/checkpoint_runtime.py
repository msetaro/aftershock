#!/usr/bin/env python3
"""Restore a paused local game in-place and in a fresh process, then continue it."""
import argparse
import hashlib
import gzip
import struct
import subprocess
import json
from pathlib import Path
import shutil
import sys
import tempfile
from run import ROOT, SCRATCH, content_maps
sys.path.insert(0,str(ROOT))
from tools.agent import Engine

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary',type=Path,required=True)
parser.add_argument('--record-v1-fixture',action='store_true',help='create the missing OpenArena v1 full-game fixture once')
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-checkpoint-runtime')
args=parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
fixture=ROOT/'tests/assets/state/checkpoint-v1.asstate.gz'
metadata=fixture.with_name('checkpoint-v1.json')
if args.record_v1_fixture:
    assert args.content=='openarena', 'only redistributable OpenArena state may become a fixture'
    assert not fixture.exists() and not metadata.exists(), 'accepted migration fixtures are immutable'


def execute(engine,command):
    engine.request('exec',command=command)
    engine.step(2)

def load(engine,path):
    before=engine.log_path.read_text(errors='replace').count('Game loaded: ')
    engine.request('exec',command='loadgame '+path)
    # The local transport must reconnect, but the saved world stays frozen until
    # its first snapshot is installed. Completion consumes no simulation tick.
    for _ in range(200):
        engine.step(1)
        log=engine.log_path.read_text(errors='replace')
        if log.count('Game loaded: ')>before:
            return
        assert 'Checkpoint owner reconstruction failed' not in log, log[-4000:]
    raise AssertionError('checkpoint reconnect did not finish: '+log[-4000:])

def toggle_pause(engine):
    engine.request('key',name='ESCAPE',down=True)
    engine.request('key',name='ESCAPE',down=False)
    engine.step(2)

def entities(engine):
    rows=[]
    offset=0
    while offset is not None:
        page=engine.request('entity.list',offset=offset,limit=32)
        rows.extend(page['entities'])
        offset=page['next']
    return rows

def assert_entities(engine,expected,label):
    actual=entities(engine)
    if actual!=expected:
        (args.output/(label+'.json')).write_text(json.dumps(dict(expected=expected,actual=actual),indent=2)+'\n')
        differences=[(a,b) for a,b in zip(expected,actual) if a!=b]
        raise AssertionError(f'{label}: entity counts {len(expected)}/{len(actual)}, first differences {differences[:3]}')

with tempfile.TemporaryDirectory(prefix='aftershock-checkpoint-') as temporary:
    home=Path(temporary)
    base=home/('baseoa' if args.content=='openarena' else 'baseq3')
    with Engine(args.binary,args.data,args.content,home=home,arguments=['+set','sv_maxclients','4']) as engine:
        try:
            engine.request('session',dt=20,seed=19)
            engine.request('map',name=content_maps(args.content)[0])
            engine.step(50)
            execute(engine,'addbot sarge 3')
            engine.step(100)
            assert any(row['entity']==1 and row['classname']=='player' for row in entities(engine)), 'checkpoint must include a live bot'
            execute(engine,'give all')
            toggle_pause(engine)
            assert engine.request('cvar.get',name='sv_paused')['value']=='1'
            before=entities(engine)
            execute(engine,'savegame acceptance')
            first=base/'saves/acceptance.000.asstate'
            assert first.is_file(), 'savegame must write a full local-game checkpoint'
            fixture_bytes=first.read_bytes()
            digest=hashlib.sha256(fixture_bytes).hexdigest()
            execute(engine,'savegame acceptance')
            assert (base/'saves/acceptance.001.asstate').is_file()
            assert hashlib.sha256(first.read_bytes()).hexdigest()==digest, 'later saves must preserve earlier revisions'
            toggle_pause(engine)
            engine.step(25)
            continued=entities(engine)
            assert continued!=before, 'bot and simulation must advance after the save'
            load(engine,'saves/acceptance.000.asstate')
            assert_entities(engine,before,'same-process-restore')
            toggle_pause(engine)
            engine.step(25)
            assert_entities(engine,continued,'same-process-continuation')
            print('PASS: full paused world restore, live bot continuation and preserved previous revision')
        finally:
            shutil.copyfile(engine.log_path,args.output/'checkpoint.log')
    with Engine(args.binary,args.data,args.content,home=home) as engine:
        try:
            engine.request('session',dt=20,seed=123)
            load(engine,'saves/acceptance.000.asstate')
            assert_entities(engine,before,'fresh-process-restore')
            toggle_pause(engine)
            engine.step(25)
            assert_entities(engine,continued,'fresh-process-continuation')
            print('PASS: fresh-process world and deterministic continuation')
            if args.record_v1_fixture:
                result=engine.request('capture',name='checkpoint-review')
                engine.step(2)
                from PIL import Image
                Image.open(base/result['path']).save(args.output/'checkpoint-review.png')
        finally:
            shutil.copyfile(engine.log_path,args.output/'checkpoint-restart.log')
    (args.output/'result.json').write_text(json.dumps(dict(before=before,continued=continued),indent=2)+'\n')

if args.record_v1_fixture:
    assert fixture_bytes[:8]==b'ASARCH\0\0'
    assert fixture_bytes[108:140].rstrip(b'\0')==b'checkpoint'
    assert struct.unpack_from('<I',fixture_bytes,140)[0]==1, 'record with the version-1 writer'
    compressed=gzip.compress(fixture_bytes,mtime=0)
    provenance=dict(writer_commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
                    binary_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest(),
                    raw_sha256=digest,gzip_sha256=hashlib.sha256(compressed).hexdigest(),
                    bytes=len(fixture_bytes),content='openarena',map=content_maps(args.content)[0],
                    format_version=1,checkpoint_version=1,dt=20,seed=19,
                    before=before,continued=continued)
    with fixture.open('xb') as output:
        output.write(compressed)
    with metadata.open('x') as output:
        json.dump(provenance,output,indent=2);output.write('\n')
    print('CREATED frozen OpenArena version-1 checkpoint:',fixture)
