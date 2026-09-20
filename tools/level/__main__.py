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

from geometry import generate, vector
from toolchain import compile_map
from validate import validate


def shaders(level):
    sky = level['materials']['sky']
    lighting = level['lighting']
    sun = ''
    if 'sun' in lighting:
        s = lighting['sun']
        x,y,z = [-v for v in s['direction']]
        azimuth = math.degrees(math.atan2(y,x))
        elevation = math.degrees(math.atan2(z,math.hypot(x,y)))
        sun = f"    q3map_sun {vector(s['color'])} {s['intensity']} {azimuth:.9g} {elevation:.9g}\n"
    return (f'textures/{sky}\n{{\n    qer_editorimage textures/{sky}.tga\n'
            '    surfaceparm sky\n    surfaceparm noimpact\n    surfaceparm nolightmap\n'
            f'{sun}    skyparms - 512 -\n    {{\n        map textures/{sky}\n        rgbGen identity\n    }}\n}}\n'
            'textures/level/playerclip\n{\n    surfaceparm nodraw\n    surfaceparm nonsolid\n    surfaceparm playerclip\n}\n')


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
        level = json.loads(source.read_bytes())
        sources,report = validate(level,source.parent/'assets')
        if output==source.parent or output.is_relative_to(source.parent/'assets'):
            raise ValueError('output must not overwrite the source/assets directory')
        for name,path in sources.items():
            target = output/name
            target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(path,target)
        (output/'maps').mkdir(parents=True,exist_ok=True)
        (output/'scripts').mkdir(parents=True,exist_ok=True)
        (output/'scripts/level.shader').write_text(shaders(level))
        (output/'scripts/shaderlist.txt').write_text('level\n')
        map_path = 'maps/'+level['name']+'.map'
        (output/map_path).write_text(generate(level))
        if not args.map_only:
            compile_map(output,level['name'])
        paths = {kind:'maps/'+level['name']+'.'+kind if kind=='map' or not args.map_only else None for kind in ('map','bsp','aas')}
        print(json.dumps(dict(version=1,name=level['name'],**paths,report=report,
                              sha256={kind:hashlib.sha256((output/path).read_bytes()).hexdigest() for kind,path in paths.items() if path}),sort_keys=True))
    except (OSError,ValueError,KeyError,TypeError,subprocess.SubprocessError) as exc:
        print('level: '+str(exc),file=sys.stderr)
        return 1
    return 0


if __name__=='__main__':
    sys.exit(main())
