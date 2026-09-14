#!/bin/sh
set -eu
exec python3 "$(dirname "$0")/gates.py" symbol "$@"
