#!/usr/bin/env python3
"""Check complete bot chat handle cleanup and reuse under sanitizers."""
import argparse
from pathlib import Path
from run import SCRATCH
import shlex

from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-bot-chat-shutdown'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output.resolve() / 'check'
run([*shlex.split(args.cxx), '-std=c++20', '-fno-exceptions', '-fno-rtti', '-O2',
     '-ffunction-sections', '-fdata-sections', '-fsanitize=address,undefined',
     '-fno-sanitize-recover=all', 'tests/probes/bot_chat_shutdown.cpp',
     '-Wl,--gc-sections', '-lm', '-o', binary])
run([binary])
print('PASS: all bot chat handles released and reusable after shutdown')
