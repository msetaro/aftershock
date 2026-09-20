#!/usr/bin/env python3
"""Explicitly author the small owned level-tool sample assets; never run in CI."""
import hashlib
import json
from pathlib import Path
import struct

root = Path(__file__).resolve().parent / 'assets'
textures = root / 'textures/level'
textures.mkdir(parents=True, exist_ok=True)
header = struct.pack('<BBBHHBHHHHBB', 0, 0, 2, 0, 0, 0, 0, 0, 16, 16, 24, 32)
for name, color in {'floor': (100, 112, 120), 'wall': (150, 140, 115), 'trim': (70, 78, 84),
                    'cover': (100, 130, 85), 'prop': (135, 92, 55), 'sky': (90, 150, 210)}.items():
    pixels = bytearray()
    for y in range(16):
        for x in range(16):
            shade = 0.85 if (x // 4 + y // 4) % 2 else 1
            pixels.extend(int(channel * shade) for channel in reversed(color))
    (textures / (name + '.tga')).write_bytes(header + pixels)
models = root / 'models'
models.mkdir(exist_ok=True)
vertices = [(-32, -32, -32), (32, -32, -32), (32, 32, -32), (-32, 32, -32),
            (-32, -32, 32), (32, -32, 32), (32, 32, 32), (-32, 32, 32)]
faces = [(1, 4, 3, 2), (5, 6, 7, 8), (1, 2, 6, 5), (4, 8, 7, 3), (1, 5, 8, 4), (2, 3, 7, 6)]
(models / 'crate.obj').write_text('# Owned 64-unit cube, GPL-2.0-or-later\n' +
    ''.join('v ' + ' '.join(map(str, row)) + '\n' for row in vertices) +
    ''.join('f ' + ' '.join(map(str, row)) + '\n' for row in faces))
manifest = {'license': 'GPL-2.0-or-later', 'author': 'Aftershock contributors',
            'method': 'export.py: six procedural checker textures and one authored cube',
            'files': {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in sorted(root.rglob('*')) if p.is_file()}}
(root.parent / 'provenance.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
