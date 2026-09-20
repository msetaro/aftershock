#!/usr/bin/env python3
"""Reject bare long types in owned sources, including inactive platform branches."""
from pathlib import Path
import re
import subprocess

from check_boundaries import ROOT, TOKENS, blank


def violations(text):
    source = TOKENS.sub(lambda match: blank(match[0]), text)
    return [source.count('\n', 0, match.start()) + 1
            for match in re.finditer(r'\blong\b', source)]


def main():
    assert not violations('int64_t value; // long\nconst char *s = "long"; /* long */')
    assert violations('unsigned long value;') == [1]
    assert violations('#if 0\nlong long value;\n#endif') == [2, 2]
    assert violations('#ifdef _WIN32\nlong value;\n#endif') == [2]
    files = subprocess.check_output(
        ['git', 'ls-files', '-z', '--', 'engine', 'game'], cwd=ROOT).decode().split('\0')
    files = [name for name in files if Path(name).suffix in ('.h', '.cpp', '.c', '.inc')
             and not name.startswith('engine/platform/asm/')
             and name != 'engine/renderervk/shaders/spirv/shader_data.cpp']
    if not files:
        raise SystemExit('no owned C/C++ files found')
    errors = [f'{name}:{line}: use an explicit-width integer or the foreign API type'
              for name in files for line in violations((ROOT / name).read_text())]
    if errors:
        raise SystemExit('\n'.join(errors))
    print(f'PASS: {len(files)} owned files; long ban and inactive-branch controls')


if __name__ == '__main__':
    main()
