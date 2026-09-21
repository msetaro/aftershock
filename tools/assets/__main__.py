#!/usr/bin/env python3
"""Fetch pinned licensed theme assets and verify their complete provenance."""
import argparse
import json
import os
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
    find = sub.add_parser('search')
    find.add_argument('--provider',choices=['polyhaven','ambientcg'],required=True)
    find.add_argument('--tag',required=True)
    find.add_argument('--limit',type=int,default=10)
    pin = sub.add_parser('pin')
    pin.add_argument('--theme',required=True)
    pin.add_argument('--asset',action='append',required=True,help='provider:id:role (repeat for each asset)')
    pin.add_argument('--resolution',type=int,default=1024)
    pin.add_argument('--out',type=Path,required=True,help='new lock file; existing locks are never replaced')
    args = parser.parse_args()
    try:
        if args.command=='validate':
            result = validate(args.root)
        elif args.command=='search':
            from tools.assets.providers import search
            result = dict(ok=True,assets=search(args.provider,args.tag,args.limit))
        elif args.command=='pin':
            from tools.assets.providers import pin
            if args.out.exists():
                raise ValueError('lock already exists; author a new lock for review')
            if args.resolution<8 or args.resolution>4096 or args.resolution&(args.resolution-1):
                raise ValueError('resolution must be a power of two in 8..4096')
            cache = Path(os.environ.get('XDG_CACHE_HOME',Path.home()/'.cache'))/'aftershock-assets'
            assets = [pin(*value.split(':'),args.resolution,cache) for value in args.asset]
            lock = dict(version=1,theme=args.theme,resolution=args.resolution,assets=assets)
            args.out.parent.mkdir(parents=True,exist_ok=True)
            with args.out.open('x') as stream:
                stream.write(json.dumps(lock,indent=2,sort_keys=True)+'\n')
            result = dict(ok=True,lock=str(args.out),assets=len(assets))
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
