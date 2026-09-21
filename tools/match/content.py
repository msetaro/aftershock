#!/usr/bin/env python3
"""Package the reviewed owned level; never search installed content or recompile BSP."""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from tools.scratch import ROOT as SCRATCH
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output',type=Path)
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='aftershock-match-content-') as temporary:
    stage = Path(temporary)
    subprocess.run([sys.executable,str(ROOT/'tools/level'),str(ROOT/'tests/assets/levels/two_lane.json'),
                    '--output',str(stage),'--map-only'],check=True,stdout=subprocess.DEVNULL)
    assert (stage/'maps/two_lane.map').read_bytes()==(ROOT/'tests/golden/levels/two_lane.map').read_bytes()
    for suffix in ('bsp','aas'):
        (stage/('maps/two_lane.'+suffix)).write_bytes((ROOT/'tests/golden/levels'/('two_lane.'+suffix)).read_bytes())
    (stage/'default.cfg').write_bytes(b'// Owned Aftershock dedicated-server sample.\nset bot_enable 0\n')
    (stage/'COPYING.txt').write_bytes((ROOT/'COPYING.txt').read_bytes())
    output = args.output/'aftershock/pak0.pk3'
    output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as pak:
        for path in sorted(stage.rglob('*')):
            if path.is_file():
                info = zipfile.ZipInfo(path.relative_to(stage).as_posix(),date_time=(2026,1,1,0,0,0))
                info.external_attr = 0o100644 << 16
                info.compress_type = zipfile.ZIP_DEFLATED
                pak.writestr(info,path.read_bytes())
    print(output)
