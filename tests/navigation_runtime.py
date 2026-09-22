#!/usr/bin/env python3
"""Run cooked navigation and behavior through native bot commands and Pmove."""
import argparse
import gzip
import json
import math
from pathlib import Path
import shutil
import subprocess
import struct
import sys
import tempfile
from cook import cook
from run import ROOT, SCRATCH
sys.path.insert(0, str(ROOT))
from tools.agent import Engine

def pause(engine):
    engine.request('key', name='ESCAPE', down=True)
    engine.request('key', name='ESCAPE', down=False)
    engine.step(2)


def load(engine, path='saves/navigation.000.asstate'):
    loaded = engine.log_path.read_text().count('Game loaded: ')
    engine.request('exec', command='loadgame '+path)
    for _ in range(200):
        engine.step()
        if engine.log_path.read_text().count('Game loaded: ') > loaded:
            return
    raise AssertionError('AI checkpoint reconnect did not complete')


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
mode = parser.add_mutually_exclusive_group()
mode.add_argument('--combat', action='store_true', help='exercise sight, weapon damage and protected cover before checkpoint continuation')
mode.add_argument('--hearing', action='store_true', help='react to a real occluded weapon shot and follow its remembered position')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-navigation-runtime')
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-navigation-runtime-', dir=SCRATCH) as temporary:
    root = Path(temporary)
    source = root/'source'
    source.mkdir()
    home = root/'home'
    base = home/('baseoa' if args.content == 'openarena' else 'baseq3')
    with (args.output/'level.log').open('w') as log:
        subprocess.run([sys.executable, 'tools/level', 'tests/assets/levels/two_lane.json', '--output', str(source)],
                       cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
    shutil.copytree(source, base)
    (base/'maps/two_lane.aas').unlink(missing_ok=True)  # New bots must not depend on legacy AAS.
    navigation = dict(version=1, collision='maps/two_lane.bsp', agent=dict(radius=15,height=56,climb=18,slope=46),
                      cell_size=4, cell_height=2, links=[])
    behavior = dict(version=1, name='patrol', initial='patrol', states=[dict(name='patrol', action='patrol', transitions=[])])
    if args.combat or args.hearing:
        behavior = dict(version=1, name='guard', initial='patrol', states=[
            dict(name='patrol', action='patrol', transitions=[dict(to='attack',field='visible',op='eq',value=1,min_ms=0)]),
            dict(name='attack', action='attack', transitions=[dict(to='cover',field='health',op='lt',value=.9,min_ms=500)]),
            dict(name='cover', action='cover', transitions=[dict(to='attack',field='covered',op='eq',value=1,min_ms=1000)])])
        entities = '''{
"classname" "worldspawn"
}
{
"classname" "info_player_deathmatch"
"origin" "0 -180 48"
"angle" "90"
"nobots" "1"
}
{
"classname" "info_player_deathmatch"
"origin" "0 80 48"
"angle" "270"
"nohumans" "1"
}
'''
        if args.hearing:
            entities = entities.replace('0 -180 48', '768 -180 48').replace('0 80 48', '768 80 48')
            behavior = dict(version=1,name='listener',initial='idle',states=[
                dict(name='idle',action='idle',transitions=[dict(to='investigate',field='heard',op='eq',value=1,min_ms=0)]),
                dict(name='investigate',action='investigate',transitions=[dict(to='attack',field='visible',op='eq',value=1,min_ms=0)]),
                dict(name='attack',action='attack',transitions=[])])
        entities = entities.encode() + b'\0'
        # Bake the two controlled spawns into this owned scratch BSP. A temporary
        # editor override would not match the installed content on checkpoint load.
        bsp = bytearray((source/'maps/two_lane.bsp').read_bytes())
        struct.pack_into('<2i', bsp, 8, len(bsp), len(entities))
        bsp.extend(entities)
        for directory in (source, base):
            (directory/'maps/two_lane.bsp').write_bytes(bsp)
    (source/'navigation.json').write_text(json.dumps(navigation))
    (source/'behavior.json').write_text(json.dumps(behavior))
    (source/'assets.json').write_text(json.dumps(dict(version=1, assets=[
        dict(name='navigation/two_lane', kind='navigation', source='navigation.json'),
        dict(name='behaviors/patrol', kind='behavior', source='behavior.json')])))
    cook(source/'assets.json', base)
    cook(ROOT/'tests/assets/weapons/assets.json', base)
    cook(ROOT/'tests/assets/range.json', base)
    rows = []
    with Engine(args.binary, args.data, args.content, home=home, arguments=[
            '+set', 'g_navigation', 'navigation/two_lane.asnav', '+set', 'g_behavior', 'behaviors/patrol.asai',
            '+set', 'g_weapons', 'weapons/range_rifle.asweapon']) as engine:
        try:
            engine.request('session', dt=20, seed=21)
            engine.request('map', name='two_lane')
            engine.step(50)
            engine.request('exec', command='god; weapon 1' if args.combat or args.hearing else 'team spectator')
            engine.step(5)
            engine.request('exec', command='addbot Sarge 3')
            engine.step(100)
            actor = engine.request('actor', owner=1)
            assert actor.get('ai'), 'cooked behavior/navigation did not activate on the native bot'
            if args.hearing:
                assert actor['ai']['state'] == 'idle' and actor['ai']['target'] == -1
                start = actor['ai']['position']
                engine.request('input',forward=0,right=0,up=0,yaw=270,pitch=0,fire=True)
                for _ in range(20):
                    engine.step()
                    heard = engine.request('actor',owner=1)['ai']
                    if heard['heard']:
                        break
                else:
                    raise AssertionError('occluded weapon shot did not reach native hearing')
                engine.request('input',forward=0,right=0,up=0,yaw=270,pitch=0,fire=False)
                assert heard['target']==0 and not heard['visible'] and heard['state']=='investigate', heard
                # Both actors have the same standing eye height, 260 units apart.
                expected = (1-(260-80)/1250)*.35
                assert abs(heard['gain']-expected)<.002, ('shared distance plus occlusion gain',heard)
                rows.append(heard)
                for _ in range(15):
                    engine.step(5)
                    rows.append(engine.request('actor',owner=1)['ai'])
                assert max(math.dist(start,row['position']) for row in rows)>32, 'heard target did not cause actual pursuit'
                assert any(row['pathCount']>1 and row['complete'] for row in rows), 'no complete investigation path'
            elif args.combat:
                assert actor['ai']['state'] == 'attack' and actor['weapons'][0]['sequence'] > 0, ('visible hostile must trigger native data-weapon fire', actor, engine.request('state')['player'], engine.request('entity.list')['entities'][:2])
                engine.request('subscribe', enabled=True)
                engine.request('exec', command='god')
                engine.step(2)
                initial = engine.request('state')['player']['health']
                for _ in range(60):
                    engine.step()
                    if engine.request('state')['player']['health'] < initial:
                        break
                else:
                    raise AssertionError('bot data weapon never damaged the player')
                engine.request('exec', command='god')
                engine.step(2)
                assert any(e['event']=='hit' and e['actor']==1 and e['target']==0 for e in engine.events)
                start = actor['ai']['position']
                for _ in range(60):
                    target = next(e for e in engine.request('entity.list')['entities'] if e['entity']==1)
                    origin = engine.request('state')['camera']['origin']
                    delta = [target['origin'][i]-origin[i] for i in range(3)]
                    delta[2] += 26
                    yaw = math.degrees(math.atan2(delta[1],delta[0]))
                    pitch = -math.degrees(math.atan2(delta[2],math.hypot(*delta[:2])))
                    engine.request('input',forward=0,right=0,up=0,yaw=yaw,pitch=pitch,fire=True)
                    engine.step()
                    target = next(e for e in engine.request('entity.list')['entities'] if e['entity']==1)
                    if target['health'] < 90:
                        break
                else:
                    raise AssertionError('player shot did not trigger the authored low-health cover rule')
                engine.request('input',forward=0,right=0,up=0,yaw=yaw,pitch=pitch,fire=False)
                for _ in range(100):
                    engine.step(5)
                    rows.append(engine.request('actor',owner=1)['ai'])
                    if any(row['state']=='cover' for row in rows) and rows[-1]['covered']:
                        break
                assert any(row['state']=='cover' for row in rows) and any(row['covered'] for row in rows), 'bot did not reach protected navmesh cover'
                assert max(math.dist(start,row['position']) for row in rows)>32, 'cover must involve actual movement'
            else:
                assert actor['ai']['state'] == 'patrol' and actor['weapons'][0], actor
                for _ in range(30):
                    engine.step(10)
                    actor = engine.request('actor', owner=1)
                    assert actor['ai']['state'] == 'patrol', actor
                    rows.append(actor['ai'])
                assert max(math.dist(rows[0]['position'], row['position']) for row in rows) > 128, 'native bot did not follow its route'
                assert any(row['pathCount'] > 1 and row['complete'] for row in rows), 'no complete navmesh path'
            engine.request('panel', name='AI')
            engine.step(2)
            capture = engine.request('capture', name='navigation-inspector')
            engine.step(2)
            from PIL import Image
            Image.open(base/capture['path']).save(args.output/'navigation-inspector.png')
            engine.request('cvar.set', name='dev_tools', value='0')
            pause(engine)
            assert engine.request('cvar.get', name='sv_paused')['value'] == '1'
            before = engine.request('actor', owner=1)
            engine.request('exec', command='savegame navigation')
            engine.step(2)
            assert (base/'saves/navigation.000.asstate').is_file(), 'AI checkpoint write failed'
            pause(engine)
            engine.step(25)
            continued = engine.request('actor', owner=1)
            assert continued['ai']['elapsed'] != before['ai']['elapsed'] or continued['ai']['transitions'] != before['ai']['transitions']
            load(engine)
            assert engine.request('actor', owner=1) == before, 'AI actor did not restore exactly'
            pause(engine)
            engine.step(25)
            assert engine.request('actor', owner=1) == continued, 'AI path/behavior/weapon continuation differs'

        finally:
            shutil.copyfile(engine.log_path, args.output/'client.log')
            (args.output/'actors.json').write_text(json.dumps(rows, indent=2))
    with Engine(args.binary, args.data, args.content, home=home) as engine:
        try:
            engine.request('session', dt=20, seed=123)
            load(engine)
            assert engine.request('actor', owner=1) == before, 'fresh-process AI restore differs'
            pause(engine)
            engine.step(25)
            assert engine.request('actor', owner=1) == continued, 'fresh-process AI continuation differs'
            if args.content == 'openarena':
                legacy = ROOT/'tests/assets/state/checkpoint-v1.asstate.gz'
                (base/'saves/pre-navigation.asstate').write_bytes(gzip.decompress(legacy.read_bytes()))
                load(engine, 'saves/pre-navigation.asstate')
                assert engine.request('actor', owner=1)['ai'] is None, 'old checkpoint must restore the legacy controller'
                for name in ('g_navigation', 'g_behavior'):
                    assert engine.request('cvar.get', name=name)['value'] == '', 'old checkpoint retained a live AI asset selection'

        finally:
            shutil.copyfile(engine.log_path, args.output/'checkpoint-restart.log')
print('PASS: cooked navmesh/behavior, native Pmove/weapon, inspector and same/fresh-process checkpoint continuation')
