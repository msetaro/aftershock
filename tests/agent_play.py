#!/usr/bin/env python3
"""Check seeded local play and input through the real snapshot/usercmd path."""
import argparse
import math
from pathlib import Path
from PIL import Image
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
args = parser.parse_args()
map_name = 'oa_dm1' if args.content == 'openarena' else 'q3dm17'
trajectories = []
for _ in range(2):
    with Engine(args.binary, args.data, args.content, arguments=('+set', 'developer', '1')) as engine:
        engine.request('hello')
        engine.request('session', dt=8, seed=123)
        engine.request('map', name=map_name)
        engine.step(200)
        before = engine.request('state')
        assert before['player'] and before['player']['health'] > 0 and before['camera'], (before, engine.log_path.read_text(errors='replace')[-5000:])
        engine.request('input', forward=1, right=0, up=0, yaw=90, pitch=0, fire=False)
        engine.step(30)
        moving = engine.request('state')
        assert before['player']['origin'] != moving['player']['origin']
        engine.request('input', forward=0, right=0, up=0, yaw=90, pitch=0, fire=False)
        engine.step(30)
        stopped = engine.request('state')
        trajectories.append([row['player'] for row in (before, moving, stopped)])
        entity = engine.request('entity.spawn', classname='target_position', x=1, y=2, z=128)['entity']
        engine.request('entity.set', entity=entity, key='targetname', value='agent-check')
        assert engine.request('entity.get', entity=entity, key='targetname')['value'] == 'agent-check'
        listed = engine.request('entity.list', offset=entity, limit=1)['entities']
        assert listed[0]['entity'] == entity and listed[0]['origin'] == [1, 2, 128]
        engine.request('entity.delete', entity=entity)
        assert all(row['entity'] != entity for row in engine.request('entity.list', offset=entity, limit=1)['entities'])
        engine.request('exec', command='screenshot agent-reference')
        capture = engine.request('capture', name='agent-check')
        engine.step(2)
        with Image.open(engine.base/capture['path']) as image:
            assert image.format == 'PNG' and image.size == (640, 480)
            assert len(image.convert('RGB').getcolors(640*480)) > 100
            with Image.open(engine.base/'screenshots/agent-reference.tga') as reference:
                assert image.convert('RGB').tobytes() == reference.convert('RGB').tobytes()
        profile = engine.request('profile')
        assert isinstance(profile['gpu'], list) and profile['memory']['hunkTotal'] > 0
        assert profile['samples'] == 262 and 0 <= profile['p50_ms'] <= profile['p95_ms'] <= profile['p99_ms']
        assert profile['cpu'] and 'snapshots' in profile['network']
        engine.request('usercmd', forwardmove=127, rightmove=0, upmove=0,
                       angles=[0, 0, 0], buttons=0, weapon=2)
        engine.step(10)
        raw = engine.request('state')
        assert raw['player']['commandTime'] > stopped['player']['commandTime']
        trajectories[-1].append(raw['player'])
        origin = [round(value)+offset for value, offset in zip(raw['camera']['origin'], (32, 0, 64))]
        engine.request('camera', mode='pose', origin=origin, angles=[30, 90, 0])
        engine.step(2)
        assert engine.request('state')['camera']['origin'] == origin, (engine.request('state')['camera']['origin'], origin)
        engine.request('camera', mode='player')
        engine.step(2)
        assert engine.request('state')['camera']['origin'] != origin
        engine.request('subscribe', enabled=True)
        engine.request('exec', command='error drop')
        try:
            engine.step(3)
        except ValueError as error:
            assert error.args[0]['code'] == 'engine_error', error
        else:
            raise AssertionError(('a dropped frame must fail the step request', engine.events, engine.log_path.read_text(errors='replace')[-3000:]))
        errors = [event for event in engine.events if event['event'] == 'error']
        assert len(errors) == 1 and errors[0]['detail'] == 'Testing drop error', errors
        assert engine.request('state')['player'] is None
assert trajectories[0] == trajectories[1], trajectories
# A paused ordinary bot supplies a controlled target; damage and death use the
# real game path. Teleport only sets up the encounter on each installed map.
with Engine(args.binary, args.data, args.content, arguments=('+set', 'bot_enable', '1')) as engine:
    engine.request('session', dt=20, seed=123)
    engine.request('map', name='oa_dm1' if args.content == 'openarena' else 'q3dm7')
    engine.step(150)
    engine.request('exec', command='addbot sarge 1; bot_pause 1; god; give all')
    engine.step(150)
    target = next(row for row in engine.request('entity.list')['entities'] if row['entity'] == 1)
    x, y, z = target['origin']
    engine.request('exec', command=f'setviewpos {x-64 if args.content == 'openarena' else x+100} {y} {z+10} 180; weapon 7')
    engine.step(30)
    engine.request('subscribe', enabled=True)
    for _ in range(40):
        target = next(row for row in engine.request('entity.list')['entities'] if row['entity'] == 1)
        origin = engine.request('state')['camera']['origin']
        delta = [target['origin'][axis] - origin[axis] for axis in range(3)]
        delta[2] += 12
        yaw = math.degrees(math.atan2(delta[1], delta[0]))
        pitch = -math.degrees(math.atan2(delta[2], math.hypot(*delta[:2])))
        engine.request('input', forward=0, right=0, up=0, yaw=yaw, pitch=pitch, fire=True)
        engine.step(10)
        if any(event['event'] == 'kill' for event in engine.events):
            break
    hits = [event for event in engine.events if event['event'] == 'hit']
    kills = [event for event in engine.events if event['event'] == 'kill']
    assert hits and all(event['actor'] == 0 and event['target'] == 1 and event['value'] > 0 for event in hits), hits
    assert len(kills) == 1 and kills[0]['actor'] == 0 and kills[0]['target'] == 1, kills
    assert hits[-1]['frame'] == kills[0]['frame'] and hits[-1]['time'] == kills[0]['time']
print('PASS: identical seeded snapshots, injected input, PNG pixels, telemetry, errors, real hits and kills')
