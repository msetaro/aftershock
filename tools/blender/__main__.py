#!/usr/bin/env python3
"""Build deterministic, licensed environment modules with pinned headless Blender."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import os
import subprocess
import sys
import tempfile

sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from tools.scratch import ROOT as SCRATCH
from tools.assets.manifest import digest,validate
from tools.blender.toolchain import blender,VERSION

ROOT=Path(__file__).resolve().parents[2]


def kit(parameters,out,binary):
    params=json.loads(parameters.read_text())
    canonical=json.dumps(params,sort_keys=True,separators=(',',':')).encode()
    script=Path(__file__).with_name('kit.py')
    out=out.resolve()
    if out.exists():
        raise ValueError('output already exists; build into a fresh directory')
    out.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='aftershock-blender-',dir=out.parent) as temporary:
        stage=Path(temporary)/'kit'
        (stage/'source').mkdir(parents=True)
        (stage/'source/parameters.json').write_bytes(canonical+b'\n')
        log=out.with_suffix('.log')
        with log.open('w') as stream:
            result=subprocess.run([str(binary),'--background','--factory-startup','--threads','1','--python-exit-code','1',
                                   '--python',str(script),'--','--parameters',str(stage/'source/parameters.json'),
                                   '--out',str(stage/'source')],stdout=stream,stderr=subprocess.STDOUT,timeout=600)
        if result.returncode:
            raise ValueError('Blender kit failed: '+log.read_text(errors='replace')[-8000:])
        modules=json.loads((stage/'source/kit.json').read_text())['modules']
        assets=[dict(name='models/theme/'+m['name']+suffix,kind='model',source=f"source/{m['name']}/{m['name']}{suffix}.gltf",
                     scale=32,material_model='metallic-roughness') for m in modules for suffix in ('','_lod1')]
        (stage/'assets.json').write_text(json.dumps(dict(version=1,assets=assets),sort_keys=True,indent=2)+'\n')
        result=subprocess.run([sys.executable,str(ROOT/'tools/cook'),str(stage/'assets.json'),'--output',str(stage/'cooked')],
                              capture_output=True,text=True,timeout=300)
        if result.returncode:
            raise ValueError('procedural model cook failed: '+result.stderr[-8000:])
        manifest=dict(version=1,assets=[dict(id='aftershock_reference_kit',
            source_url='https://github.com/msetaro/aftershock/blob/main/tools/blender/kit.py',author='Aftershock procedural kit',
            license='CC0-1.0',retrieved=datetime.date.today().isoformat(),attribution='Original parameterized Aftershock environment modules',
            generator=dict(name='Blender',version=VERSION,script_sha256=digest(script),parameters_sha256=hashlib.sha256(canonical).hexdigest()),
            originals=[dict(path=p.relative_to(stage).as_posix(),sha256=digest(p)) for p in sorted((stage/'source').rglob('*')) if p.is_file()],
            cooked=[dict(path=p.relative_to(stage).as_posix(),sha256=digest(p)) for p in sorted((stage/'cooked').rglob('*')) if p.is_file()])])
        (stage/'manifest.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
        report=validate(stage)
        os.replace(stage,out)
    return dict(report,blender=VERSION,output=str(out),credits=str(out/'CREDITS'),log=str(log),modules=modules)


def retarget(parameters,assets,out,binary):
    import copy
    sys.path.insert(0,str(ROOT/'tools/cook'))
    from gltf import Document
    assets=assets.resolve()
    # This is private conversion. Publication/assembly still uses the unmodified
    # maintainer allowlist; licenses are preserved, never inferred or upgraded.
    source_report=validate(assets,publication=False)
    params=json.loads(parameters.read_text())
    if params.get('version')!=1:
        raise ValueError('retarget parameters version must be 1')
    manifest=json.loads((assets/'manifest.json').read_text())
    records={f['path']:f for entry in manifest['assets'] for kind in ('originals','cooked') for f in entry[kind]}
    def read(path):
        path=path.resolve()
        if not path.is_relative_to(assets) or path.relative_to(assets).as_posix() not in records:
            raise ValueError('glTF dependency is outside the supplied manifest: '+str(path))
        if path.stat().st_size>256<<20:
            raise ValueError('supplied asset exceeds 256 MiB')
        return path.read_bytes()
    from urllib.parse import unquote
    for kind in ('rig','clip'):
        path=Path(params[kind])
        if path.is_absolute() or '..' in path.parts:
            raise ValueError('supplied rig/clip paths must be relative to the manifest')
        document=Document(assets/path,read)
        for item in document.data.get('buffers',[])+document.data.get('images',[]):
            uri=item.get('uri','')
            if not uri.startswith('data:') and (Path(unquote(uri)).is_absolute() or '..' in Path(unquote(uri)).parts):
                raise ValueError('glTF dependencies must remain relative to the supplied asset')
        for index in range(len(document.data.get('images',[]))):
            document.image(index)
    canonical=json.dumps(params,sort_keys=True,separators=(',',':')).encode()
    out=out.resolve()
    if out.exists():
        raise ValueError('output already exists; use a fresh directory')
    out.parent.mkdir(parents=True,exist_ok=True)
    script=Path(__file__).with_name('retarget.py')
    with tempfile.TemporaryDirectory(prefix='aftershock-retarget-',dir=out.parent) as temporary:
        stage=Path(temporary)/'result'
        source=stage/'source'
        (source/'input').mkdir(parents=True)
        for name in sorted(records):
            target=source/'input'/name
            target.parent.mkdir(parents=True,exist_ok=True)
            target.write_bytes(read(assets/name))
        (source/'parameters.json').write_bytes(canonical+b'\n')
        log=out.with_suffix('.log')
        with log.open('w') as stream:
            result=subprocess.run([str(binary),'--background','--factory-startup','--threads','1','--python-exit-code','1',
                                   '--python',str(script),'--','--parameters',str(source/'parameters.json'),
                                   '--assets',str(source/'input'),'--out',str(source)],
                                   stdout=stream,stderr=subprocess.STDOUT,timeout=600)
        if result.returncode:
            raise ValueError('Blender retarget failed: '+log.read_text(errors='replace')[-8000:])
        (stage/'assets.json').write_text(json.dumps(dict(version=1,assets=[dict(name=params['name'],kind='model',
            source='source/retargeted.gltf',scale=params.get('scale',32),fps=params['fps'],material_model='metallic-roughness')]),sort_keys=True)+'\n')
        result=subprocess.run([sys.executable,str(ROOT/'tools/cook'),str(stage/'assets.json'),'--output',str(stage/'cooked')],
                              capture_output=True,text=True,timeout=300)
        if result.returncode:
            raise ValueError('retargeted model cook failed: '+result.stderr[-8000:])
        outputs=[dict(path=p.relative_to(stage).as_posix(),sha256=digest(p))
                 for folder in (source,stage/'cooked') for p in sorted(folder.rglob('*'))
                 if p.is_file() and not p.is_relative_to(source/'input') and p!=source/'parameters.json']
        entries=[]
        for original in manifest['assets']:
            entry=copy.deepcopy(original)
            names=sorted({f['path'] for kind in ('originals','cooked') for f in original[kind]})
            entry['originals']=[dict(path='source/input/'+name,sha256=records[name]['sha256']) for name in names]
            entry['originals'].append(dict(path='source/parameters.json',sha256=digest(source/'parameters.json')))
            entry['cooked']=outputs
            entry['generator']=dict(name='Blender',version=VERSION,script_sha256=digest(script),parameters_sha256=hashlib.sha256(canonical).hexdigest())
            entries.append(entry)
        (stage/'manifest.json').write_text(json.dumps(dict(version=1,assets=entries),sort_keys=True,indent=2)+'\n')
        report=validate(stage,publication=False)
        os.replace(stage,out)
    return dict(report,publishable=source_report['publishable'],blender=VERSION,output=str(out),credits=str(out/'CREDITS'),log=str(log))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command',choices=['kit','retarget'])
    parser.add_argument('--parameters',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--blender',type=Path)
    parser.add_argument('--assets',type=Path,help='supplied input manifest directory for retarget')
    args=parser.parse_args()
    try:
        binary=blender(args.blender)
        if args.command=='retarget' and not args.assets:
            raise ValueError('retarget requires --assets with a complete supplied-source manifest')
        result=kit(args.parameters,args.out,binary) if args.command=='kit' else retarget(args.parameters,args.assets,args.out,binary)
        print(json.dumps(result,sort_keys=True))
        return 0
    except (OSError,ValueError,KeyError,TypeError,subprocess.SubprocessError) as exc:
        print(json.dumps(dict(ok=False,error=str(exc))),file=sys.stderr)
        return 1


if __name__=='__main__':
    sys.exit(main())
