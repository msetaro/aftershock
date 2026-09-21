#!/usr/bin/env python3
"""Build and run a local scripted playtest without a display."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.agent import Engine, ROOT
from tools.agent.playtest import load_script, run_script
from tools.agent.formats import KINDS, describe, validate


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    commands.add_parser('build',help='sketch-to-level pipeline; use tools/agent build --help')
    run = commands.add_parser('run')
    run.add_argument('--map', required=True)
    run.add_argument('--script', type=Path, required=True)
    run.add_argument('--out', type=Path, required=True)
    run.add_argument('--binary', type=Path, help='reuse a development client; otherwise build it')
    run.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
    run.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
    run.add_argument('--jobs', type=int, default=min(8, os.cpu_count() or 1))
    description = commands.add_parser('describe')
    description.add_argument('kind', choices=KINDS)
    validation = commands.add_parser('validate')
    validation.add_argument('kind', choices=KINDS)
    validation.add_argument('source', type=Path)
    args = parser.parse_args()
    try:
        if args.command == 'describe':
            print(json.dumps(describe(args.kind), indent=2))
            return 0
        if args.command == 'validate':
            validate(args.kind, json.loads(args.source.read_text()), args.source)
            print(json.dumps(dict(ok=True, kind=args.kind, file=str(args.source))))
            return 0
        script = load_script(args.script)
        output = args.out.resolve()
        output.mkdir(parents=True, exist_ok=True)
        if (output/'report.json').exists():
            raise ValueError('output already contains a report; choose a fresh --out directory')
        binary = args.binary
        if not binary:
            with (output/'build.log').open('w') as log:
                build = output/'build'
                subprocess.run(['cmake', '-S', str(ROOT), '-B', str(build), '-G', 'Ninja',
                                '-DCMAKE_BUILD_TYPE=Debug', '-DAFTERSHOCK_DEVTOOLS=ON', '-DBUILD_SERVER=OFF'],
                               stdout=log, stderr=subprocess.STDOUT, check=True)
                subprocess.run(['cmake', '--build', str(build), '--parallel', str(args.jobs)],
                               stdout=log, stderr=subprocess.STDOUT, check=True)
            candidates = [p for p in build.glob('debug-*/*') if p.is_file() and p.name.startswith('quake3e.')
                          and p.suffix in ('.x64', '.aarch64', '.exe') and '.ded.' not in p.name]
            if len(candidates) != 1:
                raise ValueError(f'expected one client in {build}; found {candidates}')
            binary = candidates[0]
        arguments = [part for name, value in script.get('cvars', {}).items() for part in ('+set', name, value)]
        with Engine(binary, args.data, args.content, arguments=arguments) as engine:
            report = run_script(engine, script, args.map, output)
        print(json.dumps(dict(ok=report['ok'], report=str(output/'report.json'))))
        return 0 if report['ok'] else 1
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        detail = error.args[0] if isinstance(error, ValueError) and isinstance(error.args[0], dict) else dict(
            file=str(getattr(args, 'script', getattr(args, 'source', ''))), path='$', hint=str(error)+'; check the source document or build.log')
        print(json.dumps(dict(ok=False, error=detail)), file=sys.stderr)
        return 1


if __name__ == '__main__':
    if len(sys.argv)>1 and sys.argv[1]=='build':
        sys.path.insert(0,str(ROOT/'tools/level'))
        from tools.level.build import main as build_main
        sys.exit(build_main(sys.argv[2:]))
    sys.exit(main())
