"""Headless native-engine evidence for authored levels; only JSON goes to stdout."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile

from geometry import passage, floor_at, vector
from toolchain import compile_map
from validate import validate, qpath

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def structure(base,name,authored):
    data = (base/'maps'/(name+'.bsp')).read_bytes()
    lumps = [struct.unpack_from('<ii',data,8+i*8) for i in range(17)]
    entities = data[lumps[0][0]:sum(lumps[0])].decode().rstrip('\0')
    counts = {}
    for classname in re.findall(r'"classname"\s+"([^"\n]+)"',entities):
        counts[classname] = counts.get(classname,0)+1
    offset,length = lumps[13]
    lit = sum(struct.unpack_from('<i',data,i+28)[0]>=0 for i in range(offset,offset+length,104))
    surfaces = length//104
    aas = bytearray((base/'maps'/(name+'.aas')).read_bytes())
    for i in range(8,124):
        aas[i] ^= ((i-8)*119)&255
    # AAS header contains 14 offset/length pairs after ident/version/BSP checksum.
    # Area zero is the reserved invalid area; real area records are 48 bytes.
    _,area_bytes = struct.unpack_from('<ii',aas,12+7*8)
    return {'entities':counts,'spawn_reachability':{'reachable':authored.get('reachable_spawns'),
            'method':'player-clearance grid and runtime AAS bot smoke' if authored else 'runtime AAS bot smoke'},
            'lightmaps':{'pages':lumps[14][1]//(128*128*3),'surfaces':surfaces,'lit_surfaces':lit,
                        'coverage':lit/surfaces if surfaces else 0},
            'navigation':{'backend':'aas','areas':max(0,area_bytes//48-1),'navmesh_coverage':None}}


def flythrough(level):
    rooms = {r['id']:r for r in level['rooms']}
    views = []
    for r in rooms.values():
        x,y,z = r['origin']
        views.append({'id':'auto_room_'+r['id'],'origin':[x,y,z+min(96,r['size'][2]-16)],'angles':[0,0,0]})
    # Follow each authored passage with fixed approach, midpoint and exit samples.
    for c in level['connections']:
        low,high,axis,start,end = passage(c,rooms)
        for index,fraction in enumerate((0,0.5,1)):
            p = [c['at'],c['at'],0]
            p[axis] = low[axis]+(high[axis]-low[axis])*fraction
            p[2] = floor_at(c,rooms,p)+min(64,c['height']-16)
            views.append({'id':f"auto_{c['id']}_{index}",'origin':p,'angles':[0,axis*90,0]})
    return views


def run_engine(binary,home,content,script,log,client=False):
    base = home/('baseoa' if content=='openarena' else 'baseq3')
    (base/'level-validate.cfg').write_text('\n'.join(script+['quit'])+'\n')
    env = dict(os.environ,LC_ALL='C',LP_NUM_THREADS='1')
    prefix = ['timeout','180']
    if client:
        icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
        if len(icds)!=1:
            raise ValueError('exactly one Mesa lavapipe ICD is required')
        env.update(VK_DRIVER_FILES=str(icds[0]),VK_ICD_FILENAMES=str(icds[0]))
        prefix += ['xvfb-run','-a']
    command = [*prefix,'faketime','-f','@2026-01-01 00:00:00 i0.01',str(binary.resolve()),
               '+set','fs_basepath',str(home),'+set','fs_homepath',str(home),
               *(['+set','fs_game','baseoa'] if content=='openarena' else []),
               '+set','net_enabled','0','+set','sv_pure','0']
    command += (['+set','r_mode','3','+set','r_fullscreen','0','+set','s_initsound','0','+set','com_maxfps','0',
                 '+set','cl_autoRecordDemo','0','+set','con_notifytime','0'] if client else ['+set','dedicated','1'])
    command += ['+exec','level-validate.cfg']
    with log.open('w') as stream:
        subprocess.run(command,cwd=ROOT,env=env,stdout=stream,stderr=subprocess.STDOUT,check=True)
    text = log.read_text(errors='replace')
    problems = [line for line in text.splitlines() if re.search(r'ERROR:|Signal caught|unknown cmd|AAS not initialized|Couldn.t load',line)]
    if problems or 'Static game loaded.' not in text:
        raise ValueError('native engine load/command failure: '+'; '.join(problems[-5:]))
    if client and not ('VK_RENDERER:' in text and 'llvmpipe' in text and 'Static cgame loaded.' in text):
        raise ValueError('headless client did not use native cgame and lavapipe')
    return text


def bots(args,home,name,output):
    names = ['sarge','beret'] if args.content=='openarena' else ['sarge','major']
    script = ['set g_synchronousClients 1','set fixedtime 20','set sv_fps 50',f'devmap {name}',
              *[f'addbot {bot} 4' for bot in names]]
    for start in range(0,args.bot_frames,50):
        script += [f'wait {min(50,args.bot_frames-start)}',f'echo level_bot_sample_{start}',
                   'dev_entity sample 0','dev_entity sample 1']
    text = run_engine(args.server,home,args.content,script,output/'server.log')
    samples = re.findall(r'Developer sample: id=(\d+) origin=([-\d.]+) ([-\d.]+) ([-\d.]+) health=(-?\d+) linked=(\d+)',text)
    tracks = {i:[] for i in (0,1)}
    for client,x,y,z,health,linked in samples:
        if int(client) in tracks:
            tracks[int(client)].append(((float(x),float(y),float(z)),int(health)>0 and linked=='1'))
    if not all(len(v)>=args.bot_frames//50 for v in tracks.values()):
        raise ValueError('bot position diagnostics absent; use an AFTERSHOCK_DEVTOOLS build')
    stuck = []
    for client,track in tracks.items():
        # ponytail: 10-second inactivity windows flag possible stuck bots; combat can
        # also pause movement, so expose the heuristic rather than claim path diagnosis.
        for begin in range(0,len(track)-9):
            window = track[begin:begin+10]
            if all(alive for _,alive in window) and max(math.dist(window[0][0],p) for p,_ in window)<16:
                stuck.append({'client':client,'first_sample':begin,'samples':10})
                break
    return {'frames':args.bot_frames,'clients':2,'kills':len(re.findall(r'^Kill:',text,re.M)),
            'pickups':len(re.findall(r'^Item:',text,re.M)),'samples':len(samples),'stuck':stuck,
            'stuck_rule':'alive for ten 50-frame samples with less than 16 units displacement'}


def screenshots(args,home,name,views,automatic,output):
    from PIL import Image
    script = ['set g_synchronousClients 1','set fixedtime 20','set sv_fps 50',
              'set cg_draw2D 0','set cg_drawGun 0','set cg_drawCrosshair 0',
              f'devmap {name}','wait 50','team spectator','wait 10']
    for index,v in enumerate(views+automatic):
        script += [f"cmd dev_view {vector(v['origin'])} {vector(v['angles'])}",'wait 10',
                   f'echo level_view_begin_{index}','set r_speeds 1','wait 3','vkinfo',
                   f'screenshot level_view_{index:03d}','wait 2','set r_speeds 0',f'echo level_view_end_{index}']
    text = run_engine(args.client,home,args.content,script,output/'client.log',True)
    base = home/('baseoa' if args.content=='openarena' else 'baseq3')
    results = []
    (output/'images').mkdir(exist_ok=True)
    for index,v in enumerate(views+automatic):
        section = text.split(f'level_view_begin_{index}\n',1)[1].split(f'level_view_end_{index}\n',1)[0]
        draws = re.findall(r'frame draw calls: (\d+)',section)
        triangles = re.findall(r'\d+/\d+ shaders/surfs \d+ leafs \d+ verts (\d+)/\d+ tris',section)
        if not draws or not triangles or not int(draws[-1]):
            raise ValueError('frame metrics absent; client must include the level diagnostics')
        image_path = Path('images')/(v['id']+'.png')
        with Image.open(base/'screenshots'/f'level_view_{index:03d}.tga') as image:
            image.convert('RGB').save(output/image_path)
        results.append({'name':v['id'],'origin':v['origin'],'angles':v['angles'],'image':image_path.as_posix(),
                        'sha256':hashlib.sha256((output/image_path).read_bytes()).hexdigest(),
                        'metrics':{'draw_calls':int(draws[-1]),'triangles':int(triangles[-1])}})
    return results[:len(views)],results[len(views):]


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--client',type=Path,required=True)
    parser.add_argument('--server',type=Path,required=True)
    parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
    parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
    parser.add_argument('--bot-frames',type=int,default=6000)
    args = parser.parse_args(argv)
    output = args.output.resolve()
    output.mkdir(parents=True,exist_ok=True)
    report = {'version':1,'status':'failed','errors':[],'warnings':[],'structure':None,'bots':None,'views':[],'flythrough':[]}
    try:
        if not 500<=args.bot_frames<=30000:
            raise ValueError('bot frames must be in 500..30000')
        paks = sorted(args.data.resolve().glob('*.pk3'))
        if not paks:
            raise ValueError('installed game content is required')
        source = args.source.resolve()
        level = json.loads(source.read_bytes())
        _,authored = validate(level,source.parent/'assets')
        name = level['name']
        with tempfile.TemporaryDirectory(prefix='aftershock-headless-') as temporary:
            home = Path(temporary)
            base = home/('baseoa' if args.content=='openarena' else 'baseq3')
            result = subprocess.run([sys.executable,str(HERE),str(source),'--output',str(base)],cwd=ROOT,capture_output=True,text=True)
            if (base/'compile.log').exists():
                shutil.copyfile(base/'compile.log',output/'compile.log')
                log = (base/'compile.log').read_text(errors='replace')
                report['warnings'] = [line for line in log.splitlines() if 'WARNING:' in line]
            if result.returncode:
                raise ValueError('compile failed: '+result.stderr.strip())
            report['structure'] = structure(base,name,authored)
            report['compiled'] = json.loads(result.stdout)['sha256']
            for pak in paks:
                (base/pak.name).symlink_to(pak)
            report['bots'] = bots(args,home,name,output)
            report['views'],report['flythrough'] = screenshots(args,home,name,level.get('viewpoints',[]),flythrough(level),output)
        if report['bots']['stuck']:
            report['warnings'].append('possible stuck bots: '+json.dumps(report['bots']['stuck']))
        report['status'] = 'passed'
    except (OSError,ValueError,KeyError,IndexError,TypeError,subprocess.SubprocessError) as exc:
        report['errors'].append(str(exc))
    payload = json.dumps(report,indent=2,sort_keys=True)+'\n'
    (output/'report.json').write_text(payload)
    lines = ['Level validation: '+report['status'],*['ERROR: '+e for e in report['errors']],*['WARNING: '+e for e in report['warnings']]]
    if report['structure']:
        lines += ['Structure: '+json.dumps(report['structure'],sort_keys=True),'Bots: '+json.dumps(report['bots'],sort_keys=True)]
        lines += [v['name']+': '+v['image']+' '+json.dumps(v['metrics']) for v in report['views']+report['flythrough']]
    (output/'report.txt').write_text('\n'.join(lines)+'\n')
    print(payload,end='')
    return 0 if report['status']=='passed' else 1
