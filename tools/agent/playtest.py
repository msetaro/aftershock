"""Scripted input uses the same channel as interactive clients."""
import json
import math
from pathlib import Path
import shutil

from jsonschema import Draft202012Validator
from jsonschema.exceptions import best_match

SCHEMA = Path(__file__).with_name('schemas')/'playtest.json'


def load_script(path):
    def reject_constant(value):
        raise ValueError(f'expected a finite JSON number, got {value}')
    if path.stat().st_size > 1024*1024:
        raise ValueError('playtest exceeds 1 MiB; split it into bounded scripts')
    document = json.loads(path.read_text(), parse_constant=reject_constant)
    error = best_match(Draft202012Validator(json.loads(SCHEMA.read_text())).iter_errors(document))
    if error:
        location = '$' + ''.join(f'[{part}]' if isinstance(part, int) else '.'+part for part in error.absolute_path)
        raise ValueError(dict(file=str(path), path=location, hint=error.message + '; use tools/agent/schemas/playtest.json'))
    return document


def target_position(engine, target):
    if 'position' in target:
        return target['position']
    offset = target.get('entity', 0)
    while offset is not None:
        page = engine.request('entity.list', offset=offset, limit=1 if 'entity' in target else 32)
        for row in page['entities']:
            if (row['entity'] == target.get('entity') or row['classname'] == target.get('classname')) and row['linked']:
                return [(lo+hi)/2 for lo, hi in zip(row['mins'], row['maxs'])]
        if 'entity' in target:
            break
        offset = page['next']
    raise ValueError(f'target {target} is not present/linked; load or spawn it before aiming')


def aim(engine, target):
    state = engine.request('state')
    if not state['player'] or not state['camera']:
        raise ValueError('aim requires an active local player')
    delta = [point-eye for point, eye in zip(target_position(engine, target), state['camera']['origin'])]
    return dict(yaw=math.degrees(math.atan2(delta[1], delta[0])),
                pitch=max(-89, min(89, -math.degrees(math.atan2(delta[2], math.hypot(*delta[:2]))))))


def input_state(engine, **fields):
    engine.request('input', **(dict(forward=0, right=0, up=0, yaw=0, pitch=0) | fields))


def run_script(engine, script, map_name, output, *, configure_session=True):
    report = dict(version=1, ok=False, map=map_name, results=[], captures=[])
    location = '$'
    try:
        engine.request('hello')
        if configure_session:
            engine.request('session', dt=script.get('dt', 20), seed=script.get('seed', 1))
        engine.request('subscribe', enabled=True)
        engine.request('map', name=map_name)
        engine.step(script.get('warmup', 150))
        for index, step in enumerate(script['steps']):
            location = f'$.steps[{index}]'
            op = step['op']
            result = dict(index=index, op=op)
            if op == 'request':
                result['reply'] = engine.request(**step['request'])
            elif op == 'step':
                result['reply'] = engine.step(step['frames'])
            elif op == 'aim':
                angles = aim(engine, step['target'])
                input_state(engine, **angles)
                result['angles'] = angles
            elif op == 'fire':
                angles = dict(zip(('pitch', 'yaw'), engine.request('state')['player']['angles'][:2]))
                for _ in range(step['frames']):
                    if 'target' in step:
                        angles = aim(engine, step['target'])
                    input_state(engine, fire=True, **angles)
                    engine.step()
                input_state(engine, **angles)
                engine.step()
            elif op == 'walk':
                start = engine.request('state')['player']['origin']
                destination = step.get('position') or [a+b for a, b in zip(start, step['offset'])]
                for _ in range(step.get('max_frames', 300)):
                    current = engine.request('state')['player']['origin']
                    delta = [a-b for a, b in zip(destination, current)]
                    if math.dist(destination, current) <= step.get('tolerance', 8):
                        break
                    input_state(engine, forward=min(1, math.hypot(*delta[:2])/32),
                                yaw=math.degrees(math.atan2(delta[1], delta[0])))
                    engine.step()
                else:
                    raise ValueError(f'waypoint {destination} not reached; check collision/height or increase max_frames')
                input_state(engine)
                result['destination'] = destination
                result['state'] = engine.request('state')
            elif op == 'capture':
                if 'camera' in step:
                    engine.request('camera', **step['camera'])
                    engine.step()
                capture = engine.request('capture', name=step['name'])
                engine.step(2)
                name = step['name']+'.png'
                shutil.copyfile(engine.base/capture['path'], output/name)
                report['captures'].append(name)
                result['state'] = engine.request('state')
            elif op == 'assert':
                profile = engine.request('profile')
                metric = step['metric']
                value = (sum(event['event'] == 'assert' for event in engine.events) if metric == 'asserts'
                         else profile['p99_ms'] if metric == 'p99_ms' else profile['events'][metric])
                if not step.get('min', -math.inf) <= value <= step.get('max', math.inf):
                    raise ValueError(f'{metric}={value} is outside [{step.get("min", "-inf")}, {step.get("max", "inf")}]; inspect events and engine.log')
                result['value'] = value
            report['results'].append(result)
        report['profile'] = engine.request('profile')
        report['state'] = engine.request('state')
        report['ok'] = True
    except (OSError, ValueError, RuntimeError, TimeoutError) as error:
        report['error'] = dict(path=location, hint=str(error))
    finally:
        report['events'] = engine.events
        shutil.copyfile(engine.log_path, output/'engine.log')
        (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    return report
