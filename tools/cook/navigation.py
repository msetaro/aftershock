"""Recast generation from the production collision loader; Detour runtime tile."""
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
from model import wrapped
import texture


def cook(path, name, read):
    definition = json.loads(read(path))
    source = read(path.parent/definition['collision'])
    agent = definition['agent']
    values = [agent[key] for key in ('radius', 'height', 'climb', 'slope')]
    values += [definition['cell_size'], definition['cell_height']]
    if agent['climb'] >= agent['height']:
        raise ValueError('navigation climb must be below standing height')
    links = definition['links']
    ids = [link['id'] for link in links]
    if len(set(ids)) != len(ids):
        raise ValueError('off-mesh link ids must be unique')
    parameters = bytearray(struct.pack('<6fI', *values, len(links)))
    for link in links:
        parameters.extend(struct.pack('<7f3I', *link['start'], *link['end'], link['radius'],
                                      int(link['bidirectional']), ('jump', 'drop', 'door').index(link['kind'])+1, link['id']))
    vendor = texture.ROOT/'third_party/recast'
    provenance = json.loads((vendor/'provenance.json').read_text())
    for source_path, expected in provenance['files'].items():
        if hashlib.sha256((vendor/source_path).read_bytes()).hexdigest() != expected:
            raise ValueError('navigation source differs from pinned provenance: '+source_path)
    helper = texture.encoder().with_name('aftershock-cook-navigation'+('.exe' if os.name == 'nt' else ''))
    with tempfile.TemporaryDirectory(prefix='aftershock-navigation-cook-') as temporary:
        root = Path(temporary)
        bsp, params, output = root/'map.bsp', root/'parameters.bin', root/'tile.bin'
        bsp.write_bytes(source)
        params.write_bytes(parameters)
        result = subprocess.run([str(helper), str(bsp), str(params), str(output)], capture_output=True, text=True, timeout=300)
        if result.returncode:
            raise ValueError('collision navmesh cook failed: '+result.stderr[-2000:])
        report = json.loads(result.stdout)
        tile = output.read_bytes()
    payload = struct.pack('<32s6fII', hashlib.sha256(source).digest(), *values, len(tile), report['checksum'])+tile
    return {name+'.asnav': wrapped(b'ASNAV\0\0\0', payload)}
