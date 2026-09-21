#!/usr/bin/env python3
"""Fetch pinned licensed theme assets and verify their complete provenance."""
import argparse
import json
from pathlib import Path
import sys

sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from tools.scratch import ROOT as SCRATCH
from tools.assets.manifest import validate


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command',required=True)
    check = sub.add_parser('validate')
    check.add_argument('root',type=Path)
    download = sub.add_parser('fetch')
    download.add_argument('--theme',required=True)
    download.add_argument('--lock',type=Path)
    download.add_argument('--out',type=Path,default=SCRATCH/'theme-kit')
    download.add_argument('--offline',action='store_true')
    args = parser.parse_args()
    try:
        if args.command=='validate':
            result = validate(args.root)
        else:
            from tools.assets.fetch import fetch
            lock = args.lock or Path(__file__).with_name('themes')/(args.theme+'.json')
            if json.loads(lock.read_text()).get('theme')!=args.theme:
                raise ValueError('lock belongs to another theme')
            result = fetch(lock,args.out,args.offline)
        print(json.dumps(result,sort_keys=True))
        return 0
    except (OSError,ValueError,KeyError,TypeError) as exc:
        print(json.dumps(dict(ok=False,error=str(exc))),file=sys.stderr)
        return 1


if __name__=='__main__':
    sys.exit(main())
