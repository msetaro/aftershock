"""Offline BC textures in the Khronos KTX2 container."""
from functools import lru_cache
import hashlib
import io
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import sys

from PIL import Image, ImageMath

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


@lru_cache(maxsize=1)
def encoder():
    vendor = ROOT / 'third_party/bc7enc'
    provenance = json.loads((vendor / 'provenance.json').read_text())
    for name, expected in provenance['files'].items():
        if hashlib.sha256((vendor / name).read_bytes()).hexdigest() != expected:
            raise ValueError('BC7 source differs from pinned provenance: ' + name)
    cache = Path(os.environ.get('XDG_CACHE_HOME', Path.home() / '.cache'))
    directory = cache / 'aftershock-cook' / hashlib.sha256((str(ROOT) + os.environ.get('CXX', 'c++')).encode()).hexdigest()[:12]
    subprocess.run(['cmake', '-S', str(HERE), '-B', str(directory), '-G', 'Ninja',
                    '-DCMAKE_BUILD_TYPE=Release'], check=True, stdout=sys.stderr)
    subprocess.run(['cmake', '--build', str(directory), '--config', 'Release', '-j', '2'],
                   check=True, stdout=sys.stderr)
    return directory / ('aftershock-cook-bc7.exe' if os.name == 'nt' else 'aftershock-cook-bc7')


def mipmaps(image, srgb, normal, data_alpha=False):
    levels = [image]
    # Filter premultiplied linear values; keep source texels unchanged at level zero.
    alpha = image.getchannel('A').point([v / 255 for v in range(256)], 'F')
    lut = [v / 255 for v in range(256)]
    if srgb:
        lut = [v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4 for v in lut]
    bands = [image.getchannel(c).point(lut, 'F') if data_alpha else
             ImageMath.lambda_eval(lambda a: a['c'] * a['a'],
                                   c=image.getchannel(c).point(lut, 'F'), a=alpha) for c in 'RGB']
    width, height = image.size
    while width > 1 or height > 1:
        width, height = max(1, width // 2), max(1, height // 2)
        channels = [list(b.resize((width, height), Image.Resampling.BOX).getdata()) for b in [*bands, alpha]]
        pixels = bytearray()
        for values in zip(*channels):
            a = values[3]
            rgb = list(values[:3]) if data_alpha else [c / a if a else 0 for c in values[:3]]
            if normal:
                vector = [c * 2 - 1 for c in rgb]
                length = math.sqrt(sum(c * c for c in vector))
                rgb = [(c / length + 1) / 2 for c in vector] if length else [0.5, 0.5, 1]
            if srgb:
                rgb = [v * 12.92 if v <= 0.0031308 else 1.055 * v ** (1 / 2.4) - 0.055 for v in rgb]
            pixels.extend(max(0, min(255, round(c * 255))) for c in [*rgb, a])
        levels.append(Image.frombytes('RGBA', (width, height), bytes(pixels)))
    return levels


def blocks(levels, fmt):
    if fmt != 'bc7':
        result = []
        for image in levels:
            stream = io.BytesIO()
            image.convert('RGB').save(stream, format='DDS', pixel_format='BC5')
            dds = stream.getvalue()
            if dds[:4] != b'DDS ' or dds[84:88] != b'DX10':
                raise ValueError('Pillow BC5 encoder returned an unexpected container')
            data = dds[148:]
            expected = ((image.width + 3) // 4) * ((image.height + 3) // 4) * 16
            if len(data) != expected:
                raise ValueError('Pillow BC5 encoder returned an unexpected block count')
            # A BC5 block is two independent BC4 blocks (red, then green).
            result.append(data if fmt == 'bc5' else b''.join(data[i:i + 8] for i in range(0, len(data), 16)))
        return result
    source = bytearray()
    sizes = []
    for image in levels:
        pixels = image.load()
        count = 0
        for y in range(0, image.height, 4):
            for x in range(0, image.width, 4):
                for dy in range(4):
                    for dx in range(4):
                        source.extend(pixels[min(x + dx, image.width - 1), min(y + dy, image.height - 1)])
                count += 16
        sizes.append(count)
    data = subprocess.run([str(encoder())], input=source, stdout=subprocess.PIPE, check=True).stdout
    if len(data) != sum(sizes):
        raise ValueError('BC7 encoder returned an unexpected block count')
    result, offset = [], 0
    for size in sizes:
        result.append(data[offset:offset + size])
        offset += size
    return result


def cook(source, options):
    fmt = options.get('format', 'bc7').lower()
    srgb = options.get('srgb', fmt == 'bc7')
    normal = options.get('normal', False)
    if fmt not in ('bc7', 'bc5', 'bc4') or (srgb and fmt != 'bc7') or (normal and srgb):
        raise ValueError('textures require BC7 color or linear BC5/BC4 data')
    with Image.open(io.BytesIO(source)) as opened:
        if not 1 <= opened.width <= 16384 or not 1 <= opened.height <= 16384:
            raise ValueError('texture dimensions must be between 1 and 16384')
        image = opened.convert('RGBA')
    factor = options.get('color_factor', [1, 1, 1, 1])
    if len(factor) != 4 or any(not math.isfinite(v) or not 0 <= v <= 1 for v in factor):
        raise ValueError('color factors must be four finite values in [0,1]')
    if factor != [1, 1, 1, 1]:
        channels = []
        for index, channel in enumerate('RGBA'):
            table = []
            for value in range(256):
                v = value / 255
                if srgb and index < 3:
                    v = v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4
                v *= factor[index]
                if srgb and index < 3:
                    v = v * 12.92 if v <= 0.0031308 else 1.055 * v ** (1 / 2.4) - 0.055
                table.append(max(0, min(255, round(v * 255))))
            channels.append(image.getchannel(channel).point(table))
        image = Image.merge('RGBA', channels)
    if fmt != 'bc7' or options.get('opaque', False):
        image.putalpha(255)
    levels = mipmaps(image, srgb, normal, options.get('data_alpha', False))
    encoded = blocks(levels, fmt)
    vk_format, model, block_bytes = {'bc7': (146 if srgb else 145, 134, 16),
                                    'bc5': (141, 132, 16), 'bc4': (139, 131, 8)}[fmt]
    samples = 2 if fmt == 'bc5' else 1
    descriptor_bytes = 24 + 16 * samples
    # Khronos basic DFD v1.3: compressed 4x4 blocks, BT.709 primaries, straight alpha.
    dfd = struct.pack('<7I', 4 + descriptor_bytes, 0, 2 | (descriptor_bytes << 16),
                      model | (1 << 8) | ((2 if srgb else 1) << 16), 0x303, block_bytes, 0)
    for sample in range(samples):
        bits = 128 if fmt == 'bc7' else 64
        dfd += struct.pack('<4I', sample * 64 | ((bits - 1) << 16) | (sample << 24), 0, 0, 0xFFFFFFFF)
    source_hash = hashlib.sha256(source + json.dumps(options, sort_keys=True, separators=(',', ':')).encode()).digest()
    metadata = {'KTXwriter': b'Aftershock cook 1\0', 'aftershock.contentHash': bytes(32),
                'aftershock.sourceHash': source_hash, 'aftershock.version': struct.pack('<I', 1)}
    kvd = bytearray()
    hash_offset = None
    for key, value in sorted(metadata.items()):
        key_bytes = key.encode() + b'\0'
        if key == 'aftershock.contentHash':
            hash_offset = len(kvd) + 4 + len(key_bytes)
        entry = key_bytes + value
        kvd.extend(struct.pack('<I', len(entry)) + entry)
        kvd.extend(bytes(-len(kvd) % 4))
    dfd_offset = 80 + 24 * len(levels)
    kvd_offset = dfd_offset + len(dfd)
    data = bytearray(kvd_offset + len(kvd))
    data[:12] = b'\xabKTX 20\xbb\r\n\x1a\n'
    struct.pack_into('<13I2Q', data, 12, vk_format, 1, image.width, image.height,
                     0, 0, 1, len(levels), 0, dfd_offset, len(dfd), kvd_offset, len(kvd), 0, 0)
    data[dfd_offset:kvd_offset] = dfd
    data[kvd_offset:] = kvd
    # KTX2 stores the smallest mip first; the level index remains largest first.
    for level in reversed(range(len(levels))):
        data.extend(bytes(-len(data) % block_bytes))
        struct.pack_into('<3Q', data, 80 + level * 24, len(data), len(encoded[level]), len(encoded[level]))
        data.extend(encoded[level])
    assert hash_offset is not None
    offset = kvd_offset + hash_offset
    data[offset:offset + 32] = hashlib.sha256(data).digest()
    return bytes(data)
