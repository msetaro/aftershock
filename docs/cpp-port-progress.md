# C++20 port checkpoint

Branch: `t3code/port-engine-to-cpp20`. Original C oracle: `8a7e8ed2`. Reviewed continuation starts at `6a990e7c` (T1–T23 plan amendment). All 171 scoped engine implementation/data files now use .cpp; each moved blob was verified unchanged. No engine bug fixes, vendor edits, renderer2 port, main pushes, force pushes, or history rewrites were made.

**Engine port and phases 0–3 pass, including all 14 active CI build jobs. The required T25 cleanup now passes the maintainer-approved metadata comparison.** All 257 inventory entries are done (including the two explicit exclusions). Native GCC/Clang C and strict C++ configurations all build. MinGW, ARM, AArch64 and PPC64LE C++ full links pass. T24/T25 resolve all reviewed blockers with unchanged C hashes. C++ dedicated runtime matches C under G6, and the client loads OpenGL/Vulkan renderers under Xvfb.

## Evidence archive

Full logs, object hashes, gate diffs, exact command inventories, and review fixtures are archived on the independent orphan branch `port-evidence`, commit [`823d6f1e7b0603980bb743cbba2cf6d9c9b405da`](https://github.com/msetaro/aftershock/tree/823d6f1e7b0603980bb743cbba2cf6d9c9b405da/tools/port/evidence). Every historical filename reference below is relative to that archived directory. Only the small current summary remains in-tree: `tools/port/results/review-acceptance.json`. The archive has no engine/main commit parents; prior branch history is preserved without rewriting.

```sh
mkdir -p /tmp/port-evidence
git fetch origin port-evidence
git archive 823d6f1e7b0603980bb743cbba2cf6d9c9b405da tools/port/evidence | tar -x -C /tmp/port-evidence
export PORT_EVIDENCE=/tmp/port-evidence/tools/port/evidence
```

## Next action

No implementation action remains. PR #32 is ready for maintainer review: all 15 threads answered and resolved, origin/main merged, evidence archived and pushed, and all 14 active CI jobs pass on the merged review tree; do not merge or push main without a new instruction.

## Phase checklist

- [x] Phase 0: harness, frozen warnings, C hash proof, controls; formatter advisory.
- [x] Phase 1: all scoped sources and headers, unchanged C hashes, strict native/cross builds.
- [x] Phase 2: required external linkage, G2/G3, full native links, deterministic dedicated smoke, client-driven renderer load.
- [x] Phase 3 rename, fresh native/cross builds, gates, and runtime.
- [x] MSVC x64 and ARM64 Debug/Release CI (34804759804).
- [x] T25 cleanup: GCC/Clang raw release and stripped-debug hashes unchanged (client and ded).
- [x] Final CI evidence and cold-readable checkpoint, committed and pushed to the feature branch.
- [x] PR #32: both reviews addressed, all 15 threads replied/resolved, merged-tree CI green, PR ready.

## Remaining blockers

None. PR32 review corrected the T25 verification rule: GCC/Clang release objects must match raw, and debug objects must match after objcopy --strip-debug. The C-only branch is removed; all eight client/ded compiler/configuration comparisons pass. Historical raw-DWARF source-checksum differences remain archived as the reason for the narrow, explicitly approved metadata exception.

## Module inventory

“Done” records successful port/gate verification. Complete native C++ executables now run; optional configurations and cleanup limitations are listed explicitly. qasm.h is assembly-only; sv_rankings.c is excluded and must not be renamed. Shared cgame/game/ui ABI headers are included; this checkout has no bg implementation sources.

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

## Current verification results

- **C oracle:** original `8a7e8ed2`; frozen pre-rename checkout `e49b82595c7b1a25c70d9e86b72b3ed147adbdb3`. Final original-object checks pass 295/295 native, 299/299 non-SDL, 65/65 each ARM/AArch64/PPC64LE, and 300/300 MinGW non-SDL. No raw C hash exception. MinGW uses matching non-LTO flags for inspectable deterministic object bytes.
- **G1:** 16/16 native matrix configurations pass (eight C-oracle/eight C++, GCC/Clang, release/debug, SDL/non-SDL, static OpenGL/Vulkan). Native dedicated/client/both dlopen renderers and static clients link. MinGW client/ded/renderers and ARM/AArch64/PPC64LE dedicated builds pass. See `phase3-build-matrix-results.json` and `phase3-cross-build-results.json` for exact commands, source cwd and logs.
- **G2/G3:** all **454 native contexts and 424 cross contexts pass** against the frozen C checkout. The later MSVC-only vm_aarch64 T1 cast also passes its AArch64 pair. Scope includes static-renderer boundary checks from phase 2 and both WASAPI modes from T24. Raw names of actual external boundaries and demangled undefined references are checked. Gate positive/negative controls pass.
- **G4:** 331 native and 338 cross advisory differences remain; 123 native and 86 cross pairs pass without differences. Every current diff is indexed below and in `phase3-native-context-results.json` / `phase3-cross-gates-results.json`. Compiler instruction selection, labels, source-name literals and other differences are retained, not called byte-identical behavior evidence. Pure math G4/G5 passes.
- **G5:** all 13 fixed-input groups pass; `vector_math 5c00b4de` in C and C++. Parsing/strings/info, command/cvar/FS helpers, MSG roundtrips and 256 deltas, adaptive Huffman, address helpers, 1000 collision traces and math are covered. One C driver calls either object set using test-only aliases; production G3 is separate. See `phase3-g5.log.gz`, `phase3-math.log.gz`.
- **G6:** prescribed faketime test passes C-repeat first and C-vs-C++ next. Exactly the allowed `...found N cached paks` line is removed because it still varies after warming; no other normalization. All 123 remaining lines hash `e0428e406c541d3de1640f4a07d0a2dd252cb2859f94f1743fe653a537c854f7`. Raw 124-line logs and all raw hashes are retained in `phase3-runtime-results.json`. Native C++ OpenGL/Vulkan dlopen clients and static Vulkan load q3dm17 under Xvfb and quit cleanly. No client timedemo/image equivalence is claimed.
- **G7:** clang-tidy checks 155 native sources, zero compile/tool failures, 366 existing narrowing findings retained; 16 platform/include-only inputs skipped. GCC/Clang build matrix passes. C and C++ GCC ASan/UBSan bot smoke have no diagnostics with the same two original alignment suppressions and leak checking disabled. No extra runtime suppression was added for C++.
- **G8:** 170 main-commit source moves are R100 with exact parent blob matches; shader data was a separate byte-identical move with necessary include/generator path changes. Reviewer verified compiler selection, vendor C preservation, MSVC paths/settings/CRLF, x86 CI removal and renderer2 build removal. Current engine edits stay in T1–T25; build/harness deviations are listed below.
- **CI:** first run 34804449387 reached existing macOS deprecated-declarations and MSVC frozen string-literal policy gaps. Second run 34804619682 has all macOS and MSVC x64 Debug/Release green; ARM64 exposed one catalog T1 VirtualAlloc cast, now committed. Third run **34804759804 is fully green** at source commit `4ffcd649`: all 14 active build jobs pass (MSVC x64/ARM64 Debug/Release, MinGW x86_64 Debug/Release, Linux x86_64/ARM64 Debug/Release, macOS x86_64/AArch64 Debug/Release). Four preexisting emulator jobs contain disabled build steps and are not counted; both release-publication jobs are skipped on this feature branch. Disabled legacy emulator jobs are not counted as runtime verification.
- **Unverified:** optional FreeType (headers/pkg-config unavailable), dormant Windows USE_PROFILES (not selected by Make/MSVC), cross-target executable runtime, and client timedemo/frame/image equivalence. Native renderer map-load tests do not claim those checks. `sv_rankings.c` is excluded for its proprietary SDK; `qasm.h` is assembly-only. MSVC x64/ARM64 Debug/Release pass on 34804759804.

Historical results at 7ec7e925 and intermediate T24/T25 checkpoints remain in git/evidence (`resumed-*`, `final-*`, `t25-*`). They are superseded by the current results above.

## PR #32 final acceptance (merged tree)

**Review complete:** all 15 first-review threads have a one-sentence response and are resolved; the second review’s six required actions are complete. GitHub confirms `isDraft: false` and `mergeable: MERGEABLE`; the stale draft/pending PR description is replaced. [Merged-tree CI 34849295399](https://github.com/msetaro/aftershock/actions/runs/34849295399) passes all 14 active builds at `aec9fb49082534fb5b01711086112de45b6f6f77`, including MSVC x64/ARM64 Debug/Release and both MinGW builds; the final checkpoint commit changes documentation only. Evidence branch and implementation branch are published separately, with no main push or history rewrite. Recheck CI using `gh run watch 34849295399 -R msetaro/aftershock --exit-status` and `gh run view 34849295399 -R msetaro/aftershock --log-failed`.

Merged `origin/main` at `841b35d5` in `cc114e79`; full fresh C (main) and C++ dedicated/client/both dlopen renderer builds pass. Generator fix `6ef22a39` regenerates all 74 arrays byte-identically (SHA256 `700724298e78e98017adeeceb34b56af6243fe6cdbdce94da60751b1fb187367`), including a separate `_size` linkage fixture for the existing disabled emission path. Committed SPIR-V bytes were reused because glslangValidator is unavailable; no shader compilation or bytecode substitution is claimed. T25 cleanup `80763bd2` passes all eight GCC/Clang release/debug client/ded comparisons: release hashes match raw; debug hashes match after `objcopy --strip-debug`. Full before/after hashes are in the small acceptance JSON and archived `pr32-cleanup-results.json`; G2/G3 on sv_client pass and G4 remains advisory.

Runtime acceptance was rerun locally after cleanup using two warmups and two measured runs per language at the identical executable path, following C-repeat, C++-repeat, then C-vs-C++ order. All four normalized logs contain 123 identical lines and hash `00446177640cfac27f9018572860cbf9618930bf8d38b52184e19d74dade0a01`. Only the permitted cached-paks line is removed; the working-directory line is identical because the execution path is shared. This path differs from the earlier phase-3 test, explaining its different overall log hash.

| Run | Raw SHA256 |
|---|---|
| c 1 | `e9eb578d11d6c6a0ead10379b603d13c9bf12a7330796e3b63a5270056127cf6` |
| c 2 | `b41fb28770659389102a03d97b777dc5e9ecd8929bd2319625e9736732d4fa8b` |
| cxx 1 | `e9eb578d11d6c6a0ead10379b603d13c9bf12a7330796e3b63a5270056127cf6` |
| cxx 2 | `e9eb578d11d6c6a0ead10379b603d13c9bf12a7330796e3b63a5270056127cf6` |

Whole-binary `nm -g` comparison preserves symbol kind and version suffix, demangles names and removes function parameter lists: **473 C defined globals = 473 C++ defined globals**, identical sets. Undefined imports differ exactly as the maintainer observed:

| C only | C++ only | Accepted explanation / retained G4 class |
|---|---|---|
| `ceilf@GLIBC_2.2.5`, `floorf@GLIBC_2.2.5` | none | GCC folds C float-result/double-input calls to float libm; C++ uses inlined double SSE operations, exact for these float inputs. |
| `__ctype_b_loc@GLIBC_2.3`, `__ctype_tolower_loc@GLIBC_2.3` | `isspace@GLIBC_2.2.5`, `tolower@GLIBC_2.2.5` | glibc C macros versus C++ function declarations, same tables. |
| `__stpcpy_chk@GLIBC_2.3.4` | none | Different fortify wrapper selection. |

No f-suffixed libm import appears only in C++. This production-binary comparison supplements the controlled per-object G3 checks; it does not silently waive any new G3 failure.

The maintainer independently reported the same 473-symbol comparison and repeated 123-line runtime trace (28 item pickups, five kills, same order) in [review 5197874004](https://github.com/msetaro/aftershock/pull/32#pullrequestreview-5197874004), with only the working-directory path differing between their installations. That review also reports Vulkan q3dm17/HUD rendering on RTX 3080 Ti and OpenGL menu rendering on llvmpipe, with screenshots from both; those GPU observations are the maintainer's, not this headless follow-up's tests.

Exact reproduction (first extract the evidence archive as above; game data remains in ~/.q3a/baseq3):

```sh
export SOURCE_DATE_EPOCH=1789257600 LC_ALL=C
mkdir -p /tmp/port-pr32/main-c
git archive 841b35d5faf0c2a09b8c0d0e746b92a20dd36e97 | tar -x -C /tmp/port-pr32/main-c
make -C /tmp/port-pr32/main-c -j10 BUILD_DIR=/tmp/port-pr32/build-c
make -j10 BUILD_DIR=/tmp/port-pr32/build-cxx
python3 tools/port/check_shader_generator.py /tmp/port-pr32/shaders
python3 tools/port/reproduce_t25_cleanup.py "$PWD" /tmp/port-pr32/cleanup-reproduction
python3 "$PORT_EVIDENCE/pr32-acceptance.py"
```

The last script copies each executable to `/tmp/port-pr32/runtime/quake3e.ded.x64` in sequence, performs two warmups and two measured runs, asserts repeat/equivalence checks, and writes raw/normalized logs, sorted defined/undefined symbol lists and `acceptance-results.json` under `/tmp/port-pr32`. The exact runtime command is:

```sh
timeout 90 faketime -f "@2026-01-01 00:00:00 i0.01" /tmp/port-pr32/runtime/quake3e.ded.x64 \
  +set dedicated 1 +set sv_pure 0 +set com_logfile 0 +map q3dm17 \
  +addbot sarge 3 +addbot major 3 +wait 300 +quit
```

For each binary the script runs `nm -g --defined-only <binary> | c++filt` and `nm -g --undefined-only <binary> | c++filt`, removes only parameter lists with `re.sub(r'\(.*\)', '', name)`, sorts kind/name pairs, asserts equality of defined sets and records both undefined set differences. Compare its emitted lists with:

```sh
diff -u /tmp/port-pr32/symbols-c-defined.txt /tmp/port-pr32/symbols-cxx-defined.txt
diff -u /tmp/port-pr32/symbols-c-undefined.txt /tmp/port-pr32/symbols-cxx-undefined.txt
```

Review dispositions: required generator/T25 changes are implemented; T15/T21 wording and versioned-compiler instructions are corrected. Existing inner `(int)` casts in SDL and printf were present in original `8a7e8ed2`, so they are retained. Fresh original C hashes confirm cl_cgame.o `6e11f30cf2da5013a32f3ed871b30fb853a3456848e6b729456f887663683536` and both sv_game.o contexts `a6fedc0c9eb7f9d232bbf5b90c38cd79485fb9b9126b8bbb7866e32af0599086`; only the new `(char *)VMA(1)` casts are T1. C linkage on assembly/GPU variables remains necessary for MSVC raw names; function typedef linkage documents approved external boundaries. T21 M_PI casts remain necessary for the float fallback in q_shared.h on supported configurations. Huffman T18 preserves original C external linkage. Both CINTERFACE definitions deliberately cover early SDK inclusion and header consumers; __clear_cache already has the requested target guard. The advisory C++ probe was already removed in phase 3. Detailed fixtures are archived as `pr32-nit-review.json`.

Independent G8 review reran both new verification tools successfully. Its stale inventory wording findings are corrected, and cross_gates now rejects an empty manifest/inventory match; an extracted real md4 manifest entry passes compilation/G2/G3/G4 and an empty manifest fails explicitly.

## Decisions, scope and harness details

- Only GNU Make is supported; plan section 7 supersedes early CMake references. Build mode uses C++20, no exceptions/RTTI or permissive flag; vendors remain C, assembly unchanged, renderer2 removed from the supported build in phase 3. Single objects and make -k work. CXX derives from the same compiler prefix as CC, respecting explicit CXX. SOURCE_DATE_EPOCH=1789257600 stabilizes date/time macros for raw hashes.
- T1 preserves exact existing destination types/calling conventions, including function/object pointers. T2 consistently casts to qboolean. T3 preserves the original promoted expression; no offset/operand reorder. T19 moves the SDL console-key enum intact. T20 only changes receiving locals/casts. T23 guards the pre-defined GNU feature macro. T18 adds one huffman table, codec const linkage, and 74 exact shader-array extern declarations without changing data.
- T21 adds 364 net double argument casts: 347 reviewed native casts, six conditional common.c rint calls, and eleven MSVC/fallback-M_PI sites. Only the original complete argument expression is wrapped; no FP expression is restructured. The first q_math edit briefly had seven redundant nested casts, removed in a subsequent commit without history rewrite. Native review of all 98 abs calls found no non-integer argument, so no T22 changes were needed. Two fabs calls inside #if 0 remain untouched. Dormant platform/SDK branches outside verified configurations are not claimed as compiled.
- T4 renderer field renames were atomic prerequisites: all existing uses were renamed together to keep C compiling, with identical C hashes, followed by per-file commits. No strings/comments were renamed. Phase 2 removed C linkage from six static callbacks in three files, syscall_t/dllSyscall_t, and static-renderer GetRefAPI. Kept the listed DLL typedefs, dlopen exports, assembly boundaries and GPU exports. T11 __clear_cache is an existing libgcc import, correctly declared void(void*,void*) with C linkage in vm_local.h for ARM/AArch64; target compiler ABI and unchanged C hashes were checked.
- G2 uses actual engine DWARF and pahole sizes/offsets/alignment/nested members, excluding system/vendor types by declaration source (never by intersecting results). MinGW COFF DWARF relocations are resolved in a temporary PE carrier and converted to ELF; dummy undefined definitions exist only in this never-executed layout carrier. G3 always inspects original objects.
- G3 uses separate -O2 probe objects with -fno-builtin, -fno-inline-functions, -D__NO_CTYPE=1 and -U_FORTIFY_SOURCE. GCC probes also disable small/called-once inlining and IPA scalar replacement. MinGW -Wa,-L retains actual local C functions beginning L. Exact ARM/AArch64 local instruction/data mapping markers are metadata. All other source-defined and undefined symbols are retained/demangled, preserving static/global kind. GetRefAPI stays raw for dlopen (both compiler commands must agree); absent metadata conservatively enforces raw names. Controls catch static/export changes, missing assembly C linkage and newly selected float libm calls. Plain -fno-inline was rejected because it materializes integer math template helpers.
- G2/G4/G5 and production builds retain their own real optimization flags. MinGW default -flto gives serialized IR with unstable build IDs even for untouched md4, so object oracles use matching -O2 -ffast-math -fno-lto on base/current. Default-LTO dedicated C links were separately verified. Inspection artifacts disable LTO so actual code/DWARF can be compared. No production flags were changed to make gates pass.
- All requested cross compilers are installed and used: x86_64-w64-mingw32-g++ (GCC 13), aarch64-linux-gnu-g++, arm-linux-gnueabihf-g++, powerpc64le-linux-gnu-g++ (GCC 15.2). ARM explicitly uses LONG_BIT=32. 32-bit x86 is excluded and its CI legs are removed. MSVC x64/ARM64 Debug/Release now pass. No system packages were installed.
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

| deprecated-declarations | existing Apple SDK sprintf sites | 2 per initial macOS CI leg | Darwin only |
| maybe-uninitialized | 1 instrumented GCC C warning | 1 at the same beststart site | GCC sanitizer builds only |

MSVC mirrors the accepted write-strings policy with `/Zc:strictStrings-`; initial botlib CI observed 136 unique Debug / 118 unique Release literal-conversion errors per architecture.

## Every DEVIATION

- `5a28cd29 DEVIATION: preserve source style despite formatter threshold`: 16 formatter trials could not reproduce mixed original style below 3% (cvar 25.549%, cl_main 17.852%). No engine reformat. Accepted continuation makes the formatter advisory; this historical threshold blocker is resolved.
- `dc031ab7 DEVIATION: freeze observed legacy string-literal warnings`: froze 243 observed write-strings warnings rather than change signatures/behavior. Accepted continuation folds this into the ordinary frozen list; historical commit retained.
- `7553c037 DEVIATION: freeze observed parenthesized-declarator warnings`: froze 48 existing macro-declaration warnings instead of rewriting source. Also accepted as ordinary frozen entry; historical commit retained.
- `8d827a6d DEVIATION: freeze observed Clang compatibility warnings`: added Clang-only unused-function/varargs suppressions after fresh matrix reached unchanged HasFCOM/CURLoption sites. Counts above; retained varargs hazard is logged. No engine fixes, public-signature changes or C flag changes.
- `74233370 DEVIATION: retain required T15 lexical spaces in diff check`: the literal -w heuristic erases required C++ literal/macro token separators. Per-file minimal stats agree in 132/134 files. common.c is 38/38 versus 16/16 and snd_dma.c 2/2 versus 1/1; all 23 omitted lines are T15, not formatting cleanup. Retain the explicitly authorized lexical changes; do not add fake substantive tokens to game the metric. Exact diff: `g8-stat.diff`. No engine content changed in the decision commit.

- `c2705708 DEVIATION: update embedded shader path for rename`: byte-identical shader-data move plus one include and two generator output paths; necessary to keep the main 170-file rename content-free. C hashes and native/non-SDL/MinGW consumer gates pass.
- `d56244f2 DEVIATION: freeze observed Apple SDK deprecation warnings`: platform-only existing sprintf diagnostics, no API substitution.
- `47f79834 DEVIATION: freeze existing sanitizer uninitialized warning`: only the preexisting GCC instrumentation diagnostic; later narrowed to GCC by G8. No initialization/control-flow fix.

## Bugs and compatibility hazards logged, not fixed

Full ledger: `docs/cpp-port-notes.md`. It records upstream CMake defects; preexisting unaligned unzip/vm accesses and sanitizer suppression limits; legacy linux_snd pthread signature mismatch; cl_curl's terminating-NUL slash test; FS_AllowedExtension's NULL relational comparison; the retained CURLoption va_start warning; and the possible uninitialized beststart path in AAS_Reachability_JumpArea (same original C sanitizer diagnostic). The original float-math overload hazard is resolved through T21, and the unfaked bot nondeterminism is controlled by the accepted faketime test. No unrelated source behavior was fixed.

## Exact reproduction commands

Use the same recorded compiler versions for raw object hashes. Fresh build directories prevent stale pre-rename C objects. C pair artifacts retain `.c.o` as a **language tag**; current engine source names are `.cpp`.

```sh
export SOURCE_DATE_EPOCH=1789257600 LC_ALL=C
mkdir -p /tmp/port-c-oracle
git archive e49b82595c7b1a25c70d9e86b72b3ed147adbdb3 | tar -x -C /tmp/port-c-oracle
export PORT_C_ORACLE=/tmp/port-c-oracle
make -C "$PORT_C_ORACLE" -j20 BUILD_DIR=/tmp/port-c
make -j20 BUILD_DIR=/tmp/port-cxx
make -j20 BUILD_CLIENT=0 BUILD_DIR=/tmp/port-cxx
make -j20 BUILD_SERVER=0 USE_RENDERER_DLOPEN=0 RENDERER_DEFAULT=vulkan BUILD_DIR=/tmp/port-static-vulkan

tools/port/selfcheck.sh
python3 tools/port/cross_gates.py /tmp/port-native-gates native
python3 tools/port/cross_gates.py /tmp/port-cross-gates
python3 tools/port/build_matrix.py /tmp/port-matrix
python3 tools/port/static_gate.py /tmp/port-static-analysis
python3 tools/port/differential_gate.py /tmp/port-g5
tools/port/math_gate.sh
python3 tools/port/compile_pair.py ded/q_math.o /tmp/port-qmath
tools/port/layout_gate.sh /tmp/port-qmath/q_math.c.o /tmp/port-qmath/q_math.cxx.o
tools/port/symbol_gate.sh /tmp/port-qmath/q_math.c.sym.o /tmp/port-qmath/q_math.cxx.sym.o
tools/port/codegen_gate.sh /tmp/port-qmath/q_math.c.s /tmp/port-qmath/q_math.cxx.s

command -v x86_64-w64-mingw32-g++ aarch64-linux-gnu-g++ arm-linux-gnueabihf-g++ powerpc64le-linux-gnu-g++
make -j20 PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 USE_CURL=0 'OPTIMIZE=-O2 -ffast-math -fno-lto' BUILD_DIR=/tmp/port-mingw
make -j20 BUILD_CLIENT=0 ARCH=aarch64 CC=aarch64-linux-gnu-gcc BUILD_DIR=/tmp/port-aarch64
make -j20 BUILD_CLIENT=0 ARCH=arm LONG_BIT=32 CC=arm-linux-gnueabihf-gcc BUILD_DIR=/tmp/port-arm
make -j20 BUILD_CLIENT=0 ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc BUILD_DIR=/tmp/port-ppc
```

Every matrix/pair command and working directory is in the JSON inventories or neighboring `.command` / `.cwd` files. G4 exit 1 means retained advisory diff; G2/G3 require exit 0. Do not unset PORT_C_ORACLE after rename: the harness rejects a C++ recipe passed as the C oracle.

```sh
mkdir -p /tmp/port-runtime
# Same installation path avoids the working-directory banner differing.
for port_mode in c cxx; do
  cp "/tmp/port-$port_mode/release-linux-x86_64/quake3e.ded.x64" /tmp/port-runtime/quake3e.ded.x64
  for port_run in warm1 warm2 1 2; do
    timeout 90 faketime -f "@2026-01-01 00:00:00 i0.01" /tmp/port-runtime/quake3e.ded.x64 \
      +set dedicated 1 +set sv_pure 0 +set com_logfile 0 +map q3dm17 \
      +addbot sarge 3 +addbot major 3 +wait 300 +quit > "/tmp/port-$port_mode-$port_run.log" 2>&1
    sed '/^\.\.\.found [0-9][0-9]* cached paks$/d' "/tmp/port-$port_mode-$port_run.log" > "/tmp/port-$port_mode-$port_run.normalized.log"
  done
done
diff -u /tmp/port-c-1.normalized.log /tmp/port-c-2.normalized.log
diff -u /tmp/port-c-2.normalized.log /tmp/port-cxx-1.normalized.log
sha256sum /tmp/port-c-{1,2}.log /tmp/port-cxx-1.log /tmp/port-*.normalized.log
for port_renderer in opengl vulkan; do
  timeout 90 xvfb-run -a /tmp/port-cxx/release-linux-x86_64/quake3e.x64 \
    +set cl_renderer "$port_renderer" +set r_fullscreen 0 +set r_mode 3 \
    +set s_initsound 0 +set com_introplayed 1 +map q3dm17 +wait 20 +quit
done
make -j20 BUILD_CLIENT=0 BUILD_DIR=/tmp/port-sanitize \
  'CFLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer' 'LDFLAGS=-fsanitize=address,undefined'
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS="suppressions=$PWD/tools/port/ubsan.supp:halt_on_error=1" \
  timeout 90 /tmp/port-sanitize/release-linux-x86_64/quake3e.ded.x64 \
  +set dedicated 1 +set sv_pure 0 +set com_logfile 0 +map q3dm17 \
  +addbot sarge 3 +addbot major 3 +wait 300 +quit
```

CI is explicitly dispatched on the feature branch; branch dispatch cannot publish the public latest release. Always pass the fork repository because gh otherwise selects upstream.

```sh
gh workflow run build.yml -R msetaro/aftershock --ref t3code/port-engine-to-cpp20
gh run watch RUN_ID -R msetaro/aftershock --interval 20 --exit-status
gh run view RUN_ID -R msetaro/aftershock --log-failed
```

## Per-file status

All 257 scoped .c/.h entries appear exactly once in this table. Native status refers to actual compiler contexts; header verification follows consumers. Repeated contexts and complete G4 results are in the JSON inventories/index. Earlier per-file commits preserve original transformation counts where subsequent linkage/math passes updated the row.

| File | Status | Transformations and verification |
|---|---|---|
| `code/asm/qasm.h` | done | Not applicable: included only by assembly .s files; explicitly outside C++ inputs. No rename. |
| `code/botlib/aasfile.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_bsp.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_bspq3.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_bspq3.o); G4 advisory difference retained. |
| `code/botlib/be_aas_cluster.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_cluster.o); G4 advisory difference retained. |
| `code/botlib/be_aas_cluster.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_debug.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_debug.o); G4 advisory difference retained. |
| `code/botlib/be_aas_debug.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_def.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_entity.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_entity.o); G4 PASS. |
| `code/botlib/be_aas_entity.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_file.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_file.o); G4 advisory difference retained. |
| `code/botlib/be_aas_file.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_funcs.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_main.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_main.o); G4 PASS. |
| `code/botlib/be_aas_main.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_move.cpp` | done | T1-T17: 0 (already compatible); T21/T22: 3 argument casts at 3 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_move.o, default); G4 PASS. |
| `code/botlib/be_aas_move.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_optimize.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_optimize.o); G4 PASS. |
| `code/botlib/be_aas_optimize.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_reach.cpp` | done | T1-T17: 0 (already compatible); T21: 15 argument casts at 15 calls (native and fallback M_PI); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_reach.o, default); G4 advisory difference retained. |
| `code/botlib/be_aas_reach.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_route.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_route.o); G4 advisory difference retained. |
| `code/botlib/be_aas_route.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_routealt.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_routealt.o); G4 advisory difference retained. |
| `code/botlib/be_aas_routealt.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_sample.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_sample.o); G4 advisory difference retained. |
| `code/botlib/be_aas_sample.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_char.cpp` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_char.o); G4 advisory difference retained. |
| `code/botlib/be_ai_char.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_char.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_chat.cpp` | done | T1: 4; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_chat.o); G4 advisory difference retained. |
| `code/botlib/be_ai_chat.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_chat.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_gen.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_gen.o); G4 PASS. |
| `code/botlib/be_ai_gen.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_gen.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_goal.cpp` | done | T1: 1, T16: 1 macro site (8 expanded casts); clang narrowing follow-up; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_goal.o, default); G4 PASS. |
| `code/botlib/be_ai_goal.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_goal.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_move.cpp` | done | T1: 1; T21/T22: 12 argument casts at 12 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_move.o, default); G4 advisory difference retained. |
| `code/botlib/be_ai_move.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_goal.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_weap.cpp` | done | T1: 1, T16: 2 macro sites (36 expanded casts); clang narrowing follow-up; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_weap.o, default); G4 PASS. |
| `code/botlib/be_ai_weap.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_weap.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_weight.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_weight.o); G4 PASS. |
| `code/botlib/be_ai_weight.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_goal.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ea.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ea.o); G4 PASS. |
| `code/botlib/be_ea.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_chat.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_interface.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_interface.o); G4 PASS. |
| `code/botlib/be_interface.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_debug.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/botlib.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_crc.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_crc.o); G4 PASS. |
| `code/botlib/l_crc.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_route.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_libvar.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_libvar.o); G4 PASS. |
| `code/botlib/l_libvar.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_cluster.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_log.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_log.o); G4 PASS. |
| `code/botlib/l_log.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_cluster.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_memory.cpp` | done | T1: 2; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_memory.o); G4 advisory difference retained. |
| `code/botlib/l_memory.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_precomp.cpp` | done | T1: 3 (one in inactive LoadSourceMemory), T4: 1 field (10 occurrences); T21/T22: 3 argument casts at 3 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_precomp.o, default); G4 advisory difference retained. |
| `code/botlib/l_precomp.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_script.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_script.o); G4 advisory difference retained. |
| `code/botlib/l_script.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_struct.cpp` | done | T3: 14; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_struct.o); G4 advisory difference retained. |
| `code/botlib/l_struct.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_utils.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_entity.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/cgame/cg_public.h` | done | T1-T17: 0; unchanged shared ABI header verified through cl_cgame.c actual dependency and native strict builds/G2/G3. |
| `code/client/cl_avi.cpp` | done | T1: 2; T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_avi.o, default); G4 advisory difference retained. |
| `code/client/cl_cgame.cpp` | done | T1: 116, T2: 1, T3: 6; T21/T22: 7 argument casts at 6 calls; T5 internal callback annotations removed; current C hashes/strict builds/G2/G3 PASS (client/cl_cgame.o); G4 advisory difference retained. |
| `code/client/cl_cin.cpp` | done | T1: 2, T2: 5; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_cin.o, default); G4 advisory difference retained. |
| `code/client/cl_console.cpp` | done | T1: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_console.o, default); G4 advisory difference retained. |
| `code/client/cl_curl.cpp` | done | T1: 35; T20: 1 receiving local const; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_curl.o, default); G4 advisory difference retained. |
| `code/client/cl_curl.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/cl_input.cpp` | done | T1-T17: 0 (already compatible); T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_input.o, default); G4 advisory difference retained. |
| `code/client/cl_jpeg.cpp` | done | T1: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_jpeg.o, default); G4 advisory difference retained. |
| `code/client/cl_keys.cpp` | done | T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_keys.o, default); G4 advisory difference retained. |
| `code/client/cl_main.cpp` | done | T1: 1; T2: 1; T3: 6 compound-assignment result casts; T20: 1 receiving local const; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_main.o, default); G4 advisory difference retained. |
| `code/client/cl_net_chan.cpp` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_net_chan.o, default); G4 advisory difference retained. |
| `code/client/cl_parse.cpp` | done | T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_parse.o, default); G4 advisory difference retained. |
| `code/client/cl_scrn.cpp` | done | T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_scrn.o, default); G4 advisory difference retained. |
| `code/client/cl_ui.cpp` | done | T1: 80, T2: 1, T3: 8; T21/T22: 7 argument casts at 6 calls; T5 internal callback annotations removed; current C hashes/strict builds/G2/G3 PASS (client/cl_ui.o); G4 advisory difference retained. |
| `code/client/client.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/keycodes.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/keys.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/snd_adpcm.cpp` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_adpcm.o, default); G4 PASS. |
| `code/client/snd_codec.cpp` | done | T1: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_codec.o, default); G4 PASS. |
| `code/client/snd_codec.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/snd_codec.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/snd_codec_ogg.cpp` | done | T1: 3; T18: 1 preceding extern const declaration; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_codec_ogg.o, default); G4 PASS. |
| `code/client/snd_codec_wav.cpp` | done | T1: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_codec_wav.o, default); G4 PASS. |
| `code/client/snd_dma.cpp` | done | T1: 1, T15: 2 (non-SDL branch); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_dma.o, default); G4 advisory difference retained. |
| `code/client/snd_local.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/snd_main.cpp` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_main.o, default); G4 PASS. |
| `code/client/snd_mem.cpp` | done | T1: 4; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_mem.o, default); G4 advisory difference retained. |
| `code/client/snd_mix.cpp` | done | T5: C linkage block for 3 assembly globals plus 5 assembly declarations/conditional definitions; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_mix.o, default); G4 advisory difference retained. |
| `code/client/snd_public.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/snd_wavelet.cpp` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_wavelet.o, default); G4 PASS. |
| `code/game/bg_public.h` | done | T1-T17: 0; unchanged shared ABI header verified through sv_game.c actual dependency and native strict builds/G2/G3. |
| `code/game/g_public.h` | done | T1-T17: 0; unchanged shared ABI header verified through sv_game.c actual dependency and native strict builds/G2/G3. |
| `code/qcommon/cm_load.cpp` | done | T1: 28; native ded C SHA256 unchanged (df42e0cabff475c22ebf8383e443cfe34d558abf817baf6f5cd8ad0c96700017); strict C++/G2/G3 PASS (ded/cm_load.o, default); G4 advisory difference retained. |
| `code/qcommon/cm_local.h` | done | T1-T17: 0; unchanged header checked via cm_load.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_patch.cpp` | done | T1: 3, T2: 6, T3: 3; T21/T22: 14 argument casts at 14 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_patch.o, default); G4 advisory difference retained. |
| `code/qcommon/cm_patch.h` | done | T1-T17: 0; unchanged header checked via cm_patch.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_polylib.cpp` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_polylib.o); G4 PASS. |
| `code/qcommon/cm_polylib.h` | done | T1-T17: 0; unchanged header checked via cm_polylib.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_public.h` | done | T1-T17: 0; unchanged header checked via cm_load.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_test.cpp` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_test.o); G4 PASS. |
| `code/qcommon/cm_trace.cpp` | done | T1-T17: 0 (already compatible); T21/T22: 6 argument casts at 6 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_trace.o, default); G4 advisory difference retained. |
| `code/qcommon/cmd.cpp` | done | T1: 1, T2: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cmd.o); G4 advisory difference retained. |
| `code/qcommon/common.cpp` | done | T1: 5, T2: 2, T3: 1, T15: 29; T5: 2 conditional MSVC CPUID_EX declarations/definitions; T21: six conditional rint calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/common.o, default); G4 advisory difference retained. |
| `code/qcommon/cvar.cpp` | done | T2: 2, T3: 4 (cast compound-assignment result); T21/T22: 2 argument casts at 2 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cvar.o, default); G4 advisory difference retained. |
| `code/qcommon/files.cpp` | done | T1: 13, T2: 2; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/files.o); G4 advisory difference retained. |
| `code/qcommon/history.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/history.o); G4 advisory difference retained. |
| `code/qcommon/huffman.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/huffman.o); G4 advisory difference retained. |
| `code/qcommon/huffman_static.cpp` | done | T18: one preceding extern const declaration; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/huffman_static.o, default); G4 PASS. |
| `code/qcommon/json.h` | done | Excluded: whole-tree include search finds only renderer2/tr_bsp.c; implementation is solely for excluded renderer2. Left unchanged. |
| `code/qcommon/keys.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/keys.o); G4 advisory difference retained. |
| `code/qcommon/md4.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/md4.o); G4 PASS. |
| `code/qcommon/md5.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/md5.o); G4 advisory difference retained. |
| `code/qcommon/msg.cpp` | done | T16: 3 sites (1 mask, 99 expanded field-offset casts); clang narrowing follow-up; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/msg.o, default); G4 advisory difference retained. |
| `code/qcommon/net_chan.cpp` | done | T1: 1, T4: 1 identifier (12 occurrences); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/net_chan.o); G4 advisory difference retained. |
| `code/qcommon/net_ip.cpp` | done | T1: 2, T2: 1; T1: 11 additional Winsock casts; native and MinGW C hashes, strict release/debug and G2/G3 PASS (ded/net_ip.o); G4 advisory difference retained. |
| `code/qcommon/puff.cpp` | done | T1-T17: 0 (already compatible); 3 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/puff.o); G4 advisory difference retained. |
| `code/qcommon/puff.h` | done | T1-T17: 0; unchanged header checked via puff.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/q_math.cpp` | done | T21: 20 argument casts at 18 calls, redundant cast correction; 4 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/q_math.o, default); G4 PASS. |
| `code/qcommon/q_platform.h` | done | T1-T17: 0; unchanged header checked via md4.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/q_shared.cpp` | done | T1: 4, T2: 3; 4 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/q_shared.o); G4 advisory difference retained. |
| `code/qcommon/q_shared.h` | done | T5: guarded Q_EXTERN_C macro plus 2 Windows assembly prototypes; native md4 consumer G2/G3 PASS and all 295 C hashes unchanged; Windows branch unverified. |
| `code/qcommon/qcommon.h` | done | T5 review removes internal-only annotations; 295/295 C object hashes unchanged; actual consuming objects G2/G3 PASS, G4 advisory evidence retained: ded/vm.o, client/cl_cgame.o. |
| `code/qcommon/qfiles.h` | done | T1-T17: 0; unchanged header checked via cm_load.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/surfaceflags.h` | done | T1-T17: 0; unchanged header checked via cm_load.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/unzip.cpp` | done | T1: 10, T14: 5; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/unzip.o); G4 advisory difference retained. |
| `code/qcommon/unzip.h` | done | T1-T17: 0; unchanged header checked via unzip.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/vm.cpp` | done | T1: 5; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/qvm/vm.o); G4 advisory difference retained. |
| `code/qcommon/vm_aarch64.cpp` | done | Additional MSVC ARM64 T1 VirtualAlloc cast; AArch64 pair passes; MSVC x64/ARM64 Debug/Release CI passes (34804759804).  T1: 2 allocator result casts; T11 cache prototype in vm_local.h; aarch64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (ded/qvm/vm_aarch64.o); G4 advisory difference retained. |
| `code/qcommon/vm_armv7l.cpp` | done | T5 four external libgcc assembly imports; arm original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (ded/qvm/vm_armv7l.o); G4 advisory difference retained. |
| `code/qcommon/vm_interpreted.cpp` | done | T1: 1; literal retained under frozen warning policy; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/qvm/vm_interpreted.o); G4 advisory difference retained. |
| `code/qcommon/vm_local.h` | done | Existing T4 edits; T11: correctly typed C-linkage declaration of existing GNU ARM runtime __clear_cache dependency. ARM/AArch64 65/65 C hashes unchanged; ARM vm_interpreted consumer strict C++/G2/G3 PASS; G4 advisory evidence cross-arm-vm_interpreted.codegen.diff.gz. |
| `code/qcommon/vm_optimize.h` | done | Unchanged; real native x86_64 and cross ARM/AArch64/PPC JIT consumers pass strict C++, G2/G3 and original C hashes. |
| `code/qcommon/vm_powerpc.cpp` | done | T1: 7 pointer conversions including debug-only callback; ppc64le original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (ded/qvm/vm_powerpc.o); G4 advisory difference retained. |
| `code/qcommon/vm_x86.cpp` | done | T1: 1 function-to-object pointer cast; T3: 1 macro_op_t cast; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/qvm/vm_x86.o, default); G4 advisory difference retained. |
| `code/renderer/iqm.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderer/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderer/qgl.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderer/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderer/tr_animation.cpp` | done | T1: 3, T2: 1, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_animation.o, default); G4 advisory difference retained. |
| `code/renderer/tr_arb.cpp` | done | T4: 3 occurrences (prerequisite), T2: 5, T3: 4, T17: 2; T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_arb.o, default); G4 advisory difference retained. |
| `code/renderer/tr_backend.cpp` | done | T4: 9 occurrences (prerequisite), T1: 4, T2: 2, T3: 3, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_backend.o, default); G4 advisory difference retained. |
| `code/renderer/tr_bsp.cpp` | done | T1: 42, T3: 2; T21/T22: 94 argument casts at 94 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_bsp.o, default); G4 advisory difference retained. |
| `code/renderer/tr_cmds.cpp` | done | T1: 11; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_cmds.o, default); G4 advisory difference retained. |
| `code/renderer/tr_common.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderer/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderer/tr_curve.cpp` | done | T1: 3; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_curve.o, default); G4 advisory difference retained. |
| `code/renderer/tr_flares.cpp` | done | T4: 3 occurrences (prerequisite), T2: 1; T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_flares.o, default); G4 advisory difference retained. |
| `code/renderer/tr_image.cpp` | done | T1: 9, T2: 2, T3: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image.o, default); G4 advisory difference retained. |
| `code/renderer/tr_init.cpp` | done | T1: 10, T2: 1, T3: 1, T5: dlopen definition only; T21: 1 fallback M_PI argument cast; static GetRefAPI C linkage removed; C hash/strict C++/G2/G3 PASS (rend1/tr_init.o); G4 advisory difference retained. |
| `code/renderer/tr_light.cpp` | done | T4: 10 occurrences (prerequisite); T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_light.o, default); G4 advisory difference retained. |
| `code/renderer/tr_local.h` | done | T4: 5 occurrences (prerequisite); actual consumer code/renderer/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderer/tr_main.cpp` | done | T4: 113 occurrences (prerequisite), T17: 2; T21: 8 argument casts at 8 calls (native and fallback M_PI); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_main.o, default); G4 advisory difference retained. |
| `code/renderer/tr_marks.cpp` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_marks.o, default); G4 PASS. |
| `code/renderer/tr_mesh.cpp` | done | T4: 4 occurrences (prerequisite), T1: 4, T2: 1, T17: 2; T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_mesh.o, default); G4 advisory difference retained. |
| `code/renderer/tr_model.cpp` | done | T1: 5; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_model.o, default); G4 advisory difference retained. |
| `code/renderer/tr_model_iqm.cpp` | done | T1: 4, T2: 2, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_model_iqm.o, default); G4 advisory difference retained. |
| `code/renderer/tr_scene.cpp` | done | T4: 4 occurrences (prerequisite), T1: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_scene.o, default); G4 advisory difference retained. |
| `code/renderer/tr_shade.cpp` | done | T4: 1 occurrence (prerequisite), T3: 1, T17: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_shade.o, default); G4 advisory difference retained. |
| `code/renderer/tr_shade_calc.cpp` | done | T4: 50 occurrences (prerequisite); T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_shade_calc.o, default); G4 advisory difference retained. |
| `code/renderer/tr_shader.cpp` | done | T1: 5, T3: 16, T16: 1, T17: 9; T21/T22: 5 argument casts at 5 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_shader.o, default); G4 advisory difference retained. |
| `code/renderer/tr_shadows.cpp` | done | T4: 4 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_shadows.o, default); G4 advisory difference retained. |
| `code/renderer/tr_sky.cpp` | done | T4: 7 occurrences (prerequisite); T21: 12 argument casts at 12 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_sky.o, default); G4 advisory difference retained. |
| `code/renderer/tr_surface.cpp` | done | T4: 25 occurrences (prerequisite); T21: 4 argument casts at 4 calls (native and fallback M_PI); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_surface.o, default); G4 advisory difference retained. |
| `code/renderer/tr_vbo.cpp` | done | T4: 3 occurrences (prerequisite), T1: 7, T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_vbo.o, default); G4 advisory difference retained. |
| `code/renderer/tr_world.cpp` | done | T4: 2 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_world.o, default); G4 advisory difference retained. |
| `code/renderercommon/tr_font.cpp` | done | T1: 1; dormant BUILD_FREETYPE body unverified (missing dependency); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_font.o, default); G4 PASS. |
| `code/renderercommon/tr_image_bmp.cpp` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image_bmp.o, default); G4 PASS. |
| `code/renderercommon/tr_image_jpg.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image_jpg.o, default); G4 PASS. |
| `code/renderercommon/tr_image_pcx.cpp` | done | T1: 2; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image_pcx.o, default); G4 PASS. |
| `code/renderercommon/tr_image_png.cpp` | done | T1: 18; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image_png.o, default); G4 PASS. |
| `code/renderercommon/tr_image_tga.cpp` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image_tga.o, default); G4 advisory difference retained. |
| `code/renderercommon/tr_noise.cpp` | done | T1-T17: 0 (already compatible); T21/T22: 3 argument casts at 3 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_noise.o, default); G4 advisory difference retained. |
| `code/renderercommon/tr_public.h` | done | T5 review removes internal-only annotations; 295/295 C object hashes unchanged; actual consuming objects G2/G3 PASS, G4 advisory evidence retained: rend1/tr_init.o, rendv/tr_init.o. |
| `code/renderercommon/tr_types.h` | done | T1-T17: 0; unchanged header verified through code/renderercommon/tr_font.c, strict native builds/G2/G3 PASS. |
| `code/renderercommon/vulkan/vk_platform.h` | done | T1-T17: 0; unchanged generated Khronos header; C and strict C++20 Xlib/Xrandr header fixture G2/G3 PASS. |
| `code/renderercommon/vulkan/vulkan.h` | done | T1-T17: 0; unchanged generated Khronos header; C and strict C++20 Xlib/Xrandr header fixture G2/G3 PASS. |
| `code/renderercommon/vulkan/vulkan_core.h` | done | T1-T17: 0; unchanged generated Khronos header; C and strict C++20 Xlib/Xrandr header fixture G2/G3 PASS. |
| `code/renderercommon/vulkan/vulkan_win32.h` | done | Unchanged Khronos header; actual MinGW win_qvk.c consumer passes C hash, strict release/debug C++ and G2/G3. |
| `code/renderercommon/vulkan/vulkan_xlib.h` | done | T1-T17: 0; unchanged generated Khronos header; C and strict C++20 Xlib/Xrandr header fixture G2/G3 PASS. |
| `code/renderercommon/vulkan/vulkan_xlib_xrandr.h` | done | T1-T17: 0; unchanged generated Khronos header; C and strict C++20 Xlib/Xrandr header fixture G2/G3 PASS. |
| `code/renderervk/iqm.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderervk/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderervk/shaders/bin2hex.cpp` | done | PR32 T18 array/_size declaration emission and byte-identical 74-array regeneration verified.  Standalone build utility, not linked engine code; original pre-review verification: gcc/g++ strict native -O2 compile PASS, fixed 259-byte input and append output byte-identical. Engine layout gate not applicable (no engine records). |
| `code/renderervk/shaders/spirv/shader_data.cpp` | done | T18: 74 preceding extern const declarations; unchanged initialized bytes. Verified through sole consumer rendv/vk.o: C SHA256 unchanged, strict release/debug C++, G2/G3 PASS; G4 advisory diff retained under vk.c. vk.c and this include require each other for C++ gates; consecutive per-file commits record the pair. |
| `code/renderervk/tr_animation.cpp` | done | T1: 3, T2: 1, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_animation.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_backend.cpp` | done | T4: 11 occurrences (prerequisite), T1: 4, T3: 3, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_backend.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_bsp.cpp` | done | T1: 42, T3: 2; T21/T22: 94 argument casts at 94 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_bsp.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_cmds.cpp` | done | T1: 6; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_cmds.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_common.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderervk/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderervk/tr_curve.cpp` | done | T1: 3; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_curve.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_image.cpp` | done | T1: 8, T2: 1, T3: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_image.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_init.cpp` | done | T1: 5, T5: dlopen definition only; T21: 1 fallback M_PI argument cast; static GetRefAPI C linkage removed; C hash/strict C++/G2/G3 PASS (rendv/tr_init.o); G4 advisory difference retained. |
| `code/renderervk/tr_light.cpp` | done | T4: 10 occurrences (prerequisite); T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_light.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_local.h` | done | T4: 5 occurrences (prerequisite); actual consumer code/renderervk/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderervk/tr_main.cpp` | done | T4: 113 occurrences (prerequisite), T17: 2; T21: 8 argument casts at 8 calls (native and fallback M_PI); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_main.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_marks.cpp` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_marks.o, default); G4 PASS. |
| `code/renderervk/tr_mesh.cpp` | done | T4: 4 occurrences (prerequisite), T1: 4, T2: 1, T17: 2; T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_mesh.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_model.cpp` | done | T1: 5; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_model.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_model_iqm.cpp` | done | T1: 4, T2: 2, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_model_iqm.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_scene.cpp` | done | T4: 4 occurrences (prerequisite), T1: 2, T3: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_scene.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_shade.cpp` | done | T4: 4 occurrences (prerequisite), T2: 1, T17: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_shade.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_shade_calc.cpp` | done | T4: 50 occurrences (prerequisite); T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_shade_calc.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_shader.cpp` | done | T1: 5, T3: 17, T16: 1, T17: 9; T21/T22: 5 argument casts at 5 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_shader.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_shadows.cpp` | done | T4: 4 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_shadows.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_sky.cpp` | done | T4: 7 occurrences (prerequisite); T21/T22: 12 argument casts at 12 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_sky.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_surface.cpp` | done | T4: 25 occurrences (prerequisite); T21: 4 argument casts at 4 calls (native and fallback M_PI); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_surface.o, default); G4 advisory difference retained. |
| `code/renderervk/tr_world.cpp` | done | T4: 2 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_world.o, default); G4 advisory difference retained. |
| `code/renderervk/vk.cpp` | done | Filename dependency update for embedded shader rename; separate DEVIATION; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/vk.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/vk.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderervk/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderervk/vk_flares.cpp` | done | T4: 3 occurrences (prerequisite); T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/vk_flares.o, default); G4 advisory difference retained. |
| `code/renderervk/vk_vbo.cpp` | done | T1: 6; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/vk_vbo.o, default); G4 advisory difference retained. |
| `code/sdl/sdl_gamma.cpp` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/sdl_gamma.o, default); G4 PASS. |
| `code/sdl/sdl_glimp.cpp` | done | T1: 4; T3: 5; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/sdl_glimp.o, default); G4 PASS. |
| `code/sdl/sdl_glw.h` | done | T1-T17: 0; unchanged header verified through sdl_gamma.c; native strict builds and G2/G3 PASS. |
| `code/sdl/sdl_icon.h` | done | Unchanged initializer; sdl_glimp.c consuming object passes original C hash, strict C++ and G2/G3. |
| `code/sdl/sdl_input.cpp` | done | T19: 1 enum hoisted with original body indentation; T3: 25 keyNum_t casts; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/sdl_input.o, default); G4 advisory difference retained. |
| `code/sdl/sdl_snd.cpp` | done | T1: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/sdl_snd.o, default); G4 PASS. |
| `code/server/server.h` | done | T1-T17: 0; unchanged header verified through server consumers, strict native release/debug and G2/G3 PASS. |
| `code/server/sv_bot.cpp` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_bot.o); G4 advisory difference retained. |
| `code/server/sv_ccmds.cpp` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_ccmds.o); G4 advisory difference retained. |
| `code/server/sv_client.cpp` | done | Port/runtime and approved T25 cleanup pass.  T1: 2; T2: 4; T20: 1; T25: 1 preserved original C compound line before rename; approved post-rename cleanup now removes that inactive branch; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_client.o, default); G4 advisory FAIL, full diff retained. |
| `code/server/sv_filter.cpp` | done | T3: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_filter.o); G4 advisory difference retained. |
| `code/server/sv_game.cpp` | done | T1: 199, T3: 7; T21/T22: 7 argument casts at 6 calls; T5 internal callback annotations removed; current C hashes/strict builds/G2/G3 PASS (ded/sv_game.o); G4 advisory difference retained. |
| `code/server/sv_init.cpp` | done | T1: 4; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_init.o); G4 advisory difference retained. |
| `code/server/sv_main.cpp` | done | T3: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_main.o); G4 advisory difference retained. |
| `code/server/sv_net_chan.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_net_chan.o); G4 advisory difference retained. |
| `code/server/sv_rankings.c` | done | Excluded by accepted scope: never built, proprietary rankings SDK; unchanged and must not be renamed. |
| `code/server/sv_snapshot.cpp` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_snapshot.o); G4 advisory difference retained. |
| `code/server/sv_world.cpp` | done | T3: 2 (includes bitwise assignment result); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_world.o); G4 advisory difference retained. |
| `code/server/tlds.h` | done | Unchanged initializer; T25 unblocks actual dedicated and client sv_client consumers. Both C hashes unchanged; strict C++ and G2/G3 PASS. |
| `code/ui/ui_public.h` | done | T1-T17: 0; unchanged shared ABI header verified through cl_ui.c actual dependency and native strict builds/G2/G3. |
| `code/unix/linux_glimp.cpp` | done | T1: 2 sites (4 expanded casts), T2: 3, T3: 2, T4: 1 identifier (3 occurrences); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/linux_glimp.o, nosdl); G4 advisory difference retained. |
| `code/unix/linux_joystick.cpp` | done | T1-T17: 0; dormant USE_JOYSTICK body explicitly compiled; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/linux_joystick.o, nosdl); G4 advisory difference retained. |
| `code/unix/linux_local.h` | done | T1-T17: 0; unchanged header verified through unix_main.c and linux_glimp.c; native strict builds and G2/G3 PASS. |
| `code/unix/linux_qgl.cpp` | done | T1: 1 macro site (6 expanded casts); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/linux_qgl.o, nosdl); G4 advisory difference retained. |
| `code/unix/linux_qvk.cpp` | done | T1: 1 function-to-object pointer return cast; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/linux_qvk.o, nosdl); G4 PASS. |
| `code/unix/linux_signals.cpp` | done | T1-T17: 0; renderer header T4 prerequisite resolves prior blocker; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/linux_signals.o, default); G4 PASS. |
| `code/unix/linux_snd.cpp` | done | T1: 5; original thread-function casts retained inside typed casts; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/linux_snd.o, nosdl); G4 advisory difference retained. |
| `code/unix/unix_glw.h` | done | T1-T17: 0; unchanged header verified through non-SDL linux_glimp.c, linux_qgl.c and X11 extensions; G2/G3 PASS. |
| `code/unix/unix_main.cpp` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/unix_main.o, default); G4 advisory difference retained. |
| `code/unix/unix_shared.cpp` | done | T1: 1; T23: 1 feature-test guard; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/unix_shared.o, default); G4 advisory difference retained. |
| `code/unix/x11_dga.cpp` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/x11_dga.o, nosdl); G4 PASS. |
| `code/unix/x11_randr.cpp` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/x11_randr.o, nosdl); G4 PASS. |
| `code/unix/x11_vidmode.cpp` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/x11_vidmode.o, nosdl); G4 PASS. |
| `code/win32/glw_win.h` | done | Unchanged; actual MinGW win_glimp.c and win_qgl.c consumers pass C hashes, strict release/debug C++ and G2/G3. |
| `code/win32/resource.h` | done | Unchanged; actual MinGW win_main.c and win_syscon.c consumers pass C hashes, strict release/debug C++ and G2/G3. |
| `code/win32/win_gamma.cpp` | done | T1: 1 previously inspected HMODULE cast, now verified; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_gamma.o); G4 PASS. |
| `code/win32/win_glimp.cpp` | done | T1: 2 including optional procedure macro; T2: 4; T5: 2 GPU exports; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_glimp.o); G4 advisory difference retained. |
| `code/win32/win_input.cpp` | done | T24: 2 GUID arguments; T15: 2 required lexical spaces; prior T2 retained; CINTERFACE predeclared by Windows C++ compiler flags before transitive curl SDK headers; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_input.o); G4 PASS. Explicit USE_WASAPI=0/1: original C hashes, strict release/debug C++, G2/G3 PASS; G4 recorded. |
| `code/win32/win_local.h` | done | T24 exact approved macros before SDK includes; all 300 original MinGW C object hashes unchanged; win_main C++/G2/G3 PASS. |
| `code/win32/win_main.cpp` | done | T1: 4 total, including FARPROC to void*; existing inspection casts now C-oracle verified; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_main.o); G4 advisory difference retained. |
| `code/win32/win_minimize.cpp` | done | T1-T23: 0, already compatible; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_minimize.o); G4 advisory difference retained. |
| `code/win32/win_qgl.cpp` | done | T1: 3 sites including function-to-object pointer conversion; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_qgl.o); G4 PASS. |
| `code/win32/win_qvk.cpp` | done | T1: 4 sites including function-to-object pointer conversion; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_qvk.o); G4 PASS. |
| `code/win32/win_shared.cpp` | done | T1-T23: 0; default profile configuration; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_shared.o); G4 advisory difference retained. |
| `code/win32/win_snd.cpp` | done | T24: 8 outgoing GUID arguments and 2 incoming pointer uses; T4: this identifier to self; T1: 2 Lock casts plus prior loader casts; T18: 2 GUID const declarations; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_snd.o); G4 advisory difference retained. Explicit USE_WASAPI=0/1: original C hashes, strict release/debug C++, G2/G3 PASS; G4 recorded. |
| `code/win32/win_syscon.cpp` | done | T2: 1 previously inspected boolean toggle, now verified; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_syscon.o); G4 advisory difference retained. |
| `code/win32/win_wndproc.cpp` | done | T1: 2; T2: 2 previously inspected edits, now verified; mingw64 original C SHA256 unchanged; strict release/debug C++ and G2/G3 PASS (client/win_wndproc.o); G4 advisory difference retained. |

## Every retained current G4 difference

All files below are under tools/port/evidence; use `gzip -dc` to read a diff. Reproduce with the object and Make variables shown, PORT_C_ORACLE set as above, then codegen_gate.sh on the emitted assembly pair. G4 is advisory; G2/G3 pass. Older retained differences and their exact commands remain indexed in `tools/port/evidence/historical-g4-index.md` and the historical JSON results.

| Family | Target | Object | Make variables | Diff |
|---|---|---|---|---|
| native | native | `client/be_aas_bspq3.o` | `` | `phase3-native-native-client-be_aas_bspq3.diff.gz` |
| native | native | `client/be_aas_cluster.o` | `` | `phase3-native-native-client-be_aas_cluster.diff.gz` |
| native | native | `client/be_aas_debug.o` | `` | `phase3-native-native-client-be_aas_debug.diff.gz` |
| native | native | `client/be_aas_file.o` | `` | `phase3-native-native-client-be_aas_file.diff.gz` |
| native | native | `client/be_aas_reach.o` | `` | `phase3-native-native-client-be_aas_reach.diff.gz` |
| native | native | `client/be_aas_route.o` | `` | `phase3-native-native-client-be_aas_route.diff.gz` |
| native | native | `client/be_aas_routealt.o` | `` | `phase3-native-native-client-be_aas_routealt.diff.gz` |
| native | native | `client/be_aas_sample.o` | `` | `phase3-native-native-client-be_aas_sample.diff.gz` |
| native | native | `client/be_ai_char.o` | `` | `phase3-native-native-client-be_ai_char.diff.gz` |
| native | native | `client/be_ai_chat.o` | `` | `phase3-native-native-client-be_ai_chat.diff.gz` |
| native | native | `client/be_ai_move.o` | `` | `phase3-native-native-client-be_ai_move.diff.gz` |
| native | native | `client/cl_avi.o` | `` | `phase3-native-native-client-cl_avi.diff.gz` |
| native | native | `client/cl_cgame.o` | `` | `phase3-native-native-client-cl_cgame.diff.gz` |
| native | native | `client/cl_cin.o` | `` | `phase3-native-native-client-cl_cin.diff.gz` |
| native | native | `client/cl_console.o` | `` | `phase3-native-native-client-cl_console.diff.gz` |
| native | native | `client/cl_curl.o` | `` | `phase3-native-native-client-cl_curl.diff.gz` |
| native | native | `client/cl_input.o` | `` | `phase3-native-native-client-cl_input.diff.gz` |
| native | native | `client/cl_jpeg.o` | `` | `phase3-native-native-client-cl_jpeg.diff.gz` |
| native | native | `client/cl_keys.o` | `` | `phase3-native-native-client-cl_keys.diff.gz` |
| native | native | `client/cl_main.o` | `` | `phase3-native-native-client-cl_main.diff.gz` |
| native | native | `client/cl_net_chan.o` | `` | `phase3-native-native-client-cl_net_chan.diff.gz` |
| native | native | `client/cl_parse.o` | `` | `phase3-native-native-client-cl_parse.diff.gz` |
| native | native | `client/cl_scrn.o` | `` | `phase3-native-native-client-cl_scrn.diff.gz` |
| native | native | `client/cl_ui.o` | `` | `phase3-native-native-client-cl_ui.diff.gz` |
| native | native | `client/cm_load.o` | `` | `phase3-native-native-client-cm_load.diff.gz` |
| native | native | `client/cm_patch.o` | `` | `phase3-native-native-client-cm_patch.diff.gz` |
| native | native | `client/cm_trace.o` | `` | `phase3-native-native-client-cm_trace.diff.gz` |
| native | native | `client/cmd.o` | `` | `phase3-native-native-client-cmd.diff.gz` |
| native | native | `client/common.o` | `` | `phase3-native-native-client-common.diff.gz` |
| native | native | `client/cvar.o` | `` | `phase3-native-native-client-cvar.diff.gz` |
| native | native | `client/files.o` | `` | `phase3-native-native-client-files.diff.gz` |
| native | native | `client/history.o` | `` | `phase3-native-native-client-history.diff.gz` |
| native | native | `client/huffman.o` | `` | `phase3-native-native-client-huffman.diff.gz` |
| native | native | `client/keys.o` | `` | `phase3-native-native-client-keys.diff.gz` |
| native | native | `client/l_memory.o` | `` | `phase3-native-native-client-l_memory.diff.gz` |
| native | native | `client/l_precomp.o` | `` | `phase3-native-native-client-l_precomp.diff.gz` |
| native | native | `client/l_script.o` | `` | `phase3-native-native-client-l_script.diff.gz` |
| native | native | `client/l_struct.o` | `` | `phase3-native-native-client-l_struct.diff.gz` |
| native | native | `client/md5.o` | `` | `phase3-native-native-client-md5.diff.gz` |
| native | native | `client/msg.o` | `` | `phase3-native-native-client-msg.diff.gz` |
| native | native | `client/net_chan.o` | `` | `phase3-native-native-client-net_chan.diff.gz` |
| native | native | `client/net_ip.o` | `` | `phase3-native-native-client-net_ip.diff.gz` |
| native | native | `client/puff.o` | `` | `phase3-native-native-client-puff.diff.gz` |
| native | native | `client/q_shared.o` | `` | `phase3-native-native-client-q_shared.diff.gz` |
| native | native | `client/qvm/vm.o` | `` | `phase3-native-native-client-qvm-vm.diff.gz` |
| native | native | `client/qvm/vm_interpreted.o` | `` | `phase3-native-native-client-qvm-vm_interpreted.diff.gz` |
| native | native | `client/qvm/vm_x86.o` | `` | `phase3-native-native-client-qvm-vm_x86.diff.gz` |
| native | native | `client/sdl_input.o` | `` | `phase3-native-native-client-sdl_input.diff.gz` |
| native | native | `client/snd_dma.o` | `` | `phase3-native-native-client-snd_dma.diff.gz` |
| native | native | `client/snd_mem.o` | `` | `phase3-native-native-client-snd_mem.diff.gz` |
| native | native | `client/snd_mix.o` | `` | `phase3-native-native-client-snd_mix.diff.gz` |
| native | native | `client/sv_bot.o` | `` | `phase3-native-native-client-sv_bot.diff.gz` |
| native | native | `client/sv_ccmds.o` | `` | `phase3-native-native-client-sv_ccmds.diff.gz` |
| native | native | `client/sv_client.o` | `` | `phase3-native-native-client-sv_client.diff.gz` |
| native | native | `client/sv_filter.o` | `` | `phase3-native-native-client-sv_filter.diff.gz` |
| native | native | `client/sv_game.o` | `` | `phase3-native-native-client-sv_game.diff.gz` |
| native | native | `client/sv_init.o` | `` | `phase3-native-native-client-sv_init.diff.gz` |
| native | native | `client/sv_main.o` | `` | `phase3-native-native-client-sv_main.diff.gz` |
| native | native | `client/sv_net_chan.o` | `` | `phase3-native-native-client-sv_net_chan.diff.gz` |
| native | native | `client/sv_snapshot.o` | `` | `phase3-native-native-client-sv_snapshot.diff.gz` |
| native | native | `client/sv_world.o` | `` | `phase3-native-native-client-sv_world.diff.gz` |
| native | native | `client/unix_main.o` | `` | `phase3-native-native-client-unix_main.diff.gz` |
| native | native | `client/unix_shared.o` | `` | `phase3-native-native-client-unix_shared.diff.gz` |
| native | native | `client/unzip.o` | `` | `phase3-native-native-client-unzip.diff.gz` |
| native | native | `ded/be_aas_bspq3.o` | `` | `phase3-native-native-ded-be_aas_bspq3.diff.gz` |
| native | native | `ded/be_aas_cluster.o` | `` | `phase3-native-native-ded-be_aas_cluster.diff.gz` |
| native | native | `ded/be_aas_debug.o` | `` | `phase3-native-native-ded-be_aas_debug.diff.gz` |
| native | native | `ded/be_aas_file.o` | `` | `phase3-native-native-ded-be_aas_file.diff.gz` |
| native | native | `ded/be_aas_reach.o` | `` | `phase3-native-native-ded-be_aas_reach.diff.gz` |
| native | native | `ded/be_aas_route.o` | `` | `phase3-native-native-ded-be_aas_route.diff.gz` |
| native | native | `ded/be_aas_routealt.o` | `` | `phase3-native-native-ded-be_aas_routealt.diff.gz` |
| native | native | `ded/be_aas_sample.o` | `` | `phase3-native-native-ded-be_aas_sample.diff.gz` |
| native | native | `ded/be_ai_char.o` | `` | `phase3-native-native-ded-be_ai_char.diff.gz` |
| native | native | `ded/be_ai_chat.o` | `` | `phase3-native-native-ded-be_ai_chat.diff.gz` |
| native | native | `ded/be_ai_move.o` | `` | `phase3-native-native-ded-be_ai_move.diff.gz` |
| native | native | `ded/cm_load.o` | `` | `phase3-native-native-ded-cm_load.diff.gz` |
| native | native | `ded/cm_patch.o` | `` | `phase3-native-native-ded-cm_patch.diff.gz` |
| native | native | `ded/cm_trace.o` | `` | `phase3-native-native-ded-cm_trace.diff.gz` |
| native | native | `ded/cmd.o` | `` | `phase3-native-native-ded-cmd.diff.gz` |
| native | native | `ded/common.o` | `` | `phase3-native-native-ded-common.diff.gz` |
| native | native | `ded/cvar.o` | `` | `phase3-native-native-ded-cvar.diff.gz` |
| native | native | `ded/files.o` | `` | `phase3-native-native-ded-files.diff.gz` |
| native | native | `ded/history.o` | `` | `phase3-native-native-ded-history.diff.gz` |
| native | native | `ded/huffman.o` | `` | `phase3-native-native-ded-huffman.diff.gz` |
| native | native | `ded/keys.o` | `` | `phase3-native-native-ded-keys.diff.gz` |
| native | native | `ded/l_memory.o` | `` | `phase3-native-native-ded-l_memory.diff.gz` |
| native | native | `ded/l_precomp.o` | `` | `phase3-native-native-ded-l_precomp.diff.gz` |
| native | native | `ded/l_script.o` | `` | `phase3-native-native-ded-l_script.diff.gz` |
| native | native | `ded/l_struct.o` | `` | `phase3-native-native-ded-l_struct.diff.gz` |
| native | native | `ded/md5.o` | `` | `phase3-native-native-ded-md5.diff.gz` |
| native | native | `ded/msg.o` | `` | `phase3-native-native-ded-msg.diff.gz` |
| native | native | `ded/net_chan.o` | `` | `phase3-native-native-ded-net_chan.diff.gz` |
| native | native | `ded/net_ip.o` | `` | `phase3-native-native-ded-net_ip.diff.gz` |
| native | native | `ded/q_shared.o` | `` | `phase3-native-native-ded-q_shared.diff.gz` |
| native | native | `ded/qvm/vm.o` | `` | `phase3-native-native-ded-qvm-vm.diff.gz` |
| native | native | `ded/qvm/vm_interpreted.o` | `` | `phase3-native-native-ded-qvm-vm_interpreted.diff.gz` |
| native | native | `ded/qvm/vm_x86.o` | `` | `phase3-native-native-ded-qvm-vm_x86.diff.gz` |
| native | native | `ded/sv_bot.o` | `` | `phase3-native-native-ded-sv_bot.diff.gz` |
| native | native | `ded/sv_ccmds.o` | `` | `phase3-native-native-ded-sv_ccmds.diff.gz` |
| native | native | `ded/sv_client.o` | `` | `phase3-native-native-ded-sv_client.diff.gz` |
| native | native | `ded/sv_filter.o` | `` | `phase3-native-native-ded-sv_filter.diff.gz` |
| native | native | `ded/sv_game.o` | `` | `phase3-native-native-ded-sv_game.diff.gz` |
| native | native | `ded/sv_init.o` | `` | `phase3-native-native-ded-sv_init.diff.gz` |
| native | native | `ded/sv_main.o` | `` | `phase3-native-native-ded-sv_main.diff.gz` |
| native | native | `ded/sv_net_chan.o` | `` | `phase3-native-native-ded-sv_net_chan.diff.gz` |
| native | native | `ded/sv_snapshot.o` | `` | `phase3-native-native-ded-sv_snapshot.diff.gz` |
| native | native | `ded/sv_world.o` | `` | `phase3-native-native-ded-sv_world.diff.gz` |
| native | native | `ded/unix_main.o` | `` | `phase3-native-native-ded-unix_main.diff.gz` |
| native | native | `ded/unix_shared.o` | `` | `phase3-native-native-ded-unix_shared.diff.gz` |
| native | native | `ded/unzip.o` | `` | `phase3-native-native-ded-unzip.diff.gz` |
| native | native | `rend1/puff.o` | `` | `phase3-native-native-rend1-puff.diff.gz` |
| native | native | `rend1/q_shared.o` | `` | `phase3-native-native-rend1-q_shared.diff.gz` |
| native | native | `rend1/tr_animation.o` | `` | `phase3-native-native-rend1-tr_animation.diff.gz` |
| native | native | `rend1/tr_arb.o` | `` | `phase3-native-native-rend1-tr_arb.diff.gz` |
| native | native | `rend1/tr_backend.o` | `` | `phase3-native-native-rend1-tr_backend.diff.gz` |
| native | native | `rend1/tr_bsp.o` | `` | `phase3-native-native-rend1-tr_bsp.diff.gz` |
| native | native | `rend1/tr_cmds.o` | `` | `phase3-native-native-rend1-tr_cmds.diff.gz` |
| native | native | `rend1/tr_curve.o` | `` | `phase3-native-native-rend1-tr_curve.diff.gz` |
| native | native | `rend1/tr_flares.o` | `` | `phase3-native-native-rend1-tr_flares.diff.gz` |
| native | native | `rend1/tr_image.o` | `` | `phase3-native-native-rend1-tr_image.diff.gz` |
| native | native | `rend1/tr_image_tga.o` | `` | `phase3-native-native-rend1-tr_image_tga.diff.gz` |
| native | native | `rend1/tr_init.o` | `` | `phase3-native-native-rend1-tr_init.diff.gz` |
| native | native | `rend1/tr_light.o` | `` | `phase3-native-native-rend1-tr_light.diff.gz` |
| native | native | `rend1/tr_main.o` | `` | `phase3-native-native-rend1-tr_main.diff.gz` |
| native | native | `rend1/tr_mesh.o` | `` | `phase3-native-native-rend1-tr_mesh.diff.gz` |
| native | native | `rend1/tr_model.o` | `` | `phase3-native-native-rend1-tr_model.diff.gz` |
| native | native | `rend1/tr_model_iqm.o` | `` | `phase3-native-native-rend1-tr_model_iqm.diff.gz` |
| native | native | `rend1/tr_noise.o` | `` | `phase3-native-native-rend1-tr_noise.diff.gz` |
| native | native | `rend1/tr_scene.o` | `` | `phase3-native-native-rend1-tr_scene.diff.gz` |
| native | native | `rend1/tr_shade.o` | `` | `phase3-native-native-rend1-tr_shade.diff.gz` |
| native | native | `rend1/tr_shade_calc.o` | `` | `phase3-native-native-rend1-tr_shade_calc.diff.gz` |
| native | native | `rend1/tr_shader.o` | `` | `phase3-native-native-rend1-tr_shader.diff.gz` |
| native | native | `rend1/tr_shadows.o` | `` | `phase3-native-native-rend1-tr_shadows.diff.gz` |
| native | native | `rend1/tr_sky.o` | `` | `phase3-native-native-rend1-tr_sky.diff.gz` |
| native | native | `rend1/tr_surface.o` | `` | `phase3-native-native-rend1-tr_surface.diff.gz` |
| native | native | `rend1/tr_vbo.o` | `` | `phase3-native-native-rend1-tr_vbo.diff.gz` |
| native | native | `rend1/tr_world.o` | `` | `phase3-native-native-rend1-tr_world.diff.gz` |
| native | native | `rendv/puff.o` | `` | `phase3-native-native-rendv-puff.diff.gz` |
| native | native | `rendv/q_shared.o` | `` | `phase3-native-native-rendv-q_shared.diff.gz` |
| native | native | `rendv/tr_animation.o` | `` | `phase3-native-native-rendv-tr_animation.diff.gz` |
| native | native | `rendv/tr_backend.o` | `` | `phase3-native-native-rendv-tr_backend.diff.gz` |
| native | native | `rendv/tr_bsp.o` | `` | `phase3-native-native-rendv-tr_bsp.diff.gz` |
| native | native | `rendv/tr_cmds.o` | `` | `phase3-native-native-rendv-tr_cmds.diff.gz` |
| native | native | `rendv/tr_curve.o` | `` | `phase3-native-native-rendv-tr_curve.diff.gz` |
| native | native | `rendv/tr_image.o` | `` | `phase3-native-native-rendv-tr_image.diff.gz` |
| native | native | `rendv/tr_image_tga.o` | `` | `phase3-native-native-rendv-tr_image_tga.diff.gz` |
| native | native | `rendv/tr_init.o` | `` | `phase3-native-native-rendv-tr_init.diff.gz` |
| native | native | `rendv/tr_light.o` | `` | `phase3-native-native-rendv-tr_light.diff.gz` |
| native | native | `rendv/tr_main.o` | `` | `phase3-native-native-rendv-tr_main.diff.gz` |
| native | native | `rendv/tr_mesh.o` | `` | `phase3-native-native-rendv-tr_mesh.diff.gz` |
| native | native | `rendv/tr_model.o` | `` | `phase3-native-native-rendv-tr_model.diff.gz` |
| native | native | `rendv/tr_model_iqm.o` | `` | `phase3-native-native-rendv-tr_model_iqm.diff.gz` |
| native | native | `rendv/tr_noise.o` | `` | `phase3-native-native-rendv-tr_noise.diff.gz` |
| native | native | `rendv/tr_scene.o` | `` | `phase3-native-native-rendv-tr_scene.diff.gz` |
| native | native | `rendv/tr_shade.o` | `` | `phase3-native-native-rendv-tr_shade.diff.gz` |
| native | native | `rendv/tr_shade_calc.o` | `` | `phase3-native-native-rendv-tr_shade_calc.diff.gz` |
| native | native | `rendv/tr_shader.o` | `` | `phase3-native-native-rendv-tr_shader.diff.gz` |
| native | native | `rendv/tr_shadows.o` | `` | `phase3-native-native-rendv-tr_shadows.diff.gz` |
| native | native | `rendv/tr_sky.o` | `` | `phase3-native-native-rendv-tr_sky.diff.gz` |
| native | native | `rendv/tr_surface.o` | `` | `phase3-native-native-rendv-tr_surface.diff.gz` |
| native | native | `rendv/tr_world.o` | `` | `phase3-native-native-rendv-tr_world.diff.gz` |
| native | native | `rendv/vk.o` | `` | `phase3-native-native-rendv-vk.diff.gz` |
| native | native | `rendv/vk_flares.o` | `` | `phase3-native-native-rendv-vk_flares.diff.gz` |
| native | native | `rendv/vk_vbo.o` | `` | `phase3-native-native-rendv-vk_vbo.diff.gz` |
| native | native-nosdl | `client/be_aas_bspq3.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-be_aas_bspq3.diff.gz` |
| native | native-nosdl | `client/be_aas_cluster.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-be_aas_cluster.diff.gz` |
| native | native-nosdl | `client/be_aas_debug.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-be_aas_debug.diff.gz` |
| native | native-nosdl | `client/be_aas_file.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-be_aas_file.diff.gz` |
| native | native-nosdl | `client/be_aas_reach.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-be_aas_reach.diff.gz` |
| native | native-nosdl | `client/be_aas_route.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-be_aas_route.diff.gz` |
| native | native-nosdl | `client/be_aas_routealt.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-be_aas_routealt.diff.gz` |
| native | native-nosdl | `client/be_aas_sample.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-be_aas_sample.diff.gz` |
| native | native-nosdl | `client/be_ai_char.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-be_ai_char.diff.gz` |
| native | native-nosdl | `client/be_ai_chat.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-be_ai_chat.diff.gz` |
| native | native-nosdl | `client/be_ai_move.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-be_ai_move.diff.gz` |
| native | native-nosdl | `client/cl_avi.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cl_avi.diff.gz` |
| native | native-nosdl | `client/cl_cgame.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cl_cgame.diff.gz` |
| native | native-nosdl | `client/cl_cin.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cl_cin.diff.gz` |
| native | native-nosdl | `client/cl_console.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cl_console.diff.gz` |
| native | native-nosdl | `client/cl_curl.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cl_curl.diff.gz` |
| native | native-nosdl | `client/cl_input.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cl_input.diff.gz` |
| native | native-nosdl | `client/cl_jpeg.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cl_jpeg.diff.gz` |
| native | native-nosdl | `client/cl_keys.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cl_keys.diff.gz` |
| native | native-nosdl | `client/cl_main.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cl_main.diff.gz` |
| native | native-nosdl | `client/cl_net_chan.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cl_net_chan.diff.gz` |
| native | native-nosdl | `client/cl_parse.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cl_parse.diff.gz` |
| native | native-nosdl | `client/cl_scrn.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cl_scrn.diff.gz` |
| native | native-nosdl | `client/cl_ui.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cl_ui.diff.gz` |
| native | native-nosdl | `client/cm_load.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cm_load.diff.gz` |
| native | native-nosdl | `client/cm_patch.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cm_patch.diff.gz` |
| native | native-nosdl | `client/cm_trace.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cm_trace.diff.gz` |
| native | native-nosdl | `client/cmd.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cmd.diff.gz` |
| native | native-nosdl | `client/common.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-common.diff.gz` |
| native | native-nosdl | `client/cvar.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-cvar.diff.gz` |
| native | native-nosdl | `client/files.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-files.diff.gz` |
| native | native-nosdl | `client/history.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-history.diff.gz` |
| native | native-nosdl | `client/huffman.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-huffman.diff.gz` |
| native | native-nosdl | `client/keys.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-keys.diff.gz` |
| native | native-nosdl | `client/l_memory.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-l_memory.diff.gz` |
| native | native-nosdl | `client/l_precomp.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-l_precomp.diff.gz` |
| native | native-nosdl | `client/l_script.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-l_script.diff.gz` |
| native | native-nosdl | `client/l_struct.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-l_struct.diff.gz` |
| native | native-nosdl | `client/linux_glimp.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-linux_glimp.diff.gz` |
| native | native-nosdl | `client/linux_qgl.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-linux_qgl.diff.gz` |
| native | native-nosdl | `client/linux_snd.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-linux_snd.diff.gz` |
| native | native-nosdl | `client/md5.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-md5.diff.gz` |
| native | native-nosdl | `client/msg.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-msg.diff.gz` |
| native | native-nosdl | `client/net_chan.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-net_chan.diff.gz` |
| native | native-nosdl | `client/net_ip.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-net_ip.diff.gz` |
| native | native-nosdl | `client/puff.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-puff.diff.gz` |
| native | native-nosdl | `client/q_shared.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-q_shared.diff.gz` |
| native | native-nosdl | `client/qvm/vm.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-qvm-vm.diff.gz` |
| native | native-nosdl | `client/qvm/vm_interpreted.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-qvm-vm_interpreted.diff.gz` |
| native | native-nosdl | `client/qvm/vm_x86.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-qvm-vm_x86.diff.gz` |
| native | native-nosdl | `client/snd_dma.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-snd_dma.diff.gz` |
| native | native-nosdl | `client/snd_mem.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-snd_mem.diff.gz` |
| native | native-nosdl | `client/snd_mix.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-snd_mix.diff.gz` |
| native | native-nosdl | `client/sv_bot.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-sv_bot.diff.gz` |
| native | native-nosdl | `client/sv_ccmds.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-sv_ccmds.diff.gz` |
| native | native-nosdl | `client/sv_client.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-sv_client.diff.gz` |
| native | native-nosdl | `client/sv_filter.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-sv_filter.diff.gz` |
| native | native-nosdl | `client/sv_game.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-sv_game.diff.gz` |
| native | native-nosdl | `client/sv_init.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-sv_init.diff.gz` |
| native | native-nosdl | `client/sv_main.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-sv_main.diff.gz` |
| native | native-nosdl | `client/sv_net_chan.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-sv_net_chan.diff.gz` |
| native | native-nosdl | `client/sv_snapshot.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-sv_snapshot.diff.gz` |
| native | native-nosdl | `client/sv_world.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-sv_world.diff.gz` |
| native | native-nosdl | `client/unix_main.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-unix_main.diff.gz` |
| native | native-nosdl | `client/unix_shared.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-unix_shared.diff.gz` |
| native | native-nosdl | `client/unzip.o` | `USE_SDL=0` | `phase3-native-native-nosdl-client-unzip.diff.gz` |
| native | native-nosdl | `ded/be_aas_bspq3.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-be_aas_bspq3.diff.gz` |
| native | native-nosdl | `ded/be_aas_cluster.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-be_aas_cluster.diff.gz` |
| native | native-nosdl | `ded/be_aas_debug.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-be_aas_debug.diff.gz` |
| native | native-nosdl | `ded/be_aas_file.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-be_aas_file.diff.gz` |
| native | native-nosdl | `ded/be_aas_reach.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-be_aas_reach.diff.gz` |
| native | native-nosdl | `ded/be_aas_route.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-be_aas_route.diff.gz` |
| native | native-nosdl | `ded/be_aas_routealt.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-be_aas_routealt.diff.gz` |
| native | native-nosdl | `ded/be_aas_sample.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-be_aas_sample.diff.gz` |
| native | native-nosdl | `ded/be_ai_char.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-be_ai_char.diff.gz` |
| native | native-nosdl | `ded/be_ai_chat.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-be_ai_chat.diff.gz` |
| native | native-nosdl | `ded/be_ai_move.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-be_ai_move.diff.gz` |
| native | native-nosdl | `ded/cm_load.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-cm_load.diff.gz` |
| native | native-nosdl | `ded/cm_patch.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-cm_patch.diff.gz` |
| native | native-nosdl | `ded/cm_trace.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-cm_trace.diff.gz` |
| native | native-nosdl | `ded/cmd.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-cmd.diff.gz` |
| native | native-nosdl | `ded/common.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-common.diff.gz` |
| native | native-nosdl | `ded/cvar.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-cvar.diff.gz` |
| native | native-nosdl | `ded/files.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-files.diff.gz` |
| native | native-nosdl | `ded/history.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-history.diff.gz` |
| native | native-nosdl | `ded/huffman.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-huffman.diff.gz` |
| native | native-nosdl | `ded/keys.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-keys.diff.gz` |
| native | native-nosdl | `ded/l_memory.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-l_memory.diff.gz` |
| native | native-nosdl | `ded/l_precomp.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-l_precomp.diff.gz` |
| native | native-nosdl | `ded/l_script.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-l_script.diff.gz` |
| native | native-nosdl | `ded/l_struct.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-l_struct.diff.gz` |
| native | native-nosdl | `ded/md5.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-md5.diff.gz` |
| native | native-nosdl | `ded/msg.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-msg.diff.gz` |
| native | native-nosdl | `ded/net_chan.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-net_chan.diff.gz` |
| native | native-nosdl | `ded/net_ip.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-net_ip.diff.gz` |
| native | native-nosdl | `ded/q_shared.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-q_shared.diff.gz` |
| native | native-nosdl | `ded/qvm/vm.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-qvm-vm.diff.gz` |
| native | native-nosdl | `ded/qvm/vm_interpreted.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-qvm-vm_interpreted.diff.gz` |
| native | native-nosdl | `ded/qvm/vm_x86.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-qvm-vm_x86.diff.gz` |
| native | native-nosdl | `ded/sv_bot.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-sv_bot.diff.gz` |
| native | native-nosdl | `ded/sv_ccmds.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-sv_ccmds.diff.gz` |
| native | native-nosdl | `ded/sv_client.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-sv_client.diff.gz` |
| native | native-nosdl | `ded/sv_filter.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-sv_filter.diff.gz` |
| native | native-nosdl | `ded/sv_game.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-sv_game.diff.gz` |
| native | native-nosdl | `ded/sv_init.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-sv_init.diff.gz` |
| native | native-nosdl | `ded/sv_main.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-sv_main.diff.gz` |
| native | native-nosdl | `ded/sv_net_chan.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-sv_net_chan.diff.gz` |
| native | native-nosdl | `ded/sv_snapshot.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-sv_snapshot.diff.gz` |
| native | native-nosdl | `ded/sv_world.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-sv_world.diff.gz` |
| native | native-nosdl | `ded/unix_main.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-unix_main.diff.gz` |
| native | native-nosdl | `ded/unix_shared.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-unix_shared.diff.gz` |
| native | native-nosdl | `ded/unzip.o` | `USE_SDL=0` | `phase3-native-native-nosdl-ded-unzip.diff.gz` |
| native | native-nosdl | `rend1/puff.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-puff.diff.gz` |
| native | native-nosdl | `rend1/q_shared.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-q_shared.diff.gz` |
| native | native-nosdl | `rend1/tr_animation.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_animation.diff.gz` |
| native | native-nosdl | `rend1/tr_arb.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_arb.diff.gz` |
| native | native-nosdl | `rend1/tr_backend.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_backend.diff.gz` |
| native | native-nosdl | `rend1/tr_bsp.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_bsp.diff.gz` |
| native | native-nosdl | `rend1/tr_cmds.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_cmds.diff.gz` |
| native | native-nosdl | `rend1/tr_curve.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_curve.diff.gz` |
| native | native-nosdl | `rend1/tr_flares.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_flares.diff.gz` |
| native | native-nosdl | `rend1/tr_image.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_image.diff.gz` |
| native | native-nosdl | `rend1/tr_image_tga.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_image_tga.diff.gz` |
| native | native-nosdl | `rend1/tr_init.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_init.diff.gz` |
| native | native-nosdl | `rend1/tr_light.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_light.diff.gz` |
| native | native-nosdl | `rend1/tr_main.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_main.diff.gz` |
| native | native-nosdl | `rend1/tr_mesh.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_mesh.diff.gz` |
| native | native-nosdl | `rend1/tr_model.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_model.diff.gz` |
| native | native-nosdl | `rend1/tr_model_iqm.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_model_iqm.diff.gz` |
| native | native-nosdl | `rend1/tr_noise.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_noise.diff.gz` |
| native | native-nosdl | `rend1/tr_scene.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_scene.diff.gz` |
| native | native-nosdl | `rend1/tr_shade.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_shade.diff.gz` |
| native | native-nosdl | `rend1/tr_shade_calc.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_shade_calc.diff.gz` |
| native | native-nosdl | `rend1/tr_shader.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_shader.diff.gz` |
| native | native-nosdl | `rend1/tr_shadows.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_shadows.diff.gz` |
| native | native-nosdl | `rend1/tr_sky.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_sky.diff.gz` |
| native | native-nosdl | `rend1/tr_surface.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_surface.diff.gz` |
| native | native-nosdl | `rend1/tr_vbo.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_vbo.diff.gz` |
| native | native-nosdl | `rend1/tr_world.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rend1-tr_world.diff.gz` |
| native | native-nosdl | `rendv/puff.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-puff.diff.gz` |
| native | native-nosdl | `rendv/q_shared.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-q_shared.diff.gz` |
| native | native-nosdl | `rendv/tr_animation.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_animation.diff.gz` |
| native | native-nosdl | `rendv/tr_backend.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_backend.diff.gz` |
| native | native-nosdl | `rendv/tr_bsp.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_bsp.diff.gz` |
| native | native-nosdl | `rendv/tr_cmds.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_cmds.diff.gz` |
| native | native-nosdl | `rendv/tr_curve.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_curve.diff.gz` |
| native | native-nosdl | `rendv/tr_image.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_image.diff.gz` |
| native | native-nosdl | `rendv/tr_image_tga.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_image_tga.diff.gz` |
| native | native-nosdl | `rendv/tr_init.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_init.diff.gz` |
| native | native-nosdl | `rendv/tr_light.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_light.diff.gz` |
| native | native-nosdl | `rendv/tr_main.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_main.diff.gz` |
| native | native-nosdl | `rendv/tr_mesh.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_mesh.diff.gz` |
| native | native-nosdl | `rendv/tr_model.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_model.diff.gz` |
| native | native-nosdl | `rendv/tr_model_iqm.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_model_iqm.diff.gz` |
| native | native-nosdl | `rendv/tr_noise.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_noise.diff.gz` |
| native | native-nosdl | `rendv/tr_scene.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_scene.diff.gz` |
| native | native-nosdl | `rendv/tr_shade.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_shade.diff.gz` |
| native | native-nosdl | `rendv/tr_shade_calc.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_shade_calc.diff.gz` |
| native | native-nosdl | `rendv/tr_shader.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_shader.diff.gz` |
| native | native-nosdl | `rendv/tr_shadows.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_shadows.diff.gz` |
| native | native-nosdl | `rendv/tr_sky.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_sky.diff.gz` |
| native | native-nosdl | `rendv/tr_surface.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_surface.diff.gz` |
| native | native-nosdl | `rendv/tr_world.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-tr_world.diff.gz` |
| native | native-nosdl | `rendv/vk.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-vk.diff.gz` |
| native | native-nosdl | `rendv/vk_flares.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-vk_flares.diff.gz` |
| native | native-nosdl | `rendv/vk_vbo.o` | `USE_SDL=0` | `phase3-native-native-nosdl-rendv-vk_vbo.diff.gz` |
| native | native-nosdl | `client/linux_joystick.o` | `USE_SDL=0 CFLAGS=-DUSE_JOYSTICK` | `phase3-native-native-nosdl-client-linux_joystick.diff.gz` |
| cross | mingw64 | `client/be_aas_bspq3.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-be_aas_bspq3.diff.gz` |
| cross | mingw64 | `client/be_aas_cluster.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-be_aas_cluster.diff.gz` |
| cross | mingw64 | `client/be_aas_debug.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-be_aas_debug.diff.gz` |
| cross | mingw64 | `client/be_aas_entity.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-be_aas_entity.diff.gz` |
| cross | mingw64 | `client/be_aas_file.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-be_aas_file.diff.gz` |
| cross | mingw64 | `client/be_aas_move.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-be_aas_move.diff.gz` |
| cross | mingw64 | `client/be_aas_optimize.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-be_aas_optimize.diff.gz` |
| cross | mingw64 | `client/be_aas_reach.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-be_aas_reach.diff.gz` |
| cross | mingw64 | `client/be_aas_route.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-be_aas_route.diff.gz` |
| cross | mingw64 | `client/be_aas_sample.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-be_aas_sample.diff.gz` |
| cross | mingw64 | `client/be_ai_char.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-be_ai_char.diff.gz` |
| cross | mingw64 | `client/be_ai_chat.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-be_ai_chat.diff.gz` |
| cross | mingw64 | `client/be_ai_goal.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-be_ai_goal.diff.gz` |
| cross | mingw64 | `client/be_ai_move.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-be_ai_move.diff.gz` |
| cross | mingw64 | `client/cl_avi.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cl_avi.diff.gz` |
| cross | mingw64 | `client/cl_cgame.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cl_cgame.diff.gz` |
| cross | mingw64 | `client/cl_console.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cl_console.diff.gz` |
| cross | mingw64 | `client/cl_curl.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cl_curl.diff.gz` |
| cross | mingw64 | `client/cl_input.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cl_input.diff.gz` |
| cross | mingw64 | `client/cl_jpeg.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cl_jpeg.diff.gz` |
| cross | mingw64 | `client/cl_keys.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cl_keys.diff.gz` |
| cross | mingw64 | `client/cl_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cl_main.diff.gz` |
| cross | mingw64 | `client/cl_net_chan.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cl_net_chan.diff.gz` |
| cross | mingw64 | `client/cl_parse.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cl_parse.diff.gz` |
| cross | mingw64 | `client/cl_scrn.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cl_scrn.diff.gz` |
| cross | mingw64 | `client/cl_ui.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cl_ui.diff.gz` |
| cross | mingw64 | `client/cm_load.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cm_load.diff.gz` |
| cross | mingw64 | `client/cm_patch.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cm_patch.diff.gz` |
| cross | mingw64 | `client/cm_test.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cm_test.diff.gz` |
| cross | mingw64 | `client/cm_trace.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cm_trace.diff.gz` |
| cross | mingw64 | `client/cmd.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cmd.diff.gz` |
| cross | mingw64 | `client/common.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-common.diff.gz` |
| cross | mingw64 | `client/cvar.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-cvar.diff.gz` |
| cross | mingw64 | `client/files.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-files.diff.gz` |
| cross | mingw64 | `client/history.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-history.diff.gz` |
| cross | mingw64 | `client/huffman.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-huffman.diff.gz` |
| cross | mingw64 | `client/keys.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-keys.diff.gz` |
| cross | mingw64 | `client/l_memory.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-l_memory.diff.gz` |
| cross | mingw64 | `client/l_precomp.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-l_precomp.diff.gz` |
| cross | mingw64 | `client/l_script.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-l_script.diff.gz` |
| cross | mingw64 | `client/l_struct.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-l_struct.diff.gz` |
| cross | mingw64 | `client/md5.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-md5.diff.gz` |
| cross | mingw64 | `client/msg.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-msg.diff.gz` |
| cross | mingw64 | `client/net_chan.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-net_chan.diff.gz` |
| cross | mingw64 | `client/net_ip.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-net_ip.diff.gz` |
| cross | mingw64 | `client/puff.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-puff.diff.gz` |
| cross | mingw64 | `client/q_math.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-q_math.diff.gz` |
| cross | mingw64 | `client/q_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-q_shared.diff.gz` |
| cross | mingw64 | `client/qvm/vm.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-qvm-vm.diff.gz` |
| cross | mingw64 | `client/qvm/vm_interpreted.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-qvm-vm_interpreted.diff.gz` |
| cross | mingw64 | `client/qvm/vm_x86.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-qvm-vm_x86.diff.gz` |
| cross | mingw64 | `client/snd_dma.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-snd_dma.diff.gz` |
| cross | mingw64 | `client/snd_mem.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-snd_mem.diff.gz` |
| cross | mingw64 | `client/snd_mix.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-snd_mix.diff.gz` |
| cross | mingw64 | `client/sv_bot.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-sv_bot.diff.gz` |
| cross | mingw64 | `client/sv_ccmds.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-sv_ccmds.diff.gz` |
| cross | mingw64 | `client/sv_client.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-sv_client.diff.gz` |
| cross | mingw64 | `client/sv_filter.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-sv_filter.diff.gz` |
| cross | mingw64 | `client/sv_game.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-sv_game.diff.gz` |
| cross | mingw64 | `client/sv_init.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-sv_init.diff.gz` |
| cross | mingw64 | `client/sv_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-sv_main.diff.gz` |
| cross | mingw64 | `client/sv_net_chan.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-sv_net_chan.diff.gz` |
| cross | mingw64 | `client/sv_snapshot.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-sv_snapshot.diff.gz` |
| cross | mingw64 | `client/sv_world.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-sv_world.diff.gz` |
| cross | mingw64 | `client/unzip.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-unzip.diff.gz` |
| cross | mingw64 | `client/win_glimp.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-win_glimp.diff.gz` |
| cross | mingw64 | `client/win_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-win_main.diff.gz` |
| cross | mingw64 | `client/win_minimize.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-win_minimize.diff.gz` |
| cross | mingw64 | `client/win_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-win_shared.diff.gz` |
| cross | mingw64 | `client/win_snd.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-win_snd.diff.gz` |
| cross | mingw64 | `client/win_syscon.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-win_syscon.diff.gz` |
| cross | mingw64 | `client/win_wndproc.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-client-win_wndproc.diff.gz` |
| cross | mingw64 | `ded/be_aas_bspq3.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-be_aas_bspq3.diff.gz` |
| cross | mingw64 | `ded/be_aas_cluster.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-be_aas_cluster.diff.gz` |
| cross | mingw64 | `ded/be_aas_debug.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-be_aas_debug.diff.gz` |
| cross | mingw64 | `ded/be_aas_entity.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-be_aas_entity.diff.gz` |
| cross | mingw64 | `ded/be_aas_file.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-be_aas_file.diff.gz` |
| cross | mingw64 | `ded/be_aas_move.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-be_aas_move.diff.gz` |
| cross | mingw64 | `ded/be_aas_optimize.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-be_aas_optimize.diff.gz` |
| cross | mingw64 | `ded/be_aas_reach.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-be_aas_reach.diff.gz` |
| cross | mingw64 | `ded/be_aas_route.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-be_aas_route.diff.gz` |
| cross | mingw64 | `ded/be_aas_sample.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-be_aas_sample.diff.gz` |
| cross | mingw64 | `ded/be_ai_char.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-be_ai_char.diff.gz` |
| cross | mingw64 | `ded/be_ai_chat.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-be_ai_chat.diff.gz` |
| cross | mingw64 | `ded/be_ai_goal.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-be_ai_goal.diff.gz` |
| cross | mingw64 | `ded/be_ai_move.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-be_ai_move.diff.gz` |
| cross | mingw64 | `ded/cm_load.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-cm_load.diff.gz` |
| cross | mingw64 | `ded/cm_patch.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-cm_patch.diff.gz` |
| cross | mingw64 | `ded/cm_test.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-cm_test.diff.gz` |
| cross | mingw64 | `ded/cm_trace.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-cm_trace.diff.gz` |
| cross | mingw64 | `ded/cmd.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-cmd.diff.gz` |
| cross | mingw64 | `ded/common.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-common.diff.gz` |
| cross | mingw64 | `ded/cvar.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-cvar.diff.gz` |
| cross | mingw64 | `ded/files.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-files.diff.gz` |
| cross | mingw64 | `ded/history.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-history.diff.gz` |
| cross | mingw64 | `ded/huffman.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-huffman.diff.gz` |
| cross | mingw64 | `ded/keys.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-keys.diff.gz` |
| cross | mingw64 | `ded/l_memory.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-l_memory.diff.gz` |
| cross | mingw64 | `ded/l_precomp.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-l_precomp.diff.gz` |
| cross | mingw64 | `ded/l_script.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-l_script.diff.gz` |
| cross | mingw64 | `ded/l_struct.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-l_struct.diff.gz` |
| cross | mingw64 | `ded/md5.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-md5.diff.gz` |
| cross | mingw64 | `ded/msg.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-msg.diff.gz` |
| cross | mingw64 | `ded/net_chan.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-net_chan.diff.gz` |
| cross | mingw64 | `ded/net_ip.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-net_ip.diff.gz` |
| cross | mingw64 | `ded/q_math.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-q_math.diff.gz` |
| cross | mingw64 | `ded/q_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-q_shared.diff.gz` |
| cross | mingw64 | `ded/qvm/vm.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-qvm-vm.diff.gz` |
| cross | mingw64 | `ded/qvm/vm_interpreted.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-qvm-vm_interpreted.diff.gz` |
| cross | mingw64 | `ded/qvm/vm_x86.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-qvm-vm_x86.diff.gz` |
| cross | mingw64 | `ded/sv_bot.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-sv_bot.diff.gz` |
| cross | mingw64 | `ded/sv_ccmds.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-sv_ccmds.diff.gz` |
| cross | mingw64 | `ded/sv_client.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-sv_client.diff.gz` |
| cross | mingw64 | `ded/sv_filter.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-sv_filter.diff.gz` |
| cross | mingw64 | `ded/sv_game.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-sv_game.diff.gz` |
| cross | mingw64 | `ded/sv_init.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-sv_init.diff.gz` |
| cross | mingw64 | `ded/sv_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-sv_main.diff.gz` |
| cross | mingw64 | `ded/sv_net_chan.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-sv_net_chan.diff.gz` |
| cross | mingw64 | `ded/sv_snapshot.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-sv_snapshot.diff.gz` |
| cross | mingw64 | `ded/sv_world.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-sv_world.diff.gz` |
| cross | mingw64 | `ded/unzip.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-unzip.diff.gz` |
| cross | mingw64 | `ded/win_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-win_main.diff.gz` |
| cross | mingw64 | `ded/win_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-win_shared.diff.gz` |
| cross | mingw64 | `ded/win_syscon.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-ded-win_syscon.diff.gz` |
| cross | mingw64 | `rend1/puff.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-puff.diff.gz` |
| cross | mingw64 | `rend1/q_math.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-q_math.diff.gz` |
| cross | mingw64 | `rend1/q_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-q_shared.diff.gz` |
| cross | mingw64 | `rend1/tr_animation.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_animation.diff.gz` |
| cross | mingw64 | `rend1/tr_arb.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_arb.diff.gz` |
| cross | mingw64 | `rend1/tr_backend.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_backend.diff.gz` |
| cross | mingw64 | `rend1/tr_bsp.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_bsp.diff.gz` |
| cross | mingw64 | `rend1/tr_cmds.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_cmds.diff.gz` |
| cross | mingw64 | `rend1/tr_curve.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_curve.diff.gz` |
| cross | mingw64 | `rend1/tr_flares.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_flares.diff.gz` |
| cross | mingw64 | `rend1/tr_image.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_image.diff.gz` |
| cross | mingw64 | `rend1/tr_image_tga.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_image_tga.diff.gz` |
| cross | mingw64 | `rend1/tr_init.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_init.diff.gz` |
| cross | mingw64 | `rend1/tr_light.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_light.diff.gz` |
| cross | mingw64 | `rend1/tr_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_main.diff.gz` |
| cross | mingw64 | `rend1/tr_marks.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_marks.diff.gz` |
| cross | mingw64 | `rend1/tr_mesh.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_mesh.diff.gz` |
| cross | mingw64 | `rend1/tr_model.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_model.diff.gz` |
| cross | mingw64 | `rend1/tr_model_iqm.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_model_iqm.diff.gz` |
| cross | mingw64 | `rend1/tr_scene.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_scene.diff.gz` |
| cross | mingw64 | `rend1/tr_shade.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_shade.diff.gz` |
| cross | mingw64 | `rend1/tr_shade_calc.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_shade_calc.diff.gz` |
| cross | mingw64 | `rend1/tr_shader.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_shader.diff.gz` |
| cross | mingw64 | `rend1/tr_shadows.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_shadows.diff.gz` |
| cross | mingw64 | `rend1/tr_sky.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_sky.diff.gz` |
| cross | mingw64 | `rend1/tr_surface.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_surface.diff.gz` |
| cross | mingw64 | `rend1/tr_vbo.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_vbo.diff.gz` |
| cross | mingw64 | `rend1/tr_world.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rend1-tr_world.diff.gz` |
| cross | mingw64 | `rendv/puff.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-puff.diff.gz` |
| cross | mingw64 | `rendv/q_math.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-q_math.diff.gz` |
| cross | mingw64 | `rendv/q_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-q_shared.diff.gz` |
| cross | mingw64 | `rendv/tr_animation.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_animation.diff.gz` |
| cross | mingw64 | `rendv/tr_backend.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_backend.diff.gz` |
| cross | mingw64 | `rendv/tr_bsp.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_bsp.diff.gz` |
| cross | mingw64 | `rendv/tr_cmds.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_cmds.diff.gz` |
| cross | mingw64 | `rendv/tr_curve.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_curve.diff.gz` |
| cross | mingw64 | `rendv/tr_image.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_image.diff.gz` |
| cross | mingw64 | `rendv/tr_image_tga.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_image_tga.diff.gz` |
| cross | mingw64 | `rendv/tr_init.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_init.diff.gz` |
| cross | mingw64 | `rendv/tr_light.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_light.diff.gz` |
| cross | mingw64 | `rendv/tr_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_main.diff.gz` |
| cross | mingw64 | `rendv/tr_marks.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_marks.diff.gz` |
| cross | mingw64 | `rendv/tr_mesh.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_mesh.diff.gz` |
| cross | mingw64 | `rendv/tr_model.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_model.diff.gz` |
| cross | mingw64 | `rendv/tr_model_iqm.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_model_iqm.diff.gz` |
| cross | mingw64 | `rendv/tr_scene.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_scene.diff.gz` |
| cross | mingw64 | `rendv/tr_shade.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_shade.diff.gz` |
| cross | mingw64 | `rendv/tr_shade_calc.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_shade_calc.diff.gz` |
| cross | mingw64 | `rendv/tr_shader.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_shader.diff.gz` |
| cross | mingw64 | `rendv/tr_shadows.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_shadows.diff.gz` |
| cross | mingw64 | `rendv/tr_sky.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_sky.diff.gz` |
| cross | mingw64 | `rendv/tr_surface.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_surface.diff.gz` |
| cross | mingw64 | `rendv/tr_world.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-tr_world.diff.gz` |
| cross | mingw64 | `rendv/vk.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-vk.diff.gz` |
| cross | mingw64 | `rendv/vk_flares.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-vk_flares.diff.gz` |
| cross | mingw64 | `rendv/vk_vbo.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `phase3-cross-mingw64-rendv-vk_vbo.diff.gz` |
| cross | aarch64 | `ded/be_aas_bspq3.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-be_aas_bspq3.diff.gz` |
| cross | aarch64 | `ded/be_aas_cluster.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-be_aas_cluster.diff.gz` |
| cross | aarch64 | `ded/be_aas_debug.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-be_aas_debug.diff.gz` |
| cross | aarch64 | `ded/be_aas_file.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-be_aas_file.diff.gz` |
| cross | aarch64 | `ded/be_aas_reach.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-be_aas_reach.diff.gz` |
| cross | aarch64 | `ded/be_aas_route.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-be_aas_route.diff.gz` |
| cross | aarch64 | `ded/be_aas_routealt.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-be_aas_routealt.diff.gz` |
| cross | aarch64 | `ded/be_aas_sample.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-be_aas_sample.diff.gz` |
| cross | aarch64 | `ded/be_ai_char.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-be_ai_char.diff.gz` |
| cross | aarch64 | `ded/be_ai_chat.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-be_ai_chat.diff.gz` |
| cross | aarch64 | `ded/be_ai_move.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-be_ai_move.diff.gz` |
| cross | aarch64 | `ded/cm_load.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-cm_load.diff.gz` |
| cross | aarch64 | `ded/cm_patch.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-cm_patch.diff.gz` |
| cross | aarch64 | `ded/cm_trace.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-cm_trace.diff.gz` |
| cross | aarch64 | `ded/cmd.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-cmd.diff.gz` |
| cross | aarch64 | `ded/common.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-common.diff.gz` |
| cross | aarch64 | `ded/cvar.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-cvar.diff.gz` |
| cross | aarch64 | `ded/files.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-files.diff.gz` |
| cross | aarch64 | `ded/history.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-history.diff.gz` |
| cross | aarch64 | `ded/huffman.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-huffman.diff.gz` |
| cross | aarch64 | `ded/keys.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-keys.diff.gz` |
| cross | aarch64 | `ded/l_memory.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-l_memory.diff.gz` |
| cross | aarch64 | `ded/l_precomp.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-l_precomp.diff.gz` |
| cross | aarch64 | `ded/l_script.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-l_script.diff.gz` |
| cross | aarch64 | `ded/l_struct.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-l_struct.diff.gz` |
| cross | aarch64 | `ded/md5.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-md5.diff.gz` |
| cross | aarch64 | `ded/msg.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-msg.diff.gz` |
| cross | aarch64 | `ded/net_chan.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-net_chan.diff.gz` |
| cross | aarch64 | `ded/net_ip.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-net_ip.diff.gz` |
| cross | aarch64 | `ded/q_shared.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-q_shared.diff.gz` |
| cross | aarch64 | `ded/qvm/vm.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-qvm-vm.diff.gz` |
| cross | aarch64 | `ded/qvm/vm_aarch64.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-qvm-vm_aarch64.diff.gz` |
| cross | aarch64 | `ded/qvm/vm_interpreted.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-qvm-vm_interpreted.diff.gz` |
| cross | aarch64 | `ded/sv_bot.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-sv_bot.diff.gz` |
| cross | aarch64 | `ded/sv_ccmds.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-sv_ccmds.diff.gz` |
| cross | aarch64 | `ded/sv_client.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-sv_client.diff.gz` |
| cross | aarch64 | `ded/sv_filter.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-sv_filter.diff.gz` |
| cross | aarch64 | `ded/sv_game.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-sv_game.diff.gz` |
| cross | aarch64 | `ded/sv_init.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-sv_init.diff.gz` |
| cross | aarch64 | `ded/sv_main.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-sv_main.diff.gz` |
| cross | aarch64 | `ded/sv_net_chan.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-sv_net_chan.diff.gz` |
| cross | aarch64 | `ded/sv_snapshot.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-sv_snapshot.diff.gz` |
| cross | aarch64 | `ded/sv_world.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-sv_world.diff.gz` |
| cross | aarch64 | `ded/unix_main.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-unix_main.diff.gz` |
| cross | aarch64 | `ded/unix_shared.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-unix_shared.diff.gz` |
| cross | aarch64 | `ded/unzip.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `phase3-cross-aarch64-ded-unzip.diff.gz` |
| cross | arm | `ded/be_aas_bspq3.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-be_aas_bspq3.diff.gz` |
| cross | arm | `ded/be_aas_cluster.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-be_aas_cluster.diff.gz` |
| cross | arm | `ded/be_aas_debug.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-be_aas_debug.diff.gz` |
| cross | arm | `ded/be_aas_file.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-be_aas_file.diff.gz` |
| cross | arm | `ded/be_aas_reach.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-be_aas_reach.diff.gz` |
| cross | arm | `ded/be_aas_route.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-be_aas_route.diff.gz` |
| cross | arm | `ded/be_aas_routealt.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-be_aas_routealt.diff.gz` |
| cross | arm | `ded/be_aas_sample.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-be_aas_sample.diff.gz` |
| cross | arm | `ded/be_ai_char.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-be_ai_char.diff.gz` |
| cross | arm | `ded/be_ai_chat.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-be_ai_chat.diff.gz` |
| cross | arm | `ded/be_ai_move.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-be_ai_move.diff.gz` |
| cross | arm | `ded/cm_load.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-cm_load.diff.gz` |
| cross | arm | `ded/cm_patch.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-cm_patch.diff.gz` |
| cross | arm | `ded/cm_trace.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-cm_trace.diff.gz` |
| cross | arm | `ded/cmd.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-cmd.diff.gz` |
| cross | arm | `ded/common.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-common.diff.gz` |
| cross | arm | `ded/cvar.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-cvar.diff.gz` |
| cross | arm | `ded/files.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-files.diff.gz` |
| cross | arm | `ded/history.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-history.diff.gz` |
| cross | arm | `ded/huffman.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-huffman.diff.gz` |
| cross | arm | `ded/keys.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-keys.diff.gz` |
| cross | arm | `ded/l_memory.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-l_memory.diff.gz` |
| cross | arm | `ded/l_precomp.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-l_precomp.diff.gz` |
| cross | arm | `ded/l_script.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-l_script.diff.gz` |
| cross | arm | `ded/l_struct.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-l_struct.diff.gz` |
| cross | arm | `ded/md5.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-md5.diff.gz` |
| cross | arm | `ded/msg.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-msg.diff.gz` |
| cross | arm | `ded/net_chan.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-net_chan.diff.gz` |
| cross | arm | `ded/net_ip.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-net_ip.diff.gz` |
| cross | arm | `ded/q_math.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-q_math.diff.gz` |
| cross | arm | `ded/q_shared.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-q_shared.diff.gz` |
| cross | arm | `ded/qvm/vm.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-qvm-vm.diff.gz` |
| cross | arm | `ded/qvm/vm_armv7l.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-qvm-vm_armv7l.diff.gz` |
| cross | arm | `ded/qvm/vm_interpreted.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-qvm-vm_interpreted.diff.gz` |
| cross | arm | `ded/sv_bot.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-sv_bot.diff.gz` |
| cross | arm | `ded/sv_ccmds.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-sv_ccmds.diff.gz` |
| cross | arm | `ded/sv_client.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-sv_client.diff.gz` |
| cross | arm | `ded/sv_filter.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-sv_filter.diff.gz` |
| cross | arm | `ded/sv_game.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-sv_game.diff.gz` |
| cross | arm | `ded/sv_init.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-sv_init.diff.gz` |
| cross | arm | `ded/sv_main.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-sv_main.diff.gz` |
| cross | arm | `ded/sv_net_chan.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-sv_net_chan.diff.gz` |
| cross | arm | `ded/sv_snapshot.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-sv_snapshot.diff.gz` |
| cross | arm | `ded/sv_world.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-sv_world.diff.gz` |
| cross | arm | `ded/unix_main.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-unix_main.diff.gz` |
| cross | arm | `ded/unix_shared.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-unix_shared.diff.gz` |
| cross | arm | `ded/unzip.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `phase3-cross-arm-ded-unzip.diff.gz` |
| cross | ppc64le | `ded/be_aas_bspq3.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_aas_bspq3.diff.gz` |
| cross | ppc64le | `ded/be_aas_cluster.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_aas_cluster.diff.gz` |
| cross | ppc64le | `ded/be_aas_debug.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_aas_debug.diff.gz` |
| cross | ppc64le | `ded/be_aas_entity.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_aas_entity.diff.gz` |
| cross | ppc64le | `ded/be_aas_file.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_aas_file.diff.gz` |
| cross | ppc64le | `ded/be_aas_main.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_aas_main.diff.gz` |
| cross | ppc64le | `ded/be_aas_move.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_aas_move.diff.gz` |
| cross | ppc64le | `ded/be_aas_optimize.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_aas_optimize.diff.gz` |
| cross | ppc64le | `ded/be_aas_reach.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_aas_reach.diff.gz` |
| cross | ppc64le | `ded/be_aas_route.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_aas_route.diff.gz` |
| cross | ppc64le | `ded/be_aas_routealt.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_aas_routealt.diff.gz` |
| cross | ppc64le | `ded/be_aas_sample.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_aas_sample.diff.gz` |
| cross | ppc64le | `ded/be_ai_char.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_ai_char.diff.gz` |
| cross | ppc64le | `ded/be_ai_chat.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_ai_chat.diff.gz` |
| cross | ppc64le | `ded/be_ai_gen.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_ai_gen.diff.gz` |
| cross | ppc64le | `ded/be_ai_goal.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_ai_goal.diff.gz` |
| cross | ppc64le | `ded/be_ai_move.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_ai_move.diff.gz` |
| cross | ppc64le | `ded/be_ai_weap.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_ai_weap.diff.gz` |
| cross | ppc64le | `ded/be_ai_weight.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_ai_weight.diff.gz` |
| cross | ppc64le | `ded/be_ea.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_ea.diff.gz` |
| cross | ppc64le | `ded/be_interface.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-be_interface.diff.gz` |
| cross | ppc64le | `ded/cm_load.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-cm_load.diff.gz` |
| cross | ppc64le | `ded/cm_patch.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-cm_patch.diff.gz` |
| cross | ppc64le | `ded/cm_polylib.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-cm_polylib.diff.gz` |
| cross | ppc64le | `ded/cm_test.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-cm_test.diff.gz` |
| cross | ppc64le | `ded/cm_trace.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-cm_trace.diff.gz` |
| cross | ppc64le | `ded/cmd.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-cmd.diff.gz` |
| cross | ppc64le | `ded/common.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-common.diff.gz` |
| cross | ppc64le | `ded/cvar.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-cvar.diff.gz` |
| cross | ppc64le | `ded/files.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-files.diff.gz` |
| cross | ppc64le | `ded/history.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-history.diff.gz` |
| cross | ppc64le | `ded/huffman.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-huffman.diff.gz` |
| cross | ppc64le | `ded/huffman_static.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-huffman_static.diff.gz` |
| cross | ppc64le | `ded/keys.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-keys.diff.gz` |
| cross | ppc64le | `ded/l_crc.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-l_crc.diff.gz` |
| cross | ppc64le | `ded/l_libvar.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-l_libvar.diff.gz` |
| cross | ppc64le | `ded/l_log.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-l_log.diff.gz` |
| cross | ppc64le | `ded/l_memory.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-l_memory.diff.gz` |
| cross | ppc64le | `ded/l_precomp.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-l_precomp.diff.gz` |
| cross | ppc64le | `ded/l_script.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-l_script.diff.gz` |
| cross | ppc64le | `ded/l_struct.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-l_struct.diff.gz` |
| cross | ppc64le | `ded/linux_signals.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-linux_signals.diff.gz` |
| cross | ppc64le | `ded/md4.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-md4.diff.gz` |
| cross | ppc64le | `ded/md5.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-md5.diff.gz` |
| cross | ppc64le | `ded/msg.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-msg.diff.gz` |
| cross | ppc64le | `ded/net_chan.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-net_chan.diff.gz` |
| cross | ppc64le | `ded/net_ip.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-net_ip.diff.gz` |
| cross | ppc64le | `ded/q_math.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-q_math.diff.gz` |
| cross | ppc64le | `ded/q_shared.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-q_shared.diff.gz` |
| cross | ppc64le | `ded/qvm/vm.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-qvm-vm.diff.gz` |
| cross | ppc64le | `ded/qvm/vm_interpreted.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-qvm-vm_interpreted.diff.gz` |
| cross | ppc64le | `ded/qvm/vm_powerpc.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-qvm-vm_powerpc.diff.gz` |
| cross | ppc64le | `ded/sv_bot.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-sv_bot.diff.gz` |
| cross | ppc64le | `ded/sv_ccmds.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-sv_ccmds.diff.gz` |
| cross | ppc64le | `ded/sv_client.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-sv_client.diff.gz` |
| cross | ppc64le | `ded/sv_filter.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-sv_filter.diff.gz` |
| cross | ppc64le | `ded/sv_game.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-sv_game.diff.gz` |
| cross | ppc64le | `ded/sv_init.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-sv_init.diff.gz` |
| cross | ppc64le | `ded/sv_main.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-sv_main.diff.gz` |
| cross | ppc64le | `ded/sv_net_chan.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-sv_net_chan.diff.gz` |
| cross | ppc64le | `ded/sv_snapshot.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-sv_snapshot.diff.gz` |
| cross | ppc64le | `ded/sv_world.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-sv_world.diff.gz` |
| cross | ppc64le | `ded/unix_main.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-unix_main.diff.gz` |
| cross | ppc64le | `ded/unix_shared.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-unix_shared.diff.gz` |
| cross | ppc64le | `ded/unzip.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `phase3-cross-ppc64le-ded-unzip.diff.gz` |

Later per-file G4: `phase3-msvc-vm_aarch64.diff.gz`; same Linux AArch64 body with one Windows-only T1 cast.

## Historical continuation decisions (chronological audit; current state is above)

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

- Pre-rename refresh: 454 native object contexts pass strict compilation, G2 and G3; every G4 diff is indexed in t25-native-context-results.json. Original C manifest recheck: native 295/295, non-SDL 299/299, ARM/AArch64/PPC64LE 65/65 each, MinGW non-SDL 300/300. Shader include path consumer additionally passes native/non-SDL/MinGW C hashes/G2/G3 (phase3-shader-results.json).

- Phase 3: 170 byte-identical engine moves after the separate shader-data move; exact old/new/blob inventory phase3-renames.json. GNU Make now compiles engine sources only as C++20; vendored C and assembly retain original rules. Five engine MSVC projects/filters updated with C++20, no exceptions/RTTI, and early empty CINTERFACE. renderer2 removed from GNU/solution builds. x86 Windows/Linux CI entries removed, x86_64 preserved. Branch workflow dispatch is guarded from publishing the public latest release; this enables the requested CI verification without release publication.

- Frozen pre-rename C oracle: `e49b82595c7b1a25c70d9e86b72b3ed147adbdb3`. Reproduce with `mkdir -p /tmp/port-c-oracle; git archive e49b82595c7b1a25c70d9e86b72b3ed147adbdb3 | tar -x -C /tmp/port-c-oracle`; export `PORT_C_ORACLE=/tmp/port-c-oracle` for every post-rename gate/matrix invocation. That checkout retains BUILD_CXX=0 and the original C T25 branch.

- Main rename G8: reviewer verified all 170 R100 moves, correct native/Clang/cross/explicit-CXX compiler selection, unchanged vendor C flags, valid MSVC XML/paths and CRLF, and feature-branch CI publication guard. Removed final two dead rend2 mkdir lines. Fresh build directories are mandatory after the language change.

- Post-rename integration: 16/16 native C-oracle/C++ matrix configurations PASS; four cross C++ full links PASS; G5 13 groups PASS, vector_math 5c00b4de, math PASS. G7 155 native sources checked, zero compile/tool failures, same 366 narrowing findings retained; 16 platform/include-only sources skipped. G6 again matches all 123 lines after only the allowed cached-pak line removal. Exact commands/results/logs: phase3-build-matrix-results.json, phase3-cross-build-results.json, phase3-runtime-results.json, phase3-client-runtime.json, phase3-static-summary.txt, phase3-g5.log.gz, phase3-math.log.gz. One G5 recipe query raced the final Makefile write and was rerun after commit; the committed Makefile parses and G5 passes.

- Phase 3 final object sweeps PASS: [('native', 454, 331), ('cross', 424, 338)]. Tuples are (target family, G2/G3 passing contexts, retained G4 differences). Full per-object diff indexes: phase3-native-context-results.json and phase3-cross-gates-results.json. Gate controls PASS, including rejection of a C++ object supplied as a C oracle. Both dlopen clients and static Vulkan load q3dm17 under Xvfb.

- DEVIATION: freeze Apple SDK deprecated-declarations. Initial post-rename macOS CI reaches unchanged cl_cgame sprintf calls at 720/722, two diagnostics per configuration (eight across four legs). Preserve the original calls and suppress this observed platform-only warning class. No engine code or native/C-oracle flags change. Evidence phase3-ci-macos-job.log.gz.

- MSVC frozen write-strings policy: initial /std:c++20 enables strict literal conversion checks. All 136 unique Debug and 118 unique Release failures per architecture in the initial botlib build are existing string-literal-to-char* sites. Apply the narrow /Zc:strictStrings- option to the five engine projects, matching the already accepted -Wno-write-strings policy; other C++ conformance checks remain active. Microsoft documents this override at https://learn.microsoft.com/en-us/cpp/build/reference/zc-strictstrings-disable-string-literal-type-conversion . No engine signatures or literals change.

- DEVIATION: sanitizer-only frozen maybe-uninitialized class. One original C diagnostic at be_aas_reach.c:2196 reappears under C++ instrumentation; freeze that observed class only when -fsanitize is present. The original C warning is retained in sanitizer-c-baseline-warning.log; possible uninitialized beststart path logged in cpp-port-notes.md and left unchanged. Normal production flags are unaffected.

- G8 scoped the sanitizer-only maybe-uninitialized suppression to GCC: Clang does not recognize that warning option. C++ GCC ASan/UBSan dedicated q3dm17/two-bot smoke exits cleanly with the same two original alignment suppressions and leak checking disabled; no new runtime diagnostics.

- qcommon/vm_aarch64.cpp: MSVC ARM64 reports C2440 at 2283, VirtualAlloc LPVOID to byte*. Added one T1 (byte *) cast, matching the existing field. AArch64 strict compile/G2/G3 PASS; G4 retained in phase3-msvc-vm_aarch64.diff.gz. Windows ARM64 verification continues in CI; MSVC x64 Debug/Release now pass.

- Historical T25 cleanup reproducer at `629700fa`: `python3 tools/port/reproduce_t25_cleanup.py "$PWD" /tmp/port-cleanup-proof`. At that commit, success meant the documented incompatibility was reproduced, **not** that cleanup passed; the current script instead verifies the approved cleanup. It records actual Make recipes, compiler versions, all variant hashes, raw DWARF and precisely locates the 16-byte MD5 region (offset depends on output-path length). Archived original/blank Clang objects are t25-cleanup-clang-default-{original,blank}.o.gz. Source remains unchanged.
- MSVC final catalog loop: run 34804759804 at commit 4ffcd649 passes Debug/Release x64 and ARM64 after the one Windows-only vm_aarch64 T1 cast. No remaining MSVC error needs a new transformation.

- Final CI: https://github.com/msetaro/aftershock/actions/runs/34804759804 — success at 4ffcd649. `gh run watch --exit-status` and `gh run view --log-failed` completed; no failed logs in the final run. Exact per-job conclusions and commands are in phase3-ci{1,2,3}-results.json. Later commits contain only checkpoint/evidence and the read-only cleanup reproducer; no engine/build content changes after the green source commit.
- Final scope: 257 inventory entries done (171 renamed implementation/data sources, shared headers, and two explicit exclusions). No engine compile/G2/G3 blocker remains. At that historical checkpoint T25 cleanup was the sole blocked source task; the PR32 continuation below removes the guard under the approved metadata comparison. Optional unverified configurations are listed separately above. All eight historical DEVIATION commits remain listed with reasons; all current and historical G4 differences remain indexed. No question or hash-policy exception was inferred.

## PR 32 maintainer review follow-up

- Merged origin/main 841b35d5 without conflicts; its deterministic runtime and toolchain notes are retained. Review authorizes T25 cleanup with raw release hashes and objcopy --strip-debug hashes for debug objects on GCC/Clang. No code/data hash exception is allowed.

- Plan wording now exempts T15 token-boundary spacing, limits T21 to arguments that can select float overloads (including target-dependent M_PI), and records the approved T25 debug-metadata comparison. Make documents explicit CXX for versioned CC names. The obsolete advisory CXX probe was already removed in 7856982f.

- PR32 generator T18 fix: bin2hex.cpp now emits the preceding extern declaration for arrays and its existing disabled _size-constant output path. Regenerated all 74 arrays from their committed SPIR-V payloads; shader_data.cpp is byte-identical, SHA256 700724298e78e98017adeeceb34b56af6243fe6cdbdce94da60751b1fb187367. No glslangValidator is installed; this verifies binary-to-source regeneration without recompiling/replacing shader bytecode. The disabled _size path is separately enabled in a temporary fixture and both symbols have external linkage. Reproduce: `python3 tools/port/check_shader_generator.py /tmp/port-shaders`. shader_data.cpp is generated: never hand-edit it.

- PR32 T25 cleanup removed the four preprocessor/C-only lines, retaining the exact active cast expression. Eight actual Make compiler recipes ran before/after at identical repository source and object paths.

| Compiler | Configuration | Context | Comparison | Identical SHA256 |
|---|---|---|---|---|
| gcc | release | client | raw | `b7010e61adc2ea2ec82f89642dd35f6842d1456063e398851e12391a5f7762cc` |
| gcc | release | ded | raw | `0416faec94095e85c87111dfdf06ef513b8b4035735f8e9580210aeb6bd5a3a6` |
| gcc | debug | client | stripped | `aa61af63af849757dbbd8d6639ae968b5b95c69b7949b9580e847a5919b480e9` |
| gcc | debug | ded | stripped | `34d8e684144da45d9919652ef082f4aeb8e22469feb819bc6f0482b8c5d057a7` |
| clang | release | client | raw | `3bff7c8638aa501848f2794898d802fd544ceb95cccb6634f60124af6e94a881` |
| clang | release | ded | raw | `5188a5047a73f4503ae79a0a9d0878fe1c86cb7e1f3db685938f33abab17bf0e` |
| clang | debug | client | stripped | `02e3999d9429b04e5422bbad35b0ea2254ac07d325993cf44f11785b79645379` |
| clang | debug | ded | stripped | `f4e52fe200a027a244d701e2e879e1f05aefbc778628e384ca86996deefe5909` |

- PR32 verification tools: reproduce_t25_cleanup.py now checks the approved raw-release/stripped-debug rule against pre-cleanup commit 629700fa; eight pairs pass. cross_gates.py accepts PORT_EVIDENCE for archived oracle manifests. Previous /tmp artifacts were absent in this session; the frozen C oracle was reconstructed from e49b8259, without installing tools.

- PR32 evidence move complete: every file was compared byte-for-byte against the pushed orphan snapshot before removal from this branch; archive SHA 823d6f1e7b0603980bb743cbba2cf6d9c9b405da. Cross sweeps read their manifests via PORT_EVIDENCE; the active tree retains only the compact review-acceptance JSON.
