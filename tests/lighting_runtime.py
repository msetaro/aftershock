#!/usr/bin/env python3
"""Measure normal-mapped baked lighting on an owned level through the native client."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from PIL import Image
from cook import cook
from run import ROOT, content_settings

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-lighting-runtime'))
parser.add_argument('--shadows', action='store_true', help='check point, spot and cascaded-sun occlusion')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
paks = sorted(args.data.resolve().glob('*.pk3'))
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if not paks or len(icds) != 1:
    parser.error('installed content and one lavapipe ICD are required')
with tempfile.TemporaryDirectory(prefix='aftershock-baked-light-') as temporary:
    home = Path(temporary)
    source = home/'source'
    shutil.copytree(ROOT/'tests/assets/levels', source)
    level = json.loads((source/'two_lane.json').read_text())
    level['lighting']['directional'] = True
    document = source/'level.json'
    document.write_text(json.dumps(level))
    base = home/('baseoa' if args.content == 'openarena' else 'baseq3')
    subprocess.run([sys.executable, str(ROOT/'tools/level'), str(document), '--output', str(base)], check=True)
    # New loose PBR material replaces only this test's owned wall/floor textures.
    Image.new('RGBA', (16,16), (96,96,96,255) if args.shadows else (220,180,120,255)).save(source/'base.png')
    Image.new('RGBA', (16,16), (128,128,255 if args.shadows else 0,255)).save(source/'normal.png')
    material = {'pbrMetallicRoughness': {'baseColorTexture': {'uri':'base.png'},
                 'metallicFactor':0, 'roughnessFactor':1}, 'normalTexture': {'uri':'normal.png'}}
    (source/'paint.json').write_text(json.dumps(material))
    project = source/'paint-assets.json'
    project.write_text(json.dumps({'version':1, 'assets':[
        {'name':'textures/level/'+name, 'kind':'material', 'source':'paint.json',
         'material_model':'metallic-roughness'} for name in ('wall','floor','cover','trim','prop')]}))
    cook(project, base)
    for pak in paks:
        (base/pak.name).symlink_to(pak)
    captures = {}
    for merge in ((1,) if args.shadows else (0,1)):
        commands = ['set g_synchronousClients 1', 'set fixedtime 20', 'devmap two_lane', 'wait 40',
                    'team spectator', 'wait 10', 'cmd dev_view -160 0 96 10 0 0', 'wait 10']
        if args.shadows:
            commands += ['set r_directionalLightmaps 0', 'set r_shadowSun 0']
            light = '96 -128 160 512 1 0.8 0.5 2'
            cases = [('baseline', ['dev_light off'])]
            for kind, command in [('point', 'dev_light point '+light),
                                  ('spot', 'dev_light spot '+light+' 0 0 -1 25 45'),
                                  ('sun', 'dev_light off; set r_shadowSun 1')]:
                cases += [(kind+'-flat', [command, 'set r_shadowOcclusion 0']),
                          (kind+'-shadow', ['set r_shadowOcclusion 1']),
                          (kind+'-restored', ['set r_shadowOcclusion 0'])]
        else:
            cases = [(name, [f'set r_directionalLightmaps {setting}'])
                     for name, setting in [('flat',0), ('normal',1), ('restored',0)]]
        for name, settings in cases:
            commands += settings + ['wait 4',
                         'screenshot '+name, 'wait 3']
        commands += ['quit']
        (base/'lighting.cfg').write_text('\n'.join(commands)+'\n')
        log_path = args.output/f'client-merge{merge}.log'
        env = dict(os.environ, LP_NUM_THREADS='1', VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]))
        with log_path.open('w') as log:
            subprocess.run(['timeout','90','xvfb-run','-a','faketime','-f','@2026-01-01 00:00:00 i0.01',str(args.binary.resolve()),
                '+set','fs_basepath',str(home),'+set','fs_homepath',str(home), *content_settings(args.content),
                '+set','net_enabled','0','+set','sv_pure','0','+set','r_mode','3','+set','r_fullscreen','0',
                '+set','r_mergeLightmaps',str(merge),'+set','r_gamma','1','+set','r_overBrightBits','0',
                *(['+set','r_shadowQuality','2'] if args.shadows else []),
                '+set','r_intensity','1','+set','s_initsound','0','+set','dev_tools','0',
                '+set','cl_autoRecordDemo','0','+set','com_maxfps','0','+exec','lighting.cfg'],
                cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
        text = log_path.read_text()
        assert 'Directional lightmaps: 1 pair' in text, 'native directional pages were not recognized'
        assert not any(word in text for word in ('ERROR:', 'Signal caught', 'could not find shader'))
        if args.shadows:
            assert 'Unknown command' not in text, 'native scene-light controls are missing'
        for name, _ in cases:
            path = base/'screenshots'/(name+'.tga')
            shutil.copyfile(path,args.output/f'{name}-merge{merge}.tga')
            with Image.open(path) as image:
                # Static world interior only: excludes HUD and the map's sky band.
                captures[merge,name] = image.convert('RGB').crop((20,160,620,420)).tobytes()
        if args.shadows:
            baseline = captures[merge,'baseline']
            for kind in ('point','spot','sun'):
                flat, shadow, restored = [captures[merge,kind+'-'+name] for name in ('flat','shadow','restored')]
                assert flat == restored, kind+' light/shadow round trip changed the static view'
                lit = sum(a != b for a,b in zip(baseline,flat))
                occluded = sum(a != b for a,b in zip(flat,shadow))
                assert lit > 1000 and occluded > 100, (kind, lit, occluded)
                assert sum(shadow) < sum(flat), kind+' shadow did not attenuate its light'
                print(f'PASS: {kind}, {lit} lit / {occluded} shadowed channel bytes and exact restored light')
            continue
        flat, normal, restored = [captures[merge,name] for name in ('flat','normal','restored')]
        assert flat == restored, 'baked-light quality round trip changed the static view'
        changed = sum(a != b for a,b in zip(flat,normal))
        assert changed > 10000, f'normal-mapped baked lighting did not change: {changed} bytes'
        print(f'PASS: merged={merge}, {changed} changed channel bytes and exact restored baked light')
