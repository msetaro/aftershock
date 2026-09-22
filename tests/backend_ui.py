#!/usr/bin/env python3
"""Cook and validate the owned login/queue/profile menu without changing accepted UI fixtures."""
import argparse
import json
from pathlib import Path
import shlex
import shutil
import tempfile
from cook import cook
from run import ROOT,SCRATCH,run
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-backend-ui')
parser.add_argument('--cc',default='gcc')
parser.add_argument('--cxx',default='g++')
parser.add_argument('--font',type=Path,default=Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'))
args=parser.parse_args();args.output=args.output.resolve()
with tempfile.TemporaryDirectory(prefix='aftershock-backend-ui-') as tmp:
    source=Path(tmp)
    shutil.copyfile(args.font,source/'font.ttf')
    shutil.copyfile(ROOT/'tests/assets/backend/shell.json',source/'shell.json')
    project=source/'assets.json'
    project.write_text(json.dumps(dict(version=1,assets=[dict(name='ui/backend',kind='ui',source='shell.json')])))
    cook(project,args.output)
    sha,probe=args.output/'sha.o',args.output/'probe'
    run([*shlex.split(args.cc),'-std=c99','-O2','-c','third_party/sha256/sha-256.c','-o',sha])
    run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti','-fsanitize=undefined',
         '-fno-sanitize-recover=all','tests/probes/backend_ui.cpp','engine/ui/ui.cpp',sha,'-o',probe])
    run([probe,args.output/'ui/backend.asui'])
