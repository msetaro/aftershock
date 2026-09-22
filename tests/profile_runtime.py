#!/usr/bin/env python3
"""Save and reload archived settings and bindings in an isolated real client home."""
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import sys
import tempfile
from run import ROOT, SCRATCH
sys.path.insert(0,str(ROOT))
from tools.agent import Engine

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary',type=Path,required=True)
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-profile-runtime')
args=parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-profile-') as temporary:
    home=Path(temporary)
    base=home/('baseoa' if args.content=='openarena' else 'baseq3')
    with Engine(args.binary,args.data,args.content,home=home) as engine:
        try:
            engine.request('session',dt=20,seed=19)
            def execute(command):
                engine.request('exec',command=command)
                engine.step(2)
            engine.request('cvar.set',name='s_volume',value='0.31')
            execute('bind F8 "+attack"')
            execute('saveprofile acceptance')
            first=base/'profiles/acceptance.000.asstate'
            assert first.is_file(), 'saveprofile must write a versioned user-data file'
            original=hashlib.sha256(first.read_bytes()).hexdigest()
            engine.request('cvar.set',name='s_volume',value='0.85')
            execute('bind F8 "+back"')
            execute('saveprofile acceptance')
            assert (base/'profiles/acceptance.001.asstate').is_file(), 'a later save must keep a new revision'
            assert hashlib.sha256(first.read_bytes()).hexdigest()==original, 'earlier settings must not be overwritten'
            execute('loadprofile profiles/acceptance.000.asstate')
            assert float(engine.request('cvar.get',name='s_volume')['value'])==0.31
            execute('bind F8')
            assert re.findall(r'"F8" = "([^\"]*)"',engine.log_path.read_text())[-1]=='+attack'
            print('PASS: versioned archived settings, actual bindings and preserved prior revision')
        finally:
            shutil.copyfile(engine.log_path,args.output/'profile.log')
    with Engine(args.binary,args.data,args.content,home=home) as engine:
        try:
            engine.request('session',dt=20,seed=19)
            engine.request('cvar.set',name='s_volume',value='0.9')
            engine.request('exec',command='loadprofile profiles/acceptance.001.asstate')
            engine.step(2)
            assert float(engine.request('cvar.get',name='s_volume')['value'])==0.85
            engine.request('exec',command='bind F8')
            engine.step(2)
            assert re.findall(r'"F8" = "([^\"]*)"',engine.log_path.read_text())[-1]=='+back'
            print('PASS: saved profile loads in a fresh client process')
        finally:
            shutil.copyfile(engine.log_path,args.output/'profile-restart.log')
