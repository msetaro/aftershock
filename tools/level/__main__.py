#!/usr/bin/env python3
"""Compile version-1 declarative levels to deterministic MAP/BSP/AAS content."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from geometry import generate, vector
from toolchain import compile_map
from validate import validate

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.scratch import ROOT as SCRATCH
from tools.agent.formats import validate as validate_format, diagnostic


def shaders(level,cooked=()):
    sky = level['materials']['sky']
    lighting = level['lighting']
    sun = ''
    if 'sun' in lighting:
        s = lighting['sun']
        x,y,z = [-v for v in s['direction']]
        azimuth = math.degrees(math.atan2(y,x))
        elevation = math.degrees(math.atan2(z,math.hypot(x,y)))
        sun = f"    q3map_sun {vector(s['color'])} {s['intensity']} {azimuth:.9g} {elevation:.9g}\n"
    extra = ''
    if level['version']==2:
        from polygons import surface_shaders
        extra = surface_shaders(level,cooked)+('textures/level/fence\n{\n    surfaceparm nonsolid\n    surfaceparm playerclip\n'
                 '    surfaceparm alphashadow\n    cull none\n    {\n'
                 f'        map textures/{level["materials"]["trim"]}\n'
                 '        rgbGen identity\n    }\n}\n')
    return (f'textures/{sky}\n{{\n    qer_editorimage textures/{sky}\n'
            '    surfaceparm sky\n    surfaceparm noimpact\n    surfaceparm nolightmap\n'
            f'{sun}    skyparms - 512 -\n    {{\n        map textures/{sky}\n        rgbGen identity\n    }}\n}}\n'
            'textures/level/playerclip\n{\n    surfaceparm nodraw\n    surfaceparm nonsolid\n    surfaceparm playerclip\n}\n'+extra)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--map-only',action='store_true')
    args = parser.parse_args()
    try:
        source,output = args.source.resolve(),args.output.resolve()
        if source.stat().st_size>1024*1024:
            raise ValueError('level description exceeds 1 MiB')
        level = validate_format('level', json.loads(source.read_bytes()), source)
        sources,report = validate(level,source.parent/'assets')
        if output==source.parent or output.is_relative_to(source.parent/'assets'):
            raise ValueError('output must not overwrite the source/assets directory')
        map_path = 'maps/'+level['name']+'.map'
        paths = {kind:'maps/'+level['name']+'.'+kind if kind=='map' or not args.map_only else None for kind in ('map','bsp','aas')}
        # Compile only authored inputs in private staging. Existing installed content is
        # never copied into the tool workspace, and a failure publishes no partial map.
        with tempfile.TemporaryDirectory(prefix='aftershock-level-stage-') as temporary:
            stage = Path(temporary)
            for name,path in sources.items():
                target = stage/name
                target.parent.mkdir(parents=True,exist_ok=True)
                shutil.copyfile(path,target)
            (stage/'maps').mkdir()
            (stage/'scripts').mkdir()
            (stage/'scripts/level.shader').write_bytes(shaders(level).encode())
            (stage/'scripts/shaderlist.txt').write_bytes(b'level\n')
            (stage/map_path).write_bytes(generate(level).encode())
            if not args.map_only:
                try:
                    compile_map(stage,level['name'])
                finally:
                    if (stage/'compile.log').exists():
                        output.mkdir(parents=True,exist_ok=True)
                        shutil.copyfile(stage/'compile.log',output/'compile.log')

            cooked = {role:path for role,path in level['materials'].items() if (stage/'textures'/(path+'.asmat')).is_file()}
            if level['version']==2 and cooked:
                from polygons import pieces,surface_material
                for record in pieces(level)[1]:
                    if record['material'] in cooked and not record['id'].startswith('boundary_'):
                        target = stage/'textures'/(surface_material(level,record)+'.asmat')
                        target.parent.mkdir(parents=True,exist_ok=True)
                        shutil.copyfile(stage/'textures'/(cooked[record['material']]+'.asmat'),target)
                # q3map2 uses the image/lightmap shader; native rendering resolves
                # the cooked PBR alias. An explicit runtime shader would shadow it.
                (stage/'scripts/level.shader').write_bytes(shaders(level,cooked).encode())
            for path in sorted(stage.rglob('*')):
                if path.is_file():
                    target = output/path.relative_to(stage)
                    target.parent.mkdir(parents=True,exist_ok=True)
                    shutil.copyfile(path,target)
        print(json.dumps(dict(version=level['version'],name=level['name'],**paths,report=report,
                              sha256={kind:hashlib.sha256((output/path).read_bytes()).hexdigest() for kind,path in paths.items() if path}),sort_keys=True))
    except (OSError,ValueError,KeyError,TypeError,subprocess.SubprocessError) as exc:
        print(json.dumps(dict(ok=False,error=diagnostic(exc,args.source,'check geometry/assets and tools/agent describe level'))),file=sys.stderr)
        return 1
    return 0


if __name__=='__main__':
    if len(sys.argv)>1 and sys.argv[1]=='trace':
        from sketch import main as trace_main
        sys.exit(trace_main(sys.argv[2:]))
    if len(sys.argv)>1 and sys.argv[1]=='validate':
        from headless import main as validate_main
        sys.exit(validate_main(sys.argv[2:]))
    sys.exit(main())
