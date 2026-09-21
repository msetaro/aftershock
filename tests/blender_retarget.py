#!/usr/bin/env python3
"""Retarget existing supplied GPL rigs privately; preserve licenses and publication gates."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile

from run import ROOT
sys.path.insert(0,str(ROOT/'tools/cook'))
from gltf import Document

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--blender',type=Path)
args=parser.parse_args()
with tempfile.TemporaryDirectory(prefix='aftershock-retarget-') as temporary:
    root=Path(temporary)
    supplied=root/'provided'
    supplied.mkdir()
    entries=[]
    for identity,fixture,names in [('rig','cook-character',['character.gltf','character.bin','character.png']),
                                    ('clip','animation',['body.gltf','body.bin'])]:
        source=ROOT/'tests/assets'/fixture
        provenance=json.loads((source/'provenance.json').read_text())
        target=supplied/identity
        target.mkdir()
        records=[]
        for name in names:
            assert hashlib.sha256((source/name).read_bytes()).hexdigest()==provenance['files'][name]
            shutil.copyfile(source/name,target/name)
            records.append(dict(path=identity+'/'+name,sha256=hashlib.sha256((target/name).read_bytes()).hexdigest()))
        entries.append(dict(id=identity,source_url='https://github.com/msetaro/aftershock/tree/main/tests/assets/'+fixture,
                            author='Aftershock',license='GPL-2.0-or-later',retrieved=datetime.date.today().isoformat(),
                            attribution='Existing repository-owned fixture; private conversion only',originals=records,cooked=records))
    (supplied/'manifest.json').write_text(json.dumps(dict(version=1,assets=entries)))
    params=dict(version=1,rig='rig/character.gltf',clip='clip/body.gltf',animation='walk',
                bones={'root':'root','arm.L':'upperarm.L','arm.R':'upperarm.R'},fps=30,root_motion_scale=1,
                name='models/provided',scale=32)
    parameters=root/'parameters.json'
    def run(value,output):
        parameters.write_text(json.dumps(value))
        command=[sys.executable,'tools/blender','retarget','--assets',str(supplied),'--parameters',str(parameters),'--out',str(output)]
        if args.blender:
            command+=['--blender',str(args.blender)]
        return subprocess.run(command,cwd=ROOT,capture_output=True,text=True)
    result=run(params,root/'a')
    assert result.returncode==0,result.stderr
    summary=json.loads(result.stdout)
    assert summary['ok'] and not summary['publishable'], 'supplied GPL asset silently entered the CC0 theme policy'
    report=json.loads((root/'a/source/retarget-report.json').read_text())
    assert report['bones']==params['bones'] and report['frames']>=30
    assert report['max_rotation_error_radians']<1e-4 and report['max_bind_offset_error']<1e-4,report
    assert report['root_displacement']>0 and report['animated_bones']>=2, 'clip motion was not transferred'
    output=root/'a/source/retargeted.gltf'
    document=Document(output,lambda p:p.read_bytes())
    assert len(document.data['skins'][0]['joints'])==3
    assert len(document.data['animations'])==1 and document.data['animations'][0]['name']=='walk'
    assert any(channel['target']['path']=='rotation' for channel in document.data['animations'][0]['channels'])
    iqm=(root/'a/cooked/models/provided.iqm').read_bytes()
    assert iqm[:16]==b'INTERQUAKEMODEL\0' and struct.unpack_from('<I',iqm,92)[0]>=30
    manifest=json.loads((root/'a/manifest.json').read_text())
    assert {e['license'] for e in manifest['assets']}=={'GPL-2.0-or-later'}
    blocked=subprocess.run([sys.executable,'tools/assets','validate',str(root/'a')],cwd=ROOT,capture_output=True,text=True)
    assert blocked.returncode and 'allowlist' in blocked.stderr, 'private processing bypassed theme publication policy'
    second=run(params,root/'b')
    assert second.returncode==0,second.stderr
    def hashes(folder):
        return {p.relative_to(folder).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in folder.rglob('*') if p.is_file()}
    assert hashes(root/'a')==hashes(root/'b'), 'retargeted output differs across fresh runs'
    missing=dict(params,bones=dict(params['bones'],**{'arm.L':'missing'}))
    rejected=run(missing,root/'missing')
    assert rejected.returncode and 'bone' in rejected.stderr, 'unmapped source bone was silently ignored'
    assert not (root/'missing').exists(), 'failed retarget published partial output'
    (supplied/'rig/character.bin').write_bytes(b'changed')
    rejected=run(params,root/'hash')
    assert rejected.returncode and 'SHA256' in rejected.stderr, 'supplied rig hash was not checked before import'
print('PASS: supplied rig/motion retarget, native cook, deterministic output, retained GPL provenance and publication rejection')
