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


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command',choices=['kit'])
    parser.add_argument('--parameters',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--blender',type=Path)
    args=parser.parse_args()
    try:
        print(json.dumps(kit(args.parameters,args.out,blender(args.blender)),sort_keys=True))
        return 0
    except (OSError,ValueError,KeyError,TypeError,subprocess.SubprocessError) as exc:
        print(json.dumps(dict(ok=False,error=str(exc))),file=sys.stderr)
        return 1


if __name__=='__main__':
    sys.exit(main())
