#!/usr/bin/env python3
"""Check shipping exclusion, shared panel controls, idle allocation and input release."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile

from PIL import Image
from run import ROOT, build, content_maps
sys.path.insert(0, str(ROOT))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path)
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--binary', type=Path, help='test an existing development client')
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='aftershock-devtools-', dir=os.environ.get('AFTERSHOCK_SCRATCH')) as temporary:
    output = args.output.resolve() if args.output else Path(temporary)
    output.mkdir(parents=True, exist_ok=True)
    if not args.binary:
        for enabled in (False, True):
            directory = build(output / ('enabled' if enabled else 'shipping'), ['BUILD_SERVER=0', f'AFTERSHOCK_DEVTOOLS={int(enabled)}'])
            binary = directory / 'quake3e.x64'
            symbols = subprocess.check_output(['nm', '-C', '--defined-only', binary], text=True)
            (directory / 'symbols.txt').write_text(symbols)
            for required in ('ImGui::NewFrame()', 'DevTools_Draw(', ' Dev_DrawLine\n', 'DevTools_AgentRequest(', 'Sys_AgentRead('):
                assert (required in symbols) == enabled, (required, enabled)
            assert enabled or not re.search(r'\b(?:ImGui::|DevTools_|Dev_|Sys_Agent|G_DevWeapon|G_DevAnimation)', symbols), 'shipping binary contains tooling'
            print('PASS:', 'development tooling linked' if enabled else 'shipping binary excludes tooling', flush=True)
        args.binary = binary
    out = output / 'runtime'
    out.mkdir(exist_ok=True)
    models = set()
    for pak in args.data.glob('*.pk3'):
        with zipfile.ZipFile(pak) as archive:
            models.update(name for name in archive.namelist()
                          if name.startswith('models/players/') and name.endswith('/lower.md3'))
    assert models, 'installed player model required'
    with Engine(args.binary, args.data, args.content, arguments=['+set', 'devtest', '0']) as engine:
        engine.request('session', dt=8, seed=123)
        engine.request('map', name=content_maps(args.content)[0])
        engine.step(200)
        engine.request('panel', name='Cvars')
        engine.request('cvar.set', name='devtest', value='7')
        engine.step(160)
        assert engine.request('cvar.get', name='devtest')['value'] == '7'
        before = engine.request('editor.state')
        engine.step(80)
        after = engine.request('editor.state')
        assert after['frames'] - before['frames'] >= 80, (before, after)
        assert after['allocations'] == before['allocations'], 'idle UI allocated'
        assert 0 < after['arena'] < 16777216 and after['enabled'], after
        profile = engine.request('profile')
        assert profile['cpu'] and profile['network']['snapshots'] > 0, profile
        engine.request('exec', command='vid_restart')
        engine.step(60)
        assert engine.request('editor.state')['frames'] > after['frames']
        assert engine.request('cvar.get', name='devtest')['value'] == '7'
        for panel in ('Cvars', 'Textures', 'Materials', 'Profile', 'Memory', 'Animation'):
            engine.request('panel', name=panel)
            if panel == 'Animation':
                engine.request('animation.load', path=sorted(models)[0])
                engine.step(3)
                engine.request('animation.set', field='play', value=1)
            engine.step(3)
            state = engine.request('editor.state')
            assert state['panel'] == panel, state
            if panel == 'Animation':
                engine.step(20)
                later = engine.request('editor.state')['animation']
                assert later['frame'] != state['animation']['frame'] and later['previews'] > state['animation']['previews'] > 0
            capture = engine.request('capture', name=panel.lower())
            engine.step(2)
            with Image.open(engine.base / capture['path']) as image:
                assert image.size == (640, 480)
            shutil.copyfile(engine.base / capture['path'], out / (panel.lower() + '.png'))
        # Use the normal key dispatch without an OS pointer or screen coordinates.
        engine.request('exec', command='set +devbutton 0; set -devbutton 0; bind F8 "+devbutton"; bind F9 "set dev_tools 1"; set dev_tools 0')
        engine.step(2)
        engine.request('key', name='F8', down=True)
        engine.step(2)
        assert engine.request('cvar.get', name='+devbutton')['value'] != '0'
        engine.request('key', name='F9', down=True)
        engine.request('key', name='F9', down=False)
        engine.step(3)
        assert engine.request('cvar.get', name='dev_tools')['value'] == '1'
        assert engine.request('cvar.get', name='-devbutton')['value'] != '0', 'reopening UI left a game key pressed'
        engine.request('key', name='F8', down=False)
        engine.step(2)
        shutil.copyfile(engine.log_path, out / 'client.log')
print('PASS: shared cvar/console controls, held-key release, animation, bounded memory, allocation-free idle and renderer restart')
