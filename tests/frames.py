#!/usr/bin/env python3
"""Check saved replay evidence; explicit local regeneration also reviews CI artifacts."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
from run import compare, content_maps


def check_frames(output, content, regenerate, compare_goldens=True):
    frames = {}
    versions = set()
    for map_name in content_maps(content):
        for backend in ('vulkan', 'opengl1'):
            repetitions = []
            for iteration in (1, 2):
                log = (output / f'{map_name}-{backend}-replay-{iteration}.log').read_text()
                marker = 'GL_RENDERER:' if backend == 'opengl1' else 'VK_RENDERER:'
                if marker not in log or 'llvmpipe' not in log or 'ERROR:' in log or 'Unknown command' in log:
                    raise SystemExit('FAIL: invalid renderer/replay evidence')
                pattern = r'Driver: (\d+\.\d+\.\d+)' if backend == 'vulkan' else r'Mesa (\d+\.\d+\.\d+)'
                version = re.search(pattern, log)
                if not version:
                    raise SystemExit('FAIL: missing Mesa version in replay log')
                versions.add(version[1])
                hashes = {name: hashlib.sha256((output / f'{map_name}-{backend}-{iteration}-{name}.tga').read_bytes()).hexdigest()
                          for name in ('frame050', 'frame100', 'frame200')}
                if len(set(hashes.values())) != 3:
                    raise SystemExit('FAIL: sampled frames did not advance')
                repetitions.append(hashes)
            if repetitions[0] != repetitions[1]:
                raise SystemExit('FAIL: repeated replay frames differ')
            frames[map_name + '/' + backend] = repetitions[0]
    if len(versions) != 1:
        raise SystemExit('FAIL: replay evidence mixes Mesa versions')
    name = ('openarena/' if content == 'openarena' else '') + f'frames-mesa-{versions.pop()}.json'
    if compare_goldens:
        compare(name, (json.dumps(frames, indent=2, sort_keys=True) + '\n').encode(), regenerate)
    else:
        print('PASS: fresh and retained module replay frames agree')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
    parser.add_argument('--regenerate', action='store_true')
    args = parser.parse_args()
    if args.regenerate and os.environ.get('CI'):
        parser.error('CI must never regenerate goldens')
    check_frames(args.output, args.content, args.regenerate)
