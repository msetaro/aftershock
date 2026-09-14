#!/usr/bin/env python3
"""Verify native ALSA callback types, sample submission and thread shutdown."""
import argparse
from pathlib import Path
import shlex

from run import run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-audio-tests'))
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    binary = args.output / 'audio'
    run([*shlex.split(args.cxx), '-std=c++20', '-fno-exceptions', '-fno-rtti',
         '-O2', '-fno-strict-aliasing', '-DUSE_ALSA_STATIC',
         '-ffunction-sections', '-fdata-sections', 'tests/probes/audio.cpp',
         '-Wl,--gc-sections', '-Wl,--wrap=snd_pcm_mmap_commit',
         '-Wl,--wrap=snd_pcm_writei', '-lasound', '-pthread', '-o', binary])
    run([binary], timeout=10)


if __name__ == '__main__':
    main()
