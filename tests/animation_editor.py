#!/usr/bin/env python3
"""Edit a graph through shared panel commands, recook it and preview the change."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from run import ROOT, build, content_maps
from window import wait_for
sys.path.insert(0, str(ROOT))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path)
parser.add_argument('--output', type=Path)
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='aftershock-animation-editor-', dir=os.environ.get('AFTERSHOCK_SCRATCH')) as temporary:
    output = args.output.resolve() if args.output else Path(temporary)
    output.mkdir(parents=True, exist_ok=True)
    binary = args.binary or build(output / 'build', ['BUILD_SERVER=0', 'AFTERSHOCK_DEVTOOLS=1']) / 'quake3e.x64'
    home = Path(temporary) / 'home'
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    source = base / 'animation_source'
    shutil.copytree(ROOT / 'tests/assets/animation', source)
    graph = source / 'rifle.animation.json'
    original = graph.read_bytes()
    document = json.loads(original)
    with (output / 'watch.log').open('wb') as watch_log:
        watcher = subprocess.Popen([sys.executable, 'tools/cook', str(source / 'rigs.json'), '--output', str(base), '--watch'],
                                   cwd=ROOT, stdout=watch_log, stderr=subprocess.STDOUT)
        try:
            wait_for(lambda: (base / 'cook.revision').is_file(), watcher)
            revision = (base / 'cook.revision').read_bytes()
            with Engine(binary, args.data, args.content, home=home) as engine:
                engine.request('session', dt=8, seed=123)
                engine.request('map', name=content_maps(args.content)[0])
                engine.step(200)
                engine.request('panel', name='Graph')
                engine.request('graph', action='load', text='animations/anim_rifle.asanim')
                engine.request('graph', action='source', text='animation_source/rifle.animation.json')
                engine.step(3)
                state = engine.request('editor.state')
                assert state['panel'] == 'Graph' and state['graph']['state'] == 'idle', state
                assert state['graph']['previews'] > 0 and not state['graph']['dirty'], state
                document['initial_state'] = 'ads'
                edited = json.dumps(document, indent=2) + '\n'
                engine.request('graph', action='text', text=edited)
                assert engine.request('editor.state')['graph']['dirty']
                engine.request('graph', action='undo')
                assert not engine.request('editor.state')['graph']['dirty']
                engine.request('graph', action='text', text=edited)
                engine.request('graph', action='save')
                engine.step(2)
                assert json.loads(graph.read_text())['initial_state'] == 'ads'
                backups = list(source.glob('rifle.animation.json.bak.*'))
                assert len(backups) == 1 and backups[0].read_bytes() == original
                assert not engine.request('editor.state')['graph']['dirty']
                assert engine.request('editor.state')['graph']['result'] == 'saved'
                wait_for(lambda: (base / 'cook.revision').read_bytes() != revision, watcher)
                engine.request('graph', action='load', text='animations/anim_rifle.asanim')
                engine.step(3)
                state = engine.request('editor.state')['graph']
                assert state['state'] == 'ads' and state['previews'] > 0, state
                engine.request('graph', action='parameter', text='ads', value=1)
                engine.request('graph', action='play', value=1)
                engine.step(20)
                assert engine.request('editor.state')['graph']['time'] > 0
                engine.request('graph', action='play', value=0)
                engine.request('graph', action='reset')
                assert engine.request('editor.state')['graph']['time'] == 0
                capture = engine.request('capture', name='animation_editor')
                engine.step(2)
                shutil.copyfile(base / capture['path'], output / 'animation_editor.png')
                shutil.copyfile(engine.log_path, output / 'client.log')
        finally:
            watcher.terminate()
            try:
                watcher.wait(timeout=5)
            except subprocess.TimeoutExpired:
                watcher.kill()
                watcher.wait(timeout=5)
print('PASS: shared graph source edit/undo, retained backup, watched cook and changed panel preview')
