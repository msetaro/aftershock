#!/usr/bin/env python3
"""Mount owned packages, patches and removals through the real filesystem."""
import argparse
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
            def execute(command):
                engine.request('exec', command=command)
                engine.step(3)
            def value(name):
                return engine.request('cvar.get', name=name)['value']
            for name in ('package_engine', 'package_order', 'package_dlc'):
                engine.request('cvar.set', name=name, value='missing')
            for _ in range(2):
                execute('exec package-engine.cfg; exec package-order.cfg; exec package-dlc.cfg')
                assert value('package_engine') == 'shared', 'engine data mount is missing'
                assert value('package_order') == 'patched', 'patch must override its base'
                assert value('package_dlc') == 'additional', 'additional package must mount'
                engine.request('cvar.set', name='package_removed', value='absent')
                execute('exec package-removed.cfg')
                assert value('package_removed') == 'absent', 'removal must hide package and loose fallback'
                execute('fs_restart')
            execute('writeconfig package-user.cfg')
            assert (home/game/'package-user.cfg').is_file(), 'user writes must stay in the user directory'
            assert not (install/game/'package-user.cfg').exists()
            assert not (shared/'engine/package-user.cfg').exists()
            engine.request('session', dt=20, seed=20)
            engine.request('map', name='oa_dm1' if args.content == 'openarena' else 'q3dm7')
            engine.step(20)
            assert engine.request('state')['player']['health'] > 0, 'legacy pk3 content must remain playable'
            print('PASS: engine/game/user roots, patch/removal/DLC mounts, restart and legacy pk3 gameplay')
        finally:
            shutil.copyfile(engine.log_path, args.output/'engine.log')
