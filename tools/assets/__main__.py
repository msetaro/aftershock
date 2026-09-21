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
    args = parser.parse_args()
    try:
        print(json.dumps(validate(args.root),sort_keys=True))
        return 0
    except (OSError,ValueError,KeyError,TypeError) as exc:
        print(json.dumps(dict(ok=False,error=str(exc))),file=sys.stderr)
        return 1


if __name__=='__main__':
    sys.exit(main())
