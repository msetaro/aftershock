#!/usr/bin/env python3
"""G5: one C driver links to either engine language using test-only ELF aliases.

Aliases change symbol spelling in copies, never code/data/linkage visibility.
This isolates behavior from the separately enforced G3 production linkage gate.
"""
import difflib
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import zipfile

root = Path(__file__).resolve().parents[2]
os.chdir(root)
output = Path(sys.argv[1] if len(sys.argv) > 1 else '/tmp/aftershock-cpp-port/differential').resolve()
output.mkdir(parents=True, exist_ok=True)
env = dict(os.environ, SOURCE_DATE_EPOCH='1789257600', LC_ALL='C')


def run(*args):
    return subprocess.check_output(args, text=True, env=env)


sources = 'q_shared q_math msg huffman huffman_static cmd cvar files net_chan net_ip md4 cm_load cm_patch cm_polylib cm_test cm_trace'.split()
driver = output / 'driver.o'
subprocess.run(['gcc', '-O2', '-ffunction-sections', '-fdata-sections', '-c',
                'tools/port/differential_check.c', '-o', str(driver)], check=True, env=env)
for stem in sources:
    with (output / (stem + '.log')).open('w') as log:
        subprocess.run(['python3', 'tools/port/compile_pair.py', f'ded/{stem}.o', str(output),
                        'CFLAGS=-ffunction-sections -fdata-sections'], check=True, env=env,
                       stdout=log, stderr=subprocess.STDOUT)

# Match only unambiguous global names defined by the C engine or C driver.
# No --globalize-symbol, no rewriting of const storage or function bodies.
c_names = set()
for obj in [driver, *(output / (stem + '.c.o') for stem in sources)]:
    c_names.update(line.split()[0] for line in run('nm', '-g', '--defined-only', '--format=posix', str(obj)).splitlines())
mapping = {}
for stem in sources:
    for line in run('nm', '-g', '--format=posix', str(output / (stem + '.cxx.o'))).splitlines():
        symbol = line.split()[0]
        if symbol.startswith('_Z'):
            name = run('c++filt', symbol).strip().split('(')[0]
            if name in c_names:
                assert name not in mapping or mapping[name] == symbol, ('overloaded ABI', name)
                mapping[name] = symbol
aliases = output / 'aliases.txt'
aliases.write_text(''.join(f'{mangled} {name}\n' for name, mangled in sorted(mapping.items())))
for stem in sources:
    subprocess.run(['objcopy', '--redefine-syms=' + str(aliases), str(output / (stem + '.cxx.o')),
                    str(output / (stem + '.aliased.o'))], check=True)

# Use installed, user-owned map data. Never copy or commit a pak archive.
pak = Path.home() / '.q3a/baseq3/pak0.pk3'
with zipfile.ZipFile(pak) as archive:
    bsp = archive.read('maps/q3dm17.bsp')
map_path = output / 'q3dm17.bsp'
map_path.write_bytes(bsp)
print('BSP SHA256', hashlib.sha256(bsp).hexdigest(), flush=True)
results = []
for tag, suffix in [('c', 'c'), ('cxx', 'aliased')]:
    binary = output / ('check-' + tag)
    subprocess.run(['g++', '-Wl,--gc-sections', '-Wl,--wrap=FS_ReadFile', '-o', str(binary), str(driver),
                    *(str(output / (stem + '.' + suffix + '.o')) for stem in sources), '-lm'], check=True)
    text = run(str(binary), str(map_path))
    (output / (tag + '.txt')).write_text(text)
    results.append(text)
diff = ''.join(difflib.unified_diff(results[0].splitlines(True), results[1].splitlines(True), fromfile='C', tofile='C++'))
(output / 'results.diff').write_text(diff)
print(results[0], end='')
print(diff, end='')
print(('FAIL' if diff else 'PASS') + ': G5 fixed-input differential')
sys.exit(bool(diff))
