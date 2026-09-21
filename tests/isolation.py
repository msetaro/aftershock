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

with tempfile.TemporaryDirectory(prefix='aftershock-compose-isolation-') as temporary:
    projects = []
    for side in ('a','b'):
        output = Path(temporary)/side/'same-name'
        subprocess.run([sys.executable,'tools/match/dev.py','--matches','2','--output',str(output)],cwd=ROOT,
                       check=True,stdout=subprocess.DEVNULL)
        project = json.loads((output/'compose.json').read_text())
        projects.append(project)
        assert all(service['ports'] == ['127.0.0.1::27960/udp'] for name,service in project['services'].items()
                   if name.startswith('match-') and not name.endswith('-ship'))
    assert all(project.get('name') for project in projects) and projects[0]['name'] != projects[1]['name']
print('PASS: private Compose names and OS-assigned published ports')

# Pinned compiler/tool archives may be reused, but installation cannot overlap.
with tempfile.TemporaryDirectory(prefix='aftershock-cache-lock-') as temporary:
    source = '''import sys
from pathlib import Path
from tools.scratch import cache_lock
with cache_lock(Path(sys.argv[1])):
 print('locked',flush=True)
 sys.stdin.readline()
'''
    import select
    first = subprocess.Popen([sys.executable,'-c',source,temporary],cwd=ROOT,stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
    second = None
    try:
        assert first.stdout.readline() == 'locked\n'
        second = subprocess.Popen([sys.executable,'-c',source,temporary],cwd=ROOT,stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
        assert not select.select([second.stdout],[],[],.2)[0], 'second installer entered while the first held its lock'
        first.communicate('\n',timeout=10)
        assert second.stdout.readline() == 'locked\n'
        second.communicate('\n',timeout=10)
        assert first.returncode == second.returncode == 0
    finally:
        for child in (first,second):
            if child and child.poll() is None:
                child.kill()
                child.communicate()
print('PASS: shared pinned tool installation is serialized')
