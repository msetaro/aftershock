#!/usr/bin/env python3
"""Exercise the one-command playtest driver, captures and failing assertions."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from PIL import Image
from run import ROOT, content_maps

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='aftershock-agent-cli-', dir=os.environ.get('AFTERSHOCK_SCRATCH')) as temporary:
    root = Path(temporary)
    source = root/'playtest.json'
    script = dict(version=1, dt=20, seed=123, cvars={'g_rewind': '1'}, steps=[
        {'op': 'capture', 'name': 'start'},
        {'op': 'walk', 'offset': [32, 0, 0], 'tolerance': 8, 'max_frames': 100},
        {'op': 'capture', 'name': 'walked'},
        {'op': 'request', 'request': {'op': 'exec', 'command': 'give all; weapon 2; rewind_target 0'}},
        {'op': 'step', 'frames': 30},
        {'op': 'fire', 'frames': 300, 'target': {'classname': 'rewind_target'}},
        {'op': 'capture', 'name': 'fired'},
        {'op': 'assert', 'metric': 'hits', 'min': 1},
        {'op': 'assert', 'metric': 'errors', 'max': 0},
        {'op': 'assert', 'metric': 'asserts', 'max': 0},
        {'op': 'assert', 'metric': 'p99_ms', 'max': 1000}])
    source.write_text(json.dumps(script))
    command = [sys.executable, 'tools/agent', 'run', '--binary', str(args.binary),
               '--map', content_maps(args.content)[0], '--data', str(args.data),
               '--content', args.content, '--script', str(source)]
    result = subprocess.run([*command, '--out', str(root/'out')], cwd=ROOT, capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    report = json.loads((root/'out/report.json').read_text())
    assert report['ok'] and report['profile']['events']['hits'] >= 1, report
    assert len(report['captures']) == 3 and report['profile']['samples'] > 300
    for name in report['captures']:
        with Image.open(root/'out'/name) as image:
            assert image.format == 'PNG' and image.size == (640, 480)
    start = report['results'][0]['state']['player']['origin']
    walked = report['results'][2]['state']['player']['origin']
    assert abs(walked[0] - start[0]) >= 24, (start, walked)
    script['steps'] = [{'op': 'assert', 'metric': 'hits', 'min': 1000000}]
    source.write_text(json.dumps(script))
    result = subprocess.run([*command, '--out', str(root/'failed')], cwd=ROOT, capture_output=True, text=True)
    failed = json.loads((root/'failed/report.json').read_text())
    assert result.returncode == 1 and not failed['ok'] and 'hits' in failed['error']['hint'], failed
    script['steps'] = [{'op': 'walk', 'offset': ['bad', 0, 0]}]
    source.write_text(json.dumps(script))
    result = subprocess.run([*command, '--out', str(root/'invalid')], cwd=ROOT, capture_output=True, text=True)
    error = json.loads(result.stderr)
    assert result.returncode == 1 and error['error']['file'] == str(source)
    assert 'steps' in error['error']['path'] and 'number' in error['error']['hint'], error
print('PASS: CLI route, aimed fire, hit/p99 assertions, three PNGs and actionable failures')
