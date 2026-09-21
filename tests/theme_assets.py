#!/usr/bin/env python3
"""Every theme asset must have verified source/cooked hashes and an allowed license."""
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

from PIL import Image
from run import ROOT

with tempfile.TemporaryDirectory(prefix='aftershock-license-') as temporary:
    root = Path(temporary)
    Image.new('RGB',(8,8),(80,100,120)).save(root/'owned.png')
    digest = hashlib.sha256((root/'owned.png').read_bytes()).hexdigest()
    manifest = dict(version=1,assets=[dict(id='owned',source_url='https://github.com/msetaro/aftershock',
        author='Aftershock test fixture',license='CC0-1.0',retrieved='2026-09-21',attribution='Original procedural test image',
        originals=[dict(path='owned.png',sha256=digest)],cooked=[dict(path='owned.png',sha256=digest)])])
    def check(value,expected=None):
        (root/'manifest.json').write_text(json.dumps(value))
        result = subprocess.run([sys.executable,'tools/assets','validate',str(root)],cwd=ROOT,capture_output=True,text=True)
        if expected:
            assert result.returncode and expected in result.stderr.lower(),(expected,result.stdout,result.stderr)
        else:
            assert result.returncode==0,result.stderr
            assert json.loads(result.stdout)['ok']
            credits = (root/'CREDITS').read_text()
            assert 'CC0-1.0' in credits and 'Aftershock test fixture' in credits
    check(manifest)
    Image.new('RGB',(8,8),'red').save(root/'unlicensed.png')
    check(manifest,'unlisted')
    (root/'unlicensed.png').unlink()
    changed = copy.deepcopy(manifest); changed['assets'][0]['license']='CC-BY-4.0'
    check(changed,'allowlist')
    changed = copy.deepcopy(manifest); changed['assets'][0]['originals'][0]['sha256']='0'*64
    check(changed,'sha256')
    changed = copy.deepcopy(manifest); changed['assets'][0]['source_url']=''
    check(changed,'source_url')
    changed = copy.deepcopy(manifest); changed['assets'][0]['generator']=dict(name='unapproved-image-generator',prompt='poster')
    check(changed,'generator')
print('PASS: theme manifest coverage, credits, license allowlist and original/cooked hash enforcement')
