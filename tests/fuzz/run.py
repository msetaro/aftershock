#!/usr/bin/env python3
"""Build actual engine parser entry points with libFuzzer + ASan/UBSan."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from run import ROOT, ENV, build, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('target', choices=['parse', 'msg', 'tga', 'png', 'jpeg'])
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-fuzz'))
parser.add_argument('--runs', type=int, default=10000)
args = parser.parse_args()
output = args.output.resolve() / args.target
objects = output / 'build/release-linux-x86_64/ded'
stems = ['q_shared', 'q_math']
objects_relative = ['ded/' + stem + '.o' for stem in stems]
driver = args.target
extra = []
if args.target == 'msg':
    objects_relative += ['ded/' + s + '.o' for s in ['msg', 'huffman', 'huffman_static']]
elif args.target in ('tga', 'png', 'jpeg'):
    driver = 'images'
    extra = ['-DFUZZ_' + args.target.upper()]
    if args.target == 'jpeg':
        objects_relative += ['client/cl_jpeg.o']
        # Same vendored C sources as Make's normal JPEG object list.
        import re
        objects_relative += sorted(set(re.findall(r'client/jpeg/[a-z0-9]+\.o', (ROOT/'Makefile').read_text())))
    else:
        objects_relative += ['rend1/tr_image_' + args.target + '.o']
        if args.target == 'png': objects_relative += ['rend1/puff.o']
objects = output / 'build/release-linux-x86_64'
flags = '-fsanitize=fuzzer-no-link,address,undefined -fno-omit-frame-pointer -ffunction-sections -fdata-sections'
build(output / 'build', ['CC=clang', 'CXX=clang++', 'USE_SDL=0', 'USE_CURL=0', 'CFLAGS=' + flags],
      [objects / s for s in objects_relative])
binary = output / 'fuzz'
run(['clang++', '-std=c++20', '-fno-exceptions', '-fno-rtti', '-O1', '-g', '-fsanitize=fuzzer,address,undefined',
     '-ffunction-sections', '-fdata-sections', *extra, f'tests/fuzz/{driver}.cpp',
     *[objects / s for s in objects_relative], '-Wl,--gc-sections', '-o', binary])
corpus = output / 'corpus'
corpus.mkdir(exist_ok=True)
# Tiny valid images exercise decoding, not just short-header rejection.
import struct
import zlib
if args.target == 'tga':
    seed = struct.pack('<BBBHHBHHHHBB', 0, 0, 2, 0, 0, 0, 0, 0, 1, 1, 24, 0) + bytes([0, 0, 255])
elif args.target == 'png':
    def chunk(tag, body):
        return struct.pack('>I', len(body)) + tag + body + struct.pack('>I', zlib.crc32(tag + body))
    seed = bytes.fromhex('89504e470d0a1a0a') + chunk(b'IHDR', struct.pack('>IIBBBBB', 1, 1, 8, 6, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(bytes([0, 255, 0, 0, 255]))) + chunk(b'IEND', b'')
elif args.target == 'jpeg':
    # One grayscale pixel: DC category zero, AC EOB, then one-bit padding.
    seed = bytes.fromhex('ffd8ffdb004300') + bytes([1] * 64)
    seed += bytes.fromhex('ffc0000b080001000101011100ffc4002600') + bytes([1] + [0] * 15 + [0])
    seed += bytes([16, 1] + [0] * 15 + [0]) + bytes.fromhex('ffda0008010100003f003fffd9')
elif args.target == 'msg':
    seed = bytes([2, 0, 0, 0, 0])
else:
    seed = b'// comment\n"two words" { -1.25 /* block */ last }'
(corpus / 'valid-seed').write_bytes(seed)
ENV['UBSAN_OPTIONS'] = 'halt_on_error=1:suppressions=' + str(ROOT / 'tools/port/ubsan.supp')
ENV['ASAN_OPTIONS'] = 'detect_leaks=0:halt_on_error=1'
run([binary, corpus, '-seed=1', f'-runs={args.runs}', '-timeout=5', '-max_len=65536', '-artifact_prefix=' + str(output) + '/'])
