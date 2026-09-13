#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
port_tmp=$(mktemp -d)
trap 'rm -rf "$port_tmp"' EXIT HUP INT TERM
python3 tools/port/compile_pair.py ded/q_math.o "$port_tmp"
cc -O2 -DNDEBUG tools/port/math_check.c "$port_tmp/q_math.c.o" -lm -o "$port_tmp/math-c"
c++ -x c++ -std=c++20 -fno-exceptions -fno-rtti -O2 -DNDEBUG tools/port/math_check.c -x none "$port_tmp/q_math.cxx.o" -lm -o "$port_tmp/math-cxx"
"$port_tmp/math-c" > "$port_tmp/c.txt"
"$port_tmp/math-cxx" > "$port_tmp/cxx.txt"
if diff -u "$port_tmp/c.txt" "$port_tmp/cxx.txt"; then
    echo 'PASS: G5 math fixed-input hashes'
else
    echo 'FAIL: G5 math fixed-input hashes'
    exit 1
fi
