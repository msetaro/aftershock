#!/usr/bin/env python3
"""Check the pinned Opus codec and bounded voice mixer without devices or a server."""
import argparse
import json
from pathlib import Path
import shlex
from run import ROOT, SCRATCH, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='gcc')
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=SCRATCH / 'aftershock-audio-voice')
args = parser.parse_args()
args.output = args.output.resolve()
source = args.output / 'source'
source.mkdir(parents=True, exist_ok=True)
source.joinpath('CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.25)
project(voice_probe LANGUAGES C CXX)
include("''' + str(ROOT / 'cmake/Audio.cmake') + '''")
aftershock_add_audio()
add_executable(probe "''' + str(ROOT / 'tests/probes/audio_voice.cpp') + '''"
  "''' + str(ROOT / 'engine/sound/snd_voice.cpp') + '''")
set_property(TARGET probe PROPERTY CXX_STANDARD 20)
target_compile_options(probe PRIVATE -UNDEBUG -fno-exceptions -fno-rtti -fsanitize=undefined -fno-sanitize-recover=all)
target_link_options(probe PRIVATE -fsanitize=undefined -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
target_link_libraries(probe PRIVATE opus)
''')
cxx = shlex.split(args.cxx)
run(['cmake', '-S', source, '-B', args.output / 'build', '-G', 'Ninja',
     '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_C_COMPILER=' + args.cc,
     '-DCMAKE_CXX_COMPILER=' + cxx[0], '-DCMAKE_CXX_FLAGS=' + shlex.join(cxx[1:])])
run(['cmake', '--build', args.output / 'build', '-j', '8'])
run([args.output / 'build/probe'])
