#!/usr/bin/env python3
"""The match image carries only the reproducible owned sample, never installed paks."""
from pathlib import Path
from run import SCRATCH
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='aftershock-match-content-') as temporary:
    outputs = [Path(temporary)/str(i) for i in range(2)]
    for output in outputs:
        subprocess.run([sys.executable,str(ROOT/'tools/match/content.py'),str(output)],check=True)
    a,b = [p/'aftershock/pak0.pk3' for p in outputs]
    assert a.read_bytes()==b.read_bytes(), 'content package is not reproducible'
    with zipfile.ZipFile(a) as pak:
        names = pak.namelist()
        assert names==sorted(names) and len(names)==len(set(names))
        assert 'default.cfg' in names and 'COPYING.txt' in names
        assert not any(n.endswith(('.pk3','.qvm','.so')) for n in names)
        for suffix in ('map','bsp','aas'):
            assert pak.read('maps/two_lane.'+suffix)==(ROOT/'tests/golden/levels'/('two_lane.'+suffix)).read_bytes()
        assert all(not n.startswith('/') and '..' not in Path(n).parts for n in names)
print('PASS: reproducible owned match package; accepted MAP/BSP/AAS bytes retained')
