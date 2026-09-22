#!/usr/bin/env python3
"""Cook Unicode menus/HUD and verify incremental localized source edits."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import shlex
import struct
import tempfile
from PIL import features, ImageFont
from cook import cook
from run import ROOT, SCRATCH, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--font',type=Path,default=Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'))
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-ui-framework')
parser.add_argument('--cc',default='gcc')
parser.add_argument('--cxx',default='g++')
args=parser.parse_args()
args.output=args.output.resolve()
assert args.font.is_file() and features.check('raqm'), 'a source font and Pillow FreeType/RAQM are required'
with tempfile.TemporaryDirectory(prefix='aftershock-ui-') as temporary:
    source=Path(temporary)
    shutil.copyfile(args.font,source/'font.ttf')
    definition=json.loads((ROOT/'tests/assets/ui/shell.json').read_text())
    document=source/'shell.json'
    document.write_text(json.dumps(definition,ensure_ascii=False))
    project=source/'assets.json'
    project.write_text(json.dumps(dict(version=1,assets=[dict(name='ui/shell',kind='ui',source='shell.json')])))
    assert cook(project,args.output)['built'] in ([],['ui/shell'])
    record=(args.output/'ui/shell.asui').read_bytes()
    magic,version,size=struct.unpack_from('<8sII',record)
    assert magic==b'ASUI\0\0\0\0' and version==1 and size==len(record)-48
    assert hashlib.sha256(record[48:]).digest()==record[16:48]
    counts=struct.unpack_from('<7I',record,48+104)
    pages,items,texts,locales,glyphs,width,height=counts
    assert (pages,items,texts,locales,glyphs)==(3,10,8,2,95)
    runs=48+132+pages*40+items*160+texts*36+locales*20
    arabic=struct.unpack_from('<4I3f',record,runs+texts*28)
    font=ImageFont.truetype(str(args.font),96,layout_engine=ImageFont.Layout.RAQM)
    isolated=sum(font.getlength(char) for char in definition['texts'][0]['values']['ar'])/2
    assert 0<arabic[6]<isolated*.8, 'Arabic joining must shape a run, not isolated codepoints'
    sha,probe=args.output/'sha.o',args.output/'probe'
    run([*shlex.split(args.cc),'-std=c99','-O2','-c','third_party/sha256/sha-256.c','-o',sha])
    run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-Wconversion','-Wshadow','-fsanitize=undefined','-fno-sanitize-recover=all',
         'tests/probes/ui_framework.cpp','engine/ui/ui.cpp',sha,'-o',probe])
    run([probe,args.output/'ui/shell.asui'])
    atlas=(args.output/'ui/shell-font.ktx2').read_bytes()
    assert atlas.startswith(b'\xabKTX 20\xbb\r\n\x1a\n')
    assert cook(project,args.output)['built']==[]
    definition['texts'][0]['values']['en']='AFTERSHOCK II'
    document.write_text(json.dumps(definition,ensure_ascii=False))
    assert cook(project,args.output)['built']==['ui/shell']
    assert (args.output/'ui/shell-font.ktx2').read_bytes()!=atlas
    assert (args.output/'ui/shell.asui').read_bytes()!=record
print('PASS: Unicode UI document/font atlas cook and localized source reload')
