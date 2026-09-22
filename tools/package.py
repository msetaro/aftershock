#!/usr/bin/env python3
"""Build reproducible cooked-content packages and manifest-diff patches."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import tempfile
import zlib

HEADER = struct.Struct('<8sIIQQ32s32s32s')
ENTRY = struct.Struct('<64sQQQ32sII')
MAX_ASSETS = 65536
MAX_ASSET = 256 << 20
MAX_MANIFEST = 16 << 20


def digest(data):
    return hashlib.sha256(data).digest()


def name_valid(name):
    return (isinstance(name, str) and 0 < len(name) < 64
            and re.fullmatch(r'[a-z0-9_./-]+', name)
            and all(part not in ('', '.', '..') for part in name.split('/')))


def identity(assets):
    return digest(b''.join(name.encode()+b'\0'+bytes.fromhex(assets[name]) for name in sorted(assets))).hex()


def metadata(path):
    with path.open('rb') as source:
        header = source.read(HEADER.size)
        if len(header) != HEADER.size:
            raise ValueError('incomplete package header')
        magic, version, count, offset, size, target, base, claimed = HEADER.unpack(header)
        length = path.stat().st_size
        start = HEADER.size + count*ENTRY.size
        if (magic != b'ASPACK\0\0' or version != 1 or count > MAX_ASSETS
                or offset < start or size > MAX_MANIFEST or offset+size != length):
            raise ValueError('unsupported or incomplete package')
        index = source.read(count*ENTRY.size)
        source.seek(offset)
        manifest = source.read(size)
        if digest(index+manifest) != claimed:
            raise ValueError('package metadata hash differs')
    rows = {}
    cursor = start
    removed = []
    for position in range(count):
        raw, location, plain, stored, hashed, codec, flags = ENTRY.unpack_from(index, position*ENTRY.size)
        name = raw.split(b'\0', 1)[0].decode('ascii')
        if not name_valid(name) or raw != name.encode().ljust(64, b'\0') or (rows and name <= next(reversed(rows))):
            raise ValueError('package index paths must be canonical and sorted')
        if flags == 1:
            if location or plain or stored or codec or hashed != bytes(32) or base == bytes(32):
                raise ValueError('invalid removal entry')
            removed.append(name)
        elif (flags or codec not in (0, 1) or plain > MAX_ASSET or stored > MAX_ASSET
                or location != cursor or stored > offset-cursor or (codec == 0 and plain != stored)):
            raise ValueError('invalid asset extent or codec')
        else:
            cursor += stored
        rows[name] = dict(offset=location, size=plain, stored=stored, sha256=hashed.hex(), codec=codec, removed=bool(flags))
    if cursor != offset:
        raise ValueError('unindexed package payload')
    result = json.loads(manifest)
    expected = dict(version=1, identity=target.hex(), base=None if base == bytes(32) else base.hex(),
                    assets={name: row['sha256'] for name, row in rows.items() if not row['removed']}, removed=removed)
    if result != expected:
        raise ValueError('manifest and native index differ')
    return result, rows


def payload(path, row):
    with path.open('rb') as source:
        source.seek(row['offset'])
        data = source.read(row['stored'])
    if len(data) != row['stored']:
        raise ValueError('incomplete asset payload')
    if row['codec']:
        decoder = zlib.decompressobj()
        data = decoder.decompress(data, row['size']+1)
        if not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:
            raise ValueError('invalid compressed asset extent')
    if len(data) != row['size'] or digest(data).hex() != row['sha256']:
        raise ValueError('asset content hash differs')
    return data


def mount(paths):
    view = {}
    for path in paths:
        manifest, rows = metadata(path)
        if manifest['base'] is not None:
            if identity({name: row['sha256'] for name, (_, row) in view.items()}) != manifest['base']:
                raise ValueError('patch base manifest does not match mounted content')
        elif identity(manifest['assets']) != manifest['identity']:
            raise ValueError('package identity differs')
        for name, row in rows.items():
            if row['removed']:
                if name not in view:
                    raise ValueError('patch removes an absent asset')
                del view[name]
            else:
                view[name] = (path, row)
        if manifest['base'] is not None and identity({name: row['sha256'] for name, (_, row) in view.items()}) != manifest['identity']:
            raise ValueError('patch result identity differs')
    return view


def build(root, output, bases, store):
    root, output = root.resolve(), output.resolve()
    if not root.is_dir() or output.is_relative_to(root) or any(output == path.resolve() for path in bases):
        raise ValueError('package output must be outside the content directory and distinct from every base')
    files, assets = {}, {}
    for path in sorted(root.rglob('*')):
        if path.is_symlink():
            raise ValueError('package sources must be ordinary files, not symlinks')
        if not path.is_file() or path.name.endswith('.manifest.json'):
            continue
        name = path.relative_to(root).as_posix()
        if not name_valid(name) or path.stat().st_size > MAX_ASSET:
            raise ValueError('asset path/size exceeds the native content limit: '+name)
        with path.open('rb') as source:
            assets[name] = hashlib.file_digest(source, 'sha256').hexdigest()
        files[name] = path
    if len(files) > MAX_ASSETS:
        raise ValueError('too many assets')
    previous = {name: row['sha256'] for name, (_, row) in mount(bases).items()}
    changed = {name: hashed for name, hashed in assets.items() if previous.get(name) != hashed}
    removed = sorted(previous.keys()-assets.keys())
    manifest = dict(version=1, identity=identity(assets), base=identity(previous) if bases else None,
                    assets=changed, removed=removed)
    encoded = json.dumps(manifest, sort_keys=True, separators=(',', ':')).encode()
    names = sorted(changed.keys() | set(removed))
    if len(names) > MAX_ASSETS or len(encoded) > MAX_MANIFEST:
        raise ValueError('package metadata exceeds native limits')
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=output.parent, prefix='.'+output.name, delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(bytes(HEADER.size+len(names)*ENTRY.size))
            index = bytearray()
            for name in names:
                if name in removed:
                    index.extend(ENTRY.pack(name.encode(), 0, 0, 0, bytes(32), 0, 1))
                    continue
                data = files[name].read_bytes()
                if len(data) > MAX_ASSET or digest(data).hex() != assets[name]:
                    raise ValueError('asset changed while packaging: '+name)
                compressed = data if any(files[name].relative_to(root).match(pattern) for pattern in store) else zlib.compress(data, 9)
                codec = int(len(compressed) < len(data))
                stored = compressed if codec else data
                index.extend(ENTRY.pack(name.encode(), stream.tell(), len(data), len(stored), bytes.fromhex(assets[name]), codec, 0))
                stream.write(stored)
            offset = stream.tell()
            stream.write(encoded)
            stream.seek(0)
            stream.write(HEADER.pack(b'ASPACK\0\0', 1, len(names), offset, len(encoded),
                                     bytes.fromhex(manifest['identity']), bytes.fromhex(manifest['base']) if bases else bytes(32), digest(index+encoded)))
            stream.write(index)
        os.replace(temporary, output)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return manifest


def extract(paths, output):
    view = mount(paths)
    output = output.resolve()
    if output.exists():
        raise ValueError('extraction requires a new output directory')
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=output.parent, prefix='.'+output.name) as temporary:
        stage = Path(temporary)/'content'
        stage.mkdir()
        for name, (path, row) in view.items():
            target = stage/name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(payload(path, row))
        os.rename(stage, output)
    assets = {name: row['sha256'] for name, (_, row) in view.items()}
    return dict(identity=identity(assets), assets=assets)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    writer = commands.add_parser('build')
    writer.add_argument('--root', type=Path, required=True)
    writer.add_argument('--output', type=Path, required=True)
    writer.add_argument('--base', type=Path, action='append', default=[])
    writer.add_argument('--store', action='append', default=[], help='qpath glob to store without compression')
    reader = commands.add_parser('extract')
    reader.add_argument('packages', type=Path, nargs='+', help='mount order, lowest precedence first')
    reader.add_argument('--output', type=Path, required=True)
    verifier = commands.add_parser('verify')
    verifier.add_argument('packages', type=Path, nargs='+')
    args = parser.parse_args()
    try:
        if args.command == 'build':
            result = build(args.root, args.output, args.base, args.store)
        elif args.command == 'extract':
            result = extract(args.packages, args.output)
        else:
            view = mount(args.packages)
            for path, row in view.values():
                payload(path, row)
            assets = {name: row['sha256'] for name, (_, row) in view.items()}
            result = dict(identity=identity(assets), assets=assets)
        print(json.dumps(result, sort_keys=True))
    except (OSError, ValueError, KeyError, TypeError, zlib.error) as error:
        parser.exit(1, 'package: '+str(error)+'\n')


if __name__ == '__main__':
    main()
