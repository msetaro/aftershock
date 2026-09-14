# C++20 port checkpoint

Branch: `t3code/port-engine-to-cpp20`. Original C oracle: `8a7e8ed2`. Reviewed continuation starts at `6a990e7c` (T1–T23 plan amendment). No engine files have been renamed. No engine bug fixes, vendor edits, renderer2 port, main pushes, force pushes, or history rewrites were made.

**Phases 0–2 complete.** All 257 inventory entries are done (including the two explicit exclusions). Native GCC/Clang C and strict C++ configurations all build. MinGW, ARM, AArch64 and PPC64LE C++ full links pass. T24/T25 resolve all reviewed blockers with unchanged C hashes. C++ dedicated runtime matches C under G6, and the client loads OpenGL/Vulkan renderers under Xvfb.

## Next action

Continuation: renderervk; next `phase3` under amended T1-T23. Preserve completed transformations; continue native math, remaining native blockers, cross targets, T5 review, then rename only after native gates/runtime pass.

## Phase checklist

- [x] Phase 0: harness, frozen warnings, C hash proof, controls; formatter advisory.
- [x] Phase 1: all scoped sources and headers, unchanged C hashes, strict native/cross builds.
- [x] Phase 2: required external linkage, G2/G3, full native links, deterministic dedicated smoke, client-driven renderer load.
- [ ] Phase 3: rename, fresh build/gates/runtime, MSVC CI.
- [ ] Post-rename T25 cleanup, unchanged C++ hashes, final checkpoint.

## Remaining blockers

None currently. Historical sv_client hash and Windows COM evidence remains below; T24/T25 resolved those failures without granting hash exceptions.

## Module inventory

“Done” is a per-file result, not a claim that the complete C++ engine runs. qasm.h is assembly-only; sv_rankings.c is excluded and must not be renamed. Shared cgame/game/ui ABI headers are included; this checkout has no bg implementation sources.

| Module | Done | Blocked |
|---|---:|---:|
| asm | 1 | 0 |
| botlib | 62 | 0 |
| cgame | 1 | 0 |
| client | 29 | 0 |
| game | 2 | 0 |
| qcommon | 42 | 0 |
| renderer | 28 | 0 |
| renderercommon | 15 | 0 |
| renderervk | 30 | 0 |
| sdl | 6 | 0 |
| server | 13 | 0 |
| ui | 1 | 0 |
| unix | 13 | 0 |
| win32 | 14 | 0 |

## Historical verification results at 7ec7e925 (superseded by continuation results below)

- C oracle: default native 295/295 hashes and non-SDL 299/299 hashes unchanged. Full client/ded/renderers C links pass. Each ARM, AArch64 and PPC64LE dedicated oracle retains 65/65 hashes and links. MinGW non-SDL retains 300/300 hashes; its default curl-enabled client lacks target zlib, while ded and both DLLs link. The matching USE_CURL=0 baseline/current build links client/ded/both renderers and retains 299/299 C hashes (mingw-client-c-results.json, cross-mingw64-nocurl-c.sha256). Original-base cross builds were extracted with git archive into /tmp; port work stayed in this worktree.
- Native matrix: all eight C configurations pass; all eight C++ configurations fail only in sv_client.c. GCC release/debug SDL and non-SDL plus static OpenGL/Vulkan; Clang release SDL and debug non-SDL. Exact commands and error counts: `tools/port/evidence/resumed-build-matrix-results.json`; full logs `resumed-*.log.gz`.
- Cross C++ matrix: ARM, AArch64 and PPC64LE fail only in sv_client.c; MinGW also fails win_input.c/win_snd.c. Both MinGW renderer DLLs link. Exact commands/errors: `tools/port/evidence/final-cross-results.json`; full logs `final-cross-*.log.gz`. Cross-only JIT files and nine Windows implementation files passed strict release/debug object builds and original C hashes before their per-file commits.
- native-tu: 154 compiled object pairs, G2/G3 PASS; G4 39 PASS and 115 advisory differences. Exact object/variable list: `tools/port/evidence/native-tu-final-results.json`.
- cross: 417 compiled object pairs, G2/G3 PASS; G4 85 PASS and 332 advisory differences. Exact object/variable list: `tools/port/evidence/cross-final-results.json`.
- native-context: 450 compiled object pairs, G2/G3 PASS; G4 123 PASS and 327 advisory differences. Exact object/variable list: `tools/port/evidence/native-context-final-results.json`.
- G5: all 13 groups PASS with one C-compiled driver linked to either object set; vector_math is `5c00b4de` for both. Standalone math gate PASS. Evidence: `g5-final.log`, `math-final.log`. Coverage includes parsing, strings/info, commands/cvars, FS paths, MSG roundtrips plus 256 delta trials, adaptive Huffman, addresses, 1,000 collision traces, Q_rsqrt/Q_fabs and vector math. The full memory/filesystem/runtime is not modeled. Test-only unambiguous objcopy aliases let the C driver call C++ functions; production G3 remains independent.
- G6: warm-up then two C runs under the prescribed faketime increment exit 0 and yield byte-identical **124-line logs** (`final-faketime-c-{1,2}.log.gz`). Both C++ renderer libraries from the native client build load with dlopen and expose GetRefAPI (`final-runtime-summary.log`). C++ executable smoke and client-driven renderer load are blocked by linking. Xvfb is now installed, but cannot remedy missing C++ executables; earlier no-display diagnoses are superseded. No client timedemo/image equivalence is claimed.
- G7: 115 changed native sources analyzed with clang-tidy, zero tool/compile failures; 366 bugprone-narrowing findings retained. Thirteen non-native/blocked/include sources are skipped. Exact results: `resumed-static-summary.txt`, `resumed-static-warnings.json`, `resumed-static-clang-tidy.log.gz`. No warning-driven engine cleanup was performed.
- Earlier C ASan/UBSan smoke exited 0 with no diagnostics after narrow known alignment suppressions; leak checking disabled. Evidence remains `final-runtime-sanitize.log.gz`, `tools/port/ubsan.supp`. C++ sanitizer/runtime comparison remains unavailable.
- G8: final engine diff is 134 files, 1,859 insertions / 1,758 deletions (about 0.6% of engine lines). Reviewer checked T1–T23, T18 declarations, T21 expression preservation and linkage removals; full report: final-g8-review.txt. All gate controls pass (`final-controls.log`). The explicit T15/stat exception is below.

- Final static-renderer boundary checks: OpenGL/Vulkan cl_main consumers and tr_init definitions (four pairs) pass G2/G3; G4 advisory diffs are indexed below. Exact variables: static-boundaries-final-results.json.

## Decisions, scope and harness details

- Only GNU Make is supported; plan section 7 supersedes early CMake references. Build mode uses C++20, no exceptions/RTTI or permissive flag; vendors remain C, assembly unchanged, renderer2 skipped only in C++ mode until rename. Single objects and make -k work. CXX derives from the same compiler prefix as CC, respecting explicit CXX. SOURCE_DATE_EPOCH=1789257600 stabilizes date/time macros for raw hashes.
- T1 preserves exact existing destination types/calling conventions, including function/object pointers. T2 consistently casts to qboolean. T3 preserves the original promoted expression; no offset/operand reorder. T19 moves the SDL console-key enum intact. T20 only changes receiving locals/casts. T23 guards the pre-defined GNU feature macro. T18 adds one huffman table, codec const linkage, and 74 exact shader-array extern declarations without changing data.
- T21 adds 364 net double argument casts: 347 reviewed native casts, six conditional common.c rint calls, and eleven MSVC/fallback-M_PI sites. Only the original complete argument expression is wrapped; no FP expression is restructured. The first q_math edit briefly had seven redundant nested casts, removed in a subsequent commit without history rewrite. Native review of all 98 abs calls found no non-integer argument, so no T22 changes were needed. Two fabs calls inside #if 0 remain untouched. Dormant platform/SDK branches outside verified configurations are not claimed as compiled.
- T4 renderer field renames were atomic prerequisites: all existing uses were renamed together to keep C compiling, with identical C hashes, followed by per-file commits. No strings/comments were renamed. Phase 2 removed C linkage from six static callbacks in three files, syscall_t/dllSyscall_t, and static-renderer GetRefAPI. Kept the listed DLL typedefs, dlopen exports, assembly boundaries and GPU exports. T11 __clear_cache is an existing libgcc import, correctly declared void(void*,void*) with C linkage in vm_local.h for ARM/AArch64; target compiler ABI and unchanged C hashes were checked.
- G2 uses actual engine DWARF and pahole sizes/offsets/alignment/nested members, excluding system/vendor types by declaration source (never by intersecting results). MinGW COFF DWARF relocations are resolved in a temporary PE carrier and converted to ELF; dummy undefined definitions exist only in this never-executed layout carrier. G3 always inspects original objects.
- G3 uses separate -O2 probe objects with -fno-builtin, -fno-inline-functions, -D__NO_CTYPE=1 and -U_FORTIFY_SOURCE. GCC probes also disable small/called-once inlining and IPA scalar replacement. MinGW -Wa,-L retains actual local C functions beginning L. Exact ARM/AArch64 local instruction/data mapping markers are metadata. All other source-defined and undefined symbols are retained/demangled, preserving static/global kind. GetRefAPI stays raw for dlopen (both compiler commands must agree); absent metadata conservatively enforces raw names. Controls catch static/export changes, missing assembly C linkage and newly selected float libm calls. Plain -fno-inline was rejected because it materializes integer math template helpers.
- G2/G4/G5 and production builds retain their own real optimization flags. MinGW default -flto gives serialized IR with unstable build IDs even for untouched md4, so object oracles use matching -O2 -ffast-math -fno-lto on base/current. Default-LTO dedicated C links were separately verified. Inspection artifacts disable LTO so actual code/DWARF can be compared. No production flags were changed to make gates pass.
- All requested cross compilers are installed and used: x86_64-w64-mingw32-g++ (GCC 13), aarch64-linux-gnu-g++, arm-linux-gnueabihf-g++, powerpc64le-linux-gnu-g++ (GCC 15.2). ARM explicitly uses LONG_BIT=32. 32-bit x86 is excluded; its CI legs remain until the deferred rename commit. MSVC verification is deferred with that phase; no MSVC green result is claimed. No system packages were installed.
- Optional BUILD_FREETYPE remains unverified because FreeType headers/pkg-config are absent. win_shared.c default configuration is compiled; the dormant USE_PROFILES branch has no build-system definition and is not verified. The explicitly requested cross JIT files and normal Windows configurations have actual compiler evidence, replacing prior inspection-only labels.
- Pak files stay in ~/.q3a/baseq3; no game data is committed/copied into the repository. G5 collision map is extracted only under /tmp from pak0.pk3; q3dm17.bsp SHA256 ee1394417b06d92f705088150d7609d6b9f55f5796a9a7626bfe7982e8fc94e8.

## Frozen warning inventory

Only observed classes are suppressed; C flags are unchanged. Counts include repeated build contexts.

| Class | Original C | Original C++ | Scope |
|---|---:|---:|---|
| sign-compare | 544 | 532 | all C++ |
| unused-parameter | 171 | 173 | all C++ |
| missing-field-initializers | 100 | 595 | all C++ |
| implicit-fallthrough | 39 | 39 | all C++ |
| ignored-qualifiers | 4 | 4 | all C++ |
| type-limits | 2 | 2 | all C++ |
| write-strings | tolerated by C | 243 | all C++; review accepted ordinary frozen entry |
| parentheses | tolerated by C | 48 | all C++; review accepted ordinary frozen entry |
| cast-function-type | 9 in 11 Windows TUs | observed at same legacy casts | MinGW C++ only |
| unused-function | 2 per Clang configuration | 2 per configuration | Clang C++ only; unchanged HasFCOM |
| varargs | no C diagnostic | 1 per Clang configuration | Clang C++ only; unchanged CURLoption va_start |

## Every DEVIATION

- `5a28cd29 DEVIATION: preserve source style despite formatter threshold`: 16 formatter trials could not reproduce mixed original style below 3% (cvar 25.549%, cl_main 17.852%). No engine reformat. Accepted continuation makes the formatter advisory; this historical threshold blocker is resolved.
- `dc031ab7 DEVIATION: freeze observed legacy string-literal warnings`: froze 243 observed write-strings warnings rather than change signatures/behavior. Accepted continuation folds this into the ordinary frozen list; historical commit retained.
- `7553c037 DEVIATION: freeze observed parenthesized-declarator warnings`: froze 48 existing macro-declaration warnings instead of rewriting source. Also accepted as ordinary frozen entry; historical commit retained.
- `8d827a6d DEVIATION: freeze observed Clang compatibility warnings`: added Clang-only unused-function/varargs suppressions after fresh matrix reached unchanged HasFCOM/CURLoption sites. Counts above; retained varargs hazard is logged. No engine fixes, public-signature changes or C flag changes.
- `74233370 DEVIATION: retain required T15 lexical spaces in diff check`: the literal -w heuristic erases required C++ literal/macro token separators. Per-file minimal stats agree in 132/134 files. common.c is 38/38 versus 16/16 and snd_dma.c 2/2 versus 1/1; all 23 omitted lines are T15, not formatting cleanup. Retain the explicitly authorized lexical changes; do not add fake substantive tokens to game the metric. Exact diff: `g8-stat.diff`. No engine content changed in the decision commit.

## Bugs and compatibility hazards logged, not fixed

Full ledger: `docs/cpp-port-notes.md`. It records upstream CMake defects; preexisting unaligned unzip/vm accesses and sanitizer suppression limits; legacy linux_snd pthread signature mismatch; cl_curl's terminating-NUL slash test; FS_AllowedExtension's NULL relational comparison; and the retained CURLoption va_start warning. The original float-math overload hazard is resolved through T21, and the unfaked bot nondeterminism is controlled by the accepted faketime test. No unrelated source behavior was fixed.

## Exact reproduction commands

Run from this branch/worktree. Results and compiler versions are host-dependent; use the recorded original compiler versions for raw SHA256 comparisons.

```sh
export SOURCE_DATE_EPOCH=1789257600
port_repo=$PWD
make -j20 BUILD_DIR=/tmp/aftershock-cpp-port/oracle
make -j20 BUILD_CLIENT=0 BUILD_DIR=/tmp/aftershock-cpp-port/oracle
(cd /tmp/aftershock-cpp-port/oracle && sha256sum -c "$port_repo/tools/port/evidence/phase0-c.sha256")
make -j20 USE_SDL=0 BUILD_DIR=/tmp/aftershock-cpp-port/oracle-nosdl
(cd /tmp/aftershock-cpp-port/oracle-nosdl && sha256sum -c "$port_repo/tools/port/evidence/nosdl-c.sha256")

tools/port/selfcheck.sh
python3 tools/port/completed_gates.py /tmp/aftershock-cpp-port/recheck-tus
python3 tools/port/cross_gates.py /tmp/aftershock-cpp-port/recheck-native native
python3 tools/port/cross_gates.py /tmp/aftershock-cpp-port/recheck-cross
python3 tools/port/build_matrix.py /tmp/aftershock-cpp-port/recheck-matrix
python3 tools/port/static_gate.py /tmp/aftershock-cpp-port/recheck-static
python3 tools/port/differential_gate.py /tmp/aftershock-cpp-port/recheck-g5
tools/port/math_gate.sh
```

Expected: gates, static execution and differential harness return 0; build matrix returns nonzero in the explicitly blocked source. G4 per-object exit 1 is advisory; full diffs are indexed below. Every compile_pair output records its exact compiler command in a neighboring .command file.

```sh
python3 tools/port/compile_pair.py ded/q_math.o /tmp/port-qmath
# Substitute md4.o or any object/variables from the JSON result inventories.
tools/port/layout_gate.sh /tmp/port-qmath/q_math.c.o /tmp/port-qmath/q_math.cxx.o
tools/port/symbol_gate.sh /tmp/port-qmath/q_math.c.sym.o /tmp/port-qmath/q_math.cxx.sym.o
tools/port/codegen_gate.sh /tmp/port-qmath/q_math.c.s /tmp/port-qmath/q_math.cxx.s

command -v x86_64-w64-mingw32-g++ aarch64-linux-gnu-g++ arm-linux-gnueabihf-g++ powerpc64le-linux-gnu-g++
make -j20 BUILD_CLIENT=0 PLATFORM=mingw64 ARCH=x86_64 BUILD_DIR=/tmp/port-mingw-c
make -k -j20 BUILD_CXX=1 PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 USE_CURL=0 BUILD_DIR=/tmp/port-mingw-cxx
make -k -j20 BUILD_CXX=1 BUILD_CLIENT=0 ARCH=aarch64 CC=aarch64-linux-gnu-gcc BUILD_DIR=/tmp/port-aarch64-cxx
make -k -j20 BUILD_CXX=1 BUILD_CLIENT=0 ARCH=arm LONG_BIT=32 CC=arm-linux-gnueabihf-gcc BUILD_DIR=/tmp/port-arm-cxx
make -k -j20 BUILD_CXX=1 BUILD_CLIENT=0 ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc BUILD_DIR=/tmp/port-ppc-cxx
python3 tools/port/compile_pair.py ded/vm_aarch64.o /tmp/port-aarch64-gates ARCH=aarch64 CC=aarch64-linux-gnu-gcc
python3 tools/port/compile_pair.py client/win_main.o /tmp/port-win-gates PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 'OPTIMIZE=-O2 -ffast-math -fno-lto'
```

For the full cross C oracle commands and corresponding manifest filenames use `final-oracle-results.json`; unchanged-base commands and working directories are in `cross-oracles.json`. Do not compare serialized MinGW LTO IR hashes. No cross executable runtime emulation is claimed.

```sh
port_binary=/tmp/aftershock-cpp-port/oracle/release-linux-x86_64/quake3e.ded.x64
for port_run in warm 1 2; do
  timeout 90 faketime -f "@2026-01-01 00:00:00 i0.01" "$port_binary"     +set dedicated 1 +set sv_pure 0 +set com_logfile 0 +map q3dm17     +addbot sarge 3 +addbot major 3 +wait 300 +quit > "/tmp/port-c-$port_run.log" 2>&1
done
diff -u /tmp/port-c-1.log /tmp/port-c-2.log
# After a valid C++ executable exists, repeat with it and diff against C run 2.
# timeout must stay outside faketime; no additional log normalization is used.
```

GetRefAPI library proof used ctypes.CDLL(path).GetRefAPI on both libraries listed in `final-runtime-summary.log` (built in gcc-c1-release-sdl). It exercises dlopen/dlsym, not a client frame. The production client/static executable path remains blocked.

## Per-file status

All 257 scoped .c/.h entries appear exactly once in this table. Native status refers to actual compiler contexts; header verification follows consumers. Repeated contexts and complete G4 results are in the JSON inventories/index. Earlier per-file commits preserve original transformation counts where subsequent linkage/math passes updated the row.

| File | Status | Transformations and verification |
|---|---|---|
| `code/asm/qasm.h` | done | Not applicable: included only by assembly .s files; explicitly outside C++ inputs. No rename. |
| `code/botlib/aasfile.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_bsp.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_bspq3.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_bspq3.o); G4 advisory difference retained. |
| `code/botlib/be_aas_cluster.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_cluster.o); G4 advisory difference retained. |
| `code/botlib/be_aas_cluster.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_debug.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_debug.o); G4 advisory difference retained. |
| `code/botlib/be_aas_debug.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_def.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_entity.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_entity.o); G4 PASS. |
| `code/botlib/be_aas_entity.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_file.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_file.o); G4 advisory difference retained. |
| `code/botlib/be_aas_file.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_funcs.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_main.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_main.o); G4 PASS. |
| `code/botlib/be_aas_main.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_move.c` | done | T1-T17: 0 (already compatible); T21/T22: 3 argument casts at 3 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_move.o, default); G4 PASS. |
| `code/botlib/be_aas_move.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_optimize.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_optimize.o); G4 PASS. |
| `code/botlib/be_aas_optimize.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_reach.c` | done | T1-T17: 0 (already compatible); T21: 15 argument casts at 15 calls (native and fallback M_PI); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_reach.o, default); G4 advisory difference retained. |
| `code/botlib/be_aas_reach.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_route.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_route.o); G4 advisory difference retained. |
| `code/botlib/be_aas_route.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_routealt.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_routealt.o); G4 advisory difference retained. |
| `code/botlib/be_aas_routealt.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_sample.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_sample.o); G4 advisory difference retained. |
| `code/botlib/be_aas_sample.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_char.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_char.o); G4 advisory difference retained. |
| `code/botlib/be_ai_char.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_char.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_chat.c` | done | T1: 4; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_chat.o); G4 advisory difference retained. |
| `code/botlib/be_ai_chat.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_chat.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_gen.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_gen.o); G4 PASS. |
| `code/botlib/be_ai_gen.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_gen.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_goal.c` | done | T1: 1, T16: 1 macro site (8 expanded casts); clang narrowing follow-up; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_goal.o, default); G4 PASS. |
| `code/botlib/be_ai_goal.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_goal.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_move.c` | done | T1: 1; T21/T22: 12 argument casts at 12 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_move.o, default); G4 advisory difference retained. |
| `code/botlib/be_ai_move.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_goal.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_weap.c` | done | T1: 1, T16: 2 macro sites (36 expanded casts); clang narrowing follow-up; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_weap.o, default); G4 PASS. |
| `code/botlib/be_ai_weap.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_weap.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_weight.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_weight.o); G4 PASS. |
| `code/botlib/be_ai_weight.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_goal.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ea.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ea.o); G4 PASS. |
| `code/botlib/be_ea.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_chat.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_interface.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_interface.o); G4 PASS. |
| `code/botlib/be_interface.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_debug.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/botlib.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_crc.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_crc.o); G4 PASS. |
| `code/botlib/l_crc.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_route.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_libvar.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_libvar.o); G4 PASS. |
| `code/botlib/l_libvar.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_cluster.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_log.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_log.o); G4 PASS. |
| `code/botlib/l_log.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_cluster.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_memory.c` | done | T1: 2; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_memory.o); G4 advisory difference retained. |
| `code/botlib/l_memory.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_precomp.c` | done | T1: 3 (one in inactive LoadSourceMemory), T4: 1 field (10 occurrences); T21/T22: 3 argument casts at 3 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_precomp.o, default); G4 advisory difference retained. |
| `code/botlib/l_precomp.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_script.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_script.o); G4 advisory difference retained. |
| `code/botlib/l_script.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_struct.c` | done | T3: 14; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_struct.o); G4 advisory difference retained. |
| `code/botlib/l_struct.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_utils.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_entity.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/cgame/cg_public.h` | done | T1-T17: 0; unchanged shared ABI header verified through cl_cgame.c actual dependency and native strict builds/G2/G3. |
| `code/client/cl_avi.c` | done | T1: 2; T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_avi.o, default); G4 advisory difference retained. |
| `code/client/cl_cgame.c` | done | T1: 116, T2: 1, T3: 6; T21/T22: 7 argument casts at 6 calls; T5 internal callback annotations removed; current C hashes/strict builds/G2/G3 PASS (client/cl_cgame.o); G4 advisory difference retained. |
| `code/client/cl_cin.c` | done | T1: 2, T2: 5; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_cin.o, default); G4 advisory difference retained. |
| `code/client/cl_console.c` | done | T1: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_console.o, default); G4 advisory difference retained. |
| `code/client/cl_curl.c` | done | T1: 35; T20: 1 receiving local const; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_curl.o, default); G4 advisory difference retained. |
| `code/client/cl_curl.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/cl_input.c` | done | T1-T17: 0 (already compatible); T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_input.o, default); G4 advisory difference retained. |
| `code/client/cl_jpeg.c` | done | T1: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_jpeg.o, default); G4 advisory difference retained. |
| `code/client/cl_keys.c` | done | T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_keys.o, default); G4 advisory difference retained. |
| `code/client/cl_main.c` | done | T1: 1; T2: 1; T3: 6 compound-assignment result casts; T20: 1 receiving local const; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_main.o, default); G4 advisory difference retained. |
| `code/client/cl_net_chan.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_net_chan.o, default); G4 advisory difference retained. |
| `code/client/cl_parse.c` | done | T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_parse.o, default); G4 advisory difference retained. |
| `code/client/cl_scrn.c` | done | T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_scrn.o, default); G4 advisory difference retained. |
| `code/client/cl_ui.c` | done | T1: 80, T2: 1, T3: 8; T21/T22: 7 argument casts at 6 calls; T5 internal callback annotations removed; current C hashes/strict builds/G2/G3 PASS (client/cl_ui.o); G4 advisory difference retained. |
| `code/client/client.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/keycodes.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/keys.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/snd_adpcm.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_adpcm.o, default); G4 PASS. |
| `code/client/snd_codec.c` | done | T1: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_codec.o, default); G4 PASS. |
| `code/client/snd_codec.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/snd_codec.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/snd_codec_ogg.c` | done | T1: 3; T18: 1 preceding extern const declaration; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_codec_ogg.o, default); G4 PASS. |
| `code/client/snd_codec_wav.c` | done | T1: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_codec_wav.o, default); G4 PASS. |
| `code/client/snd_dma.c` | done | T1: 1, T15: 2 (non-SDL branch); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_dma.o, default); G4 advisory difference retained. |
| `code/client/snd_local.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/snd_main.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_main.o, default); G4 PASS. |
| `code/client/snd_mem.c` | done | T1: 4; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_mem.o, default); G4 advisory difference retained. |
| `code/client/snd_mix.c` | done | T5: C linkage block for 3 assembly globals plus 5 assembly declarations/conditional definitions; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_mix.o, default); G4 advisory difference retained. |
| `code/client/snd_public.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/snd_wavelet.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_wavelet.o, default); G4 PASS. |
| `code/game/bg_public.h` | done | T1-T17: 0; unchanged shared ABI header verified through sv_game.c actual dependency and native strict builds/G2/G3. |
| `code/game/g_public.h` | done | T1-T17: 0; unchanged shared ABI header verified through sv_game.c actual dependency and native strict builds/G2/G3. |
| `code/qcommon/cm_load.c` | done | T1: 28; native ded C SHA256 unchanged (df42e0cabff475c22ebf8383e443cfe34d558abf817baf6f5cd8ad0c96700017); strict C++/G2/G3 PASS (ded/cm_load.o, default); G4 advisory difference retained. |
| `code/qcommon/cm_local.h` | done | T1-T17: 0; unchanged header checked via cm_load.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_patch.c` | done | T1: 3, T2: 6, T3: 3; T21/T22: 14 argument casts at 14 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_patch.o, default); G4 advisory difference retained. |
| `code/qcommon/cm_patch.h` | done | T1-T17: 0; unchanged header checked via cm_patch.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_polylib.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_polylib.o); G4 PASS. |
| `code/qcommon/cm_polylib.h` | done | T1-T17: 0; unchanged header checked via cm_polylib.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_public.h` | done | T1-T17: 0; unchanged header checked via cm_load.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_test.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_test.o); G4 PASS. |
| `code/qcommon/cm_trace.c` | done | T1-T17: 0 (already compatible); T21/T22: 6 argument casts at 6 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_trace.o, default); G4 advisory difference retained. |
| `code/qcommon/cmd.c` | done | T1: 1, T2: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cmd.o); G4 advisory difference retained. |
| `code/qcommon/common.c` | done | T1: 5, T2: 2, T3: 1, T15: 29; T5: 2 conditional MSVC CPUID_EX declarations/definitions; T21: six conditional rint calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/common.o, default); G4 advisory difference retained. |
| `code/qcommon/cvar.c` | done | T2: 2, T3: 4 (cast compound-assignment result); T21/T22: 2 argument casts at 2 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cvar.o, default); G4 advisory difference retained. |
| `code/qcommon/files.c` | done | T1: 13, T2: 2; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/files.o); G4 advisory difference retained. |
| `code/qcommon/history.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/history.o); G4 advisory difference retained. |
| `code/qcommon/huffman.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/huffman.o); G4 advisory difference retained. |
| `code/qcommon/huffman_static.c` | done | T18: one preceding extern const declaration; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/huffman_static.o, default); G4 PASS. |
| `code/qcommon/json.h` | done | Excluded: whole-tree include search finds only renderer2/tr_bsp.c; implementation is solely for excluded renderer2. Left unchanged. |
| `code/qcommon/keys.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/keys.o); G4 advisory difference retained. |
| `code/qcommon/md4.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/md4.o); G4 PASS. |
| `code/qcommon/md5.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/md5.o); G4 advisory difference retained. |
| `code/qcommon/msg.c` | done | T16: 3 sites (1 mask, 99 expanded field-offset casts); clang narrowing follow-up; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/msg.o, default); G4 advisory difference retained. |
| `code/qcommon/net_chan.c` | done | T1: 1, T4: 1 identifier (12 occurrences); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/net_chan.o); G4 advisory difference retained. |
| `code/qcommon/net_ip.c` | done | T1: 2, T2: 1; T1: 11 additional Winsock casts; native and MinGW C hashes, strict release/debug and G2/G3 PASS (ded/net_ip.o); G4 advisory difference retained. |
| `code/qcommon/puff.c` | done | T1-T17: 0 (already compatible); 3 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/puff.o); G4 advisory difference retained. |
| `code/qcommon/puff.h` | done | T1-T17: 0; unchanged header checked via puff.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/q_math.c` | done | T21: 20 argument casts at 18 calls, redundant cast correction; 4 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/q_math.o, default); G4 PASS. |
| `code/qcommon/q_platform.h` | done | T1-T17: 0; unchanged header checked via md4.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/q_shared.c` | done | T1: 4, T2: 3; 4 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/q_shared.o); G4 advisory difference retained. |
| `code/qcommon/q_shared.h` | done | T5: guarded Q_EXTERN_C macro plus 2 Windows assembly prototypes; native md4 consumer G2/G3 PASS and all 295 C hashes unchanged; Windows branch unverified. |
| `code/qcommon/qcommon.h` | done | T5 review removes internal-only annotations; 295/295 C object hashes unchanged; actual consuming objects G2/G3 PASS, G4 advisory evidence retained: ded/vm.o, client/cl_cgame.o. |
| `code/qcommon/qfiles.h` | done | T1-T17: 0; unchanged header checked via cm_load.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/surfaceflags.h` | done | T1-T17: 0; unchanged header checked via cm_load.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/unzip.c` | done | T1: 10, T14: 5; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/unzip.o); G4 advisory difference retained. |
| `code/qcommon/unzip.h` | done | T1-T17: 0; unchanged header checked via unzip.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/vm.c` | done | T1: 5; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/qvm/vm.o); G4 advisory difference retained. |
| `code/qcommon/vm_aarch64.c` | done | T1: 2 allocator result casts; T11 cache prototype in vm_local.h; aarch64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (ded/qvm/vm_aarch64.o); G4 advisory difference retained. |
| `code/qcommon/vm_armv7l.c` | done | T5 four external libgcc assembly imports; arm original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (ded/qvm/vm_armv7l.o); G4 advisory difference retained. |
| `code/qcommon/vm_interpreted.c` | done | T1: 1; literal retained under frozen warning policy; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/qvm/vm_interpreted.o); G4 advisory difference retained. |
| `code/qcommon/vm_local.h` | done | Existing T4 edits; T11: correctly typed C-linkage declaration of existing GNU ARM runtime __clear_cache dependency. ARM/AArch64 65/65 C hashes unchanged; ARM vm_interpreted consumer strict C++/G2/G3 PASS; G4 advisory evidence cross-arm-vm_interpreted.codegen.diff.gz. |
| `code/qcommon/vm_optimize.h` | done | Unchanged; real native x86_64 and cross ARM/AArch64/PPC JIT consumers pass strict C++, G2/G3 and original C hashes. |
| `code/qcommon/vm_powerpc.c` | done | T1: 7 pointer conversions including debug-only callback; ppc64le original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (ded/qvm/vm_powerpc.o); G4 advisory difference retained. |
| `code/qcommon/vm_x86.c` | done | T1: 1 function-to-object pointer cast; T3: 1 macro_op_t cast; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/qvm/vm_x86.o, default); G4 advisory difference retained. |
| `code/renderer/iqm.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderer/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderer/qgl.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderer/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderer/tr_animation.c` | done | T1: 3, T2: 1, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_animation.o, default); G4 advisory difference retained. |
| `code/renderer/tr_arb.c` | done | T4: 3 occurrences (prerequisite), T2: 5, T3: 4, T17: 2; T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_arb.o, default); G4 advisory difference retained. |
| `code/renderer/tr_backend.c` | done | T4: 9 occurrences (prerequisite), T1: 4, T2: 2, T3: 3, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_backend.o, default); G4 advisory difference retained. |
| `code/renderer/tr_bsp.c` | done | T1: 42, T3: 2; T21/T22: 94 argument casts at 94 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_bsp.o, default); G4 advisory difference retained. |
| `code/renderer/tr_cmds.c` | done | T1: 11; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_cmds.o, default); G4 advisory difference retained. |
| `code/renderer/tr_common.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderer/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderer/tr_curve.c` | done | T1: 3; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_curve.o, default); G4 advisory difference retained. |
| `code/renderer/tr_flares.c` | done | T4: 3 occurrences (prerequisite), T2: 1; T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_flares.o, default); G4 advisory difference retained. |
| `code/renderer/tr_image.c` | done | T1: 9, T2: 2, T3: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image.o, default); G4 advisory difference retained. |
| `code/renderer/tr_init.c` | done | T1: 10, T2: 1, T3: 1, T5: dlopen definition only; T21: 1 fallback M_PI argument cast; static GetRefAPI C linkage removed; C hash/strict C++/G2/G3 PASS (rend1/tr_init.o); G4 advisory difference retained. |
| `code/renderer/tr_light.c` | done | T4: 10 occurrences (prerequisite); T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_light.o, default); G4 advisory difference retained. |
| `code/renderer/tr_local.h` | done | T4: 5 occurrences (prerequisite); actual consumer code/renderer/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderer/tr_main.c` | done | T4: 113 occurrences (prerequisite), T17: 2; T21: 8 argument casts at 8 calls (native and fallback M_PI); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_main.o, default); G4 advisory difference retained. |
| `code/renderer/tr_marks.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_marks.o, default); G4 PASS. |
| `code/renderer/tr_mesh.c` | done | T4: 4 occurrences (prerequisite), T1: 4, T2: 1, T17: 2; T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_mesh.o, default); G4 advisory difference retained. |
| `code/renderer/tr_model.c` | done | T1: 5; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_model.o, default); G4 advisory difference retained. |
| `code/renderer/tr_model_iqm.c` | done | T1: 4, T2: 2, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_model_iqm.o, default); G4 advisory difference retained. |
| `code/renderer/tr_scene.c` | done | T4: 4 occurrences (prerequisite), T1: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_scene.o, default); G4 advisory difference retained. |
| `code/renderer/tr_shade.c` | done | T4: 1 occurrence (prerequisite), T3: 1, T17: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_shade.o, default); G4 advisory difference retained. |
| `code/renderer/tr_shade_calc.c` | done | T4: 50 occurrences (prerequisite); T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_shade_calc.o, default); G4 advisory difference retained. |
| `code/renderer/tr_shader.c` | done | T1: 5, T3: 16, T16: 1, T17: 9; T21/T22: 5 argument casts at 5 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_shader.o, default); G4 advisory difference retained. |
| `code/renderer/tr_shadows.c` | done | T4: 4 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_shadows.o, default); G4 advisory difference retained. |
| `code/renderer/tr_sky.c` | done | T4: 7 occurrences (prerequisite); T21: 12 argument casts at 12 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_sky.o, default); G4 advisory difference retained. |
| `code/renderer/tr_surface.c` | done | T4: 25 occurrences (prerequisite); T21: 4 argument casts at 4 calls (native and fallback M_PI); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_surface.o, default); G4 advisory difference retained. |
| `code/renderer/tr_vbo.c` | done | T4: 3 occurrences (prerequisite), T1: 7, T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_vbo.o, default); G4 advisory difference retained. |
| `code/renderer/tr_world.c` | done | T4: 2 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_world.o, default); G4 advisory difference retained. |
| `code/renderercommon/tr_font.c` | done | T1: 1; dormant BUILD_FREETYPE body unverified (missing dependency); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_font.o, default); G4 PASS. |
| `code/renderercommon/tr_image_bmp.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image_bmp.o, default); G4 PASS. |
| `code/renderercommon/tr_image_jpg.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image_jpg.o, default); G4 PASS. |
| `code/renderercommon/tr_image_pcx.c` | done | T1: 2; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image_pcx.o, default); G4 PASS. |
| `code/renderercommon/tr_image_png.c` | done | T1: 18; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image_png.o, default); G4 PASS. |
| `code/renderercommon/tr_image_tga.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image_tga.o, default); G4 advisory difference retained. |
| `code/renderercommon/tr_noise.c` | done | T1-T17: 0 (already compatible); T21/T22: 3 argument casts at 3 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_noise.o, default); G4 advisory difference retained. |
| `code/renderercommon/tr_public.h` | done | T5 review removes internal-only annotations; 295/295 C object hashes unchanged; actual consuming objects G2/G3 PASS, G4 advisory evidence retained: rend1/tr_init.o, rendv/tr_init.o. |
| `code/renderercommon/tr_types.h` | done | T1-T17: 0; unchanged header verified through code/renderercommon/tr_font.c, strict native builds/G2/G3 PASS. |
| `code/renderercommon/vulkan/vk_platform.h` | done | T1-T17: 0; unchanged generated Khronos header; C and strict C++20 Xlib/Xrandr header fixture G2/G3 PASS. |
| `code/renderercommon/vulkan/vulkan.h` | done | T1-T17: 0; unchanged generated Khronos header; C and strict C++20 Xlib/Xrandr header fixture G2/G3 PASS. |
| `code/renderercommon/vulkan/vulkan_core.h` | done | T1-T17: 0; unchanged generated Khronos header; C and strict C++20 Xlib/Xrandr header fixture G2/G3 PASS. |
| `code/renderercommon/vulkan/vulkan_win32.h` | done | Unchanged Khronos header; actual MinGW win_qvk.c consumer passes C hash, strict release/debug C++ and G2/G3. |
| `code/renderercommon/vulkan/vulkan_xlib.h` | done | T1-T17: 0; unchanged generated Khronos header; C and strict C++20 Xlib/Xrandr header fixture G2/G3 PASS. |
| `code/renderercommon/vulkan/vulkan_xlib_xrandr.h` | done | T1-T17: 0; unchanged generated Khronos header; C and strict C++20 Xlib/Xrandr header fixture G2/G3 PASS. |
| `code/renderervk/iqm.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderervk/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderervk/shaders/bin2hex.c` | done | Unchanged standalone build utility, not linked engine code; gcc/g++ strict native -O2 compile PASS, fixed 259-byte input and append output byte-identical. Engine layout gate not applicable (no engine records). |
| `code/renderervk/shaders/spirv/shader_data.cpp` | done | T18: 74 preceding extern const declarations; unchanged initialized bytes. Verified through sole consumer rendv/vk.o: C SHA256 unchanged, strict release/debug C++, G2/G3 PASS; G4 advisory diff retained under vk.c. vk.c and this include require each other for C++ gates; consecutive per-file commits record the pair. |
| `code/renderervk/tr_animation.c` | done | T1: 3, T2: 1, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_animation.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_backend.c` | done | T4: 11 occurrences (prerequisite), T1: 4, T3: 3, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_backend.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_bsp.c` | done | T1: 42, T3: 2; T21/T22: 94 argument casts at 94 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_bsp.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_cmds.c` | done | T1: 6; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_cmds.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_common.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderervk/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderervk/tr_curve.c` | done | T1: 3; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_curve.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_image.c` | done | T1: 8, T2: 1, T3: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_image.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_init.c` | done | T1: 5, T5: dlopen definition only; T21: 1 fallback M_PI argument cast; static GetRefAPI C linkage removed; C hash/strict C++/G2/G3 PASS (rendv/tr_init.o); G4 advisory difference retained. |
| `code/renderervk/tr_light.c` | done | T4: 10 occurrences (prerequisite); T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_light.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_local.h` | done | T4: 5 occurrences (prerequisite); actual consumer code/renderervk/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderervk/tr_main.c` | done | T4: 113 occurrences (prerequisite), T17: 2; T21: 8 argument casts at 8 calls (native and fallback M_PI); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_main.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_marks.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_marks.o, default); G4 PASS. |
| `code/renderervk/tr_mesh.c` | done | T4: 4 occurrences (prerequisite), T1: 4, T2: 1, T17: 2; T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_mesh.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_model.c` | done | T1: 5; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_model.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_model_iqm.c` | done | T1: 4, T2: 2, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_model_iqm.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_scene.c` | done | T4: 4 occurrences (prerequisite), T1: 2, T3: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_scene.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_shade.c` | done | T4: 4 occurrences (prerequisite), T2: 1, T17: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_shade.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_shade_calc.c` | done | T4: 50 occurrences (prerequisite); T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_shade_calc.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_shader.c` | done | T1: 5, T3: 17, T16: 1, T17: 9; T21/T22: 5 argument casts at 5 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_shader.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_shadows.c` | done | T4: 4 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_shadows.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_sky.c` | done | T4: 7 occurrences (prerequisite); T21/T22: 12 argument casts at 12 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_sky.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_surface.c` | done | T4: 25 occurrences (prerequisite); T21: 4 argument casts at 4 calls (native and fallback M_PI); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_surface.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_world.c` | done | T4: 2 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_world.o, default); G4 advisory difference retained. |
| `code/renderervk/vk.c` | done | Filename dependency update for embedded shader rename; separate DEVIATION; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/vk.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/vk.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderervk/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderervk/vk_flares.c` | done | T4: 3 occurrences (prerequisite); T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/vk_flares.o, default); G4 advisory difference retained. |
| `code/renderervk/vk_vbo.c` | done | T1: 6; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/vk_vbo.o, default); G4 advisory difference retained. |
| `code/sdl/sdl_gamma.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/sdl_gamma.o, default); G4 PASS. |
| `code/sdl/sdl_glimp.c` | done | T1: 4; T3: 5; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/sdl_glimp.o, default); G4 PASS. |
| `code/sdl/sdl_glw.h` | done | T1-T17: 0; unchanged header verified through sdl_gamma.c; native strict builds and G2/G3 PASS. |
| `code/sdl/sdl_icon.h` | done | Unchanged initializer; sdl_glimp.c consuming object passes original C hash, strict C++ and G2/G3. |
| `code/sdl/sdl_input.c` | done | T19: 1 enum hoisted with original body indentation; T3: 25 keyNum_t casts; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/sdl_input.o, default); G4 advisory difference retained. |
| `code/sdl/sdl_snd.c` | done | T1: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/sdl_snd.o, default); G4 PASS. |
| `code/server/server.h` | done | T1-T17: 0; unchanged header verified through server consumers, strict native release/debug and G2/G3 PASS. |
| `code/server/sv_bot.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_bot.o); G4 advisory difference retained. |
| `code/server/sv_ccmds.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_ccmds.o); G4 advisory difference retained. |
| `code/server/sv_client.c` | done | T1: 2; T2: 4; T20: 1; T25: 1 preserving original C compound line; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_client.o, default); G4 advisory FAIL, full diff retained. |
| `code/server/sv_filter.c` | done | T3: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_filter.o); G4 advisory difference retained. |
| `code/server/sv_game.c` | done | T1: 199, T3: 7; T21/T22: 7 argument casts at 6 calls; T5 internal callback annotations removed; current C hashes/strict builds/G2/G3 PASS (ded/sv_game.o); G4 advisory difference retained. |
| `code/server/sv_init.c` | done | T1: 4; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_init.o); G4 advisory difference retained. |
| `code/server/sv_main.c` | done | T3: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_main.o); G4 advisory difference retained. |
| `code/server/sv_net_chan.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_net_chan.o); G4 advisory difference retained. |
| `code/server/sv_rankings.c` | done | Excluded by accepted scope: never built, proprietary rankings SDK; unchanged and must not be renamed. |
| `code/server/sv_snapshot.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_snapshot.o); G4 advisory difference retained. |
| `code/server/sv_world.c` | done | T3: 2 (includes bitwise assignment result); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_world.o); G4 advisory difference retained. |
| `code/server/tlds.h` | done | Unchanged initializer; T25 unblocks actual dedicated and client sv_client consumers. Both C hashes unchanged; strict C++ and G2/G3 PASS. |
| `code/ui/ui_public.h` | done | T1-T17: 0; unchanged shared ABI header verified through cl_ui.c actual dependency and native strict builds/G2/G3. |
| `code/unix/linux_glimp.c` | done | T1: 2 sites (4 expanded casts), T2: 3, T3: 2, T4: 1 identifier (3 occurrences); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/linux_glimp.o, nosdl); G4 advisory difference retained. |
| `code/unix/linux_joystick.c` | done | T1-T17: 0; dormant USE_JOYSTICK body explicitly compiled; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/linux_joystick.o, nosdl); G4 advisory difference retained. |
| `code/unix/linux_local.h` | done | T1-T17: 0; unchanged header verified through unix_main.c and linux_glimp.c; native strict builds and G2/G3 PASS. |
| `code/unix/linux_qgl.c` | done | T1: 1 macro site (6 expanded casts); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/linux_qgl.o, nosdl); G4 advisory difference retained. |
| `code/unix/linux_qvk.c` | done | T1: 1 function-to-object pointer return cast; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/linux_qvk.o, nosdl); G4 PASS. |
| `code/unix/linux_signals.c` | done | T1-T17: 0; renderer header T4 prerequisite resolves prior blocker; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/linux_signals.o, default); G4 PASS. |
| `code/unix/linux_snd.c` | done | T1: 5; original thread-function casts retained inside typed casts; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/linux_snd.o, nosdl); G4 advisory difference retained. |
| `code/unix/unix_glw.h` | done | T1-T17: 0; unchanged header verified through non-SDL linux_glimp.c, linux_qgl.c and X11 extensions; G2/G3 PASS. |
| `code/unix/unix_main.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/unix_main.o, default); G4 advisory difference retained. |
| `code/unix/unix_shared.c` | done | T1: 1; T23: 1 feature-test guard; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/unix_shared.o, default); G4 advisory difference retained. |
| `code/unix/x11_dga.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/x11_dga.o, nosdl); G4 PASS. |
| `code/unix/x11_randr.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/x11_randr.o, nosdl); G4 PASS. |
| `code/unix/x11_vidmode.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/x11_vidmode.o, nosdl); G4 PASS. |
| `code/win32/glw_win.h` | done | Unchanged; actual MinGW win_glimp.c and win_qgl.c consumers pass C hashes, strict release/debug C++ and G2/G3. |
| `code/win32/resource.h` | done | Unchanged; actual MinGW win_main.c and win_syscon.c consumers pass C hashes, strict release/debug C++ and G2/G3. |
| `code/win32/win_gamma.c` | done | T1: 1 previously inspected HMODULE cast, now verified; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_gamma.o); G4 PASS. |
| `code/win32/win_glimp.c` | done | T1: 2 including optional procedure macro; T2: 4; T5: 2 GPU exports; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_glimp.o); G4 advisory difference retained. |
| `code/win32/win_input.c` | done | T24: 2 GUID arguments; T15: 2 required lexical spaces; prior T2 retained; CINTERFACE predeclared by Windows C++ compiler flags before transitive curl SDK headers; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_input.o); G4 PASS. Explicit USE_WASAPI=0/1: original C hashes, strict release/debug C++, G2/G3 PASS; G4 recorded. |
| `code/win32/win_local.h` | done | T24 exact approved macros before SDK includes; all 300 original MinGW C object hashes unchanged; win_main C++/G2/G3 PASS. |
| `code/win32/win_main.c` | done | T1: 4 total, including FARPROC to void*; existing inspection casts now C-oracle verified; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_main.o); G4 advisory difference retained. |
| `code/win32/win_minimize.c` | done | T1-T23: 0, already compatible; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_minimize.o); G4 advisory difference retained. |
| `code/win32/win_qgl.c` | done | T1: 3 sites including function-to-object pointer conversion; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_qgl.o); G4 PASS. |
| `code/win32/win_qvk.c` | done | T1: 4 sites including function-to-object pointer conversion; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_qvk.o); G4 PASS. |
| `code/win32/win_shared.c` | done | T1-T23: 0; default profile configuration; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_shared.o); G4 advisory difference retained. |
| `code/win32/win_snd.c` | done | T24: 8 outgoing GUID arguments and 2 incoming pointer uses; T4: this identifier to self; T1: 2 Lock casts plus prior loader casts; T18: 2 GUID const declarations; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_snd.o); G4 advisory difference retained. Explicit USE_WASAPI=0/1: original C hashes, strict release/debug C++, G2/G3 PASS; G4 recorded. |
| `code/win32/win_syscon.c` | done | T2: 1 previously inspected boolean toggle, now verified; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_syscon.o); G4 advisory difference retained. |
| `code/win32/win_wndproc.c` | done | T1: 2; T2: 2 previously inspected edits, now verified; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_wndproc.o); G4 advisory difference retained. |

## Every retained current G4 difference

G4 is advisory. These are complete normalized -O2 C/C++ assembly diffs, not accepted evidence of identical behavior. The matching G2/G3 comparisons pass. Open each with `gzip -dc tools/port/evidence/<artifact>`. Reproduce with `python3 tools/port/compile_pair.py <Object> /tmp/port-pair <Make variables>` and `tools/port/codegen_gate.sh` on its .c.s/.cxx.s pair. Linux joystick is an empty default translation unit; both native inventories explicitly enable USE_JOYSTICK for its meaningful body gates. Its unchanged default C object hash is checked separately with the full manifest. Historical superseded diffs remain in git/evidence for audit (notably the original q_math failure); current q_math is G4/G5 PASS.

| Context | Object | Make variables | Full diff artifact under tools/port/evidence |
|---|---|---|---|
| native-tu | `ded/be_aas_bspq3.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_bspq3.diff.gz` |
| native-tu | `ded/be_aas_cluster.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_cluster.diff.gz` |
| native-tu | `ded/be_aas_debug.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_debug.diff.gz` |
| native-tu | `ded/be_aas_file.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_file.diff.gz` |
| native-tu | `ded/be_aas_reach.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_reach.diff.gz` |
| native-tu | `ded/be_aas_route.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_route.diff.gz` |
| native-tu | `ded/be_aas_routealt.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_routealt.diff.gz` |
| native-tu | `ded/be_aas_sample.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_sample.diff.gz` |
| native-tu | `ded/be_ai_char.o` | `defaults` | `current-native-tu-botlib-ded-be_ai_char.diff.gz` |
| native-tu | `ded/be_ai_chat.o` | `defaults` | `current-native-tu-botlib-ded-be_ai_chat.diff.gz` |
| native-tu | `ded/be_ai_move.o` | `defaults` | `current-native-tu-botlib-ded-be_ai_move.diff.gz` |
| native-tu | `ded/l_memory.o` | `defaults` | `current-native-tu-botlib-ded-l_memory.diff.gz` |
| native-tu | `ded/l_precomp.o` | `defaults` | `current-native-tu-botlib-ded-l_precomp.diff.gz` |
| native-tu | `ded/l_script.o` | `defaults` | `current-native-tu-botlib-ded-l_script.diff.gz` |
| native-tu | `ded/l_struct.o` | `defaults` | `current-native-tu-botlib-ded-l_struct.diff.gz` |
| native-tu | `client/cl_avi.o` | `defaults` | `current-native-tu-client-client-cl_avi.diff.gz` |
| native-tu | `client/cl_cgame.o` | `defaults` | `current-native-tu-client-client-cl_cgame.diff.gz` |
| native-tu | `client/cl_cin.o` | `defaults` | `current-native-tu-client-client-cl_cin.diff.gz` |
| native-tu | `client/cl_console.o` | `defaults` | `current-native-tu-client-client-cl_console.diff.gz` |
| native-tu | `client/cl_curl.o` | `defaults` | `current-native-tu-client-client-cl_curl.diff.gz` |
| native-tu | `client/cl_input.o` | `defaults` | `current-native-tu-client-client-cl_input.diff.gz` |
| native-tu | `client/cl_jpeg.o` | `defaults` | `current-native-tu-client-client-cl_jpeg.diff.gz` |
| native-tu | `client/cl_keys.o` | `defaults` | `current-native-tu-client-client-cl_keys.diff.gz` |
| native-tu | `client/cl_main.o` | `defaults` | `current-native-tu-client-client-cl_main.diff.gz` |
| native-tu | `client/cl_net_chan.o` | `defaults` | `current-native-tu-client-client-cl_net_chan.diff.gz` |
| native-tu | `client/cl_parse.o` | `defaults` | `current-native-tu-client-client-cl_parse.diff.gz` |
| native-tu | `client/cl_scrn.o` | `defaults` | `current-native-tu-client-client-cl_scrn.diff.gz` |
| native-tu | `client/cl_ui.o` | `defaults` | `current-native-tu-client-client-cl_ui.diff.gz` |
| native-tu | `client/snd_dma.o` | `defaults` | `current-native-tu-client-client-snd_dma.diff.gz` |
| native-tu | `client/snd_mem.o` | `defaults` | `current-native-tu-client-client-snd_mem.diff.gz` |
| native-tu | `client/snd_mix.o` | `defaults` | `current-native-tu-client-client-snd_mix.diff.gz` |
| native-tu | `ded/cm_load.o` | `defaults` | `current-native-tu-qcommon-ded-cm_load.diff.gz` |
| native-tu | `ded/cm_patch.o` | `defaults` | `current-native-tu-qcommon-ded-cm_patch.diff.gz` |
| native-tu | `ded/cm_trace.o` | `defaults` | `current-native-tu-qcommon-ded-cm_trace.diff.gz` |
| native-tu | `ded/cmd.o` | `defaults` | `current-native-tu-qcommon-ded-cmd.diff.gz` |
| native-tu | `ded/common.o` | `defaults` | `current-native-tu-qcommon-ded-common.diff.gz` |
| native-tu | `ded/cvar.o` | `defaults` | `current-native-tu-qcommon-ded-cvar.diff.gz` |
| native-tu | `ded/files.o` | `defaults` | `current-native-tu-qcommon-ded-files.diff.gz` |
| native-tu | `ded/history.o` | `defaults` | `current-native-tu-qcommon-ded-history.diff.gz` |
| native-tu | `ded/huffman.o` | `defaults` | `current-native-tu-qcommon-ded-huffman.diff.gz` |
| native-tu | `ded/keys.o` | `defaults` | `current-native-tu-qcommon-ded-keys.diff.gz` |
| native-tu | `ded/md5.o` | `defaults` | `current-native-tu-qcommon-ded-md5.diff.gz` |
| native-tu | `ded/msg.o` | `defaults` | `current-native-tu-qcommon-ded-msg.diff.gz` |
| native-tu | `ded/net_chan.o` | `defaults` | `current-native-tu-qcommon-ded-net_chan.diff.gz` |
| native-tu | `ded/net_ip.o` | `defaults` | `current-native-tu-qcommon-ded-net_ip.diff.gz` |
| native-tu | `client/puff.o` | `defaults` | `current-native-tu-qcommon-client-puff.diff.gz` |
| native-tu | `ded/q_shared.o` | `defaults` | `current-native-tu-qcommon-ded-q_shared.diff.gz` |
| native-tu | `ded/unzip.o` | `defaults` | `current-native-tu-qcommon-ded-unzip.diff.gz` |
| native-tu | `ded/qvm/vm.o` | `defaults` | `current-native-tu-qcommon-ded-qvm-vm.diff.gz` |
| native-tu | `ded/qvm/vm_interpreted.o` | `defaults` | `current-native-tu-qcommon-ded-qvm-vm_interpreted.diff.gz` |
| native-tu | `ded/qvm/vm_x86.o` | `defaults` | `current-native-tu-qcommon-ded-qvm-vm_x86.diff.gz` |
| native-tu | `rend1/tr_animation.o` | `defaults` | `current-native-tu-renderer-rend1-tr_animation.diff.gz` |
| native-tu | `rend1/tr_arb.o` | `defaults` | `current-native-tu-renderer-rend1-tr_arb.diff.gz` |
| native-tu | `rend1/tr_backend.o` | `defaults` | `current-native-tu-renderer-rend1-tr_backend.diff.gz` |
| native-tu | `rend1/tr_bsp.o` | `defaults` | `current-native-tu-renderer-rend1-tr_bsp.diff.gz` |
| native-tu | `rend1/tr_cmds.o` | `defaults` | `current-native-tu-renderer-rend1-tr_cmds.diff.gz` |
| native-tu | `rend1/tr_curve.o` | `defaults` | `current-native-tu-renderer-rend1-tr_curve.diff.gz` |
| native-tu | `rend1/tr_flares.o` | `defaults` | `current-native-tu-renderer-rend1-tr_flares.diff.gz` |
| native-tu | `rend1/tr_image.o` | `defaults` | `current-native-tu-renderer-rend1-tr_image.diff.gz` |
| native-tu | `rend1/tr_init.o` | `defaults` | `current-native-tu-renderer-rend1-tr_init.diff.gz` |
| native-tu | `rend1/tr_light.o` | `defaults` | `current-native-tu-renderer-rend1-tr_light.diff.gz` |
| native-tu | `rend1/tr_main.o` | `defaults` | `current-native-tu-renderer-rend1-tr_main.diff.gz` |
| native-tu | `rend1/tr_mesh.o` | `defaults` | `current-native-tu-renderer-rend1-tr_mesh.diff.gz` |
| native-tu | `rend1/tr_model.o` | `defaults` | `current-native-tu-renderer-rend1-tr_model.diff.gz` |
| native-tu | `rend1/tr_model_iqm.o` | `defaults` | `current-native-tu-renderer-rend1-tr_model_iqm.diff.gz` |
| native-tu | `rend1/tr_scene.o` | `defaults` | `current-native-tu-renderer-rend1-tr_scene.diff.gz` |
| native-tu | `rend1/tr_shade.o` | `defaults` | `current-native-tu-renderer-rend1-tr_shade.diff.gz` |
| native-tu | `rend1/tr_shade_calc.o` | `defaults` | `current-native-tu-renderer-rend1-tr_shade_calc.diff.gz` |
| native-tu | `rend1/tr_shader.o` | `defaults` | `current-native-tu-renderer-rend1-tr_shader.diff.gz` |
| native-tu | `rend1/tr_shadows.o` | `defaults` | `current-native-tu-renderer-rend1-tr_shadows.diff.gz` |
| native-tu | `rend1/tr_sky.o` | `defaults` | `current-native-tu-renderer-rend1-tr_sky.diff.gz` |
| native-tu | `rend1/tr_surface.o` | `defaults` | `current-native-tu-renderer-rend1-tr_surface.diff.gz` |
| native-tu | `rend1/tr_vbo.o` | `defaults` | `current-native-tu-renderer-rend1-tr_vbo.diff.gz` |
| native-tu | `rend1/tr_world.o` | `defaults` | `current-native-tu-renderer-rend1-tr_world.diff.gz` |
| native-tu | `rend1/tr_image_tga.o` | `defaults` | `current-native-tu-renderercommon-rend1-tr_image_tga.diff.gz` |
| native-tu | `rend1/tr_noise.o` | `defaults` | `current-native-tu-renderercommon-rend1-tr_noise.diff.gz` |
| native-tu | `rendv/tr_animation.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_animation.diff.gz` |
| native-tu | `rendv/tr_backend.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_backend.diff.gz` |
| native-tu | `rendv/tr_bsp.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_bsp.diff.gz` |
| native-tu | `rendv/tr_cmds.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_cmds.diff.gz` |
| native-tu | `rendv/tr_curve.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_curve.diff.gz` |
| native-tu | `rendv/tr_image.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_image.diff.gz` |
| native-tu | `rendv/tr_init.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_init.diff.gz` |
| native-tu | `rendv/tr_light.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_light.diff.gz` |
| native-tu | `rendv/tr_main.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_main.diff.gz` |
| native-tu | `rendv/tr_mesh.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_mesh.diff.gz` |
| native-tu | `rendv/tr_model.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_model.diff.gz` |
| native-tu | `rendv/tr_model_iqm.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_model_iqm.diff.gz` |
| native-tu | `rendv/tr_scene.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_scene.diff.gz` |
| native-tu | `rendv/tr_shade.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_shade.diff.gz` |
| native-tu | `rendv/tr_shade_calc.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_shade_calc.diff.gz` |
| native-tu | `rendv/tr_shader.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_shader.diff.gz` |
| native-tu | `rendv/tr_shadows.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_shadows.diff.gz` |
| native-tu | `rendv/tr_sky.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_sky.diff.gz` |
| native-tu | `rendv/tr_surface.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_surface.diff.gz` |
| native-tu | `rendv/tr_world.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_world.diff.gz` |
| native-tu | `rendv/vk.o` | `defaults` | `current-native-tu-renderervk-rendv-vk.diff.gz` |
| native-tu | `rendv/vk_flares.o` | `defaults` | `current-native-tu-renderervk-rendv-vk_flares.diff.gz` |
| native-tu | `rendv/vk_vbo.o` | `defaults` | `current-native-tu-renderervk-rendv-vk_vbo.diff.gz` |
| native-tu | `client/sdl_input.o` | `defaults` | `current-native-tu-sdl-client-sdl_input.diff.gz` |
| native-tu | `ded/sv_bot.o` | `defaults` | `current-native-tu-server-ded-sv_bot.diff.gz` |
| native-tu | `ded/sv_ccmds.o` | `defaults` | `current-native-tu-server-ded-sv_ccmds.diff.gz` |
| native-tu | `ded/sv_filter.o` | `defaults` | `current-native-tu-server-ded-sv_filter.diff.gz` |
| native-tu | `ded/sv_game.o` | `defaults` | `current-native-tu-server-ded-sv_game.diff.gz` |
| native-tu | `ded/sv_init.o` | `defaults` | `current-native-tu-server-ded-sv_init.diff.gz` |
| native-tu | `ded/sv_main.o` | `defaults` | `current-native-tu-server-ded-sv_main.diff.gz` |
| native-tu | `ded/sv_net_chan.o` | `defaults` | `current-native-tu-server-ded-sv_net_chan.diff.gz` |
| native-tu | `ded/sv_snapshot.o` | `defaults` | `current-native-tu-server-ded-sv_snapshot.diff.gz` |
| native-tu | `ded/sv_world.o` | `defaults` | `current-native-tu-server-ded-sv_world.diff.gz` |
| native-tu | `client/linux_glimp.o` | `USE_SDL=0` | `current-native-tu-unix-client-linux_glimp.diff.gz` |
| native-tu | `client/linux_joystick.o` | `USE_SDL=0 CFLAGS=-DUSE_JOYSTICK` | `current-native-tu-unix-client-linux_joystick.diff.gz` |
| native-tu | `client/linux_qgl.o` | `USE_SDL=0` | `current-native-tu-unix-client-linux_qgl.diff.gz` |
| native-tu | `client/linux_snd.o` | `USE_SDL=0` | `current-native-tu-unix-client-linux_snd.diff.gz` |
| native-tu | `ded/unix_main.o` | `defaults` | `current-native-tu-unix-ded-unix_main.diff.gz` |
| native-tu | `ded/unix_shared.o` | `defaults` | `current-native-tu-unix-ded-unix_shared.diff.gz` |
| mingw64 | `client/be_aas_bspq3.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_bspq3.diff.gz` |
| mingw64 | `client/be_aas_cluster.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_cluster.diff.gz` |
| mingw64 | `client/be_aas_debug.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_debug.diff.gz` |
| mingw64 | `client/be_aas_entity.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_entity.diff.gz` |
| mingw64 | `client/be_aas_file.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_file.diff.gz` |
| mingw64 | `client/be_aas_move.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_move.diff.gz` |
| mingw64 | `client/be_aas_optimize.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_optimize.diff.gz` |
| mingw64 | `client/be_aas_reach.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_reach.diff.gz` |
| mingw64 | `client/be_aas_route.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_route.diff.gz` |
| mingw64 | `client/be_aas_sample.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_sample.diff.gz` |
| mingw64 | `client/be_ai_char.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_ai_char.diff.gz` |
| mingw64 | `client/be_ai_chat.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_ai_chat.diff.gz` |
| mingw64 | `client/be_ai_goal.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_ai_goal.diff.gz` |
| mingw64 | `client/be_ai_move.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_ai_move.diff.gz` |
| mingw64 | `client/cl_avi.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_avi.diff.gz` |
| mingw64 | `client/cl_cgame.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_cgame.diff.gz` |
| mingw64 | `client/cl_console.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_console.diff.gz` |
| mingw64 | `client/cl_curl.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_curl.diff.gz` |
| mingw64 | `client/cl_input.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_input.diff.gz` |
| mingw64 | `client/cl_jpeg.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_jpeg.diff.gz` |
| mingw64 | `client/cl_keys.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_keys.diff.gz` |
| mingw64 | `client/cl_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_main.diff.gz` |
| mingw64 | `client/cl_net_chan.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_net_chan.diff.gz` |
| mingw64 | `client/cl_parse.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_parse.diff.gz` |
| mingw64 | `client/cl_scrn.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_scrn.diff.gz` |
| mingw64 | `client/cl_ui.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_ui.diff.gz` |
| mingw64 | `client/cm_load.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cm_load.diff.gz` |
| mingw64 | `client/cm_patch.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cm_patch.diff.gz` |
| mingw64 | `client/cm_test.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cm_test.diff.gz` |
| mingw64 | `client/cm_trace.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cm_trace.diff.gz` |
| mingw64 | `client/cmd.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cmd.diff.gz` |
| mingw64 | `client/common.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-common.diff.gz` |
| mingw64 | `client/cvar.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cvar.diff.gz` |
| mingw64 | `client/files.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-files.diff.gz` |
| mingw64 | `client/history.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-history.diff.gz` |
| mingw64 | `client/huffman.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-huffman.diff.gz` |
| mingw64 | `client/keys.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-keys.diff.gz` |
| mingw64 | `client/l_memory.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-l_memory.diff.gz` |
| mingw64 | `client/l_precomp.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-l_precomp.diff.gz` |
| mingw64 | `client/l_script.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-l_script.diff.gz` |
| mingw64 | `client/l_struct.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-l_struct.diff.gz` |
| mingw64 | `client/md5.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-md5.diff.gz` |
| mingw64 | `client/msg.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-msg.diff.gz` |
| mingw64 | `client/net_chan.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-net_chan.diff.gz` |
| mingw64 | `client/net_ip.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-net_ip.diff.gz` |
| mingw64 | `client/puff.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-puff.diff.gz` |
| mingw64 | `client/q_math.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-q_math.diff.gz` |
| mingw64 | `client/q_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-q_shared.diff.gz` |
| mingw64 | `client/qvm/vm.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-qvm-vm.diff.gz` |
| mingw64 | `client/qvm/vm_interpreted.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-qvm-vm_interpreted.diff.gz` |
| mingw64 | `client/qvm/vm_x86.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-qvm-vm_x86.diff.gz` |
| mingw64 | `client/snd_dma.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-snd_dma.diff.gz` |
| mingw64 | `client/snd_mem.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-snd_mem.diff.gz` |
| mingw64 | `client/snd_mix.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-snd_mix.diff.gz` |
| mingw64 | `client/sv_bot.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_bot.diff.gz` |
| mingw64 | `client/sv_ccmds.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_ccmds.diff.gz` |
| mingw64 | `client/sv_filter.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_filter.diff.gz` |
| mingw64 | `client/sv_game.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_game.diff.gz` |
| mingw64 | `client/sv_init.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_init.diff.gz` |
| mingw64 | `client/sv_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_main.diff.gz` |
| mingw64 | `client/sv_net_chan.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_net_chan.diff.gz` |
| mingw64 | `client/sv_snapshot.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_snapshot.diff.gz` |
| mingw64 | `client/sv_world.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_world.diff.gz` |
| mingw64 | `client/unzip.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-unzip.diff.gz` |
| mingw64 | `client/win_glimp.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-win_glimp.diff.gz` |
| mingw64 | `client/win_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-win_main.diff.gz` |
| mingw64 | `client/win_minimize.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-win_minimize.diff.gz` |
| mingw64 | `client/win_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-win_shared.diff.gz` |
| mingw64 | `client/win_syscon.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-win_syscon.diff.gz` |
| mingw64 | `client/win_wndproc.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-win_wndproc.diff.gz` |
| mingw64 | `ded/be_aas_bspq3.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_bspq3.diff.gz` |
| mingw64 | `ded/be_aas_cluster.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_cluster.diff.gz` |
| mingw64 | `ded/be_aas_debug.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_debug.diff.gz` |
| mingw64 | `ded/be_aas_entity.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_entity.diff.gz` |
| mingw64 | `ded/be_aas_file.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_file.diff.gz` |
| mingw64 | `ded/be_aas_move.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_move.diff.gz` |
| mingw64 | `ded/be_aas_optimize.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_optimize.diff.gz` |
| mingw64 | `ded/be_aas_reach.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_reach.diff.gz` |
| mingw64 | `ded/be_aas_route.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_route.diff.gz` |
| mingw64 | `ded/be_aas_sample.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_sample.diff.gz` |
| mingw64 | `ded/be_ai_char.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_ai_char.diff.gz` |
| mingw64 | `ded/be_ai_chat.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_ai_chat.diff.gz` |
| mingw64 | `ded/be_ai_goal.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_ai_goal.diff.gz` |
| mingw64 | `ded/be_ai_move.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_ai_move.diff.gz` |
| mingw64 | `ded/cm_load.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-cm_load.diff.gz` |
| mingw64 | `ded/cm_patch.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-cm_patch.diff.gz` |
| mingw64 | `ded/cm_test.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-cm_test.diff.gz` |
| mingw64 | `ded/cm_trace.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-cm_trace.diff.gz` |
| mingw64 | `ded/cmd.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-cmd.diff.gz` |
| mingw64 | `ded/common.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-common.diff.gz` |
| mingw64 | `ded/cvar.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-cvar.diff.gz` |
| mingw64 | `ded/files.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-files.diff.gz` |
| mingw64 | `ded/history.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-history.diff.gz` |
| mingw64 | `ded/huffman.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-huffman.diff.gz` |
| mingw64 | `ded/keys.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-keys.diff.gz` |
| mingw64 | `ded/l_memory.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-l_memory.diff.gz` |
| mingw64 | `ded/l_precomp.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-l_precomp.diff.gz` |
| mingw64 | `ded/l_script.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-l_script.diff.gz` |
| mingw64 | `ded/l_struct.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-l_struct.diff.gz` |
| mingw64 | `ded/md5.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-md5.diff.gz` |
| mingw64 | `ded/msg.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-msg.diff.gz` |
| mingw64 | `ded/net_chan.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-net_chan.diff.gz` |
| mingw64 | `ded/net_ip.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-net_ip.diff.gz` |
| mingw64 | `ded/q_math.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-q_math.diff.gz` |
| mingw64 | `ded/q_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-q_shared.diff.gz` |
| mingw64 | `ded/qvm/vm.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-qvm-vm.diff.gz` |
| mingw64 | `ded/qvm/vm_interpreted.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-qvm-vm_interpreted.diff.gz` |
| mingw64 | `ded/qvm/vm_x86.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-qvm-vm_x86.diff.gz` |
| mingw64 | `ded/sv_bot.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_bot.diff.gz` |
| mingw64 | `ded/sv_ccmds.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_ccmds.diff.gz` |
| mingw64 | `ded/sv_filter.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_filter.diff.gz` |
| mingw64 | `ded/sv_game.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_game.diff.gz` |
| mingw64 | `ded/sv_init.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_init.diff.gz` |
| mingw64 | `ded/sv_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_main.diff.gz` |
| mingw64 | `ded/sv_net_chan.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_net_chan.diff.gz` |
| mingw64 | `ded/sv_snapshot.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_snapshot.diff.gz` |
| mingw64 | `ded/sv_world.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_world.diff.gz` |
| mingw64 | `ded/unzip.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-unzip.diff.gz` |
| mingw64 | `ded/win_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-win_main.diff.gz` |
| mingw64 | `ded/win_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-win_shared.diff.gz` |
| mingw64 | `ded/win_syscon.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-win_syscon.diff.gz` |
| mingw64 | `rend1/puff.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-puff.diff.gz` |
| mingw64 | `rend1/q_math.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-q_math.diff.gz` |
| mingw64 | `rend1/q_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-q_shared.diff.gz` |
| mingw64 | `rend1/tr_animation.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_animation.diff.gz` |
| mingw64 | `rend1/tr_arb.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_arb.diff.gz` |
| mingw64 | `rend1/tr_backend.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_backend.diff.gz` |
| mingw64 | `rend1/tr_bsp.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_bsp.diff.gz` |
| mingw64 | `rend1/tr_cmds.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_cmds.diff.gz` |
| mingw64 | `rend1/tr_curve.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_curve.diff.gz` |
| mingw64 | `rend1/tr_flares.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_flares.diff.gz` |
| mingw64 | `rend1/tr_image.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_image.diff.gz` |
| mingw64 | `rend1/tr_image_tga.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_image_tga.diff.gz` |
| mingw64 | `rend1/tr_init.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_init.diff.gz` |
| mingw64 | `rend1/tr_light.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_light.diff.gz` |
| mingw64 | `rend1/tr_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_main.diff.gz` |
| mingw64 | `rend1/tr_marks.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_marks.diff.gz` |
| mingw64 | `rend1/tr_mesh.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_mesh.diff.gz` |
| mingw64 | `rend1/tr_model.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_model.diff.gz` |
| mingw64 | `rend1/tr_model_iqm.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_model_iqm.diff.gz` |
| mingw64 | `rend1/tr_scene.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_scene.diff.gz` |
| mingw64 | `rend1/tr_shade.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_shade.diff.gz` |
| mingw64 | `rend1/tr_shade_calc.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_shade_calc.diff.gz` |
| mingw64 | `rend1/tr_shader.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_shader.diff.gz` |
| mingw64 | `rend1/tr_shadows.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_shadows.diff.gz` |
| mingw64 | `rend1/tr_sky.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_sky.diff.gz` |
| mingw64 | `rend1/tr_surface.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_surface.diff.gz` |
| mingw64 | `rend1/tr_vbo.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_vbo.diff.gz` |
| mingw64 | `rend1/tr_world.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_world.diff.gz` |
| mingw64 | `rendv/puff.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-puff.diff.gz` |
| mingw64 | `rendv/q_math.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-q_math.diff.gz` |
| mingw64 | `rendv/q_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-q_shared.diff.gz` |
| mingw64 | `rendv/tr_animation.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_animation.diff.gz` |
| mingw64 | `rendv/tr_backend.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_backend.diff.gz` |
| mingw64 | `rendv/tr_bsp.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_bsp.diff.gz` |
| mingw64 | `rendv/tr_cmds.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_cmds.diff.gz` |
| mingw64 | `rendv/tr_curve.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_curve.diff.gz` |
| mingw64 | `rendv/tr_image.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_image.diff.gz` |
| mingw64 | `rendv/tr_image_tga.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_image_tga.diff.gz` |
| mingw64 | `rendv/tr_init.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_init.diff.gz` |
| mingw64 | `rendv/tr_light.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_light.diff.gz` |
| mingw64 | `rendv/tr_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_main.diff.gz` |
| mingw64 | `rendv/tr_marks.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_marks.diff.gz` |
| mingw64 | `rendv/tr_mesh.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_mesh.diff.gz` |
| mingw64 | `rendv/tr_model.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_model.diff.gz` |
| mingw64 | `rendv/tr_model_iqm.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_model_iqm.diff.gz` |
| mingw64 | `rendv/tr_scene.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_scene.diff.gz` |
| mingw64 | `rendv/tr_shade.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_shade.diff.gz` |
| mingw64 | `rendv/tr_shade_calc.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_shade_calc.diff.gz` |
| mingw64 | `rendv/tr_shader.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_shader.diff.gz` |
| mingw64 | `rendv/tr_shadows.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_shadows.diff.gz` |
| mingw64 | `rendv/tr_sky.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_sky.diff.gz` |
| mingw64 | `rendv/tr_surface.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_surface.diff.gz` |
| mingw64 | `rendv/tr_world.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_world.diff.gz` |
| mingw64 | `rendv/vk.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-vk.diff.gz` |
| mingw64 | `rendv/vk_flares.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-vk_flares.diff.gz` |
| mingw64 | `rendv/vk_vbo.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-vk_vbo.diff.gz` |
| aarch64 | `ded/be_aas_bspq3.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_bspq3.diff.gz` |
| aarch64 | `ded/be_aas_cluster.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_cluster.diff.gz` |
| aarch64 | `ded/be_aas_debug.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_debug.diff.gz` |
| aarch64 | `ded/be_aas_file.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_file.diff.gz` |
| aarch64 | `ded/be_aas_reach.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_reach.diff.gz` |
| aarch64 | `ded/be_aas_route.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_route.diff.gz` |
| aarch64 | `ded/be_aas_routealt.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_routealt.diff.gz` |
| aarch64 | `ded/be_aas_sample.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_sample.diff.gz` |
| aarch64 | `ded/be_ai_char.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_ai_char.diff.gz` |
| aarch64 | `ded/be_ai_chat.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_ai_chat.diff.gz` |
| aarch64 | `ded/be_ai_move.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_ai_move.diff.gz` |
| aarch64 | `ded/cm_load.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-cm_load.diff.gz` |
| aarch64 | `ded/cm_patch.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-cm_patch.diff.gz` |
| aarch64 | `ded/cm_trace.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-cm_trace.diff.gz` |
| aarch64 | `ded/cmd.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-cmd.diff.gz` |
| aarch64 | `ded/common.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-common.diff.gz` |
| aarch64 | `ded/cvar.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-cvar.diff.gz` |
| aarch64 | `ded/files.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-files.diff.gz` |
| aarch64 | `ded/history.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-history.diff.gz` |
| aarch64 | `ded/huffman.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-huffman.diff.gz` |
| aarch64 | `ded/keys.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-keys.diff.gz` |
| aarch64 | `ded/l_memory.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-l_memory.diff.gz` |
| aarch64 | `ded/l_precomp.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-l_precomp.diff.gz` |
| aarch64 | `ded/l_script.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-l_script.diff.gz` |
| aarch64 | `ded/l_struct.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-l_struct.diff.gz` |
| aarch64 | `ded/md5.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-md5.diff.gz` |
| aarch64 | `ded/msg.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-msg.diff.gz` |
| aarch64 | `ded/net_chan.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-net_chan.diff.gz` |
| aarch64 | `ded/net_ip.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-net_ip.diff.gz` |
| aarch64 | `ded/q_shared.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-q_shared.diff.gz` |
| aarch64 | `ded/qvm/vm.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-qvm-vm.diff.gz` |
| aarch64 | `ded/qvm/vm_aarch64.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-qvm-vm_aarch64.diff.gz` |
| aarch64 | `ded/qvm/vm_interpreted.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-qvm-vm_interpreted.diff.gz` |
| aarch64 | `ded/sv_bot.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_bot.diff.gz` |
| aarch64 | `ded/sv_ccmds.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_ccmds.diff.gz` |
| aarch64 | `ded/sv_filter.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_filter.diff.gz` |
| aarch64 | `ded/sv_game.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_game.diff.gz` |
| aarch64 | `ded/sv_init.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_init.diff.gz` |
| aarch64 | `ded/sv_main.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_main.diff.gz` |
| aarch64 | `ded/sv_net_chan.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_net_chan.diff.gz` |
| aarch64 | `ded/sv_snapshot.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_snapshot.diff.gz` |
| aarch64 | `ded/sv_world.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_world.diff.gz` |
| aarch64 | `ded/unix_main.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-unix_main.diff.gz` |
| aarch64 | `ded/unix_shared.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-unix_shared.diff.gz` |
| aarch64 | `ded/unzip.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-unzip.diff.gz` |
| arm | `ded/be_aas_bspq3.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_bspq3.diff.gz` |
| arm | `ded/be_aas_cluster.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_cluster.diff.gz` |
| arm | `ded/be_aas_debug.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_debug.diff.gz` |
| arm | `ded/be_aas_file.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_file.diff.gz` |
| arm | `ded/be_aas_reach.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_reach.diff.gz` |
| arm | `ded/be_aas_route.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_route.diff.gz` |
| arm | `ded/be_aas_routealt.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_routealt.diff.gz` |
| arm | `ded/be_aas_sample.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_sample.diff.gz` |
| arm | `ded/be_ai_char.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_ai_char.diff.gz` |
| arm | `ded/be_ai_chat.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_ai_chat.diff.gz` |
| arm | `ded/be_ai_move.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_ai_move.diff.gz` |
| arm | `ded/cm_load.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-cm_load.diff.gz` |
| arm | `ded/cm_patch.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-cm_patch.diff.gz` |
| arm | `ded/cm_trace.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-cm_trace.diff.gz` |
| arm | `ded/cmd.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-cmd.diff.gz` |
| arm | `ded/common.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-common.diff.gz` |
| arm | `ded/cvar.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-cvar.diff.gz` |
| arm | `ded/files.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-files.diff.gz` |
| arm | `ded/history.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-history.diff.gz` |
| arm | `ded/huffman.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-huffman.diff.gz` |
| arm | `ded/keys.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-keys.diff.gz` |
| arm | `ded/l_memory.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-l_memory.diff.gz` |
| arm | `ded/l_precomp.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-l_precomp.diff.gz` |
| arm | `ded/l_script.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-l_script.diff.gz` |
| arm | `ded/l_struct.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-l_struct.diff.gz` |
| arm | `ded/md5.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-md5.diff.gz` |
| arm | `ded/msg.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-msg.diff.gz` |
| arm | `ded/net_chan.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-net_chan.diff.gz` |
| arm | `ded/net_ip.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-net_ip.diff.gz` |
| arm | `ded/q_math.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-q_math.diff.gz` |
| arm | `ded/q_shared.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-q_shared.diff.gz` |
| arm | `ded/qvm/vm.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-qvm-vm.diff.gz` |
| arm | `ded/qvm/vm_armv7l.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-qvm-vm_armv7l.diff.gz` |
| arm | `ded/qvm/vm_interpreted.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-qvm-vm_interpreted.diff.gz` |
| arm | `ded/sv_bot.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_bot.diff.gz` |
| arm | `ded/sv_ccmds.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_ccmds.diff.gz` |
| arm | `ded/sv_filter.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_filter.diff.gz` |
| arm | `ded/sv_game.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_game.diff.gz` |
| arm | `ded/sv_init.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_init.diff.gz` |
| arm | `ded/sv_main.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_main.diff.gz` |
| arm | `ded/sv_net_chan.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_net_chan.diff.gz` |
| arm | `ded/sv_snapshot.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_snapshot.diff.gz` |
| arm | `ded/sv_world.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_world.diff.gz` |
| arm | `ded/unix_main.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-unix_main.diff.gz` |
| arm | `ded/unix_shared.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-unix_shared.diff.gz` |
| arm | `ded/unzip.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-unzip.diff.gz` |
| ppc64le | `ded/be_aas_bspq3.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_bspq3.diff.gz` |
| ppc64le | `ded/be_aas_cluster.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_cluster.diff.gz` |
| ppc64le | `ded/be_aas_debug.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_debug.diff.gz` |
| ppc64le | `ded/be_aas_entity.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_entity.diff.gz` |
| ppc64le | `ded/be_aas_file.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_file.diff.gz` |
| ppc64le | `ded/be_aas_main.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_main.diff.gz` |
| ppc64le | `ded/be_aas_move.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_move.diff.gz` |
| ppc64le | `ded/be_aas_optimize.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_optimize.diff.gz` |
| ppc64le | `ded/be_aas_reach.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_reach.diff.gz` |
| ppc64le | `ded/be_aas_route.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_route.diff.gz` |
| ppc64le | `ded/be_aas_routealt.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_routealt.diff.gz` |
| ppc64le | `ded/be_aas_sample.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_sample.diff.gz` |
| ppc64le | `ded/be_ai_char.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ai_char.diff.gz` |
| ppc64le | `ded/be_ai_chat.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ai_chat.diff.gz` |
| ppc64le | `ded/be_ai_gen.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ai_gen.diff.gz` |
| ppc64le | `ded/be_ai_goal.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ai_goal.diff.gz` |
| ppc64le | `ded/be_ai_move.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ai_move.diff.gz` |
| ppc64le | `ded/be_ai_weap.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ai_weap.diff.gz` |
| ppc64le | `ded/be_ai_weight.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ai_weight.diff.gz` |
| ppc64le | `ded/be_ea.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ea.diff.gz` |
| ppc64le | `ded/be_interface.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_interface.diff.gz` |
| ppc64le | `ded/cm_load.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-cm_load.diff.gz` |
| ppc64le | `ded/cm_patch.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-cm_patch.diff.gz` |
| ppc64le | `ded/cm_polylib.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-cm_polylib.diff.gz` |
| ppc64le | `ded/cm_test.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-cm_test.diff.gz` |
| ppc64le | `ded/cm_trace.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-cm_trace.diff.gz` |
| ppc64le | `ded/cmd.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-cmd.diff.gz` |
| ppc64le | `ded/common.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-common.diff.gz` |
| ppc64le | `ded/cvar.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-cvar.diff.gz` |
| ppc64le | `ded/files.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-files.diff.gz` |
| ppc64le | `ded/history.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-history.diff.gz` |
| ppc64le | `ded/huffman.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-huffman.diff.gz` |
| ppc64le | `ded/huffman_static.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-huffman_static.diff.gz` |
| ppc64le | `ded/keys.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-keys.diff.gz` |
| ppc64le | `ded/l_crc.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-l_crc.diff.gz` |
| ppc64le | `ded/l_libvar.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-l_libvar.diff.gz` |
| ppc64le | `ded/l_log.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-l_log.diff.gz` |
| ppc64le | `ded/l_memory.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-l_memory.diff.gz` |
| ppc64le | `ded/l_precomp.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-l_precomp.diff.gz` |
| ppc64le | `ded/l_script.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-l_script.diff.gz` |
| ppc64le | `ded/l_struct.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-l_struct.diff.gz` |
| ppc64le | `ded/linux_signals.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-linux_signals.diff.gz` |
| ppc64le | `ded/md4.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-md4.diff.gz` |
| ppc64le | `ded/md5.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-md5.diff.gz` |
| ppc64le | `ded/msg.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-msg.diff.gz` |
| ppc64le | `ded/net_chan.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-net_chan.diff.gz` |
| ppc64le | `ded/net_ip.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-net_ip.diff.gz` |
| ppc64le | `ded/q_math.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-q_math.diff.gz` |
| ppc64le | `ded/q_shared.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-q_shared.diff.gz` |
| ppc64le | `ded/qvm/vm.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-qvm-vm.diff.gz` |
| ppc64le | `ded/qvm/vm_interpreted.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-qvm-vm_interpreted.diff.gz` |
| ppc64le | `ded/qvm/vm_powerpc.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-qvm-vm_powerpc.diff.gz` |
| ppc64le | `ded/sv_bot.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_bot.diff.gz` |
| ppc64le | `ded/sv_ccmds.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_ccmds.diff.gz` |
| ppc64le | `ded/sv_filter.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_filter.diff.gz` |
| ppc64le | `ded/sv_game.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_game.diff.gz` |
| ppc64le | `ded/sv_init.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_init.diff.gz` |
| ppc64le | `ded/sv_main.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_main.diff.gz` |
| ppc64le | `ded/sv_net_chan.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_net_chan.diff.gz` |
| ppc64le | `ded/sv_snapshot.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_snapshot.diff.gz` |
| ppc64le | `ded/sv_world.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_world.diff.gz` |
| ppc64le | `ded/unix_main.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-unix_main.diff.gz` |
| ppc64le | `ded/unix_shared.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-unix_shared.diff.gz` |
| ppc64le | `ded/unzip.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-unzip.diff.gz` |
| native | `client/be_aas_bspq3.o` | `defaults` | `current-native-context-native-client-be_aas_bspq3.diff.gz` |
| native | `client/be_aas_cluster.o` | `defaults` | `current-native-context-native-client-be_aas_cluster.diff.gz` |
| native | `client/be_aas_debug.o` | `defaults` | `current-native-context-native-client-be_aas_debug.diff.gz` |
| native | `client/be_aas_file.o` | `defaults` | `current-native-context-native-client-be_aas_file.diff.gz` |
| native | `client/be_aas_reach.o` | `defaults` | `current-native-context-native-client-be_aas_reach.diff.gz` |
| native | `client/be_aas_route.o` | `defaults` | `current-native-context-native-client-be_aas_route.diff.gz` |
| native | `client/be_aas_routealt.o` | `defaults` | `current-native-context-native-client-be_aas_routealt.diff.gz` |
| native | `client/be_aas_sample.o` | `defaults` | `current-native-context-native-client-be_aas_sample.diff.gz` |
| native | `client/be_ai_char.o` | `defaults` | `current-native-context-native-client-be_ai_char.diff.gz` |
| native | `client/be_ai_chat.o` | `defaults` | `current-native-context-native-client-be_ai_chat.diff.gz` |
| native | `client/be_ai_move.o` | `defaults` | `current-native-context-native-client-be_ai_move.diff.gz` |
| native | `client/cl_avi.o` | `defaults` | `current-native-context-native-client-cl_avi.diff.gz` |
| native | `client/cl_cgame.o` | `defaults` | `current-native-context-native-client-cl_cgame.diff.gz` |
| native | `client/cl_cin.o` | `defaults` | `current-native-context-native-client-cl_cin.diff.gz` |
| native | `client/cl_console.o` | `defaults` | `current-native-context-native-client-cl_console.diff.gz` |
| native | `client/cl_curl.o` | `defaults` | `current-native-context-native-client-cl_curl.diff.gz` |
| native | `client/cl_input.o` | `defaults` | `current-native-context-native-client-cl_input.diff.gz` |
| native | `client/cl_jpeg.o` | `defaults` | `current-native-context-native-client-cl_jpeg.diff.gz` |
| native | `client/cl_keys.o` | `defaults` | `current-native-context-native-client-cl_keys.diff.gz` |
| native | `client/cl_main.o` | `defaults` | `current-native-context-native-client-cl_main.diff.gz` |
| native | `client/cl_net_chan.o` | `defaults` | `current-native-context-native-client-cl_net_chan.diff.gz` |
| native | `client/cl_parse.o` | `defaults` | `current-native-context-native-client-cl_parse.diff.gz` |
| native | `client/cl_scrn.o` | `defaults` | `current-native-context-native-client-cl_scrn.diff.gz` |
| native | `client/cl_ui.o` | `defaults` | `current-native-context-native-client-cl_ui.diff.gz` |
| native | `client/cm_load.o` | `defaults` | `current-native-context-native-client-cm_load.diff.gz` |
| native | `client/cm_patch.o` | `defaults` | `current-native-context-native-client-cm_patch.diff.gz` |
| native | `client/cm_trace.o` | `defaults` | `current-native-context-native-client-cm_trace.diff.gz` |
| native | `client/cmd.o` | `defaults` | `current-native-context-native-client-cmd.diff.gz` |
| native | `client/common.o` | `defaults` | `current-native-context-native-client-common.diff.gz` |
| native | `client/cvar.o` | `defaults` | `current-native-context-native-client-cvar.diff.gz` |
| native | `client/files.o` | `defaults` | `current-native-context-native-client-files.diff.gz` |
| native | `client/history.o` | `defaults` | `current-native-context-native-client-history.diff.gz` |
| native | `client/huffman.o` | `defaults` | `current-native-context-native-client-huffman.diff.gz` |
| native | `client/keys.o` | `defaults` | `current-native-context-native-client-keys.diff.gz` |
| native | `client/l_memory.o` | `defaults` | `current-native-context-native-client-l_memory.diff.gz` |
| native | `client/l_precomp.o` | `defaults` | `current-native-context-native-client-l_precomp.diff.gz` |
| native | `client/l_script.o` | `defaults` | `current-native-context-native-client-l_script.diff.gz` |
| native | `client/l_struct.o` | `defaults` | `current-native-context-native-client-l_struct.diff.gz` |
| native | `client/md5.o` | `defaults` | `current-native-context-native-client-md5.diff.gz` |
| native | `client/msg.o` | `defaults` | `current-native-context-native-client-msg.diff.gz` |
| native | `client/net_chan.o` | `defaults` | `current-native-context-native-client-net_chan.diff.gz` |
| native | `client/net_ip.o` | `defaults` | `current-native-context-native-client-net_ip.diff.gz` |
| native | `client/puff.o` | `defaults` | `current-native-context-native-client-puff.diff.gz` |
| native | `client/q_shared.o` | `defaults` | `current-native-context-native-client-q_shared.diff.gz` |
| native | `client/qvm/vm.o` | `defaults` | `current-native-context-native-client-qvm-vm.diff.gz` |
| native | `client/qvm/vm_interpreted.o` | `defaults` | `current-native-context-native-client-qvm-vm_interpreted.diff.gz` |
| native | `client/qvm/vm_x86.o` | `defaults` | `current-native-context-native-client-qvm-vm_x86.diff.gz` |
| native | `client/sdl_input.o` | `defaults` | `current-native-context-native-client-sdl_input.diff.gz` |
| native | `client/snd_dma.o` | `defaults` | `current-native-context-native-client-snd_dma.diff.gz` |
| native | `client/snd_mem.o` | `defaults` | `current-native-context-native-client-snd_mem.diff.gz` |
| native | `client/snd_mix.o` | `defaults` | `current-native-context-native-client-snd_mix.diff.gz` |
| native | `client/sv_bot.o` | `defaults` | `current-native-context-native-client-sv_bot.diff.gz` |
| native | `client/sv_ccmds.o` | `defaults` | `current-native-context-native-client-sv_ccmds.diff.gz` |
| native | `client/sv_filter.o` | `defaults` | `current-native-context-native-client-sv_filter.diff.gz` |
| native | `client/sv_game.o` | `defaults` | `current-native-context-native-client-sv_game.diff.gz` |
| native | `client/sv_init.o` | `defaults` | `current-native-context-native-client-sv_init.diff.gz` |
| native | `client/sv_main.o` | `defaults` | `current-native-context-native-client-sv_main.diff.gz` |
| native | `client/sv_net_chan.o` | `defaults` | `current-native-context-native-client-sv_net_chan.diff.gz` |
| native | `client/sv_snapshot.o` | `defaults` | `current-native-context-native-client-sv_snapshot.diff.gz` |
| native | `client/sv_world.o` | `defaults` | `current-native-context-native-client-sv_world.diff.gz` |
| native | `client/unix_main.o` | `defaults` | `current-native-context-native-client-unix_main.diff.gz` |
| native | `client/unix_shared.o` | `defaults` | `current-native-context-native-client-unix_shared.diff.gz` |
| native | `client/unzip.o` | `defaults` | `current-native-context-native-client-unzip.diff.gz` |
| native | `ded/be_aas_bspq3.o` | `defaults` | `current-native-context-native-ded-be_aas_bspq3.diff.gz` |
| native | `ded/be_aas_cluster.o` | `defaults` | `current-native-context-native-ded-be_aas_cluster.diff.gz` |
| native | `ded/be_aas_debug.o` | `defaults` | `current-native-context-native-ded-be_aas_debug.diff.gz` |
| native | `ded/be_aas_file.o` | `defaults` | `current-native-context-native-ded-be_aas_file.diff.gz` |
| native | `ded/be_aas_reach.o` | `defaults` | `current-native-context-native-ded-be_aas_reach.diff.gz` |
| native | `ded/be_aas_route.o` | `defaults` | `current-native-context-native-ded-be_aas_route.diff.gz` |
| native | `ded/be_aas_routealt.o` | `defaults` | `current-native-context-native-ded-be_aas_routealt.diff.gz` |
| native | `ded/be_aas_sample.o` | `defaults` | `current-native-context-native-ded-be_aas_sample.diff.gz` |
| native | `ded/be_ai_char.o` | `defaults` | `current-native-context-native-ded-be_ai_char.diff.gz` |
| native | `ded/be_ai_chat.o` | `defaults` | `current-native-context-native-ded-be_ai_chat.diff.gz` |
| native | `ded/be_ai_move.o` | `defaults` | `current-native-context-native-ded-be_ai_move.diff.gz` |
| native | `ded/cm_load.o` | `defaults` | `current-native-context-native-ded-cm_load.diff.gz` |
| native | `ded/cm_patch.o` | `defaults` | `current-native-context-native-ded-cm_patch.diff.gz` |
| native | `ded/cm_trace.o` | `defaults` | `current-native-context-native-ded-cm_trace.diff.gz` |
| native | `ded/cmd.o` | `defaults` | `current-native-context-native-ded-cmd.diff.gz` |
| native | `ded/common.o` | `defaults` | `current-native-context-native-ded-common.diff.gz` |
| native | `ded/cvar.o` | `defaults` | `current-native-context-native-ded-cvar.diff.gz` |
| native | `ded/files.o` | `defaults` | `current-native-context-native-ded-files.diff.gz` |
| native | `ded/history.o` | `defaults` | `current-native-context-native-ded-history.diff.gz` |
| native | `ded/huffman.o` | `defaults` | `current-native-context-native-ded-huffman.diff.gz` |
| native | `ded/keys.o` | `defaults` | `current-native-context-native-ded-keys.diff.gz` |
| native | `ded/l_memory.o` | `defaults` | `current-native-context-native-ded-l_memory.diff.gz` |
| native | `ded/l_precomp.o` | `defaults` | `current-native-context-native-ded-l_precomp.diff.gz` |
| native | `ded/l_script.o` | `defaults` | `current-native-context-native-ded-l_script.diff.gz` |
| native | `ded/l_struct.o` | `defaults` | `current-native-context-native-ded-l_struct.diff.gz` |
| native | `ded/md5.o` | `defaults` | `current-native-context-native-ded-md5.diff.gz` |
| native | `ded/msg.o` | `defaults` | `current-native-context-native-ded-msg.diff.gz` |
| native | `ded/net_chan.o` | `defaults` | `current-native-context-native-ded-net_chan.diff.gz` |
| native | `ded/net_ip.o` | `defaults` | `current-native-context-native-ded-net_ip.diff.gz` |
| native | `ded/q_shared.o` | `defaults` | `current-native-context-native-ded-q_shared.diff.gz` |
| native | `ded/qvm/vm.o` | `defaults` | `current-native-context-native-ded-qvm-vm.diff.gz` |
| native | `ded/qvm/vm_interpreted.o` | `defaults` | `current-native-context-native-ded-qvm-vm_interpreted.diff.gz` |
| native | `ded/qvm/vm_x86.o` | `defaults` | `current-native-context-native-ded-qvm-vm_x86.diff.gz` |
| native | `ded/sv_bot.o` | `defaults` | `current-native-context-native-ded-sv_bot.diff.gz` |
| native | `ded/sv_ccmds.o` | `defaults` | `current-native-context-native-ded-sv_ccmds.diff.gz` |
| native | `ded/sv_filter.o` | `defaults` | `current-native-context-native-ded-sv_filter.diff.gz` |
| native | `ded/sv_game.o` | `defaults` | `current-native-context-native-ded-sv_game.diff.gz` |
| native | `ded/sv_init.o` | `defaults` | `current-native-context-native-ded-sv_init.diff.gz` |
| native | `ded/sv_main.o` | `defaults` | `current-native-context-native-ded-sv_main.diff.gz` |
| native | `ded/sv_net_chan.o` | `defaults` | `current-native-context-native-ded-sv_net_chan.diff.gz` |
| native | `ded/sv_snapshot.o` | `defaults` | `current-native-context-native-ded-sv_snapshot.diff.gz` |
| native | `ded/sv_world.o` | `defaults` | `current-native-context-native-ded-sv_world.diff.gz` |
| native | `ded/unix_main.o` | `defaults` | `current-native-context-native-ded-unix_main.diff.gz` |
| native | `ded/unix_shared.o` | `defaults` | `current-native-context-native-ded-unix_shared.diff.gz` |
| native | `ded/unzip.o` | `defaults` | `current-native-context-native-ded-unzip.diff.gz` |
| native | `rend1/puff.o` | `defaults` | `current-native-context-native-rend1-puff.diff.gz` |
| native | `rend1/q_shared.o` | `defaults` | `current-native-context-native-rend1-q_shared.diff.gz` |
| native | `rend1/tr_animation.o` | `defaults` | `current-native-context-native-rend1-tr_animation.diff.gz` |
| native | `rend1/tr_arb.o` | `defaults` | `current-native-context-native-rend1-tr_arb.diff.gz` |
| native | `rend1/tr_backend.o` | `defaults` | `current-native-context-native-rend1-tr_backend.diff.gz` |
| native | `rend1/tr_bsp.o` | `defaults` | `current-native-context-native-rend1-tr_bsp.diff.gz` |
| native | `rend1/tr_cmds.o` | `defaults` | `current-native-context-native-rend1-tr_cmds.diff.gz` |
| native | `rend1/tr_curve.o` | `defaults` | `current-native-context-native-rend1-tr_curve.diff.gz` |
| native | `rend1/tr_flares.o` | `defaults` | `current-native-context-native-rend1-tr_flares.diff.gz` |
| native | `rend1/tr_image.o` | `defaults` | `current-native-context-native-rend1-tr_image.diff.gz` |
| native | `rend1/tr_image_tga.o` | `defaults` | `current-native-context-native-rend1-tr_image_tga.diff.gz` |
| native | `rend1/tr_init.o` | `defaults` | `current-native-context-native-rend1-tr_init.diff.gz` |
| native | `rend1/tr_light.o` | `defaults` | `current-native-context-native-rend1-tr_light.diff.gz` |
| native | `rend1/tr_main.o` | `defaults` | `current-native-context-native-rend1-tr_main.diff.gz` |
| native | `rend1/tr_mesh.o` | `defaults` | `current-native-context-native-rend1-tr_mesh.diff.gz` |
| native | `rend1/tr_model.o` | `defaults` | `current-native-context-native-rend1-tr_model.diff.gz` |
| native | `rend1/tr_model_iqm.o` | `defaults` | `current-native-context-native-rend1-tr_model_iqm.diff.gz` |
| native | `rend1/tr_noise.o` | `defaults` | `current-native-context-native-rend1-tr_noise.diff.gz` |
| native | `rend1/tr_scene.o` | `defaults` | `current-native-context-native-rend1-tr_scene.diff.gz` |
| native | `rend1/tr_shade.o` | `defaults` | `current-native-context-native-rend1-tr_shade.diff.gz` |
| native | `rend1/tr_shade_calc.o` | `defaults` | `current-native-context-native-rend1-tr_shade_calc.diff.gz` |
| native | `rend1/tr_shader.o` | `defaults` | `current-native-context-native-rend1-tr_shader.diff.gz` |
| native | `rend1/tr_shadows.o` | `defaults` | `current-native-context-native-rend1-tr_shadows.diff.gz` |
| native | `rend1/tr_sky.o` | `defaults` | `current-native-context-native-rend1-tr_sky.diff.gz` |
| native | `rend1/tr_surface.o` | `defaults` | `current-native-context-native-rend1-tr_surface.diff.gz` |
| native | `rend1/tr_vbo.o` | `defaults` | `current-native-context-native-rend1-tr_vbo.diff.gz` |
| native | `rend1/tr_world.o` | `defaults` | `current-native-context-native-rend1-tr_world.diff.gz` |
| native | `rendv/puff.o` | `defaults` | `current-native-context-native-rendv-puff.diff.gz` |
| native | `rendv/q_shared.o` | `defaults` | `current-native-context-native-rendv-q_shared.diff.gz` |
| native | `rendv/tr_animation.o` | `defaults` | `current-native-context-native-rendv-tr_animation.diff.gz` |
| native | `rendv/tr_backend.o` | `defaults` | `current-native-context-native-rendv-tr_backend.diff.gz` |
| native | `rendv/tr_bsp.o` | `defaults` | `current-native-context-native-rendv-tr_bsp.diff.gz` |
| native | `rendv/tr_cmds.o` | `defaults` | `current-native-context-native-rendv-tr_cmds.diff.gz` |
| native | `rendv/tr_curve.o` | `defaults` | `current-native-context-native-rendv-tr_curve.diff.gz` |
| native | `rendv/tr_image.o` | `defaults` | `current-native-context-native-rendv-tr_image.diff.gz` |
| native | `rendv/tr_image_tga.o` | `defaults` | `current-native-context-native-rendv-tr_image_tga.diff.gz` |
| native | `rendv/tr_init.o` | `defaults` | `current-native-context-native-rendv-tr_init.diff.gz` |
| native | `rendv/tr_light.o` | `defaults` | `current-native-context-native-rendv-tr_light.diff.gz` |
| native | `rendv/tr_main.o` | `defaults` | `current-native-context-native-rendv-tr_main.diff.gz` |
| native | `rendv/tr_mesh.o` | `defaults` | `current-native-context-native-rendv-tr_mesh.diff.gz` |
| native | `rendv/tr_model.o` | `defaults` | `current-native-context-native-rendv-tr_model.diff.gz` |
| native | `rendv/tr_model_iqm.o` | `defaults` | `current-native-context-native-rendv-tr_model_iqm.diff.gz` |
| native | `rendv/tr_noise.o` | `defaults` | `current-native-context-native-rendv-tr_noise.diff.gz` |
| native | `rendv/tr_scene.o` | `defaults` | `current-native-context-native-rendv-tr_scene.diff.gz` |
| native | `rendv/tr_shade.o` | `defaults` | `current-native-context-native-rendv-tr_shade.diff.gz` |
| native | `rendv/tr_shade_calc.o` | `defaults` | `current-native-context-native-rendv-tr_shade_calc.diff.gz` |
| native | `rendv/tr_shader.o` | `defaults` | `current-native-context-native-rendv-tr_shader.diff.gz` |
| native | `rendv/tr_shadows.o` | `defaults` | `current-native-context-native-rendv-tr_shadows.diff.gz` |
| native | `rendv/tr_sky.o` | `defaults` | `current-native-context-native-rendv-tr_sky.diff.gz` |
| native | `rendv/tr_surface.o` | `defaults` | `current-native-context-native-rendv-tr_surface.diff.gz` |
| native | `rendv/tr_world.o` | `defaults` | `current-native-context-native-rendv-tr_world.diff.gz` |
| native | `rendv/vk.o` | `defaults` | `current-native-context-native-rendv-vk.diff.gz` |
| native | `rendv/vk_flares.o` | `defaults` | `current-native-context-native-rendv-vk_flares.diff.gz` |
| native | `rendv/vk_vbo.o` | `defaults` | `current-native-context-native-rendv-vk_vbo.diff.gz` |
| native-nosdl | `client/be_aas_bspq3.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_bspq3.diff.gz` |
| native-nosdl | `client/be_aas_cluster.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_cluster.diff.gz` |
| native-nosdl | `client/be_aas_debug.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_debug.diff.gz` |
| native-nosdl | `client/be_aas_file.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_file.diff.gz` |
| native-nosdl | `client/be_aas_reach.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_reach.diff.gz` |
| native-nosdl | `client/be_aas_route.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_route.diff.gz` |
| native-nosdl | `client/be_aas_routealt.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_routealt.diff.gz` |
| native-nosdl | `client/be_aas_sample.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_sample.diff.gz` |
| native-nosdl | `client/be_ai_char.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_ai_char.diff.gz` |
| native-nosdl | `client/be_ai_chat.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_ai_chat.diff.gz` |
| native-nosdl | `client/be_ai_move.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_ai_move.diff.gz` |
| native-nosdl | `client/cl_avi.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_avi.diff.gz` |
| native-nosdl | `client/cl_cgame.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_cgame.diff.gz` |
| native-nosdl | `client/cl_cin.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_cin.diff.gz` |
| native-nosdl | `client/cl_console.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_console.diff.gz` |
| native-nosdl | `client/cl_curl.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_curl.diff.gz` |
| native-nosdl | `client/cl_input.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_input.diff.gz` |
| native-nosdl | `client/cl_jpeg.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_jpeg.diff.gz` |
| native-nosdl | `client/cl_keys.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_keys.diff.gz` |
| native-nosdl | `client/cl_main.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_main.diff.gz` |
| native-nosdl | `client/cl_net_chan.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_net_chan.diff.gz` |
| native-nosdl | `client/cl_parse.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_parse.diff.gz` |
| native-nosdl | `client/cl_scrn.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_scrn.diff.gz` |
| native-nosdl | `client/cl_ui.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_ui.diff.gz` |
| native-nosdl | `client/cm_load.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cm_load.diff.gz` |
| native-nosdl | `client/cm_patch.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cm_patch.diff.gz` |
| native-nosdl | `client/cm_trace.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cm_trace.diff.gz` |
| native-nosdl | `client/cmd.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cmd.diff.gz` |
| native-nosdl | `client/common.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-common.diff.gz` |
| native-nosdl | `client/cvar.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cvar.diff.gz` |
| native-nosdl | `client/files.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-files.diff.gz` |
| native-nosdl | `client/history.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-history.diff.gz` |
| native-nosdl | `client/huffman.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-huffman.diff.gz` |
| native-nosdl | `client/keys.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-keys.diff.gz` |
| native-nosdl | `client/l_memory.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-l_memory.diff.gz` |
| native-nosdl | `client/l_precomp.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-l_precomp.diff.gz` |
| native-nosdl | `client/l_script.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-l_script.diff.gz` |
| native-nosdl | `client/l_struct.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-l_struct.diff.gz` |
| native-nosdl | `client/linux_glimp.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-linux_glimp.diff.gz` |
| native-nosdl | `client/linux_qgl.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-linux_qgl.diff.gz` |
| native-nosdl | `client/linux_snd.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-linux_snd.diff.gz` |
| native-nosdl | `client/md5.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-md5.diff.gz` |
| native-nosdl | `client/msg.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-msg.diff.gz` |
| native-nosdl | `client/net_chan.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-net_chan.diff.gz` |
| native-nosdl | `client/net_ip.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-net_ip.diff.gz` |
| native-nosdl | `client/puff.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-puff.diff.gz` |
| native-nosdl | `client/q_shared.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-q_shared.diff.gz` |
| native-nosdl | `client/qvm/vm.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-qvm-vm.diff.gz` |
| native-nosdl | `client/qvm/vm_interpreted.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-qvm-vm_interpreted.diff.gz` |
| native-nosdl | `client/qvm/vm_x86.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-qvm-vm_x86.diff.gz` |
| native-nosdl | `client/snd_dma.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-snd_dma.diff.gz` |
| native-nosdl | `client/snd_mem.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-snd_mem.diff.gz` |
| native-nosdl | `client/snd_mix.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-snd_mix.diff.gz` |
| native-nosdl | `client/sv_bot.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_bot.diff.gz` |
| native-nosdl | `client/sv_ccmds.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_ccmds.diff.gz` |
| native-nosdl | `client/sv_filter.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_filter.diff.gz` |
| native-nosdl | `client/sv_game.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_game.diff.gz` |
| native-nosdl | `client/sv_init.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_init.diff.gz` |
| native-nosdl | `client/sv_main.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_main.diff.gz` |
| native-nosdl | `client/sv_net_chan.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_net_chan.diff.gz` |
| native-nosdl | `client/sv_snapshot.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_snapshot.diff.gz` |
| native-nosdl | `client/sv_world.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_world.diff.gz` |
| native-nosdl | `client/unix_main.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-unix_main.diff.gz` |
| native-nosdl | `client/unix_shared.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-unix_shared.diff.gz` |
| native-nosdl | `client/unzip.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-unzip.diff.gz` |
| native-nosdl | `ded/be_aas_bspq3.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_bspq3.diff.gz` |
| native-nosdl | `ded/be_aas_cluster.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_cluster.diff.gz` |
| native-nosdl | `ded/be_aas_debug.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_debug.diff.gz` |
| native-nosdl | `ded/be_aas_file.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_file.diff.gz` |
| native-nosdl | `ded/be_aas_reach.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_reach.diff.gz` |
| native-nosdl | `ded/be_aas_route.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_route.diff.gz` |
| native-nosdl | `ded/be_aas_routealt.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_routealt.diff.gz` |
| native-nosdl | `ded/be_aas_sample.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_sample.diff.gz` |
| native-nosdl | `ded/be_ai_char.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_ai_char.diff.gz` |
| native-nosdl | `ded/be_ai_chat.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_ai_chat.diff.gz` |
| native-nosdl | `ded/be_ai_move.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_ai_move.diff.gz` |
| native-nosdl | `ded/cm_load.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-cm_load.diff.gz` |
| native-nosdl | `ded/cm_patch.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-cm_patch.diff.gz` |
| native-nosdl | `ded/cm_trace.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-cm_trace.diff.gz` |
| native-nosdl | `ded/cmd.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-cmd.diff.gz` |
| native-nosdl | `ded/common.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-common.diff.gz` |
| native-nosdl | `ded/cvar.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-cvar.diff.gz` |
| native-nosdl | `ded/files.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-files.diff.gz` |
| native-nosdl | `ded/history.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-history.diff.gz` |
| native-nosdl | `ded/huffman.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-huffman.diff.gz` |
| native-nosdl | `ded/keys.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-keys.diff.gz` |
| native-nosdl | `ded/l_memory.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-l_memory.diff.gz` |
| native-nosdl | `ded/l_precomp.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-l_precomp.diff.gz` |
| native-nosdl | `ded/l_script.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-l_script.diff.gz` |
| native-nosdl | `ded/l_struct.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-l_struct.diff.gz` |
| native-nosdl | `ded/md5.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-md5.diff.gz` |
| native-nosdl | `ded/msg.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-msg.diff.gz` |
| native-nosdl | `ded/net_chan.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-net_chan.diff.gz` |
| native-nosdl | `ded/net_ip.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-net_ip.diff.gz` |
| native-nosdl | `ded/q_shared.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-q_shared.diff.gz` |
| native-nosdl | `ded/qvm/vm.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-qvm-vm.diff.gz` |
| native-nosdl | `ded/qvm/vm_interpreted.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-qvm-vm_interpreted.diff.gz` |
| native-nosdl | `ded/qvm/vm_x86.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-qvm-vm_x86.diff.gz` |
| native-nosdl | `ded/sv_bot.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_bot.diff.gz` |
| native-nosdl | `ded/sv_ccmds.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_ccmds.diff.gz` |
| native-nosdl | `ded/sv_filter.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_filter.diff.gz` |
| native-nosdl | `ded/sv_game.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_game.diff.gz` |
| native-nosdl | `ded/sv_init.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_init.diff.gz` |
| native-nosdl | `ded/sv_main.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_main.diff.gz` |
| native-nosdl | `ded/sv_net_chan.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_net_chan.diff.gz` |
| native-nosdl | `ded/sv_snapshot.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_snapshot.diff.gz` |
| native-nosdl | `ded/sv_world.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_world.diff.gz` |
| native-nosdl | `ded/unix_main.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-unix_main.diff.gz` |
| native-nosdl | `ded/unix_shared.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-unix_shared.diff.gz` |
| native-nosdl | `ded/unzip.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-unzip.diff.gz` |
| native-nosdl | `rend1/puff.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-puff.diff.gz` |
| native-nosdl | `rend1/q_shared.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-q_shared.diff.gz` |
| native-nosdl | `rend1/tr_animation.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_animation.diff.gz` |
| native-nosdl | `rend1/tr_arb.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_arb.diff.gz` |
| native-nosdl | `rend1/tr_backend.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_backend.diff.gz` |
| native-nosdl | `rend1/tr_bsp.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_bsp.diff.gz` |
| native-nosdl | `rend1/tr_cmds.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_cmds.diff.gz` |
| native-nosdl | `rend1/tr_curve.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_curve.diff.gz` |
| native-nosdl | `rend1/tr_flares.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_flares.diff.gz` |
| native-nosdl | `rend1/tr_image.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_image.diff.gz` |
| native-nosdl | `rend1/tr_image_tga.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_image_tga.diff.gz` |
| native-nosdl | `rend1/tr_init.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_init.diff.gz` |
| native-nosdl | `rend1/tr_light.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_light.diff.gz` |
| native-nosdl | `rend1/tr_main.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_main.diff.gz` |
| native-nosdl | `rend1/tr_mesh.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_mesh.diff.gz` |
| native-nosdl | `rend1/tr_model.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_model.diff.gz` |
| native-nosdl | `rend1/tr_model_iqm.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_model_iqm.diff.gz` |
| native-nosdl | `rend1/tr_noise.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_noise.diff.gz` |
| native-nosdl | `rend1/tr_scene.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_scene.diff.gz` |
| native-nosdl | `rend1/tr_shade.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_shade.diff.gz` |
| native-nosdl | `rend1/tr_shade_calc.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_shade_calc.diff.gz` |
| native-nosdl | `rend1/tr_shader.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_shader.diff.gz` |
| native-nosdl | `rend1/tr_shadows.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_shadows.diff.gz` |
| native-nosdl | `rend1/tr_sky.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_sky.diff.gz` |
| native-nosdl | `rend1/tr_surface.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_surface.diff.gz` |
| native-nosdl | `rend1/tr_vbo.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_vbo.diff.gz` |
| native-nosdl | `rend1/tr_world.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_world.diff.gz` |
| native-nosdl | `rendv/puff.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-puff.diff.gz` |
| native-nosdl | `rendv/q_shared.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-q_shared.diff.gz` |
| native-nosdl | `rendv/tr_animation.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_animation.diff.gz` |
| native-nosdl | `rendv/tr_backend.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_backend.diff.gz` |
| native-nosdl | `rendv/tr_bsp.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_bsp.diff.gz` |
| native-nosdl | `rendv/tr_cmds.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_cmds.diff.gz` |
| native-nosdl | `rendv/tr_curve.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_curve.diff.gz` |
| native-nosdl | `rendv/tr_image.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_image.diff.gz` |
| native-nosdl | `rendv/tr_image_tga.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_image_tga.diff.gz` |
| native-nosdl | `rendv/tr_init.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_init.diff.gz` |
| native-nosdl | `rendv/tr_light.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_light.diff.gz` |
| native-nosdl | `rendv/tr_main.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_main.diff.gz` |
| native-nosdl | `rendv/tr_mesh.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_mesh.diff.gz` |
| native-nosdl | `rendv/tr_model.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_model.diff.gz` |
| native-nosdl | `rendv/tr_model_iqm.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_model_iqm.diff.gz` |
| native-nosdl | `rendv/tr_noise.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_noise.diff.gz` |
| native-nosdl | `rendv/tr_scene.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_scene.diff.gz` |
| native-nosdl | `rendv/tr_shade.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_shade.diff.gz` |
| native-nosdl | `rendv/tr_shade_calc.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_shade_calc.diff.gz` |
| native-nosdl | `rendv/tr_shader.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_shader.diff.gz` |
| native-nosdl | `rendv/tr_shadows.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_shadows.diff.gz` |
| native-nosdl | `rendv/tr_sky.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_sky.diff.gz` |
| native-nosdl | `rendv/tr_surface.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_surface.diff.gz` |
| native-nosdl | `rendv/tr_world.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_world.diff.gz` |
| native-nosdl | `rendv/vk.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-vk.diff.gz` |
| native-nosdl | `rendv/vk_flares.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-vk_flares.diff.gz` |
| native-nosdl | `rendv/vk_vbo.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-vk_vbo.diff.gz` |
| native-nosdl | `client/linux_joystick.o` | `USE_SDL=0 CFLAGS=-DUSE_JOYSTICK` | `current-native-context-native-nosdl-client-linux_joystick.diff.gz` |
| static-opengl | `client/cl_main.o` | `USE_RENDERER_DLOPEN=0 RENDERER_DEFAULT=opengl` | `current-static-opengl-cl_main.diff.gz` |
| static-opengl | `rend1/tr_init.o` | `USE_RENDERER_DLOPEN=0 RENDERER_DEFAULT=opengl` | `current-static-opengl-tr_init.diff.gz` |
| static-vulkan | `client/cl_main.o` | `USE_RENDERER_DLOPEN=0 RENDERER_DEFAULT=vulkan` | `current-static-vulkan-cl_main.diff.gz` |
| static-vulkan | `rendv/tr_init.o` | `USE_RENDERER_DLOPEN=0 RENDERER_DEFAULT=vulkan` | `current-static-vulkan-tr_init.diff.gz` |

## T24/T25 continuation decisions

- First commit adds the exact authorized catalog entries. No C SHA256 exception. Reviewer prefixes: original/T25 4a9f0e56..., single-source cast 96b95b5a.... Existing local diagnostic artifacts use a different command: original af7af4b97d9a5753c8c3451059cbb6c4b61bdef5e38d55813b235c0bfc2f8c91; single-source cast 737a9615cf6a8499e41fb05d92966c6bb74ed1abe161bc8f5c25f08300869110. Production manifest hashes will be measured again.
- The T24 site list contains ten outgoing arguments total (two in win_input, eight in win_snd), despite the prompt introductory count of eight. Apply the explicit sites, plus the two incoming memcmp pointer uses.

- T25 production release-linux-x86_64/client/sv_client.o: original and T25 SHA256 `af7af4b97d9a5753c8c3451059cbb6c4b61bdef5e38d55813b235c0bfc2f8c91`. Dedicated and client G2/G3 PASS; G4 `sv_client.codegen.diff.gz` and `t25-client-sv_client.diff.gz`.

- T25 production release-linux-x86_64/ded/sv_client.o: original and T25 SHA256 `4a9f0e564093f0277ee5c558d269fc3a00b01b92a5d40665d06df0a14298815c`. Dedicated and client G2/G3 PASS; G4 `sv_client.codegen.diff.gz` and `t25-client-sv_client.diff.gz`.

- T24 win_local.h verification: all 300 MinGW original C hashes unchanged; client win_main G2/G3 passes. Header defines CINTERFACE only under __cplusplus. Next win_input.c.

- T24 include-order resolution: client.h reaches Windows SDK through curl before win_local.h. Windows C++ Make commands therefore predefine the same empty CINTERFACE macro (`-DCINTERFACE=`), matching the approved header definition with no redefinition diagnostic. This is required to satisfy before-any-SDK-include; C flags remain untouched. win_input C hash and strict release/debug/G2/G3/G4 now all PASS. MSVC project definitions will carry the same early setting in phase 3.
- win_input adds two more required T15 lexical lines to the previously documented -w heuristic exception; no formatting-only edits.

- win_input: explicit WASAPI=0/1 verification [{"source": "code/win32/win_input.c", "wasapi": 0, "c_sha256": "3a02f70ee975c673204548c0727afd4611fa56b779f98470efb6b9dd1dda272b", "variables": ["PLATFORM=mingw64", "ARCH=x86_64", "USE_SDL=0", "OPTIMIZE=-O2 -ffast-math -fno-lto", "CFLAGS=-DUSE_WASAPI=0"], "layout": 0, "symbol": 0, "codegen": 0}, {"source": "code/win32/win_input.c", "wasapi": 1, "c_sha256": "3a02f70ee975c673204548c0727afd4611fa56b779f98470efb6b9dd1dda272b", "variables": ["PLATFORM=mingw64", "ARCH=x86_64", "USE_SDL=0", "OPTIMIZE=-O2 -ffast-math -fno-lto", "CFLAGS=-DUSE_WASAPI=1"], "layout": 0, "symbol": 0, "codegen": 0}]

- win_snd: explicit WASAPI=0/1 verification [{"source": "code/win32/win_snd.c", "wasapi": 0, "c_sha256": "48775a7a2b2aa5937b8389918e55122524625743ba36b4f93e639637a908cf51", "variables": ["PLATFORM=mingw64", "ARCH=x86_64", "USE_SDL=0", "OPTIMIZE=-O2 -ffast-math -fno-lto", "CFLAGS=-DUSE_WASAPI=0"], "layout": 0, "symbol": 0, "codegen": 0}, {"source": "code/win32/win_snd.c", "wasapi": 1, "c_sha256": "d775a18c66a7ecece6be67e165cbf69d36ff4d08afe49d647c51c7371204f431", "variables": ["PLATFORM=mingw64", "ARCH=x86_64", "USE_SDL=0", "OPTIMIZE=-O2 -ffast-math -fno-lto", "CFLAGS=-DUSE_WASAPI=1"], "layout": 0, "symbol": 0, "codegen": 1}]

- T24/T25 native integration PASS: dedicated, client, OpenGL/Vulkan dlopen renderers, static Vulkan; G5 all 13 groups and math PASS. Runtime uses sequential C/C++ executable copies at one temporary installation path to match the printed working directory. Cache counts still fluctuate between 0 and 9 despite warm-ups; apply only the existing G6 allowance to remove that line. All 123 remaining lines are byte-identical. Raw and normalized logs retained; no other normalization or engine timing change.
- Runtime c 1 raw SHA256 `e995c70a11859559aa280ac47252d4e0b8dd7b9c5bf56469674b081139911a83`; normalized SHA256 `e0428e406c541d3de1640f4a07d0a2dd252cb2859f94f1743fe653a537c854f7`.
- Runtime c 2 raw SHA256 `694dcaafed5c2bce289015cda34a318155dd8d53a8830199832011097a629125`; normalized SHA256 `e0428e406c541d3de1640f4a07d0a2dd252cb2859f94f1743fe653a537c854f7`.
- Runtime cxx 1 raw SHA256 `e995c70a11859559aa280ac47252d4e0b8dd7b9c5bf56469674b081139911a83`; normalized SHA256 `e0428e406c541d3de1640f4a07d0a2dd252cb2859f94f1743fe653a537c854f7`.
- Post-rename harness safeguards: PORT_C_ORACLE selects the recorded pre-rename source checkout for C recipes; compile_pair fails if the alleged C command is C++. Compiler command and cwd are both recorded. Source discovery accepts .cpp, and clang-tidy follows rename pairs for changed-line filtering.

- ARM full link exposed four libgcc assembler imports (__aeabi_idiv/uidiv/idivmod/uidivmod) with mangled names. T5 applies because these are resolved from external assembly by name, unlike internal static JIT callbacks. Add Q_EXTERN_C to the existing declarations and enforce raw names in G3. No signatures or call expressions change.

- Current integration evidence: t25-build-matrix-results.json (16/16 PASS), t25-cross-build-results.json (4/4 full links PASS), t25-cross-gate-results.json (424/424 G2/G3 PASS; every advisory diff named in each row). ARM libgcc raw-linkage gate also passes after the four T5 annotations. OpenGL, Vulkan and static Vulkan clients load q3dm17 and shut down cleanly under Xvfb; logs t25-client-*.log.gz.

- DEVIATION: embedded shader path prerequisite to phase 3. shader_data.c is included directly by vk.c. Move its blob unchanged to .cpp and update the one include plus two generator output paths separately, leaving the later vk.c rename content-free. Both C and C++ can include the .cpp data. This is a necessary filename dependency update, not an engine logic change. Reviewer preflight approved this minimal sequence.
