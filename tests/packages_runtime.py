#!/usr/bin/env python3
"""Mount owned packages, patches and removals through the real filesystem."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from run import ROOT, SCRATCH
sys.path.insert(0, str(ROOT))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
parser.add_argument('--server', type=Path, help='also prove package reads during a pure client/server session')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-packages-runtime')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-packages-') as temporary:
    root = Path(temporary)
    home, install, shared = [root/name for name in ('user', 'game', 'engine')]
    game = 'baseoa' if args.content == 'openarena' else 'baseq3'
    for directory in (home/game, install/game, shared/'engine'):
        directory.mkdir(parents=True)
    source = root/'source'
    source.mkdir()

    def pack(directory, name, files, bases=()):
        stage = source/name
        stage.mkdir()
        for path, text in files.items():
            target = stage/path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(text)
        target = directory/(name+'.aspack')
        command = [sys.executable, 'tools/package.py', 'build', '--root', str(stage), '--output', str(target)]
        for base in bases:
            command += ['--base', str(base)]
        subprocess.run(command, cwd=ROOT, check=True, capture_output=True)
        return target

    shared_files = {'package-engine.cfg': 'set package_engine shared\n'}
    shared_pack = pack(shared/'engine', '00-engine', shared_files)
    game_files = {'package-order.cfg': 'set package_order base\n',
                  'package-removed.cfg': 'set package_removed base\n'}
    base_pack = pack(install/game, '00-base', game_files)
    patched = dict(shared_files, **game_files)
    patched['package-order.cfg'] = 'set package_order patched\n'
    del patched['package-removed.cfg']
    pack(install/game, '10-patch', patched, (shared_pack, base_pack))
    # A lower loose file must not reappear behind a package removal.
    (install/game/'package-removed.cfg').write_text('set package_removed leaked\n')
    pack(home/game, '20-dlc', {'package-dlc.cfg': 'set package_dlc additional\n'})
    arguments = ['+set', 'fs_basepath', str(install), '+set', 'fs_enginepath', str(shared)]
    with Engine(args.binary, args.data, args.content, home=home, arguments=arguments) as engine:
        try:
            engine.request('session', dt=20, seed=20)
            def execute(command):
                engine.request('exec', command=command)
                engine.step(3)
            def value(name):
                return engine.request('cvar.get', name=name)['value']
            for name in ('package_engine', 'package_order', 'package_dlc'):
                execute(f'set {name} missing')
            for _ in range(2):
                execute('exec package-engine.cfg; exec package-order.cfg; exec package-dlc.cfg')
                assert value('package_engine') == 'shared', 'engine data mount is missing'
                assert value('package_order') == 'patched', 'patch must override its base'
                assert value('package_dlc') == 'additional', 'additional package must mount'
                execute('set package_removed absent')
                execute('exec package-removed.cfg')
                assert value('package_removed') == 'absent', 'removal must hide package and loose fallback'
                execute('fs_restart')
            execute('writeconfig package-user.cfg')
            assert (home/game/'package-user.cfg').is_file(), 'user writes must stay in the user directory'
            assert not (install/game/'package-user.cfg').exists()
            assert not (shared/'engine/package-user.cfg').exists()
            engine.request('map', name='oa_dm1' if args.content == 'openarena' else 'q3dm7')
            engine.step(20)
            assert engine.request('state')['player']['health'] > 0, 'legacy pk3 content must remain playable'
            print('PASS: engine/game/user roots, patch/removal/DLC mounts, restart and legacy pk3 gameplay')
        finally:
            shutil.copyfile(engine.log_path, args.output/'engine.log')

    mod = home/'package_mod'
    mod.mkdir()
    pack(mod, '00-mod', {'package-order.cfg': 'set package_order mod\n'})
    with Engine(args.binary, args.data, args.content, home=home, arguments=[
            *arguments, '+set', 'fs_basegame', game, '+set', 'fs_game', 'package_mod']) as engine:
        try:
            engine.request('session', dt=20, seed=20)
            engine.request('exec', command='exec package-order.cfg; writeconfig package-mod-user.cfg')
            engine.step(3)
            assert engine.request('cvar.get', name='package_order')['value'] == 'mod'
            assert (mod/'package-mod-user.cfg').is_file()
            assert not (install/'package_mod/package-mod-user.cfg').exists()
            print('PASS: explicit mod package overrides game/patch/DLC and owns only user writes')
        finally:
            shutil.copyfile(engine.log_path, args.output/'mod.log')

# Ship the owned textured character as one package, then mount only its small
# texture delta. Separate processes ensure no loose file or renderer cache helps.
from cook import cook
from PIL import Image, ImageChops
with tempfile.TemporaryDirectory(prefix='aftershock-package-character-') as temporary:
    root = Path(temporary)
    source, cooked, install = root/'source', root/'cooked', root/'game'
    shutil.copytree(ROOT/'tests/assets/cook-character', source)
    cook(source/'assets.json', cooked)
    (cooked/'package-pure.cfg').write_text('echo package_pure_mounted\n')
    destination = install/game
    destination.mkdir(parents=True)
    base = destination/'00-character.aspack'

    def build_package(output, *bases):
        command = [sys.executable, 'tools/package.py', 'build', '--root', str(cooked), '--output', str(output)]
        for baseline in bases:
            command += ['--base', str(baseline)]
        result = subprocess.run(command, cwd=ROOT, check=True, capture_output=True, text=True)
        return json.loads(result.stdout)

    build_package(base)
    images = []
    for version in ('base', 'patched'):
        if version == 'patched':
            Image.new('RGBA', (16, 16), (32, 240, 64, 255)).save(source/'character.png')
            changed = cook(source/'assets.json', cooked)
            assert len(changed['built']) == 1, changed
            patch = destination/'10-texture.aspack'
            manifest = build_package(patch, base)
            assert len(manifest['assets']) == 3, manifest
            assert patch.stat().st_size < base.stat().st_size//2
            (args.output/'sizes.json').write_text(json.dumps(dict(base=base.stat().st_size, patch=patch.stat().st_size)))
        with Engine(args.binary, args.data, args.content, home=root/version, arguments=[
                '+set', 'fs_basepath', str(install)]) as engine:
            try:
                engine.request('session', dt=20, seed=20)
                engine.request('map', name='oa_dm1' if args.content == 'openarena' else 'q3dm7')
                engine.step(100)
                engine.request('cvar.set', name='timescale', value='0')
                engine.request('panel', name='Animation')
                engine.request('animation.load', path='models/character.iqm')
                engine.step(3)
                assets = engine.request('assets', kind='images', filter='models/character')['items']
                assert any(asset['format'] == 8 for asset in assets), 'packaged BC7 texture must reach the GPU'
                capture = engine.request('capture', name='package-'+version)
                engine.step(2)
                state = engine.request('editor.state')['animation']
                assert state['model'] > 0
                x, y, width, height = state['viewport']
                path = engine.base/capture['path']
                with Image.open(path) as image:
                    images.append(image.convert('RGB').crop((x, y, x+width, y+height)))
                shutil.copyfile(path, args.output/(version+'.png'))
            finally:
                shutil.copyfile(engine.log_path, args.output/(version+'.log'))
    difference = ImageChops.difference(*images).tobytes()
    changed = sum(max(difference[i:i+3]) > 40 for i in range(0, len(difference), 3))
    assert changed > 200, changed
    print(f'PASS: one cooked character package renders; small texture delta changes {changed} preview pixels')
    if args.server:
        subprocess.run([sys.executable, 'tests/native_pure_runtime.py', '--client', str(args.binary),
                        '--server', str(args.server), '--content', args.content, '--data', str(args.data),
                        '--package', str(base), '--package', str(patch), '--output', str(args.output/'pure')],
                       cwd=ROOT, check=True)
