#!/usr/bin/env python3
"""Pinned offline theme downloads cook reproducibly through the production cooker."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from PIL import Image
from run import ROOT

with tempfile.TemporaryDirectory(prefix='aftershock-fetch-') as temporary:
    root=Path(temporary)
    cache=root/'cache/aftershock-assets'
    cache.mkdir(parents=True)
    files={}
    for channel,color in [('color',(96,64,48)),('normal',(128,128,255)),('roughness',(180,180,180)),('ao',(240,240,240)),('displacement',(128,128,128))]:
        image=root/(channel+'.png')
        Image.new('RGB',(8,8),color).save(image)
        data=image.read_bytes(); sha=hashlib.sha256(data).hexdigest()
        (cache/sha).write_bytes(data)
        files[channel]=dict(url='https://dl.polyhaven.org/owned-test-'+channel+'.png',sha256=sha,bytes=len(data))
    lock=dict(version=1,theme='owned_test',resolution=8,assets=[dict(provider='polyhaven',id='owned_test',version='test-v1',
        role='facade',source_url='https://polyhaven.com',author='Aftershock owned fixture',license='CC0-1.0',
        retrieved='2026-09-21',attribution='Owned procedural test inputs; no Poly Haven download',files=files)])
    source=root/'assets.lock.json'; source.write_text(json.dumps(lock))
    env=dict(os.environ,XDG_CACHE_HOME=str(root/'cache'))
    def fetch(out):
        result=subprocess.run([sys.executable,'tools/assets','fetch','--theme','owned_test','--lock',str(source),
                               '--offline','--out',str(out)],cwd=ROOT,env=env,text=True,capture_output=True)
        assert result.returncode==0,result.stderr
        manifest=json.loads((out/'manifest.json').read_text())
        assert len(manifest['assets'])==1 and manifest['assets'][0]['license']=='CC0-1.0'
        assert 'Powered by Poly Haven' in (out/'CREDITS').read_text()
        assert (out/'cooked/textures/theme/facade.asmat').is_file()
        assert (out/'source/facade/ao.png').is_file() and (out/'source/facade/displacement.png').is_file()
        check=subprocess.run([sys.executable,'tools/assets','validate',str(out)],cwd=ROOT,capture_output=True,text=True)
        assert check.returncode==0,check.stderr
        return {p.relative_to(out).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in out.rglob('*') if p.is_file()}
    a,b=fetch(root/'a'),fetch(root/'b')
    assert a==b,'pinned theme content depends on output path or fetch run'
    (cache/files['color']['sha256']).write_bytes(b'changed')
    result=subprocess.run([sys.executable,'tools/assets','fetch','--theme','owned_test','--lock',str(source),
                           '--offline','--out',str(root/'bad')],cwd=ROOT,env=env,text=True,capture_output=True)
    assert result.returncode and 'sha256' in result.stderr.lower(),result.stderr
print('PASS: offline pinned files, complete PBR sources, real ASMAT cooking, repeated bytes and cache hash guard')
