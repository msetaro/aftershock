#!/usr/bin/env python3
"""Render the owned reference effects, PBR LOD model, decals and temporal/filmic passes together."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from PIL import Image, ImageChops, ImageStat
from cook import cook, model_header
from materials_runtime import source as sphere_source
from run import ROOT, SCRATCH
sys.path.insert(0, str(ROOT))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--record-reference', action='store_true', help='create new reviewed software references; refuses existing files')
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-fidelity-runtime')
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
reference = ROOT/'tests/golden/fidelity'
if args.record_reference:
    reference.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-fidelity-') as temporary:
    root = Path(temporary)
    source = root/'source'
    source.mkdir()
    compiled = args.output/'compiled'
    subprocess.run([sys.executable, 'tools/level', str(ROOT/'tests/assets/levels/two_lane.json'),
                    '--output', str(compiled)], cwd=ROOT, check=True)
    project, document = sphere_source(source)
    recipe = json.loads(project.read_text())
    recipe['assets'][0].update(lod_ratios=[.5, .25], lod_error=.1)
    project.write_text(json.dumps(recipe))
    cooked = args.output/'cooked'
    cook(project, cooked)
    triangles = [model_header((cooked/('models/sphere'+suffix+'.iqm')).read_bytes())[10]
                 for suffix in ('', '_lod1', '_lod2')]
    assert triangles[0] > triangles[1] > triangles[2] > 0, triangles
    for kind in ('decals', 'effects'):
        cook(ROOT/('tests/assets/'+kind+'/assets.json'), cooked)
    definition = dict(version=1, name='sphere', emitters=[dict(name='sphere', kind='mesh',
                      material='models/sphere_material0', model='models/sphere.iqm', capacity=1,
                      rate=0, burst=1, lifetime_ms=60000, size=1, color=[1, 1, 1, 1])])
    (source/'sphere.effect.json').write_text(json.dumps(definition))
    (source/'post.json').write_text(json.dumps(dict(version=1, name='fidelity', exposure_ev=0, sharpen=.2)))
    project.write_text(json.dumps(dict(version=1, assets=[
        dict(name='effects/fidelity_sphere', kind='effect', source='sphere.effect.json'),
        dict(name='post/fidelity', kind='post', source='post.json')])))
    cook(project, cooked)
    settings = ['+set', 'r_fbo', '1', '+set', 'r_postProcess', '1', '+set', 'r_postProfile', 'post/fidelity.aspost',
                '+set', 'r_taa', '1', '+set', 'r_decals', '1', '+set', 'r_softParticles', '1',
                '+set', 'r_lodbias', '0', '+set', 'r_drawentities', '1',
                '+set', 'cg_draw2D', '0', '+set', 'cg_drawGun', '0', '+set', 'con_notifytime', '0',
                '+set', 'r_mode', '-1', '+set', 'r_customwidth', '640', '+set', 'r_customheight', '480']
    with Engine(args.binary, args.data, args.content, home=root/'home', arguments=settings) as engine:
        shutil.copytree(compiled, engine.base, dirs_exist_ok=True)
        shutil.copytree(cooked, engine.base, dirs_exist_ok=True)
        shutil.copyfile(ROOT/'tests/assets/effects/reference.shader', engine.base/'scripts/reference.shader')
        engine.request('session', dt=20, seed=161)
        engine.request('map', name='two_lane')
        engine.step(160)
        engine.request('camera', mode='pose', origin=[-230, 0, 110], angles=[18, 0, 0])
        engine.step(32)

        def capture(name):
            result = engine.request('capture', name=name)
            engine.step(2)
            shutil.copyfile(engine.log_path, args.output/'engine.log')
            path = engine.base/result['path']
            shutil.copyfile(path, args.output/(name+'.png'))
            with Image.open(path) as image:
                return image.convert('RGB')

        frames = {}
        baseline = frames['baseline'] = capture('baseline')
        sphere = engine.request('effects.load', path='effects/fidelity_sphere.asfx')['handle']
        assert sphere
        engine.request('effects.start', asset=sphere, origin=[80, 0, 55], angles=[0, 0, 0], seed=161)
        names = ('muzzle', 'impact_metal', 'impact_stone', 'smoke', 'sparks', 'dust', 'shell', 'explosion', 'tracer')
        handles = [engine.request('effects.load', path='effects/reference/'+name+'.asfx')['handle'] for name in names]
        assert all(handles)
        decals = [engine.request('decals.load', path='decals/reference/'+name+'.asdc')['handle']
                  for name in ('bullet', 'scorch', 'blood')]
        assert all(decals)
        for i, asset in enumerate(decals):
            assert engine.request('decals.project', asset=asset, origin=[-100, (i-1)*65, 0], angles=[0, 0, 0])['handle']
        for i, asset in enumerate(handles):
            assert engine.request('effects.start', asset=asset, origin=[-90+(i//3)*70, (i%3-1)*90, 35],
                                  angles=[0, 0, 0], seed=161+i)['handle']
        engine.step(2)
        active = frames['combined'] = capture('combined')
        profile = engine.request('profile')
        effects, decals = engine.request('effects'), engine.request('decals')
        materials = engine.request('assets', kind='materials', filter='models/sphere')['items']
        models = engine.request('assets', kind='models', filter='models/sphere.iqm')['items']
        report = dict(triangles=triangles, profile=profile, effects=effects, decals=decals, materials=materials, models=models)
        assert any(row['metallicRoughness'] for row in materials), materials
        assert models[0]['lods'] == 3 and sum(models[0]['lodDraws']) > 0, models
        assert effects['draws'] > 0 and effects['softDraws'] > 0 and effects['lightDraws'] > 0, effects
        assert decals['active'] >= 3 and decals['draws'] > 0 and decals['dropped'] == 0, decals
        assert profile['post']['draws'] > 0 and profile['post']['dropped'] == 0, profile['post']
        assert sum(ImageStat.Stat(ImageChops.difference(baseline, active)).sum) > 100000, 'combined presentation absent'
        timings = profile['presentationCpuUsec']
        assert all(timings[name] > 0 for name in ('effects', 'decals', 'effectsDraw', 'lod')), timings
        names = {row['name'] for row in profile['gpu']}
        assert 'decals' in names, names
        assert {'camera motion', 'object motion', 'temporal resolve', 'temporal copy'} <= names, names
        assert profile['temporal']['frames'] > 0 and profile['temporal']['dropped'] == 0, profile['temporal']
        engine.step(12)
        frames['settling'] = capture('settling')
        engine.request('cvar.set', name='r_lodscale', value='1')
        engine.step(32)
        frames['reduced'] = capture('reduced')
        reduced = engine.request('assets', kind='models', filter='models/sphere.iqm')['items'][0]
        assert reduced['lodDraws'][2] > models[0]['lodDraws'][2], reduced
        report['reduced_model'] = reduced
        report['lowest_lod_triangle_saving'] = 1-triangles[2]/triangles[0]
        (args.output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        for name, frame in frames.items():
            expected = reference/(name+'.png')
            if args.record_reference:
                with expected.open('xb') as stream:
                    frame.save(stream, format='PNG')
            else:
                with Image.open(expected) as image:
                    assert frame.size == image.size and frame.tobytes() == image.convert('RGB').tobytes(), name+' differs from reviewed software reference'

print('PASS: combined PBR/LOD model, reference effects, projected decals and temporal/filmic presentation')
