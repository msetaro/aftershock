#!/usr/bin/env python3
"""Offline asset cooker. Emit native payloads and dependency manifests with content hashes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import tempfile

import model
import texture

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), allow_nan=False).encode()


def tool_hash():
    paths = [p for p in HERE.iterdir() if p.suffix in ('.py', '.cpp', '.txt')]
    vendor = ROOT / 'third_party/bc7enc'
    paths.extend(vendor / name for name in ('bc7enc.cpp', 'bc7enc.h', 'provenance.json'))
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
    built, skipped, names, owners = [], [], set(), {}
    for asset in definition['assets']:
        name = asset['name']
        if not re.fullmatch(r'[a-z0-9_/-]+', name) or name.startswith('/') or '..' in name or len(name) > 59 or name in names:
            raise ValueError('asset names must be unique relative lowercase qpaths, at most 59 characters')
        names.add(name)
        recipe = digest(canonical({'asset': asset, 'tool': tool}))
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
            if asset['kind'] == 'model':
                payloads = model.cook(source, name, asset, read)
            elif asset['kind'] == 'texture':
                payloads = {name + '.ktx2': texture.cook(read(source), asset)}
            else:
                raise ValueError('asset kind is not implemented yet: ' + asset['kind'])
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
    return {'version': 1, 'built': built, 'skipped': skipped}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('project', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        result = cook(args.project.resolve(), args.output)
    except (OSError, ValueError, KeyError, IndexError, TypeError) as error:
        print('cook: ' + str(error), file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
