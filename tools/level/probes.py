"""Bake bounded reflection atlases from native development-client captures."""
import argparse
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from PIL import Image

sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from tools.scratch import ROOT as SCRATCH

# Forward/right/up for cg_fov=90 and square engine views. Image rows go downward.
ANGLES = ((0,0,0),(0,180,0),(0,90,0),(0,270,0),(-90,0,0),(90,0,0))
AXES = (((1,0,0),(0,-1,0),(0,0,1)),((-1,0,0),(0,1,0),(0,0,1)),
        ((0,1,0),(1,0,0),(0,0,1)),((0,-1,0),(-1,0,0),(0,0,1)),
        ((0,0,1),(0,-1,0),(-1,0,0)),((0,0,-1),(0,-1,0),(1,0,0)))
LEVELS = 5


def unit(v):
    length = math.sqrt(sum(x*x for x in v))
    return tuple(x/length for x in v)


def dot(a,b):
    return sum(x*y for x,y in zip(a,b))


def cross(a,b):
    return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])


def direction(face,u,v):
    forward,right,up = AXES[face]
    return unit(tuple(forward[i]+right[i]*u-up[i]*v for i in range(3)))


def face_uv(v):
    axis = max(range(3),key=lambda i:abs(v[i]))
    face = axis*2+(v[axis]<0)
    forward,right,up = AXES[face]
    scale = dot(v,forward)
    return face,dot(v,right)/scale,-dot(v,up)/scale


def linear(c):
    return c/12.92 if c<=0.04045 else ((c+0.055)/1.055)**2.4


def prefilter(faces,size,samples=64):
    """GGX half-vector importance samples, N=V, weighted by N.L."""
    if len(faces)!=6 or any(face.size!=faces[0].size for face in faces):
        raise ValueError('six equally sized square faces required')
    width,height = faces[0].size
    if width!=height or size<1 or samples<1:
        raise ValueError('invalid reflection sampling dimensions')
    table = [linear(c/255) for c in range(256)]
    pixels = [[tuple(table[c] for c in rgb) for rgb in struct.iter_unpack('3B', face.convert('RGB').tobytes())] for face in faces]
    atlas = Image.new('RGBA',(6*size,LEVELS*size))
    output = atlas.load()
    # Equations: https://google.github.io/filament/main/filament.html
    # "Importance sampling for the IBL". Hammersley samples are deterministic.
    radical = [int(f'{i:032b}'[::-1],2)/2**32 for i in range(samples)]
    for level in range(LEVELS):
        roughness = level/(LEVELS-1)
        local = []
        for i in range(samples if level else 1):
            phi = math.tau*i/samples
            nh = math.sqrt((1-radical[i])/(1+(roughness**4-1)*radical[i])) if level else 1
            nl = 2*nh*nh-1
            if nl>0:
                sine = math.sqrt(max(0,1-nh*nh))
                local.append((2*nh*sine*math.cos(phi),2*nh*sine*math.sin(phi),nl))
        weight = sum(v[2] for v in local)
        for face in range(6):
            for y in range(size):
                for x in range(size):
                    n = direction(face,2*(x+.5)/size-1,2*(y+.5)/size-1)
                    tangent = unit(cross((0,0,1) if abs(n[2])<.99 else (1,0,0),n))
                    bitangent = cross(n,tangent)
                    color = [0.,0.,0.]
                    for a,b,c in local:
                        light = tuple(a*tangent[j]+b*bitangent[j]+c*n[j] for j in range(3))
                        index,u,v = face_uv(light)
                        sx = min(width-1,max(0,int((u+1)*.5*width)))
                        sy = min(height-1,max(0,int((v+1)*.5*height)))
                        texel = pixels[index][sy*width+sx]
                        for channel in range(3):
                            color[channel] += texel[channel]*c
                    output[face*size+x,level*size+y] = tuple(round(min(1,max(0,c/weight))*255) for c in color)+(255,)
    return atlas


def write_probes(path,probes,atlases):
    if not 1<=len(probes)<=32 or len(probes)!=len(atlases):
        raise ValueError('one atlas per probe, maximum 32 probes')
    size = atlases[0].width//6
    if size not in (16,32,64,128) or any(a.size!=(6*size,LEVELS*size) or a.mode!='RGBA' for a in atlases):
        raise ValueError('invalid probe atlas size/format')
    data = bytearray(struct.pack('<8sIIII',b'ASPROBE\0',1,len(probes),size,LEVELS))
    for probe,atlas in zip(probes,atlases):
        origin,radius = probe['origin'],probe['radius']
        if len(origin)!=3 or any(not math.isfinite(v) or abs(v)>32752 for v in origin) or not math.isfinite(radius) or not 1<=radius<=32752:
            raise ValueError('invalid probe position/radius')
        data += struct.pack('<4f',*origin,radius)+atlas.tobytes()
    path = Path(path)
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source',type=Path,help='JSON: map and 1..32 probes with origin/radius')
    parser.add_argument('--client',required=True,type=Path)
    parser.add_argument('--base',required=True,type=Path,help='authored loose content directory')
    parser.add_argument('--data',required=True,type=Path,help='installed game pk3 directory (symlinked only)')
    parser.add_argument('--content',choices=('quake3','openarena'),default='quake3')
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--size',type=int,choices=(16,32,64,128),default=32)
    parser.add_argument('--samples',type=int,choices=(32,64,128,256),default=64)
    args = parser.parse_args()
    document = json.loads(args.source.read_text())
    name,probes = document['map'],document['probes']
    if not isinstance(name,str) or not name or len(name)>48 or any(c not in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-' for c in name):
        parser.error('invalid map name')
    # Validate the exact record contract before launching a client.
    for p in probes:
        if set(p)!={'origin','radius'}:
            parser.error('probe requires origin and radius')
    if not 1<=len(probes)<=32:
        parser.error('require 1..32 probes')
    paks = sorted(args.data.resolve().glob('*.pk3'))
    icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
    if not paks or len(icds)!=1 or not (args.base/'maps'/(name+'.bsp')).is_file():
        parser.error('installed content, authored BSP and one lavapipe ICD are required')
    args.output.mkdir(parents=True,exist_ok=True)
    atlases = []
    with tempfile.TemporaryDirectory(prefix='aftershock-probes-') as temporary:
        home=Path(temporary);base=home/('baseoa' if args.content=='openarena' else 'baseq3')
        base.mkdir()
        for child in args.base.resolve().iterdir():
            if child.name not in ('screenshots','q3config.cfg') and child.suffix!='.pk3':
                (base/child.name).symlink_to(child,target_is_directory=child.is_dir())
        for pak in paks:
            (base/pak.name).symlink_to(pak)
        # Capture one probe per client so the bounded command buffer cannot fill.
        for index,probe in enumerate(probes):
            origin,radius=probe['origin'],probe['radius']
            if len(origin)!=3 or any(not math.isfinite(v) or abs(v)>32752 for v in origin) or not math.isfinite(radius) or not 1<=radius<=32752:
                parser.error('invalid probe position/radius')
            commands=['set g_synchronousClients 1','set fixedtime 20',f'devmap {name}','wait 40','team spectator','wait 10']
            for face,angles in enumerate(ANGLES):
                commands += ['cmd dev_view '+' '.join(str(v) for v in (*origin,*angles)),'wait 10',f'screenshot probe_{index}_{face}','wait 2']
            commands+=['quit'];(base/'probe-bake.cfg').write_text('\n'.join(commands)+'\n')
            log=args.output/f'probe-{index}.log'
            env=dict(os.environ,LP_NUM_THREADS='1',VK_DRIVER_FILES=str(icds[0]),VK_ICD_FILENAMES=str(icds[0]))
            with log.open('w') as stream:
                subprocess.run(['timeout','120','xvfb-run','-a','faketime','-f','@2026-01-01 00:00:00 i0.01',str(args.client.resolve()),
                    '+set','fs_basepath',str(home),'+set','fs_homepath',str(home),
                    *(['+set','fs_game','baseoa'] if args.content=='openarena' else []),
                    '+set','net_enabled','0','+set','sv_pure','0','+set','r_mode','-1',
                    '+set','r_customwidth',str(args.size),'+set','r_customheight',str(args.size),
                    '+set','r_fullscreen','0','+set','s_initsound','0','+set','com_maxfps','0',
                    '+set','r_gamma','1','+set','r_intensity','1','+set','r_overBrightBits','0',
                    '+set','r_shadowQuality','0','+set','r_reflectionProbes','0',
                    '+set','cg_fov','90','+set','cg_draw2D','0','+set','cg_drawGun','0',
                    '+set','con_notifytime','0','+set','cl_autoRecordDemo','0','+set','dev_tools','0',
                    '+exec','probe-bake.cfg'],env=env,stdout=stream,stderr=subprocess.STDOUT,check=True)
            text=log.read_text()
            if any(error in text for error in ('ERROR:','Unknown command','Signal caught')) or 'llvmpipe' not in text:
                raise ValueError('probe capture failed: '+str(log))
            faces=[]
            for face in range(6):
                with Image.open(base/'screenshots'/f'probe_{index}_{face}.tga') as image:
                    faces.append(image.convert('RGB'))
            atlases.append(prefilter(faces,args.size,args.samples))
        write_probes(args.output/'maps'/(name+'.asprobe'),probes,atlases)
    print(f'PASS: baked {len(probes)} reflection probes for {name}, {args.size}px faces, {LEVELS} roughness levels')


if __name__=='__main__':
    main()
