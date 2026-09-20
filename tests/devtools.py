#!/usr/bin/env python3
"""Require real developer tooling only in explicitly enabled client builds."""
import argparse
from pathlib import Path
import subprocess

from run import build


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-devtools-tests'))
    args = parser.parse_args()
    for enabled in (False, True):
        directory = build(args.output.resolve() / ('enabled' if enabled else 'shipping'),
                          ['BUILD_SERVER=0', f'AFTERSHOCK_DEVTOOLS={int(enabled)}'])
        binary = directory / 'quake3e.x64'
        symbols = subprocess.check_output(['nm', '-C', '--defined-only', binary], text=True)
        (directory / 'symbols.txt').write_text(symbols)
        for required in ('ImGui::NewFrame()', 'DevTools_Draw('):
            if (required in symbols) != enabled:
                raise SystemExit(f'FAIL: {required} must be present only with AFTERSHOCK_DEVTOOLS=ON')
        print('PASS:', 'development tooling linked' if enabled else 'shipping binary excludes tooling')


if __name__ == '__main__':
    main()
