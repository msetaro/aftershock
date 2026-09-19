#!/usr/bin/env python3
"""Run owned-source clang-tidy policy with each renderer's production flags."""
import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import re
import shlex
import subprocess

from run import ROOT, ENV, configure, compilation_commands


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--clang-tidy', default='clang-tidy')
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-tidy'))
    parser.add_argument('--jobs', type=int, default=4)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    tool = shlex.split(args.clang_tidy)
    config = ROOT / '.clang-tidy'
    control = output / 'control.cpp'
    header = output / 'control.h'
    header.write_text('#pragma once\n')
    control.write_text('#include "control.h"\nint check(bool b) { if (b) { return 1; } return 0; }\n')
    command = [*tool, str(control), '--config-file=' + str(config), '--', '-std=c++20']
    positive = subprocess.run(command, env=ENV, text=True, capture_output=True)
    (output / 'positive.log').write_text(positive.stdout + positive.stderr)
    positive.check_returncode()
    control.write_text('#include "control.h"\n#include "control.h"\n')
    negative = subprocess.run(command, env=ENV, text=True, capture_output=True)
    diagnostic = negative.stdout + negative.stderr
    (output / 'negative.log').write_text(diagnostic)
    if not negative.returncode or '[readability-duplicate-include,-warnings-as-errors]' not in diagnostic:
        raise RuntimeError('duplicate-include negative control escaped the policy')
    commands = []
    for renderer in ('opengl', 'vulkan'):
        directory = output / renderer
        configure(directory, ['CC=clang', 'CXX=clang++', 'USE_RENDERER_DLOPEN=0',
                              f'RENDERER_DEFAULT={renderer}'])
        for row in compilation_commands(directory):
            source = Path(row['file']).relative_to(ROOT)
            if source.suffix == '.cpp' and source.parts[0] in ('engine', 'game') and \
                    str(source) != 'engine/renderervk/shaders/spirv/shader_data.cpp':
                commands.append(row)
    if not commands:
        raise RuntimeError('no owned compilation commands found')
    (output / 'compile_commands.json').write_text(json.dumps(commands, indent=2) + '\n')

    def check(item):
        index, row = item
        flags = []
        arguments = iter(row['arguments'][1:])
        for arg in arguments:
            if arg in ('-o', '-MF', '-MT', '-MQ'):
                next(arguments)
            elif arg not in ('-c', '-MD', '-MMD', row['file']):
                flags.append(arg)
        result = subprocess.run([*tool, row['file'], '--quiet', '--config-file=' + str(config),
                                 '--', *flags], cwd=row['directory'], env=ENV,
                                text=True, capture_output=True)
        diagnostic = result.stdout + result.stderr
        log = output / (str(index) + '.log')
        log.write_text(diagnostic)
        return {'file': row['file'], 'status': result.returncode, 'log': str(log),
                'findings': dict(Counter(re.findall(r'(?:warning|error):.*\[([^]]+)\]', diagnostic)))}

    with ThreadPoolExecutor(args.jobs) as pool:
        results = list(pool.map(check, enumerate(commands)))
    (output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    findings = Counter()
    for row in results:
        findings.update(row['findings'])
    print('Advisory findings:', dict(findings))
    failures = [row['log'] for row in results if row['status']]
    if failures:
        raise SystemExit('FAIL: clang-tidy; see ' + ', '.join(failures))
    print(f'PASS: {len(commands)} production configurations; both renderers and policy controls')


if __name__ == '__main__':
    main()
