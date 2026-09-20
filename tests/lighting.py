#!/usr/bin/env python3
"""Check opt-in directional baking with the pinned level compiler."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile

from run import ROOT

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--compile', action='store_true', help='also check two independent directional BSP/AAS bakes')
args = parser.parse_args()
source = ROOT / 'tests/assets/levels'
level = json.loads((source / 'two_lane.json').read_text())
level['lighting']['directional'] = True

with tempfile.TemporaryDirectory(prefix='aftershock-lighting-') as temporary:
    folder = Path(temporary)
    shutil.copytree(source / 'assets', folder / 'assets')
    document = folder / 'level.json'
    document.write_text(json.dumps(level))

    def bake(name):
        output = folder / name
        result = subprocess.run([sys.executable, str(ROOT / 'tools/level'), str(document),
                                 '--output', str(output), *([] if args.compile else ['--map-only'])],
                                capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        report = json.loads(result.stdout)
        files = {kind:(output / report[kind]).read_bytes()
                 for kind in ('map', 'bsp', 'aas') if report[kind]}
        assert all(hashlib.sha256(data).hexdigest() == report['sha256'][kind] for kind, data in files.items())
        return files

    first = bake('a')
    assert b'"_aftershock_deluxe" "1"' in first['map']
    assert first == bake('b'), 'directional bake is not repeatable'
    if args.compile:
        data = first['bsp']
        assert struct.unpack_from('<4sI', data) == (b'IBSP', 46)
        lumps = [struct.unpack_from('<ii', data, 8+i*8) for i in range(17)]
        def lump(index):
            offset, length = lumps[index]
            return data[offset:offset+length]
        assert b'"_aftershock_deluxe" "1"' in lump(0)
        light = lump(14)
        page = 128 * 128 * 3
        assert len(light) and len(light) % (2*page) == 0
        surfaces = lump(13)
        used = {struct.unpack_from('<i', surfaces, i+28)[0] for i in range(0, len(surfaces), 104)}
        used = {i for i in used if i >= 0}
        assert used and all(i % 2 == 0 and (i+2)*page <= len(light) for i in used)
        directions = b''.join(light[(i+1)*page:(i+2)*page] for i in sorted(used))
        # Nonempty, varied model-space directions, not a second copy of intensity.
        assert len(set(directions)) > 16
        assert any(light[i*page:(i+1)*page] != light[(i+1)*page:(i+2)*page] for i in used)
        assert len(lump(15)) >= 8 and len(lump(15)) % 8 == 0, 'baked light probes were lost'
        assert struct.unpack_from('<4sI', first['aas']) == (b'EAAS', 5)
        print(f'PASS: {len(light)//(2*page)} repeated intensity/direction pairs and {len(lump(15))//8} light-grid probes')
    # Omitted and explicitly disabled options keep the accepted default MAP.
    level['lighting']['directional'] = False
    document.write_text(json.dumps(level))
    disabled = bake('disabled')
    assert disabled['map'] == (ROOT / 'tests/golden/levels/two_lane.map').read_bytes()
    if args.compile:
        for kind in ('bsp', 'aas'):
            assert disabled[kind] == (ROOT / 'tests/golden/levels' / ('two_lane.'+kind)).read_bytes()
    print('PASS: opt-in directional lighting and unchanged default level output')
