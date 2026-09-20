#!/usr/bin/env python3
"""Require opt-in native match completion to exit, while normal servers keep running."""
import argparse
import os
from pathlib import Path
import subprocess
import signal
import time
import tempfile

from run import ROOT, content_bots, content_maps, content_settings

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--server',type=Path,required=True)
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output',type=Path,default=Path('/tmp/aftershock-match-exit'))
args = parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
paks = sorted(args.data.resolve().glob('*.pk3'))
assert paks, 'installed game content is required'
for enabled in (0,1):
    with tempfile.TemporaryDirectory(prefix='aftershock-match-exit-') as temporary:
        home = Path(temporary)
        base = home/('baseoa' if args.content=='openarena' else 'baseq3')
        base.mkdir()
        for pak in paks:
            (base/pak.name).symlink_to(pak)
        commands = ['set g_synchronousClients 1','set fixedtime 20','set sv_fps 50','set fraglimit 1',
                    f'set sv_exitOnMatchEnd {enabled}',f'map {content_maps(args.content)[0]}',
                    *[f'addbot {bot} 4' for bot in content_bots(args.content)],
                    'echo match_smoke_started']
        (base/'match-exit.cfg').write_text('\n'.join(commands)+'\n')
        log = args.output/f'exit-{enabled}.log'
        with log.open('w') as stream:
            process = subprocess.Popen(['faketime','-f','@2026-01-01 00:00:00 i0.01',str(args.server.resolve()),
                                        '+set','fs_basepath',str(home),'+set','fs_homepath',str(home),*content_settings(args.content),
                                        '+set','dedicated','1','+set','net_enabled','0','+set','sv_pure','0','+exec','match-exit.cfg'],
                                       cwd=ROOT,env=dict(os.environ,LC_ALL='C'),stdout=stream,stderr=subprocess.STDOUT,
                                       stdin=subprocess.PIPE,text=True,start_new_session=True)
            try:
                deadline = time.monotonic()+10
                while process.poll() is None and time.monotonic()<deadline:
                    if not enabled and 'Exit: Fraglimit hit.' in log.read_text():
                        time.sleep(0.25)
                        assert process.poll() is None, 'default server exited after the match'
                        process.stdin.write('echo match_smoke_deadline\nquit\n')
                        process.stdin.flush()
                        break
                    time.sleep(0.025)
                if enabled:
                    assert process.poll() is not None, 'opt-in match completion did not exit'
                assert process.wait(timeout=5)==0
            finally:
                if process.poll() is None:
                    os.killpg(process.pid,signal.SIGTERM)
                    try:
                        process.wait(timeout=3)
                    except subprocess.TimeoutExpired:
                        os.killpg(process.pid,signal.SIGKILL)
                        process.wait(timeout=3)
        text = log.read_text()
        assert 'Kill:' in text and 'Exit: Fraglimit hit.' in text and 'Static game loaded.' in text
        assert ('match_smoke_deadline' not in text)==bool(enabled), (enabled,'wrong process lifetime')
        assert ('Match complete: exiting server' in text)==bool(enabled), (enabled,'completion diagnostic')
        assert 'Server Shutdown (Server quit)' in text
print('PASS: native match completion exits only when requested; default lifetime is preserved')
