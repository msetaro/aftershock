#!/usr/bin/env python3
"""Offline asset cooker. Emit native payloads and dependency manifests with content hashes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import struct
import time
import tempfile

import audio
import animation
import model
import texture
import shader
import weapon
import effect
import post
import entities
import ui
import navigation
import behavior

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(ROOT))
from tools.scratch import ROOT as SCRATCH
from tools.agent.formats import validate as validate_format, diagnostic


def digest(data):
    return hashlib.sha256(data).hexdigest()


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), allow_nan=False).encode()


def tool_hash():
    paths = [p for p in HERE.iterdir() if p.suffix in ('.py', '.cpp', '.txt')]
    vendor = ROOT / 'third_party/bc7enc'
    paths.extend(vendor / name for name in ('bc7enc.cpp', 'bc7enc.h', 'provenance.json'))
    paths.extend(p for p in (ROOT / 'third_party/meshoptimizer').rglob('*') if p.is_file())
    paths.extend(ROOT / path for path in ('cmake/Sources.cmake', 'tools/shaders/build.py', 'engine/renderervk/shaders/manifest.json', 'tools/agent/formats.py'))
    paths.extend(p for p in (ROOT / 'third_party/recast').rglob('*') if p.is_file())
    paths.extend(ROOT / 'engine/qcommon' / name for name in ('cm_load.cpp','cm_patch.cpp','cm_physics.cpp','cm_polylib.cpp','cm_test.cpp','q_math.cpp','q_shared.cpp','md4.cpp','state.cpp',
        'cm_local.h','cm_patch.h','cm_polylib.h','cm_public.h','q_shared.h','qcommon_public.h','qfiles_public.h','surfaceflags_public.h'))
    paths.extend(ROOT / path for path in ('engine/public/state_public.h','engine/public/assert_public.h',
        'engine/platform/file_types_public.h','engine/platform/platform_public.h','third_party/sha256/sha-256.c','third_party/sha256/sha-256.h'))
    for codec in ('libogg', 'libvorbis'):
        paths.extend(p for p in (ROOT / 'third_party' / codec).rglob('*') if p.suffix in ('.c', '.h'))
    return digest(b''.join(str(p.relative_to(ROOT)).encode() + b'\0' + p.read_bytes() for p in sorted(paths)))


def below(root, path):
    result = (root / path).resolve()
    if not result.is_relative_to(root):
        raise ValueError('asset path escapes its project/output directory: ' + str(path))
    return result


def write(path, data):
    if path.is_file() and path.read_bytes() == data:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix='.' + path.name, delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(data)
        os.replace(temporary, path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def cook(project, output):
    root = project.parent.resolve()
    output = output.resolve()
    definition = json.loads(project.read_bytes())
    if definition.get('version') != 1:
        raise ValueError('asset project version must be 1')
    tool = tool_hash()
    built, skipped, names, owners, resources = [], [], set(), {}, {}
    for asset in definition['assets']:
        name = asset['name']
        if not re.fullmatch(r'[a-z0-9_/-]+', name) or name.startswith('/') or '..' in name or len(name) > 59 or name in names:
            raise ValueError('asset names must be unique relative lowercase qpaths, at most 59 characters')
        names.add(name)
        recipe_data = {'asset': asset, 'tool': tool}
        if asset['kind'] == 'animation':
            recipe_data['models'] = [a for a in definition['assets'] if a['kind'] == 'model']
        recipe = digest(canonical(recipe_data))
        manifest_path = below(output, name + '.manifest.json')
        previous = None
        try:
            previous = json.loads(manifest_path.read_bytes())
            unchanged = previous['version'] == 1 and previous['name'] == name and previous['recipe'] == recipe
            unchanged = unchanged and all(digest(below(root, item['path']).read_bytes()) == item['sha256'] for item in previous['inputs'])
            unchanged = unchanged and all(digest(below(output, item['path']).read_bytes()) == item['sha256'] for item in previous['outputs'])
        except (OSError, ValueError, KeyError, TypeError):
            unchanged = False
        if unchanged:
            skipped.append(name)
            records = previous['outputs']
        else:
            inputs = {}

            def read(path):
                source = below(root, path)
                if source.stat().st_size > 256 << 20:
                    raise ValueError('source exceeds the 256 MiB cooker input limit: ' + str(source))
                data = source.read_bytes()
                inputs[source.relative_to(root).as_posix()] = digest(data)
                return data

            source = below(root, asset['source'])
            try:
                if asset['kind'] in ('weapon', 'animation', 'material', 'effect', 'decal', 'post', 'entities', 'ui', 'sound-event', 'navigation', 'behavior'):
                    validate_format(asset['kind'], json.loads(read(source)), source)
                if asset['kind'] == 'model':
                    payloads = model.cook(source, name, asset, read)
                elif asset['kind'] == 'animation':
                    models = [dict(a, _source=below(root, a['source'])) for a in definition['assets'] if a['kind'] == 'model']
                    payloads = animation.cook(source, name, asset, read, models)
                elif asset['kind'] == 'weapon':
                    payloads = weapon.cook(source, name, read)
                elif asset['kind'] == 'behavior':
                    payloads = behavior.cook(source, name, read)
                elif asset['kind'] == 'navigation':
                    payloads = navigation.cook(source, name, read)
                elif asset['kind'] == 'entities':
                    payloads = entities.cook(source, name, read)
                elif asset['kind'] == 'ui':
                    payloads = ui.cook(source, name, read)
                elif asset['kind'] == 'post':
                    payloads = post.cook(source, name, read)
                elif asset['kind'] == 'decal':
                    payloads = effect.cook_decal(source, name, read)
                elif asset['kind'] == 'effect':
                    payloads = effect.cook(source, name, read)
                elif asset['kind'] == 'material':
                    payloads = model.cook_material(source, name, read, asset)
                elif asset['kind'] == 'sound-event':
                    payloads = {name + '.asevt': audio.cook_event(read(source))}
                elif asset['kind'] == 'audio':
                    payloads = {name + '.wav': audio.cook(read(source), source.suffix.lower())}
                elif asset['kind'] == 'shader':
                    payloads = {name + '.asspv': shader.cook(source, root, asset, read)}
                elif asset['kind'] == 'texture':
                    payloads = {name + '.ktx2': texture.cook(read(source), asset)}
                else:
                    raise ValueError('asset kind is not implemented yet: ' + asset['kind'])
            except (OSError, ValueError, KeyError, IndexError, TypeError) as error:
                raise ValueError(diagnostic(error, source, 'check the asset references and tools/agent describe '+asset['kind'])) from error
            records = []
            for path, data in sorted(payloads.items()):
                if len(path.encode()) >= 64:
                    raise ValueError('cooked resource path exceeds the 63-byte engine limit: ' + path)
                target = below(output, path)
                if target == project or any(target == root / item for item in inputs):
                    raise ValueError('cooked output would overwrite a source file: ' + path)
                if path in owners and owners[path] != name:
                    raise ValueError('two assets own the same cooked resource: ' + path)
                write(target, data)
                records.append({'path': path, 'sha256': digest(data)})
            manifest = {'version': 1, 'name': name, 'kind': asset['kind'], 'recipe': recipe,
                        'inputs': [{'path': path, 'sha256': value} for path, value in sorted(inputs.items())],
                        'outputs': records}
            # Publish only after every payload is completely written.
            write(manifest_path, (json.dumps(manifest, indent=2, sort_keys=True) + '\n').encode())
            built.append(name)
        for item in records:
            if item['path'] in owners and owners[item['path']] != name:
                raise ValueError('two assets own the same cooked resource: ' + item['path'])
            owners[item['path']] = name
            resources[item['path']] = item['sha256']
    if len(resources) > 4096:
        raise ValueError('project exceeds the 4096-resource development index limit')
    index = bytearray(struct.pack('<I', len(resources)))
    kinds = {'.iqm': 1, '.ktx2': 2, '.asmat': 3, '.wav': 4, '.asspv': 5, '.asanim': 6, '.asweapon': 7, '.asfx': 8, '.aslod': 9, '.asdc': 10, '.aspost': 11, '.asevt': 12, '.asui': 13, '.asent': 14, '.asnav': 15, '.asai': 16}
    for path, hashed in sorted(resources.items()):
        size = below(output, path).stat().st_size
        index.extend(struct.pack('<64s32sII', path.encode(), bytes.fromhex(hashed), size, kinds[Path(path).suffix]))
    index = model.wrapped(b'ASIDX\0\0\0', index)
    write(output / 'cook.index', index)
    # The watcher-visible commit marker is published only after the whole project.
    write(output / 'cook.revision', hashlib.sha256(index).digest())
    return {'version': 1, 'built': built, 'skipped': skipped}


def snapshot(project, output):
    paths = {project}
    definition = json.loads(project.read_bytes())
    for asset in definition['assets']:
        paths.add(below(project.parent, asset['source']))
        manifest_path = below(output, asset['name'] + '.manifest.json')
        paths.add(manifest_path)
        try:
            manifest = json.loads(manifest_path.read_bytes())
            paths.update(below(project.parent, item['path']) for item in manifest['inputs'])
            paths.update(below(output, item['path']) for item in manifest['outputs'])
        except (OSError, ValueError, KeyError, TypeError):
            pass  # The next cook creates or repairs the cached manifest.
    result = []
    for path in sorted(paths):
        try:
            stat = path.stat()
            result.append((str(path), stat.st_mtime_ns, stat.st_size))
        except OSError:
            result.append((str(path), None, None))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('project', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--watch', action='store_true', help='poll source changes every 100 ms and publish successful cooks')
    args = parser.parse_args()
    args.project = args.project.resolve()
    args.output = args.output.resolve()
    previous = None
    error_message = None
    while True:
        try:
            before = snapshot(args.project, args.output)
            if previous is None or before != previous:
                result = cook(args.project, args.output)
                after = snapshot(args.project, args.output)
                # A cook may discover dependencies or overlap an edit. Verify
                # again until a complete pass observes a stable set of files.
                previous = after if before == after else None
                print(json.dumps(result, sort_keys=True), flush=True)
                error_message = None
        except (OSError, ValueError, KeyError, IndexError, TypeError) as error:
            if str(error) != error_message:
                print(json.dumps(dict(ok=False, error=diagnostic(error, args.project, 'check the project and referenced sources'))), file=sys.stderr, flush=True)
                error_message = str(error)
            if not args.watch:
                return 1
            previous = None
        if not args.watch:
            return 0
        time.sleep(0.1)


if __name__ == '__main__':
    raise SystemExit(main())
