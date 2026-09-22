#!/usr/bin/env python3
"""Cook Unicode menus/HUD and verify incremental localized source edits."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import tempfile
from PIL import features
from cook import cook
from run import ROOT, SCRATCH

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--font',type=Path,default=Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'))
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-ui-framework')
args=parser.parse_args()
assert args.font.is_file() and features.check('raqm'), 'a source font and Pillow FreeType/RAQM are required'
with tempfile.TemporaryDirectory(prefix='aftershock-ui-') as temporary:
    source=Path(temporary)
    shutil.copyfile(args.font,source/'font.ttf')
    definition=json.loads((ROOT/'tests/assets/ui/shell.json').read_text())
    document=source/'shell.json'
    document.write_text(json.dumps(definition,ensure_ascii=False))
    project=source/'assets.json'
    project.write_text(json.dumps(dict(version=1,assets=[dict(name='ui/shell',kind='ui',source='shell.json')])))
    assert cook(project,args.output)['built']==['ui/shell']
    record=(args.output/'ui/shell.asui').read_bytes()
    magic,version,size=struct.unpack_from('<8sII',record)
    assert magic==b'ASUI\0\0\0\0' and version==1 and size==len(record)-48
    assert hashlib.sha256(record[48:]).digest()==record[16:48]
    atlas=(args.output/'ui/shell-font.ktx2').read_bytes()
    assert atlas.startswith(b'\xabKTX 20\xbb\r\n\x1a\n')
    assert cook(project,args.output)['built']==[]
    definition['texts'][0]['values']['en']='AFTERSHOCK II'
    document.write_text(json.dumps(definition,ensure_ascii=False))
    assert cook(project,args.output)['built']==['ui/shell']
    assert (args.output/'ui/shell-font.ktx2').read_bytes()!=atlas
    assert (args.output/'ui/shell.asui').read_bytes()!=record
print('PASS: Unicode UI document/font atlas cook and localized source reload')
