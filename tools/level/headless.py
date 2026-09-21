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
from validate import validate, qpath, camera_specs

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
    if level.get('version')==2:
        from polygons import pieces,navigation
        _,records = pieces(level)
        routes = []
        navigation(level,records,routes)
        views = []
        for route in routes:
            # Keep endpoints and sample roughly 64 interior path positions.
            stride = max(8,math.ceil(sum(len(r) for r in routes)/max(1,64-len(routes))))
            for index in sorted(set(range(0,len(route),stride))|{len(route)-1}):
                a = route[index]
                b = route[min(index+1,len(route)-1)] if index<len(route)-1 else route[max(0,index-1)]
                yaw = math.degrees(math.atan2(b[1]-a[1],b[0]-a[0]))
                views.append(dict(id=f'auto_{len(views):03d}_path',origin=[a[0],a[1],a[2]+48],angles=[0,yaw,0]))
        return views
    rooms = {r['id']:r for r in level['rooms']}
    views,used = [],set()

    def room_view(name):
        r = rooms[name]
        x,y,z = r['origin']
        views.append({'id':f'auto_{len(views):03d}_room_{name}',
                      'origin':[x,y,z+min(96,r['size'][2]-16)],'angles':[0,0,0]})

    def edge_views(c,reverse):
        low,high,axis,_,_ = passage(c,rooms)
        for fraction in ((1,0.5,0) if reverse else (0,0.5,1)):
            p = [c['at'],c['at'],0]
            p[axis] = low[axis]+(high[axis]-low[axis])*fraction
            if fraction==0:
                p[axis] -= 16
            elif fraction==1:
                p[axis] += 16
            p[2] = floor_at(c,rooms,p)+min(64,c['height']-16)
            views.append({'id':f"auto_{len(views):03d}_{c['id']}",'origin':p,'angles':[0,axis*90+(180 if reverse else 0),0]})

    def walk(room):
        room_view(room)
        for c in level['connections']:
            if c['id'] in used or room not in (c['from'],c['to']):
                continue
            used.add(c['id'])
            reverse = room==c['to']
            other = c['from'] if reverse else c['to']
            edge_views(c,reverse)
            walk(other)
            edge_views(c,not reverse)
            room_view(room)
    walk(next(iter(rooms)))
    return views


def run_engine(binary,home,content,script,log,client=False):
    base = home/('baseoa' if content=='openarena' else 'baseq3')
    commands = '\n'.join(script+['quit'])+'\n'
    if len(commands.encode())>60000:
        raise ValueError('validation command script exceeds the 60000-byte engine command budget')
    (base/'level-validate.cfg').write_text(commands)
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


def stuck_bots(tracks):
    stuck = []
    for client,track in tracks.items():
        # ponytail: 10-second inactivity windows flag possible stuck bots; combat can
        # also pause movement, so expose the heuristic rather than claim path diagnosis.
        for begin in range(0,len(track)-9):
            window = track[begin:begin+10]
            if all(alive for _,alive in window) and max(math.dist(window[0][0],p) for p,_ in window)<16:
                stuck.append({'client':client,'first_sample':begin,'samples':10})
                break
    return stuck


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
    stuck = stuck_bots(tracks)
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
    parser.add_argument('--viewpoints',type=Path,help='optional named camera array for an existing MAP source')
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
        if source.stat().st_size>(1024*1024 if source.suffix.lower()=='.json' else 16*1024*1024):
            raise ValueError('level description exceeds its input size limit')
        if output==source.parent or output.is_relative_to(source.parent/'assets'):
            raise ValueError('report output must not overwrite source/assets')
        level = None
        authored = {}
        if source.suffix.lower()=='.json':
            sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
            from tools.agent.formats import validate as validate_format
            level = validate_format('level',json.loads(source.read_bytes()),source)
            _,authored = validate(level,source.parent/'assets')
            name = level['name']
        elif source.suffix.lower()=='.map':
            name = source.stem
            if not re.fullmatch(r'[a-z][a-z0-9_]{0,31}',name):
                raise ValueError('invalid MAP name')
            report['warnings'].append('Existing MAP: declarative design-rule and grid reachability data are unavailable')
        else:
            raise ValueError('source must be a level JSON or existing MAP')
        with tempfile.TemporaryDirectory(prefix='aftershock-headless-') as temporary:
            home = Path(temporary)
            base = home/('baseoa' if args.content=='openarena' else 'baseq3')
            compile_error = None
            if level is not None:
                result = subprocess.run([sys.executable,str(HERE),str(source),'--output',str(base)],cwd=ROOT,capture_output=True,text=True)
                if result.returncode:
                    compile_error = result.stderr.strip()
            else:
                assets = source.parent/'assets'
                for path in sorted(assets.rglob('*')):
                    if path.is_file():
                        relative = path.relative_to(assets)
                        qpath(relative.as_posix())
                        if path.suffix=='.pk3' or not path.resolve().is_relative_to(assets.resolve()):
                            raise ValueError('MAP assets must be loose project files inside assets/')
                        target = base/relative
                        target.parent.mkdir(parents=True,exist_ok=True)
                        shutil.copyfile(path,target)
                (base/'maps').mkdir(parents=True,exist_ok=True)
                shutil.copyfile(source,base/'maps'/(name+'.map'))
                try:
                    compile_map(base,name)
                except (OSError,ValueError,subprocess.SubprocessError) as exc:
                    compile_error = str(exc)
            log = ''
            if (base/'compile.log').exists():
                shutil.copyfile(base/'compile.log',output/'compile.log')
                log = (base/'compile.log').read_text(errors='replace')
                report['warnings'] += [line for line in log.splitlines() if 'WARNING:' in line]
            if compile_error:
                detail = 'leak detected; see compile.log' if 'LEAKED' in log else compile_error
                raise ValueError('compile failed: '+detail)
            missing = [line for line in log.splitlines() if "Couldn't find image" in line or 'Failed to load model' in line]
            if missing:
                raise ValueError('missing assets: '+'; '.join(missing))
            report['structure'] = structure(base,name,authored)
            report['compiled'] = {kind:hashlib.sha256((base/'maps'/(name+'.'+kind)).read_bytes()).hexdigest() for kind in ('map','bsp','aas')}
            for pak in paks:
                (base/pak.name).symlink_to(pak)
            report['bots'] = bots(args,home,name,output)
            if level is not None:
                views,automatic = level.get('viewpoints',[]),flythrough(level)
            else:
                views = camera_specs(json.loads(args.viewpoints.read_bytes())) if args.viewpoints else []
                data = (base/'maps'/(name+'.bsp')).read_bytes()
                start,length = struct.unpack_from('<ii',data,8)
                entities = data[start:start+length].decode().rstrip('\0')
                automatic = []
                for entity in re.findall(r'\{([^{}]+)\}',entities):
                    if '"info_player_deathmatch"' in entity:
                        position = re.search(r'"origin"\s+"([^"\n]+)"',entity)
                        if position:
                            origin = [float(v) for v in position[1].split()]
                            origin[2] += 32
                            automatic.append({'id':f'auto_spawn_{len(automatic)}','origin':origin,'angles':[0,0,0]})
                if not automatic:
                    raise ValueError('existing MAP has no deathmatch spawn for automatic cameras')
            report['views'],report['flythrough'] = screenshots(args,home,name,views,automatic,output)
        for filename in ('client.log','server.log'):
            report['warnings'] += sorted(set(line for line in (output/filename).read_text(errors='replace').splitlines() if 'WARNING:' in line))
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
