#!/usr/bin/env python3
"""Build the pinned offline shader package, reusing verified committed SPIR-V."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'engine/renderervk/shaders'


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def recipe_hash(manifest, row):
    recipe = {field: manifest[field] for field in ('schema', 'compiler', 'compiler_version', 'target', 'options')}
    recipe.update({field: row[field] for field in ('name', 'source', 'stage', 'defines', 'source_sha256')})
    recipe['includes'] = row.get('includes', {})
    return sha256(json.dumps(recipe, sort_keys=True, separators=(',', ':')).encode())


def committed_shaders():
    data = (SOURCE / 'spirv/shader_data.cpp').read_text(encoding='utf-8').encode()
    blocks = re.findall(rb'extern const unsigned char (\w+)\[(\d+)\];\nconst unsigned char \1\[\2\] = \{\n(.*?)\n\};\n', data, re.S)
    result = {}
    for name, length, text in blocks:
        payload = bytes(int(x, 16) for x in re.findall(rb'0x([0-9A-F]{2})', text))
        if len(payload) != int(length):
            raise ValueError('incorrect committed shader length: ' + name.decode())
        result[name.decode()] = payload
    return result


def source_inputs(path):
    inputs = {}
    pending = [path]
    while pending:
        current = pending.pop().resolve()
        name = current.relative_to(SOURCE.resolve()).as_posix()
        if name in inputs:
            continue
        text = current.read_text(encoding='utf-8')
        inputs[name] = sha256(text.encode())
        for include in re.findall(r'^\s*#\s*include\s+"([^"\n]+)"', text, re.M):
            pending.append(current.parent / include)
    return inputs


def layout(data):
    # Reflect the compiler-produced interface; no runtime SPIR-V parser is added.
    words = struct.unpack('<' + 'I' * (len(data) // 4), data)
    if words[0] != 0x07230203:
        raise ValueError('expected SPIR-V')
    decorations = {}
    entries = []
    member_offsets = []
    offset = 5
    while offset < len(words):
        size, op = words[offset] >> 16, words[offset] & 0xffff
        if not size or offset + size > len(words):
            raise ValueError('invalid compiler output instruction length')
        args = words[offset + 1:offset + size]
        if op == 15:  # OpEntryPoint
            name = struct.pack('<' + 'I' * len(args[2:]), *args[2:]).split(b'\0', 1)[0].decode()
            entries.append({'stage': {0: 'vert', 4: 'frag'}[args[0]], 'entry': name})
        elif op == 71 and len(args) == 3:  # OpDecorate
            field = {1: 'specialization', 30: 'location', 33: 'binding', 34: 'set'}.get(args[1])
            if field:
                decorations.setdefault(args[0], {})[field] = args[2]
        elif op == 72 and len(args) == 4 and args[2] == 35:  # OpMemberDecorate Offset
            member_offsets.append({'type': args[0], 'member': args[1], 'offset': args[3]})
        offset += size
    return {'entry_points': entries, 'decorations': decorations, 'member_offsets': member_offsets}


def write_if_changed(path, data):
    if not path.exists() or path.read_bytes() != data:
        path.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--compiler', help='glslang executable, required on a source-cache miss')
    parser.add_argument('--compile', action='store_true', help='compile every shader instead of reusing committed bytes')
    parser.add_argument('--cooked', type=Path, help='cooker output directory containing shaders/<package-name>.asspv')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((SOURCE / 'manifest.json').read_text(encoding='utf-8'))
    cached = committed_shaders()
    if set(cached) != {row['name'] for row in manifest['shaders']}:
        raise ValueError('manifest and committed shader names differ')
    overrides = {}
    if args.cooked:
        paths = sorted((args.cooked / 'shaders').glob('*.asspv'))
        if not paths:
            parser.error('cooked shader directory contains no shaders/*.asspv')
        for path in paths:
            data = path.read_bytes()
            if len(data) < 68 or len(data) > 16 << 20:
                parser.error('invalid cooked shader size: ' + str(path))
            magic, version, size, digest = struct.unpack_from('<8sII32s', data)
            if magic != b'ASSPV\0\0\0' or version != 1 or size != len(data) - 48 or size % 4 or hashlib.sha256(data[48:]).digest() != digest:
                parser.error('invalid cooked shader header/hash: ' + str(path))
            if path.stem not in cached:
                parser.error('shader is not in the current renderer package: ' + path.stem)
            overrides[path.stem] = data[48:]
    compiler = None
    package = {'schema': manifest['schema'], 'compiler': manifest['compiler'],
               'compiler_version': manifest['compiler_version'], 'target': manifest['target'],
               'options': manifest['options'], 'shaders': []}
    header = ['// Generated by tools/shaders/build.py; do not edit.\n']
    compiled = 0
    for original in manifest['shaders']:
        row = dict(original)
        source = SOURCE / row['source']
        inputs = source_inputs(source)
        row['source_sha256'] = inputs.pop(row['source'])
        row['includes'] = inputs
        row['recipe_sha256'] = recipe_hash(manifest, row)
        payload = cached[row['name']]
        if sha256(payload) != original['spirv_sha256']:
            raise ValueError('committed shader hash mismatch: ' + row['name'])
        if args.compile or row['recipe_sha256'] != original['recipe_sha256']:
            if compiler is None:
                compiler = args.compiler or shutil.which('glslang') or shutil.which('glslangValidator')
                if not compiler:
                    parser.error('shader recipe changed: provide pinned glslang ' + manifest['compiler_version'])
                version = subprocess.check_output([compiler, '--version'], text=True)
                if not re.search(r'^Glslang Version: \d+:' + re.escape(manifest['compiler_version']) + r'$', version, re.M):
                    parser.error('expected glslang ' + manifest['compiler_version'] + ': ' + version.splitlines()[0])
            binary = args.output / (row['name'] + '.spv')
            subprocess.run([compiler, '-S', row['stage'], *manifest['options'], '-o', str(binary.resolve()),
                            row['source'], *['-D' + flag for flag in row['defines']]], cwd=SOURCE, check=True,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            payload = binary.read_bytes()
            compiled += 1
        if row['name'] in overrides:
            replacement = overrides[row['name']]
            # ponytail: exact reflected records, including IDs; richer compatible
            # material interfaces belong to #13, not an unchecked ABI replacement.
            if layout(replacement) != layout(cached[row['name']]):
                parser.error('cooked shader interface differs from the renderer contract: ' + row['name'])
            payload = replacement
            row['cooked_sha256'] = sha256(payload)
        write_if_changed(args.output / (row['name'] + '.spv'), payload)
        row['spirv_sha256'] = sha256(payload)
        row['layout'] = layout(payload)
        if row['layout']['entry_points'] != [{'stage': row['stage'], 'entry': 'main'}]:
            raise ValueError('unexpected shader entry point: ' + row['name'])
        package['shaders'].append(row)
        name, size = row['name'], len(payload)
        header.append(f'extern const unsigned char {name}[{size}];\nalignas(4) const unsigned char {name}[{size}] = {{\n')
        for start in range(0, size, 16):
            header.append('\t' + ', '.join(f'0x{b:02X}' for b in payload[start:start + 16]) + ',\n')
        header.append('};\n')
    key = sha256(json.dumps(package, sort_keys=True, separators=(',', ':')).encode())
    package['sha256'] = key
    header.append(f'constexpr char rhi_shader_package_hash[] = "{key}";\n')
    write_if_changed(args.output / 'shader_package.h', ''.join(header).encode())
    write_if_changed(args.output / 'shader_package.json', (json.dumps(package, indent=2) + '\n').encode())
    print(f'PASS: {len(package["shaders"])} shaders, {compiled} compiled, package {key}')


if __name__ == '__main__':
    main()
