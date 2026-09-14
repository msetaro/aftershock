#!/usr/bin/env python3
"""Regenerate committed SPIR-V bytes through bin2hex and check T18 output."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
output = Path(sys.argv[1] if len(sys.argv) > 1 else tempfile.mkdtemp(prefix='port-shaders-')).resolve()
output.mkdir(parents=True, exist_ok=True)
source = root / 'code/renderervk/shaders/bin2hex.cpp'
data = root / 'code/renderervk/shaders/spirv/shader_data.cpp'
original = data.read_bytes()
blocks = re.findall(rb'extern const unsigned char (\w+)\[(\d+)\];\nconst unsigned char \1\[\2\] = \{\n(.*?)\n\};\n', original, re.S)
assert len(blocks) == 74
compiler = ['g++', '-std=c++20', '-fno-exceptions', '-fno-rtti', '-Wall', '-Wextra', '-Werror']
subprocess.run([*compiler, str(source), '-o', str(output / 'bin2hex')], check=True)
regenerated = output / 'shader_data.cpp'
regenerated.write_bytes(b'')
for name, length, text in blocks:
    payload = bytes(int(x, 16) for x in re.findall(rb'0x([0-9A-F]{2})', text))
    assert len(payload) == int(length)
    binary = output / (name.decode() + '.spv')
    binary.write_bytes(payload)
    subprocess.run([str(output / 'bin2hex'), str(binary), '+' + str(regenerated), name.decode()], check=True)
assert regenerated.read_bytes() == original, 'Generated source differs'
# The upstream _size emission is disabled; check its declarations without enabling it in production.
text = source.read_text()
needle = '#if 0\n\tn = sprintf( buf, "extern const int'
assert text.count(needle) == 1
fixture = output / 'bin2hex-size.cpp'
fixture.write_text(text.replace(needle, '#if 1\n\tn = sprintf( buf, "extern const int'))
subprocess.run([*compiler, str(fixture), '-o', str(output / 'bin2hex-size')], check=True)
(output / 'sample.bin').write_bytes(b'\x00\x7f\xff')
subprocess.run([str(output / 'bin2hex-size'), str(output / 'sample.bin'), str(output / 'sample.cpp'), 'sample'], check=True)
assert (output / 'sample.cpp').read_text() == 'extern const unsigned char sample[3];\nconst unsigned char sample[3] = {\n\t0x00, 0x7F, 0xFF\n};\nextern const int sample_size;\nconst int sample_size = 3;\n'
subprocess.run([*compiler, '-c', str(output / 'sample.cpp'), '-o', str(output / 'sample.o')], check=True)
symbols = subprocess.check_output(['nm', '-g', '--defined-only', str(output / 'sample.o')], text=True)
assert re.search(r' R sample$', symbols, re.M) and re.search(r' R sample_size$', symbols, re.M)
result = dict(arrays=len(blocks), original_sha256=hashlib.sha256(original).hexdigest(), regenerated_sha256=hashlib.sha256(regenerated.read_bytes()).hexdigest(), size_fixture='PASS', source=str(data), command=['python3', 'tools/port/check_shader_generator.py', str(output)])
(output / 'results.json').write_text(json.dumps(result, indent=2) + '\n')
print('PASS: 74 regenerated arrays byte-identical; array and _size external linkage verified')
print(result['original_sha256'])
