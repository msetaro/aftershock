#!/usr/bin/env python3
"""Restore a paused local game in-place and in a fresh process, then continue it."""
import argparse
import hashlib
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
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-checkpoint-runtime')
args=parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)

def execute(engine,command):
    engine.request('exec',command=command)
    engine.step(2)

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
            digest=hashlib.sha256(first.read_bytes()).hexdigest()
            execute(engine,'savegame acceptance')
            assert (base/'saves/acceptance.001.asstate').is_file()
            assert hashlib.sha256(first.read_bytes()).hexdigest()==digest, 'later saves must preserve earlier revisions'
            toggle_pause(engine)
            engine.step(25)
            continued=entities(engine)
            assert continued!=before, 'bot and simulation must advance after the save'
            execute(engine,'loadgame saves/acceptance.000.asstate')
            assert entities(engine)==before, 'load must restore the paused world and bot slots exactly'
            toggle_pause(engine)
            engine.step(25)
            assert entities(engine)==continued, 'restored game must continue identically'
            print('PASS: full paused world restore, live bot continuation and preserved previous revision')
        finally:
            shutil.copyfile(engine.log_path,args.output/'checkpoint.log')
    with Engine(args.binary,args.data,args.content,home=home) as engine:
        try:
            engine.request('session',dt=20,seed=123)
            execute(engine,'loadgame saves/acceptance.000.asstate')
            assert entities(engine)==before, 'fresh process must rebuild map, references and paused state'
            toggle_pause(engine)
            engine.step(25)
            assert entities(engine)==continued, 'fresh-process continuation must preserve saved simulation and RNG state'
            print('PASS: fresh-process world and deterministic continuation')
        finally:
            shutil.copyfile(engine.log_path,args.output/'checkpoint-restart.log')
    (args.output/'result.json').write_text(json.dumps(dict(before=before,continued=continued),indent=2)+'\n')
