#!/usr/bin/env python3
"""Check fresh invocation roots and inherited subprocess temporary directories."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
code = '''import json,os,tempfile
from tools.scratch import ROOT
with tempfile.TemporaryDirectory() as child:
 print(json.dumps(dict(root=str(ROOT),child=child,env=os.environ['AFTERSHOCK_SCRATCH'],tmp=os.environ['TMPDIR'])))
'''
env = dict(os.environ)
env.pop('AFTERSHOCK_SCRATCH',None)
children = [subprocess.Popen([sys.executable,'-c',code],cwd=ROOT,env=env,stdout=subprocess.PIPE,text=True) for _ in range(2)]
roots = []
try:
    for child in children:
        output,_ = child.communicate(timeout=10)
        assert child.returncode == 0
        report = json.loads(output)
        root = Path(report['root'])
        roots.append(root)
        assert root.is_dir() and Path(report['child']).is_relative_to(root)
        assert report['root'] == report['env'] == report['tmp']
    assert roots[0] != roots[1], roots
    explicit = roots[0]/'nested root'
    output = subprocess.check_output([sys.executable,'-c',code],cwd=ROOT,
                                     env=dict(env,AFTERSHOCK_SCRATCH=str(explicit)),text=True)
    report = json.loads(output)
    assert Path(report['root']) == explicit and Path(report['child']).is_relative_to(explicit)
finally:
    for child in children:
        if child.poll() is None:
            child.kill()
            child.wait()
    for root in roots:
        shutil.rmtree(root)
print('PASS: concurrent fresh roots, explicit root creation and inherited temporary directories')
