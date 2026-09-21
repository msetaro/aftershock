#!/usr/bin/env python3
"""Require headless level reports, named/fly-through PNGs and deterministic repeats."""
import argparse
import copy
import hashlib
import json
import os
import re
from pathlib import Path
from run import SCRATCH
import shutil
import subprocess
import sys
import tempfile

from run import ROOT

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--client',type=Path,required=True)
parser.add_argument('--server',type=Path,required=True)
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output',type=Path,default=(SCRATCH / 'aftershock-level-validation'))
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True,exist_ok=True)
sys.path.insert(0,str(ROOT/'tools/level'))
from headless import stuck_bots
assert stuck_bots({0:[((0,0,0),True)]*10})[0]['client']==0
assert stuck_bots({0:[((i*32,0,0),True) for i in range(10)]})==[]
assert stuck_bots({0:[((0,0,0),True)]*5+[((0,0,0),False)]+[((0,0,0),True)]*4})==[]
fixture = ROOT/'tests/assets/levels'
source = json.loads((fixture/'two_lane.json').read_text())
source['viewpoints'] = [
    {'id':'west','origin':[-160,0,96],'angles':[0,0,0]},
    {'id':'east','origin':[1696,0,128],'angles':[10,180,0]}]
with tempfile.TemporaryDirectory(prefix='aftershock-validation-test-') as temporary:
    project = Path(temporary)
    shutil.copytree(fixture/'assets',project/'assets')
    path = project/'level.json'

    def validate(document,name,failed=None):
        path.write_text(json.dumps(document))
        output = args.output/name
        result = subprocess.run([sys.executable,str(ROOT/'tools/level'),'validate',str(path),
                                 '--output',str(output),'--client',str(args.client.resolve()),
                                 '--server',str(args.server.resolve()),'--content',args.content,
                                 '--data',str(args.data.resolve()),'--bot-frames','6000'],
                                cwd=ROOT,capture_output=True,text=True,env=dict(os.environ,DISPLAY=''))
        assert result.stdout.strip().startswith('{'), (result.returncode,result.stdout,result.stderr)
        report = json.loads(result.stdout)
        assert report['version']==1 and report==json.loads((output/'report.json').read_text())
        assert (output/'report.txt').is_file()
        if failed:
            assert result.returncode and report['status']=='failed'
            assert any(failed in error.lower() for error in report['errors']), report
            return report
        assert result.returncode==0 and report['status']=='passed', report
        assert not report['errors'] and report['structure']['spawn_reachability']['reachable']==4
        assert report['structure']['entities']['worldspawn']==1
        assert report['structure']['lightmaps']['pages']>0
        assert report['structure']['lightmaps']['lit_surfaces']>0
        assert report['structure']['navigation']['backend']=='aas'
        assert report['bots']['kills']>=2 and report['bots']['pickups']>=2
        assert report['bots']['stuck']==[] and report['bots']['samples']>=100
        assert {v['name'] for v in report['views']}=={'west','east'}
        assert len(report['flythrough'])>=5
        images = {}
        for view in report['views']+report['flythrough']:
            image = output/view['image']
            data = image.read_bytes()
            assert data.startswith(b'\x89PNG\r\n\x1a\n')
            assert hashlib.sha256(data).hexdigest()==view['sha256']
            assert view['metrics']['draw_calls']>0 and view['metrics']['triangles']>0
            images[view['name']] = data
        assert len(set(images.values()))>=5
        return report,images

    first,images = validate(source,'first')
    second,repeated = validate(source,'second')
    assert images==repeated, 'fixed-camera PNGs vary between identical runs'
    assert [v['metrics'] for v in first['views']+first['flythrough']]==[v['metrics'] for v in second['views']+second['flythrough']]
    bad = copy.deepcopy(source)
    bad['connections'] = bad['connections'][:2]
    validate(bad,'unreachable','unreachable spawn')
    bad = copy.deepcopy(source)
    bad['viewpoints'][0]['origin'] = [10000,0,96]
    validate(bad,'outside-view','viewpoint outside')
    # A structural leak is valid MAP syntax with the west room's ceiling omitted.
    text = (ROOT/'tests/golden/levels/two_lane.map').read_text()
    brushes = re.findall(r'\{\n(?:\([^\n]+\n){6}\}\n',text)
    assert len(brushes)>6
    leak = project/'leak.map'
    leak.write_text(text.replace(brushes[1],'',1))
    result = subprocess.run([sys.executable,str(ROOT/'tools/level'),'validate',str(leak),
                             '--output',str(args.output/'leak'),'--client',str(args.client.resolve()),
                             '--server',str(args.server.resolve()),'--content',args.content,'--data',str(args.data.resolve())],
                            cwd=ROOT,capture_output=True,text=True)
    report = json.loads(result.stdout)
    assert result.returncode and report['status']=='failed' and any('leak' in e.lower() for e in report['errors']), report
print('PASS: one-command headless reports, deterministic named/fly-through PNGs, bot movement and design failures')
