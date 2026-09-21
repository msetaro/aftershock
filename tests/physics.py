#!/usr/bin/env python3
"""Replay recorded cosmetic props and enforce allocation-free Jolt steps."""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import subprocess

from run import ENV, ROOT, SCRATCH, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-physics')
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
for library in ('jolt', 'joltc'):
    base = ROOT/'third_party'/library
    provenance = json.loads((base/'provenance.json').read_text())
    patches = provenance.get('local_changes', {}).get('files', {})
    for name, original in provenance['files'].items():
        assert hashlib.sha256((base/name).read_bytes()).hexdigest() == patches.get(name, original), name
project = args.output/'source'
project.mkdir(exist_ok=True)
(project/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.25)
project(physics_probe LANGUAGES C CXX)
include("{ROOT}/cmake/Physics.cmake")
aftershock_add_physics()
add_executable(physics_probe "{ROOT}/tests/probes/physics.cpp")
target_link_libraries(physics_probe PRIVATE joltc Jolt)
target_compile_features(physics_probe PRIVATE cxx_std_20)
target_compile_options(physics_probe PRIVATE -UNDEBUG -Wall -Wextra -Werror)
add_executable(physics_boundary "{ROOT}/tests/probes/physics_boundary.cpp" "{ROOT}/engine/physics/physics.cpp"
  "{ROOT}/engine/animation/animation.cpp" "{ROOT}/third_party/sha256/sha-256.c")
target_include_directories(physics_boundary PRIVATE "{ROOT}")
target_link_libraries(physics_boundary PRIVATE joltc Jolt)
target_compile_features(physics_boundary PRIVATE cxx_std_20)
target_compile_options(physics_boundary PRIVATE -UNDEBUG -Wall -Wextra -Werror)
add_executable(physics_collision "{ROOT}/tests/probes/physics_collision.cpp"
  "{ROOT}/engine/qcommon/cm_physics.cpp" "{ROOT}/engine/qcommon/cm_polylib.cpp" "{ROOT}/engine/qcommon/q_math.cpp")
target_include_directories(physics_collision PRIVATE "{ROOT}")
target_compile_features(physics_collision PRIVATE cxx_std_20)
target_compile_options(physics_collision PRIVATE -UNDEBUG -Wall -Wextra -Werror -ffunction-sections -fdata-sections)
target_link_options(physics_collision PRIVATE -Wl,--gc-sections)
add_executable(physics_ragdoll "{ROOT}/tests/probes/physics_ragdoll.cpp" "{ROOT}/engine/physics/physics.cpp"
  "{ROOT}/engine/animation/animation.cpp" "{ROOT}/third_party/sha256/sha-256.c")
target_include_directories(physics_ragdoll PRIVATE "{ROOT}")
target_link_libraries(physics_ragdoll PRIVATE joltc Jolt)
target_compile_features(physics_ragdoll PRIVATE cxx_std_20)
target_compile_options(physics_ragdoll PRIVATE -UNDEBUG -Wall -Wextra -Werror)
''')
compiler = shlex.split(args.cxx)
flags = shlex.join(compiler[1:]) + ' -fno-exceptions -fno-rtti -ffp-contract=off -fsanitize=undefined -fno-sanitize-recover=all'
build = args.output/'build'
run(['cmake', '-S', project, '-B', build, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release',
     '-DCMAKE_CXX_COMPILER='+compiler[0], '-DCMAKE_CXX_FLAGS='+flags])
run(['cmake', '--build', build, '-j', '4'])
from cook import cook
owned = args.output/'animation'
cook(ROOT/'tests/assets/animation/rigs.json', owned)
run([build/'physics_ragdoll', owned/'animations/anim_body.asanim'])
run([build/'physics_collision'])
run([build/'physics_boundary'])
missing_joint = subprocess.run([str(build/'physics_boundary'), 'without-joint'],
                               cwd=ROOT, env=ENV, capture_output=True, text=True, timeout=30)
(args.output/'missing-joint.log').write_text(missing_joint.stderr)
assert missing_joint.returncode and 'separation < 1.05f' in missing_joint.stderr, missing_joint.stderr
probe = build/'physics_probe'
scene = ROOT/'tests/assets/physics/props.txt'
outputs = [args.output/name for name in ('first.bin', 'repeat.bin', 'changed.bin')]
for index, output in enumerate(outputs):
    run([probe, scene, output, *(['changed'] if index == 2 else [])])
first, repeat, changed = [path.read_bytes() for path in outputs]
assert len(first) == 32*7*4, 'one position/quaternion per recorded prop'
assert first == repeat, 'same ordered commands must reproduce final transforms'
assert first != changed, 'changed impulse must fail the replay comparison'
exhausted = subprocess.run([str(probe), str(scene), str(args.output/'tiny.bin'), 'tiny'],
                          cwd=ROOT, env=ENV, capture_output=True, text=True, timeout=30)
(args.output/'temporary-exhaustion.log').write_text(exhausted.stderr)
assert exhausted.returncode and 'TempAllocator: Out of memory' in exhausted.stderr, exhausted.stderr
assert 'physics step' not in exhausted.stderr, 'temporary exhaustion must not fall back to allocation'
print('PASS: recorded props, constraints, allocation-free steps, bounded temporary storage and replay negative control',
      hashlib.sha256(first).hexdigest())
