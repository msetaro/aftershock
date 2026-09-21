"""Trace, theme, compile and measure one original sketch in a fresh output directory."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from PIL import Image

from sketch import measure,overlays
from theme import assemble
from overhead import compare
from intents import analyze,play_routes
from tools.agent import Engine,ROOT
from tools.agent.playtest import load_script,run_script


def main(argv):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sketch',type=Path,required=True)
    parser.add_argument('--notes',type=Path,required=True)
    parser.add_argument('--theme',choices=['manhattan'],required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--previous',type=Path,help='earlier interpretation.json for stable IDs')
    parser.add_argument('--library',type=Path,help='reuse a validated material kit')
    parser.add_argument('--modules',type=Path,help='reuse a validated Blender module kit')
    parser.add_argument('--client',type=Path)
    parser.add_argument('--server',type=Path)
    parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
    parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
    parser.add_argument('--threshold',type=float,default=.93)
    parser.add_argument('--playtest',type=Path,help='optional additional agent playtest, using this build session')
    parser.add_argument('--jobs',type=int,default=min(8,os.cpu_count() or 1))
    args=parser.parse_args(argv)
    output=args.out.resolve()
    report=dict(version=1,status='failed',errors=[])
    created=False
    def command(arguments):
        result=subprocess.run([sys.executable,*map(str,arguments)],cwd=ROOT,capture_output=True,text=True)
        if result.returncode:
            raise ValueError(result.stderr.strip() or result.stdout.strip())
        return json.loads(result.stdout)
    def save(folder,name,value):
        (folder/name).write_text(json.dumps(value,sort_keys=True,indent=2)+'\n')
    try:
        if output.exists():
            raise ValueError('output already exists; choose a fresh --out directory')
        if bool(args.client)!=bool(args.server):
            raise ValueError('provide both --client and --server, or neither to build them')
        if not 0<args.threshold<=1 or not 1<=args.jobs<=256:
            raise ValueError('threshold must be in (0,1] and jobs in 1..256')
        if not list(args.data.resolve().glob('*.pk3')):
            raise ValueError('installed game content is required; supply --data and --content')
        if args.notes.stat().st_size>1024*1024:
            raise ValueError('notes exceed 1 MiB')
        notes=json.loads(args.notes.read_text())
        if notes.get('theme',args.theme)!=args.theme:
            raise ValueError('notes and command theme disagree')
        if notes.get('mode','ffa')!='ffa':
            raise ValueError('integrated bot acceptance currently supports ffa; other modes require their native mode gate')
        seed=notes.get('seed',164)
        script=load_script(args.playtest) if args.playtest else None
        if script and (script.get('dt',20)!=20 or script.get('seed',seed)!=seed or script.get('cvars')):
            raise ValueError('following playtest must use this build seed, dt=20 and no session cvar overrides')
        previous=json.loads(args.previous.read_text()) if args.previous else None
        output.mkdir(parents=True)
        created=True
        trace=output/'trace'
        trace.mkdir()
        with Image.open(args.sketch) as source:
            interpretation,level,image=measure(source,notes,previous)
        overlays(image,interpretation,trace)
        interpretation['source_sha256']=hashlib.sha256(args.sketch.read_bytes()).hexdigest()
        save(trace,'interpretation.json',interpretation)
        save(trace,'level.json',level)
        report['assumptions']=interpretation['assumptions']
        if report['assumptions']:
            raise ValueError('unresolved drawing assumptions; review trace/overlay.png and revise the structured notes')
        theme=json.loads((Path(__file__).parent/'themes'/(args.theme+'.json')).read_text())
        library=args.library or output/'kits/materials'
        modules=args.modules or output/'kits/modules'
        if not args.library:
            command(['tools/assets','fetch','--theme',args.theme,'--out',library])
        if not args.modules:
            save(output,'blender-parameters.json',dict(version=1,seed=seed,bay_width=4,storey_height=4,wall_thickness=.5))
            command(['tools/blender','kit','--parameters',output/'blender-parameters.json','--out',modules])
        level=assemble(level,theme,library,modules,output/'project',seed)
        report['assembly']=json.loads((output/'project/assembly.json').read_text())
        compiled=output/'compiled'
        compilation=command(['tools/level',output/'project/level.json','--output',compiled])
        report['compile']=compilation
        report['overhead']=compare((compiled/'maps'/(level['name']+'.bsp')).read_bytes(),interpretation,output/'overhead',args.threshold)
        if not report['overhead']['passed']:
            raise ValueError('compiled overhead is below the per-class overlap threshold; inspect overhead/overhead-difference.png')
        client,server=args.client,args.server
        if not client:
            build=output/'build'
            with (output/'build.log').open('w') as log:
                subprocess.run(['cmake','-S',str(ROOT),'-B',str(build),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',
                                '-DAFTERSHOCK_DEVTOOLS=ON'],stdout=log,stderr=subprocess.STDOUT,check=True)
                subprocess.run(['cmake','--build',str(build),'--parallel',str(args.jobs)],stdout=log,stderr=subprocess.STDOUT,check=True)
            binaries=[p for p in build.glob('release-*/*') if p.is_file() and p.name.startswith('quake3e.') and p.suffix in ('.x64','.aarch64','.exe')]
            clients=[p for p in binaries if '.ded.' not in p.name]
            servers=[p for p in binaries if '.ded.' in p.name]
            if len(clients)!=1 or len(servers)!=1:
                raise ValueError('expected one development client and dedicated server; inspect build.log')
            client,server=clients[0],servers[0]
        with Engine(client,args.data,args.content) as engine:
            shutil.copytree(compiled,engine.base,dirs_exist_ok=True)
            engine.request('session',dt=20,seed=seed)
            engine.request('map',name=level['name'])
            engine.step(150)
            report['shooter']=analyze(engine,level,theme.get('limits'))
            save(output,'shooter.json',report['shooter'])
            if not report['shooter']['passed']:
                raise ValueError('compiled shooter checks failed; inspect shooter.json for suggested fixes')
            report['routes']=play_routes(engine,level,output/'routes',seed)
            if not report['routes']['passed']:
                raise ValueError('native route playback failed; inspect routes/routes.json')
            if script:
                (output/'playtest').mkdir()
                report['playtest']=run_script(engine,script,level['name'],output/'playtest',configure_session=False)
                if not report['playtest']['ok']:
                    raise ValueError('additional agent playtest failed; inspect playtest/report.json')
        report['runtime']=command(['tools/level','validate',output/'project/level.json','--output',output/'runtime',
                                   '--client',client,'--server',server,'--content',args.content,'--data',args.data])
        runtime=report['runtime']
        if runtime['compiled']!=compilation['sha256']:
            raise ValueError('repeated compilation differs from the published map')
        if runtime['status']!='passed' or runtime['bots']['kills']<2 or runtime['bots']['stuck']:
            raise ValueError('bot match did not meet activity/clearance requirements; inspect runtime/report.json')
        if not runtime['views'] or not runtime['flythrough']:
            raise ValueError('native eye-level captures or flythrough are absent; author viewpoints')
        report['status']='passed'
    except (OSError,ValueError,KeyError,TypeError,RuntimeError,TimeoutError,subprocess.SubprocessError) as error:
        report['errors'].append(str(error))
        print(json.dumps(dict(ok=False,error=str(error),report=str(output/'report.json') if created else None)),file=sys.stderr)
    if created:
        save(output,'report.json',report)
    print(json.dumps(report,sort_keys=True))
    return 0 if report['status']=='passed' else 1
