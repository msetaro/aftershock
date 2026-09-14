# C -> C++ port plan (strict, no behavior change)

Status: initial decisions resolved 2026-09-13; continuation review amendments below are authoritative. Current execution state is docs/cpp-port-progress.md.

## 1. Goal and non-goals

Goal: every engine translation unit compiles as C++ and the resulting binaries behave identically
to the C build. "Identically" means same network protocol, same file formats, same QVM ABI, same
struct layouts, same console output, same demo playback, same collision results, same sound mix.

Non-goals (explicitly out of scope until the port is done and verified):
modernization, classes, RAII, STL, bug fixes, warning cleanups beyond what C++ requires,
performance changes, build-system rewrites.

## 2. Baseline measurements (2026-09-13, gcc 15.2, x86_64 Linux)

Compiling the dedicated-server object set (qcommon + server + botlib + unix, ~90 files) with
`g++ -x c++ -std=c++20 -fpermissive` gives:

| Category | Count |
|---|---|
| Hard errors | 41 |
| `-fpermissive` conversions (errors without the flag) | 332 |
| Files with hard errors | 10 |

Hard-error breakdown: `bool` -> `qboolean` (`x = a && b;` / `return p != NULL;`) 22, identifier
`new` in `net_chan.c` 10, identifier `operator` in `botlib/l_precomp.c` 9.
The 332 conversions are all implicit `void *` -> `T *` (allocator results, `memcpy`-style
plumbing) and implicit `int` -> `qboolean`.

A second probe over the client + renderervk + renderercommon set (`BUILD_SERVER=0 USE_SDL=0
USE_CURL=0`, `-k`; 32 GL/X11 objects skipped for missing headers) adds:

| Category | Count | Root cause |
|---|---|---|
| `renderervk` hard errors | 1031 | one struct field named `or` in `tr_local.h` (`or` is a C++ alternative token); every include cascades. Rename it and the count collapses to a handful of `bool` -> `qboolean` sites |
| `client` hard errors | 11 | `bool` -> `qboolean`, one more `new` identifier |
| `-Wregister` | 319 | `register` storage class: gcc warns, **clang and MSVC reject it in C++17**. 28 sites in `common.c`, the rest in vendored libjpeg (stays C) |
| `-Wliteral-suffix` | 29 | `"literal"MACRO` needs a space in C++11+ |
| `-Wnarrowing` | 2 | `0xFFFFFFFF` / `0x80000000` into `int` in an initializer or enum (`msg.c`, `surfaceflags.h`) |
| `-Wwrite-strings` | 123 | string literal -> `char *` |

Whole-tree scan of engine sources (vendored libs excluded):

| Construct | Occurrences | Notes |
|---|---|---|
| compound literals `(type){...}` | ~700 regex hits | needs manual verification; many are casts, true compound literals must become named temporaries |
| `goto` | 116 | fine in C++ unless it jumps over an initialization |
| typedef enums | 350 | enum <-> int arithmetic is legal in C++17, deprecated in C++20 |
| anonymous struct/union members | 85 | legal in C++ (as extension on gcc/clang/MSVC) |
| `setjmp`/`longjmp` sites | 31 | `Com_Error` unwinding; fine with `-fno-exceptions`, must never cross a non-trivial destructor (there are none) |
| `restrict` | 4 | -> `__restrict` |
| files with inline asm | 5 | unchanged, gcc accepts the same syntax in C++ |
| designated initializers | 0 | |
| `_Generic`, VLAs, K&R prototypes | 0 found | |

Extrapolating to the client + three renderers + platform layers (~200 more files), expect roughly
100-150 hard errors and 1000-1500 conversions across the whole tree. This is a mechanical job.

## 3. C++ standard — DECIDED: C++20

Decision (Matt, 2026-09-13): **C++20**, with `-fno-exceptions -fno-rtti`
(MSVC: `/std:c++20 /EHs-c- /GR-`). The port itself uses no C++20 feature; the standard is
chosen for the modernization phase that follows, and because C++20 is the last standard fully
implemented by every toolchain that matters later, including console SDK compilers (MSVC for
Xbox, clang/libc++ forks for PlayStation and Switch). C++23 is off the table until those
toolchains support it. Consequences: the legacy `msys32` CI job (gcc 7.1/9.3) is dropped;
the ~57 `shaderSort_t`-vs-`float` sites get a T17 cast during the port.

The comparison that led here, kept for the record:

Measured on 2026-09-13 with gcc 15.2 (same sources, `-Wall -Wextra -fpermissive`):

| Standard | Hard errors | Warnings | Difference vs C++17 |
|---|---|---|---|
| C++17 | 41 (ded) / 1152 (client+vk) | 1103 / 2739 | baseline |
| C++20 | identical | +0 / +20 | 20 `-Wdeprecated-enum-float-conversion`, all `shaderSort_t` compared with a `float` in renderervk; ~57 such sites across all three renderers |
| C++23 | identical | identical to C++20 | nothing new |

So the earlier claim that C++20 would cost "hundreds of non-mechanical edits" was wrong; the
measured cost is ~57 one-token casts (cast the enum to `float`, never the float to the enum,
so the comparison is unchanged). The standard choice is therefore almost entirely about
**toolchain floor**, because a strict port uses no feature from any of the three standards.

| Standard | Toolchain floor | Status in the current CI matrix |
|---|---|---|
| C++17 | gcc 7, clang 5, MSVC 2017 15.7, Apple clang 10 | every job passes, including legacy `msys32` (gcc 7.1/9.3) |
| C++20 | gcc 10, clang 10 (16 for full), MSVC 2019 16.10, Apple clang 13 | ubuntu-22.04-arm gcc 11 OK; `msys32` gcc 9.3 only via `-std=c++2a`, gcc 7.1 fails |
| C++23 | gcc 13, clang 17, MSVC 2022 17.7 (partial), Apple clang 16 (partial) | ubuntu-22.04-arm (gcc 11) fails, `msys32` fails, macos-14 partial |

Arguments that were made for C++17 (superseded by the decision above):
- Highest standard every existing CI toolchain accepts, so nothing in the matrix changes.
- Same codegen as C for this code; the differential gates (section 5) compare like with like.
- Zero deprecation noise; the `register` removals and literal-suffix spaces are needed for
  C++17 anyway and are already in the catalog.

What C++20 would buy later (not during the port): designated initializers (this code has none,
but it is the C idiom the port would otherwise lose), `std::bit_cast` for the `floatint_u`
punning, `<bit>` for the hand-rolled bit tricks in `q_math.c`, `std::span`, concepts,
`[[likely]]`. What C++23 adds on top: `std::expected`, `std::print`, deducing `this`,
`std::mdspan`, none of which matters to an engine port in 2026 unless the improvement phase
is designed around them.

Cost of C++20 today: dropping the `msys32` CI job and 57 enum-to-float casts. Accepted.

## 4. Strategy — "compile as C++ first, rename last"

Phase 0 — harness (no engine source changes)
1. Makefile: add `BUILD_CXX=1` that compiles engine sources with `$(CXX) -x c++ -std=c++20
   -fno-exceptions -fno-rtti`, drops C-only flags (`-Wstrict-prototypes`, `-Wimplicit`), and
   keeps vendored libs compiled as C. Same for CMake (`CMAKE_CXX_STANDARD 20`) and the vcxproj
   files (`<CompileAs>CompileAsCpp</CompileAs>`, libjpeg/ogg/vorbis stay `CompileAsC`).
2. CI: add a matrix leg that builds `BUILD_CXX=1` next to the existing C legs (Linux gcc + clang,
   macOS clang, MSVC). Keep C legs until phase 3. Remove the `msys32` job.
3. Local tooling installed (section 7), `compile_commands.json` via `bear` for clang-tidy.
4. Golden artifacts captured from the C build (section 5) and checked into `tests/golden/` or
   produced on the fly by CI from the C leg.

Phase 1 — make the tree compile as C++20, module by module, C build stays green
Order (dependency order, easiest first, each is one PR):
`qcommon` -> `server` -> `botlib` -> `unix` + `sdl` -> `client` -> `renderercommon` ->
`renderer` -> `renderervk` -> `win32` (CI-only via the windows-msvc and msys2 jobs). `renderer2` is not ported.
Exit criterion per module: compiles with `-std=c++20 -Wall -Wextra -Werror` (no `-fpermissive`),
and all verification gates (section 5) pass.

Phase 2 — `extern "C"` boundaries
Only where linkage is externally observable: `GetRefAPI` (dlopen'd renderers),
`dllEntry`/`vmMain` typedefs (native mod DLLs), symbols referenced from `code/asm/*.s`/`*.asm`
(`S_WriteLinearBlastStereo16_*`, `snd_p`, `snd_out`, `snd_linear_count`, `Q_setjmp_c`,
`Q_longjmp_c`, `CPUID_EX`, `Q_GetFPUCW`, `Q_SetFPUCW`), and `NvOptimusEnablement`/`AmdPowerXpressRequestHighPerformance`. Everything else gets C++
linkage. Only symbols resolved by name outside the C++ objects require T5. Address-taken
static functions called by generated JIT code require no C linkage. Verified by the symbol gate.

Phase 3 — rename
`git mv code/**/*.c -> *.cpp` for engine dirs only, update Makefile/CMake/vcxproj lists, remove
`BUILD_CXX` switch and the C legs. This commit must contain no content changes so `git log
--follow` and blame stay clean.

DECIDED: vendored libs (`libjpeg`, `libogg`, `libvorbis`, `libcurl` headers, `libsdl`)
stay C. Their headers already carry `extern "C"` guards.

DECIDED: the legacy `msys32` Windows CI job is dropped (gcc 7.1/9.3 cannot do C++20).

## 5. Verification gates (how we know agent output is a faithful port)

The core idea: the C build is the oracle. Every gate compares C-build output with C++-build output
produced from the same commit with the same flags.

G1. Build gate. Both builds green for: release + debug, client + ded, SDL + non-SDL, opengl + vulkan.
    C++ build under `-Wall -Wextra -Werror` (with an explicit, reviewed `-Wno-*` list that
    matches what the C build already tolerates).

G2. Struct layout gate (ABI). Compile both builds with `-g`, extract struct sizes/offsets from
    DWARF (`pahole` from the `dwarves` package, or a small script over `readelf --debug-dump`),
    diff. Zero differences allowed. Covers network structs, `entityState_t`/`playerState_t`,
    QVM interface structs, BSP/MD3/AAS file formats, `refexport_t`/`refimport_t`, botlib.

G3. Symbol gate. Run nm on defined and undefined symbols of each object, demangle with
    c++filt, normalize, and diff against the C object. Preserve static/external linkage and
    raw names for actual dlopen/assembly/export boundaries. Any f-suffixed libm reference
    present in C++ and absent in C fails; T21/T22 must pin the C semantics. Undefined
    ordinary C++ engine references are compared after demangling, not discarded.

G4. Codegen diff (advisory, not blocking). Compile each TU with `-O2 -S` under C and C++,
    demangle, normalize label names, diff. For pure-computation files (`q_math.c`, `cm_*.c`,
    `huffman*.c`, `msg.c`, `md4.c`, `md5.c`, `puff.c`, `unzip.c`, `snd_mix.c`) the diff should
    be empty or trivial. Non-empty diffs are routed to a human for review. This is the cheapest
    way to catch semantic drift (sign of `char`, enum width, short-circuit-to-`qboolean`
    conversion, `float`/`double` promotion).

G5. Differential unit harness. A small C harness (so it links against either build's objects)
    that drives pure functions with fixed seeds and prints a hash: `COM_Parse*`, `Info_*`,
    `Q_str*`, `va`, `Com_sprintf`, `MSG_Write*/Read*` round trips, Huffman encode/decode,
    `CM_LoadMap` + `CM_BoxTrace` sweeps on a bundled BSP, `Cvar_*`, `Cmd_Tokenize*`,
    `FS_*` path helpers, `Q_rsqrt`/`Q_fabs`/vector math, `NET_StringToAdr`. Output must be
    byte-identical between C and C++ builds.

G6. Runtime differential. Use the installed game data and libfaketime increment mode:

    timeout 90 faketime -f "@2026-01-01 00:00:00 i0.01" <binary> +set dedicated 1 \
      +set sv_pure 0 +set com_logfile 0 +map q3dm17 +addbot sarge 3 +addbot major 3 \
      +wait 300 +quit

    Keep timeout outside faketime. Increment 0.01 finishes in about one second; 0.001
    takes minutes. Do a warm-up run or ignore only the "...found N cached paks" line.
    Accept two identical C runs first, then identical C and C++ logs. If bots still
    diverge, compare a bot-less map load and record the limitation. This procedure was
    independently verified with three byte-identical 124-line C logs. The clocks seed
    both server and game module, so plain unfaked runs are not a determinism oracle.
    Client timedemo/frame/image comparison uses the same GPU/driver when a display and
    compatible demo are available; record unavailable verification explicitly.

    Verified deterministic procedure (2026-09-13): the bot smoke is nondeterministic on its own
    because the server and game module seed from the millisecond clock. Under libfaketime's
    increment mode the clock is a pure function of the call sequence and three consecutive C runs
    produced byte-identical logs (124 lines, 28 item events, ~1 s wall each):
      timeout 90 faketime -f "@2026-01-01 00:00:00 i0.01" <binary> +set dedicated 1 +set sv_pure 0 \
        +set com_logfile 0 +map q3dm17 +addbot sarge 3 +addbot major 3 +wait 300 +quit
    `timeout` must wrap `faketime`, not the reverse, or its clock is faked too and never fires.
    Increment 0.001 works but takes minutes; 0.01 finishes in about a second. Do one warm-up run
    or ignore the "...found N cached paks" line, which differs only between a cold and warm pak
    cache. Acceptance order: two C runs identical, then C vs C++ identical.

G7. Static analysis. Build with both gcc and clang (clang is not installed yet). clang-tidy with
    a deliberately tiny check list aimed at port mistakes only (e.g. `bugprone-signed-char-misuse`,
    `bugprone-narrowing-conversions`, `bugprone-suspicious-string-compare`), not style. No
    `modernize-*` checks: they push in the wrong direction for this project.

G8. Diff-shape review. For a mechanical port, changed lines per module should be a few percent.
    PRs are reviewed with `git diff -w --word-diff`; every hunk must map to an entry in the
    allowed-transformation catalog (section 6). A reviewer agent with that catalog as its
    checklist runs before a human looks.

## 6. Allowed transformation catalog (the whole allowed vocabulary for agents)

T1. Any implicit pointer conversion accepted by C and rejected by C++ in either direction,
    including function pointer <-> object pointer, gets a C-style cast to exactly the
    destination type already used by the C code. Preserve calling conventions and qualifiers.
T2. `qboolean` from a boolean expression: `x = (a && b);` -> `x = (a && b) ? qtrue : qfalse;`
    or `(qboolean)( expr )`. Pick one form per module and use it consistently.
T3. `int` -> enum: add a cast to the enum type. Never change the enum definition.
T4. Rename identifiers that are C++ keywords or alternative tokens (`new`, `operator`, `class`,
    `this`, `template`, `delete`, `and`, `or`, `not`, `xor`, `bool`, `true`, `false`, ...) with
    a minimal, local rename (`new` -> `newPacket`, `operator` -> `op`). Struct field renames must
    update every use; grep the whole tree.
T5. `extern "C"` only on the symbols enumerated in phase 2, via a `Q_EXTERN_C` macro guarded by
    `__cplusplus` so headers stay valid C. Applies only to symbols resolved by name outside
    the C++ objects (dlsym, assembly, exported entry points); address-taken static JIT
    callbacks need no linkage annotation.
T6. Compound literal -> named local temporary with identical initializer.
T7. `goto`/`switch` jumping over an initialization: move the declaration up (uninitialized
    declaration at block top, assignment where the initializer was).
T8. String literal constness: add `const` to the pointer variable/parameter only if it does not
    change a public header signature; otherwise cast at the call site.
T9. `restrict` -> `__restrict`. `static ID_INLINE` unchanged.
T10. Forward declaration order fixes (C++ needs the full type earlier) by moving a declaration,
     never by changing it.
T11. Missing prototype before use: add the prototype in the existing header where the definition's
     prototypes live, matching upstream style.
T12. `char` array initialization with a too-long string literal (C allows dropping the NUL):
     widen the array by one only if the array is local to the file; else flag as DEVIATION.
T13. Any `#ifdef __cplusplus` guard that upstream headers already use may be extended, not removed.
T14. Delete the `register` storage-class specifier (removed in C++17; gcc warns, clang/MSVC reject).
T15. Insert a space between a string literal and a following macro (`"x"MACRO` -> `"x" MACRO`).
T16. Narrowing in a brace initializer or enum (`0xFFFFFFFF` into `int`): add the cast that
     yields the same bit pattern; never change the constant.
T17. Enum compared with or converted from `float` (`shaderSort_t`): cast the enum to `float`
     at the use site. Never cast the float to the enum (truncation changes the comparison).

T18. A namespace-scope const object with external linkage in C gets a preceding extern
     declaration of the same object in the same file so C++ preserves that linkage.
T19. Hoist an enum nested in a struct immediately above that struct as a typedef with
     identical enumerators in identical order; use the typedef for the field.
T20. At strchr/strrchr/strstr/strpbrk/memchr calls whose C++ overload returns const, add
     const to the receiving local if compilation then succeeds; otherwise cast the call
     result to (char *). Never remove const from a parameter.
T21. At a C math call where C++ selects a float overload, cast each float argument to
     (double), selecting the same double function as C. Leave result conversion unchanged.
     This pins C semantics and does not permit expression restructuring.
T22. At abs() with a non-integer argument, cast that argument to (int).
T23. Wrap a feature-test macro definition in #ifndef when the C++ compiler predefines it.

T24. COM reference parameters (Windows only). In code/win32/win_local.h, before
     SDK includes, use the following C/C++ boundary macros:

    #ifdef __cplusplus
    #define CINTERFACE
    #define Q_REFGUID( g )     ( g )
    #define Q_REFGUID_PTR( r ) ( &( r ) )
    #else
    #define Q_REFGUID( g )     ( &( g ) )
    #define Q_REFGUID_PTR( r ) ( r )
    #endif

     Replace &SomeGuid passed to REFGUID/REFIID/REFCLSID with Q_REFGUID( SomeGuid ).
     Wrap a REFIID used as a pointer (win_snd.c memcmp) in Q_REFGUID_PTR( riid ).
     For the DIPROP_BUFFERSIZE pointer use Q_REFGUID( *guid ). C expansion preserves
     the original expression in parentheses; byte-identical C objects must be measured.
     REFGUID is const GUID* in C and const GUID& in C++, passed as a pointer under
     Microsoft x64 and Itanium ABIs; vtable ABI and calling conventions stay unchanged.
T25. Dual-source compound assignment on an enum. Only when the ordinary T3 form
     of e &= x or e |= x has been measured to change the C object hash, record both
     hashes and use:

    #ifdef __cplusplus
    <the T3 cast form>
    #else
    <the original line, untouched>
    #endif

     Still measure the unchanged C object hash; no hash exception is granted.
     After phase 3 remove these #else branches in a separate cleanup commit,
     verified by unchanged C++ object hashes. Review measured sv_client.c:1639:
     original C 4a9f0e56..., single-source casts 96b95b5a..., T25 path 4a9f0e56....
     These reviewer prefixes supplement the exact local artifact hashes in progress.md.

Everything not listed is a DEVIATION and gets its own commit and justification.

## 7. Local setup gaps found on this machine (2026-09-13)

Present: gcc/g++ 15.2, GNU make 4.4, binutils, 20 cores. Dedicated server builds.
Installed 2026-09-13 with:

```
sudo apt install libcurl4-openssl-dev libsdl2-dev mesa-common-dev libgl1-mesa-dev \
  libxxf86dga-dev libxrandr-dev libxxf86vm-dev libasound2-dev libvulkan-dev \
  clang clang-tidy clang-format cmake ninja-build ccache bear cppcheck dwarves
```

Verified after install (2026-09-13): default gcc build (client + ded + both dlopen renderers),
non-SDL X11 client, full clang build, and the CI-style static Vulkan client all pass. Tool
versions: gcc 15.2, clang 21.1, cmake 4.2, clang-format/tidy 21, pahole 1.31, SDL2 2.32,
libcurl 8.18. `libc++-dev` is not installed; only needed for the console-proxy CI leg later.

**CMake is broken upstream and is not a supported build for the port.** Three defects: the
`CMAKE_SYSTEM_PROCESSOR` regex classifies x86_64 as x86 (outputs named `_x86`), the renderer
shared libraries are not linked with `-lm` (`powf@GLIBC` versioned-symbol link error), and
`code/asm/snd_mix_x86_64.s` is missing from the client sources (`S_WriteLinearBlastStereo16_SSE_x64`
undefined). Only the dedicated server links. The Makefile is the only build the port targets;
CMake support is a modernization-phase item.

The client runtime gate (G6) needs a display. This shell has none (`DISPLAY` unset), so client
demo comparisons run from a desktop session or under Xvfb with Mesa's software drivers
(`llvmpipe` for OpenGL, `lavapipe` for Vulkan, package `mesa-vulkan-drivers`), which is also
how CI would run them. The dedicated-server half of G6 needs no display.

Optional: `gcc-multilib` for `ARCH=x86` builds (the x87 JIT and 32-bit determinism paths),
`mingw-w64` for cross-checking `code/win32` locally.

Game data: `~/.q3a/baseq3/pak0-8.pk3` (from the Steam install). Needed for G6 only.

Cross toolchains installed 2026-09-13 and verified with a C++20 test program: mingw
`x86_64-w64-mingw32-g++` 13 (the Makefile auto-detects it with `PLATFORM=mingw64 ARCH=x86_64`;
a dedicated-server cross-build produced a valid PE32+ executable), `aarch64-linux-gnu-g++`,
`arm-linux-gnueabihf-g++`, `powerpc64le-linux-gnu-g++` (all gcc 15.2), clang with libc++,
faketime, Xvfb. 32-bit multilib is deliberately not installed: it conflicts with the cross
compilers and 32-bit targets are out of the port's matrix.

## 8. Agent workflow

- Read `AGENTS.md` and this file. One module per PR, branch named `port/<module>`.
- Work loop per file: compile the single TU as C++ (`make BUILD_DIR=/tmp/cxx BUILD_CXX=1
  <object>` once phase 0 lands), fix only with catalog transformations, re-run C build for the
  module, run G2/G3/G4 for the touched objects.
- PR description template: module, list of transformation counts by catalog ID, DEVIATION
  commits with reasons, gate results (paste the commands and their outputs), anything logged to
  `docs/cpp-port-notes.md`.
- Reviewer agent checklist: every hunk maps to T1-T25; no whitespace-only churn; no reordering;
  no removed code; no new includes except `<cstdint>`-style shims if a header needs one;
  gates pass; diff proportion sane.

## 9. Decisions (all resolved 2026-09-13)

1. C++ standard: **C++20** (section 3).
2. Vendored libs (`libjpeg`, `libogg`, `libvorbis`, `libcurl`, `libsdl`): **stay C**. Their headers
   already carry `extern "C"` guards; they are upstream code, not engine code.
3. Legacy `msys32` CI job: **dropped**.
4. Game data for gate G6: Quake III Arena (Steam) paks copied to `~/.q3a/baseq3/` (pak0-pak8,
   483 MB), which the engine finds with no flags. Verified 2026-09-13: dedicated server loads
   q3dm17/q3dm7 with bots and shuts down cleanly. The two demos in pak0 are old `.dm3` format
   and will not play on protocol 68; record fresh `.dm_68` demos for the client half of G6.
   **OpenArena** paks for CI.
5. MSVC / vcxproj: **CI-only** during the port. No Windows machine here; the vcxproj file lists
   are updated in the rename phase and verified by the existing windows-msvc CI job.
6. `renderer2` (OpenGL2, disabled by default, upstream calls it unmaintained): **out of scope**.
   It is not ported and is removed from the build in the rename phase. The console direction
   (section 10) makes the Vulkan renderer the reference renderer anyway.

7. Continuation review: 32-bit x86 is excluded from port verification; remove ubuntu-x86
   and Windows x86 CI legs in phase 3. The explicitly requested arm-linux-gnueabihf cross
   target remains covered; x86 multilib is not required.
8. code/server/sv_rankings.c is excluded: no build configuration uses it and its SDK is
   proprietary. Do not port or rename it. code/asm/qasm.h is assembly-preprocessor input,
   not C++ input: done/not-applicable.
9. Installed cross compilers: x86_64-w64-mingw32-g++ (GCC 13), aarch64-linux-gnu-g++,
   arm-linux-gnueabihf-g++, powerpc64le-linux-gnu-g++ (GCC 15.2). Confirm availability;
   install nothing. Derive CXX from the same tool prefix as CC. Build each C oracle and
   record target object hashes before target-specific transformations. MinGW uses
   PLATFORM=mingw64 ARCH=x86_64. Replace inspection-only statuses with actual target gates.
10. Historical write-strings and parentheses warning deviations are accepted as ordinary
    frozen-list entries (243 and 48 observed C++ diagnostics respectively). The formatter
    is advisory as specified in section 11. Keep historical commits; do not rewrite history.
11. Continue per-file commits on t3code/port-engine-to-cpp20, push after modules, never
    main. After phase 3 push and watch CI; fix MSVC errors by inspection using T1-T25,
    repeat until green or an error genuinely requires an uncataloged transformation.

## 10. Deferred until after the port (recorded so it is not lost)

Modernization goal: keep the classic id Tech 3 / early Call of Duty feel (fixed-timestep sim,
snapshot netcode with prediction, cvars, pk3, POD structs, no per-frame allocation) while
targeting modern hardware including consoles. Constraints that follow, to become agent rules
when that phase starts, not now:
- No exceptions, no RTTI, fixed-width integers only (`long` is 32-bit on MSVC/Xbox), explicit
  `char` signedness where it matters (unsigned on aarch64).
- No JIT (console cert forbids executable memory): QVM compilers are PC-only, game code goes native.
- No runtime `dlopen`: static renderer linking becomes the primary configuration.
- OS access only inside `code/<platform>` and `files.c`; portable code never calls SDL/POSIX/Win32.
- Renderer behind an RHI with the Vulkan renderer as the base (D3D12 for Xbox, AGC for PS5,
  Vulkan/NVN for Switch). `renderer2` dropped; OpenGL1 renderer is legacy.
- CI proxies for console toolchains: clang + libc++ (x86_64 and aarch64 cross) and MSVC.
- 64-bit little-endian only; 32-bit x86, armv7 and ppc64 leave the matrix.
- Console SDKs are NDA-gated (ID@Xbox, PlayStation Partners, Nintendo Developer Portal); real
  console builds happen only once access exists.

## 11. Code rules (port phase vs modernization phase)

Two phases, two rule sets. The port phase rules are enforced now and are copied into
`AGENTS.md`. The modernization rules are recorded so they are decided once, not rediscovered.

| Topic | Port phase (now) | Modernization phase (later) |
|---|---|---|
| Language features | None. No `nullptr`, `auto`, references, classes, templates, STL, `constexpr`, namespaces, `using`. | C++20, feature-by-feature allowlist; never "because it is new". |
| Warnings | `-Werror` with a **frozen, checked-in list of disabled warnings** matching what the C build already tolerates (C build: 279 warnings under `-Wall -Wextra`; C++ adds ~800, mostly `-Wwrite-strings` and `-Wmissing-field-initializers`). | Re-enable one warning class per PR, each verified by the gates. Vendored libs stay `-w`. |
| Formatting | Match the surrounding line exactly: tabs, spaces inside parentheses `( a, b )`, `NULL`, C casts, `qboolean`. `.clang-format` is advisory. Enforce no whitespace-only engine hunks: per-file `git diff --stat` with and without `-w` must agree. No tree-wide reformat. | One tree-wide reformat commit after the port is verified, checked by byte-identical `-S` output before and after. From then on clang-format is authoritative. |
| clang-tidy | Small `bugprone-*` + `portability-*` subset, changed lines only (`clang-tidy-diff`). No `modernize-*`, no `cppcoreguidelines-*`. | Add `performance-*` and a readability subset. `modernize-*` advisory, enabled one check at a time. `cppcoreguidelines-*` cherry-picked, never wholesale. |
| Sanitizers | ASan + UBSan run on the **C build first** to record the baseline (needs game data); anything new in the C++ build is a port bug. UBSan blocklist for known-benign alignment in BSP loading. Same source has more UB as C++ than as C (union punning is defined in C11, undefined in C++). | ASan + UBSan on every CI run; TSan periodically for the SDL audio callback, WASAPI thread, and curl. |
| Memory / ownership | Untouched. | Ownership is expressed by arena (hunk = level lifetime with mark/free-to-mark, zone = tagged small allocs, temp hunk), not by per-object smart pointers. RAII only for OS/GPU resources at the platform boundary. `new`/`delete`/`malloc` forbidden outside the allocator layer. |
| Error handling | Untouched. `Com_Error` is a `longjmp`. | Decide explicitly, before any RAII lands in engine code: a `longjmp` over a stack object with a non-trivial destructor leaks or corrupts. Either RAII stays out of code paths that can `Com_Error`, or `Com_Error` changes shape first. Exceptions and RTTI stay off. |
| Assertions | None added (a firing assert is a behavior change). | `Q_ASSERT` with no side effects, compiled out in release identically; debug and release must compute the same simulation. |
| Integer types | Untouched. | Fixed-width types in every struct that hits the wire, a demo, or a file format. `long` is banned (32-bit on MSVC). Explicit `char` signedness where it matters (unsigned on aarch64). |
| Undefined-behavior patterns | Keep them: `Q_rsqrt` punning, file buffers cast to structs, `-ffast-math` on mingw. They define behavior demos and netcode depend on. | Replace with `std::bit_cast`/`memcpy` only when the codegen gate shows identical output. |
| Floating point / determinism | No floating-point expression restructuring in `qcommon/cm_*`, `q_math.c`, `bg_*`, `msg.c`, or server snapshot code; only T21/T22 argument casts that restore C evaluation are permitted. | Same rule, permanently. Cross-build determinism is what netcode and demos rest on. |
| Layout | `static_assert(sizeof)` table for wire/file/QVM structs, generated from the C build (gate G2). | Add `is_trivially_copyable` / `is_standard_layout` assertions for the same structs. |
| Subsystem boundaries | Keep the existing encoding: `Sys_`/`Com_`/`FS_`/`CL_`/`SV_`/`R_`/`S_`/`Cvar_`/`Cmd_` prefixes and `*_public.h` vs `*_local.h`. A subsystem includes only other subsystems' public headers. | Enforce with a CI grep. Namespaces, if ever, map one-to-one onto the prefixes. Each subsystem gets a short responsibility/ownership paragraph in `docs/`. |
| Scope per PR | One module or file group, only catalog transformations, `DEVIATION:` commits for anything else. | One feature or one warning class per PR. No unrelated refactoring. |
| Definition of done | Both builds green, every gate green, every hunk mapped to T1-T25, diff size proportionate, PR description lists transformation counts and gate output. | Builds green, sanitizers clean, tests (the differential harness plus whatever the feature adds) green, style checks green. |

Concrete artifacts this implies for phase 0: the frozen `-Wno-*` list in the Makefile/CMake,
`.clang-format` tuned against real files (tabs, `( a, b )` spacing, function brace on the same
line as in most of `code/client` and `code/qcommon`), a `.clang-tidy` with the small subset,
and a UBSan blocklist seeded from a C-build run.

