#!/bin/sh
# Positive and negative controls for the gate implementations; no engine edits.
set -eu
cd "$(dirname "$0")/../.."
port_tmp=$(mktemp -d)
trap 'rm -rf "$port_tmp"' EXIT HUP INT TERM
python3 tools/port/compile_pair.py ded/md4.o "$port_tmp"
for gate in layout symbol; do
    "tools/port/${gate}_gate.sh" "$port_tmp/md4.c.o" "$port_tmp/md4.cxx.o"
done
tools/port/codegen_gate.sh "$port_tmp/md4.c.s" "$port_tmp/md4.cxx.s"
objcopy --localize-symbol=Com_BlockChecksum "$port_tmp/md4.c.o" "$port_tmp/local.o"
if tools/port/symbol_gate.sh "$port_tmp/md4.c.o" "$port_tmp/local.o" > "$port_tmp/symbol.log"; then
    echo 'FAIL: symbol gate missed external-to-static change'; exit 1
fi
# Inject a changed member into a temporary fixture, whose DWARF path is mapped
# into engine scope. The real code/ directory is never modified.
printf 'struct sample { char a; int b; }; struct sample value;\n' > "$port_tmp/sample.c"
gcc -g -fdebug-prefix-map="$port_tmp=code/port-gate-selfcheck" -c "$port_tmp/sample.c" -o "$port_tmp/layout.o"
gcc -g -fpack-struct=1 -fdebug-prefix-map="$port_tmp=code/port-gate-selfcheck" -c "$port_tmp/sample.c" -o "$port_tmp/packed.o"
if tools/port/layout_gate.sh "$port_tmp/layout.o" "$port_tmp/packed.o" > "$port_tmp/layout.log"; then
    echo 'FAIL: layout gate missed changed offsets'; exit 1
fi
# Require an actual diff, not an extraction failure.
grep '^@@' "$port_tmp/layout.log"
sed '0,/ret/s/ret/nop/' "$port_tmp/md4.c.s" > "$port_tmp/changed.s"
if tools/port/codegen_gate.sh "$port_tmp/md4.c.s" "$port_tmp/changed.s" > "$port_tmp/codegen.log"; then
    echo 'FAIL: codegen gate missed changed instruction'; exit 1
fi
printf 'void GetRefAPI(void) {}\n' > "$port_tmp/export.c"
gcc -c "$port_tmp/export.c" -o "$port_tmp/export.c.o"
g++ -x c++ -c "$port_tmp/export.c" -o "$port_tmp/export.cxx.o"
if tools/port/symbol_gate.sh "$port_tmp/export.c.o" "$port_tmp/export.cxx.o" > "$port_tmp/export.log"; then
    echo 'FAIL: symbol gate missed mangled external ABI'; exit 1
fi
# Assembly callees can be undefined in the calling object: test those too.
printf 'void S_WriteLinearBlastStereo16_SSE_x64(int*, short*, int); void invoke(void) { S_WriteLinearBlastStereo16_SSE_x64(0, 0, 0); }\n' > "$port_tmp/import.c"
gcc -c "$port_tmp/import.c" -o "$port_tmp/import.c.o"
g++ -x c++ -c "$port_tmp/import.c" -o "$port_tmp/import.cxx.o"
if tools/port/symbol_gate.sh "$port_tmp/import.c.o" "$port_tmp/import.cxx.o" > "$port_tmp/import.log"; then
    echo 'FAIL: symbol gate missed mangled assembly reference'; exit 1
fi
grep '^@@' "$port_tmp/import.log"
# The GNU private CPUID helper is not the external MSVC assembly entry.
printf 'static void CPUID_EX(void) {} void invoke(void) { CPUID_EX(); }\n' > "$port_tmp/private-cpuid.c"
gcc -c "$port_tmp/private-cpuid.c" -o "$port_tmp/private-cpuid.c.o"
g++ -x c++ -c "$port_tmp/private-cpuid.c" -o "$port_tmp/private-cpuid.cxx.o"
tools/port/symbol_gate.sh "$port_tmp/private-cpuid.c.o" "$port_tmp/private-cpuid.cxx.o"
# Address-taken/static JIT helpers are not external name boundaries.
printf 'static __attribute__((noinline)) int BadJump(int x) { return x+1; } int wrapper(void) { return BadJump(2); }\n' > "$port_tmp/clone.c"
gcc -O2 -c "$port_tmp/clone.c" -o "$port_tmp/clone.c.o"
g++ -x c++ -O2 -c "$port_tmp/clone.c" -o "$port_tmp/clone.cxx.o"
tools/port/symbol_gate.sh "$port_tmp/clone.c.o" "$port_tmp/clone.cxx.o"
# A new float libm reference is a behavior-relevant undefined-symbol difference.
printf 'extern double sin(double); double compute(double x) { return sin(x); }\n' > "$port_tmp/libm-c.c"
printf 'extern float sinf(float); double compute(double x) { return sinf(x); }\n' > "$port_tmp/libm-cxx.c"
gcc -O2 -c "$port_tmp/libm-c.c" -o "$port_tmp/libm-c.o"
gcc -O2 -c "$port_tmp/libm-cxx.c" -o "$port_tmp/libm-cxx.o"
if tools/port/symbol_gate.sh "$port_tmp/libm-c.o" "$port_tmp/libm-cxx.o" > "$port_tmp/libm.log"; then
    echo 'FAIL: symbol gate missed float libm reference'; exit 1
fi
grep '^@@' "$port_tmp/libm.log"
python3 tools/port/compile_pair.py ded/md5.o "$port_tmp"
tools/port/layout_gate.sh "$port_tmp/md5.c.o" "$port_tmp/md5.cxx.o"
# C++ scopes named nested records inside the parent; compare their members too.
printf 'struct outer { struct inner { int a; int b; } child; }; struct outer value;\n' > "$port_tmp/nested.c"
gcc -g -fdebug-prefix-map="$port_tmp=code/port-gate-selfcheck" -c "$port_tmp/nested.c" -o "$port_tmp/nested.c.o"
g++ -x c++ -g -fdebug-prefix-map="$port_tmp=code/port-gate-selfcheck" -c "$port_tmp/nested.c" -o "$port_tmp/nested.cxx.o"
tools/port/layout_gate.sh "$port_tmp/nested.c.o" "$port_tmp/nested.cxx.o"
sed 's/int a; int b;/int b; int a;/' "$port_tmp/nested.c" > "$port_tmp/nested-bad.c"
g++ -x c++ -g -fdebug-prefix-map="$port_tmp=code/port-gate-selfcheck" -c "$port_tmp/nested-bad.c" -o "$port_tmp/nested-bad.o"
if tools/port/layout_gate.sh "$port_tmp/nested.c.o" "$port_tmp/nested-bad.o" > "$port_tmp/nested.log"; then
    echo 'FAIL: layout gate missed changed nested members'; exit 1
fi
grep '^@@' "$port_tmp/nested.log"
# The same nested-layout controls must work with MinGW COFF when available.
if command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
    x86_64-w64-mingw32-gcc -g -fdebug-prefix-map="$port_tmp=code/port-gate-selfcheck" -c "$port_tmp/nested.c" -o "$port_tmp/coff.c.o"
    x86_64-w64-mingw32-g++ -x c++ -g -fdebug-prefix-map="$port_tmp=code/port-gate-selfcheck" -c "$port_tmp/nested.c" -o "$port_tmp/coff.cxx.o"
    x86_64-w64-mingw32-g++ -x c++ -g -fdebug-prefix-map="$port_tmp=code/port-gate-selfcheck" -c "$port_tmp/nested-bad.c" -o "$port_tmp/coff-bad.o"
    tools/port/layout_gate.sh "$port_tmp/coff.c.o" "$port_tmp/coff.cxx.o"
    x86_64-w64-mingw32-gcc -ffunction-sections -c "$port_tmp/private-cpuid.c" -o "$port_tmp/coff-symbol.c.o"
    x86_64-w64-mingw32-g++ -x c++ -ffunction-sections -c "$port_tmp/private-cpuid.c" -o "$port_tmp/coff-symbol.cxx.o"
    tools/port/symbol_gate.sh "$port_tmp/coff-symbol.c.o" "$port_tmp/coff-symbol.cxx.o"
    if tools/port/layout_gate.sh "$port_tmp/coff.c.o" "$port_tmp/coff-bad.o" > "$port_tmp/coff.log"; then
        echo 'FAIL: COFF layout gate missed changed nested members'; exit 1
    fi
    grep '^@@' "$port_tmp/coff.log"
fi
echo 'PASS: gate positive and negative controls'
