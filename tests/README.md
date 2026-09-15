# Permanent regression suite (#3)

Run from the repository root with Python 3, GNU Make, GCC or Clang, and binutils.
Tests call real engine functions with production flags. Test drivers provide only
isolated allocator/log/file stubs and instrumentation; production code is unchanged.

```
python3 tests/native_math.py
python3 tests/check_lifetimes.py
python3 tests/run.py unit --negative-control
python3 tests/run.py unit --cc clang --cxx 'clang++ -stdlib=libc++' --output /tmp/tests-clang
python3 tests/check_known_bugs.py
python3 tests/check_frames.py
python3 tests/run.py unit --cc clang --cxx clang++ --sanitize --known-bugs --output /tmp/tests-sanitized
python3 tests/run.py unit --cc clang --cxx clang++ --sanitize --pointer-compare --output /tmp/tests-pointers
```

The thirteen asset-free groups include wire/file layout. The negative control
moves the active GCC SSE Q_rsqrt return one ULP toward infinity in a temporary
source copy; the golden comparison must reject it. Clang requires libc++-dev and
libc++abi-dev. Cross compilation and MSVC builds run in CI; Linux executable tests
do not claim to execute on those targets.

`python3 tests/download.py` checks the real download begin/cleanup path with
libcurl, without a network transfer. It also compiles the option wrapper with
`-Werror=varargs` and checks long/pointer/offset forwarding through a local-file
transfer, including body suppression, private-data identity and a size limit. It requires libcurl development headers
and the library (`libcurl4-openssl-dev` on Ubuntu); hosted runtime CI installs them.
The URL cases cover bases with/without a trailing slash, `%1` templates, escaping,
and an empty base. File/cvar/UI operations are isolated by test stubs. Explicit
`python3 tests/download.py --regenerate` creates the reviewed URL golden; CI never
regenerates it. `--cc` and `--cxx` select the compiler as in the unit driver.

`python3 tests/audio.py` verifies the native ALSA callbacks have pthread-compatible
types, submits samples through both MMAP and DIRECT paths to ALSA's `null` output,
and joins both threads. It needs ALSA development files (`libasound2-dev` on Ubuntu)
and no physical audio device. CI installs those files in the runtime job. The test
uses the real ALSA implementation; wrappers count successful paths without replacing
the calls. `--cxx` selects the compiler. A missing device/library, no sample submission,
wrong callback type, or shutdown timeout fails the test.

`python3 tests/check_lifetimes.py` checks non-trivial locals, parameters, globals,
statics and temporaries in active Linux engine code and included engine headers,
using both renderer configurations. It requires clang-query (clang-tools in CI),
Clang and the client build headers. `--clang-query` selects a versioned executable;
`--output` retains the compile database and AST evidence. Its controls reject seven
owning objects (including aliases, inheritance, arrays and std::string), and accept
trivial/defaulted destructors and pointers. Platform/vendor directories are excluded;
inactive preprocessor branches remain part of self-review. See plan section 11 for
the permanent longjmp decision and narrowly permitted resource-wrapper boundary.

`python3 tests/native_math.py --cc clang` checks the imported native game Q_rsqrt
in optimized and ASan builds. Eight fixed result words come from the unmodified
GPL routine measured in a freestanding 32-bit SSE C executable. CI needs no 32-bit
runtime: it checks those words using the selected native C compiler. This source
is a dependency of #2 and is not linked into the engine yet. Its original GPL
source and import hashes are documented in docs/cpp-port-notes.md.

## Local Quake 3 content

```
python3 tests/run.py differential
python3 tests/run.py runtime
python3 tests/run.py runtime --sanitize --output /tmp/tests-runtime-ubsan
python3 tests/demo.py
```

These use the user's installed `~/.q3a/baseq3` paks and maps q3dm17/q3dm7.
`--data /path/to/baseq3` selects another installation. No Quake 3 paks are copied
into git or uploaded anywhere. The original unit, collision and smoke goldens
remain unchanged. Runtime needs faketime; demos also need Xvfb and Mesa's
llvmpipe/lavapipe. Tests fail when required content or tools are missing.

The completed in-process network check is `python3 tests/network.py`: both sides
use real engine loopback with `net_enabled=0`, test-only latency/loss/reordering,
and a 64-unit correction bound. Its `--max-error 0` negative control was run once
on 2026-09-14 and rejected the measured 8.875-unit correction. This requires local
Quake 3 content and is not run on the public-content hosted runtime runner.

## Hosted OpenArena content

The runtime CI job installs `openarena-data`, `faketime`, `xvfb`, and
`mesa-vulkan-drivers`, `libsdl2-dev`, `libcurl4-openssl-dev`, and `mesa-common-dev` for client compilation. It then runs:

```
python3 tests/openarena.py
python3 tests/run.py differential --content openarena --data /tmp/aftershock-openarena-baseoa
python3 tests/run.py runtime --content openarena --data /tmp/aftershock-openarena-baseoa
python3 tests/demo.py --content openarena --data /tmp/aftershock-openarena-baseoa
```

OpenArena uses `+set fs_game baseoa`, oa_dm1/oa_dm7, Sarge/Beret, and separate
`tests/golden/openarena/` outputs. The collision sweep uses oa_dm1. Its smoke
runs 900 waits to exercise combat; the accepted Quake 3 scenario stays at 300.

Ubuntu/Debian data packages replace QVMs with native-module markers unsupported
by this engine. `openarena.py` symlinks installed paks into temporary storage and
adds the official OpenArena gamecode oaxB52 release, verified by SHA256
`91cb4e677d1a9f1741391ebc5fe06054ed25073addf050baf80cfc7550f98c94`.
Release/source: https://github.com/OpenArena/gamecode/releases/tag/oaxB52
(GPL-2.0-or-later, source commit 331464ca396d80e91cf9be273588f2b5f4b7afc8).
Its license is retained alongside the staged data; no downloaded paks enter git.
Use `openarena.py --data /path/to/baseoa` for data extracted without installing
packages. Local system package installation is prohibited; runner installs are
expected. A download/checksum/content failure fails CI.

## Runtime and fixed-demo oracle

Dedicated smoke warms the pak cache and requires two identical traces before
comparing the golden. Timeout wraps faketime, SOURCE_DATE_EPOCH is fixed, and each
map has an isolated home. Normalization removes cached-paks/working-directory
lines and replaces home/data installation prefixes. OpenArena additionally
normalizes the CPU model label and disables networking; gameplay events are kept.
Raw logs are retained in `--output`, including failed invocations.

Normal demo runs **never record**. They replay each committed `.dm_68` twice per
software renderer, sample TGA frames at waits 50/100/200, require changing samples
and repeated byte-identical frame hashes, then compare `frames-mesa-VERSION.json`. Unknown Mesa versions fail. Pixel hashes
are exact within each version; cross-version rasterization is not assumed identical. Screenshots
and logs remain in `--output` for review. Fixtures are small engine-generated
artifacts, not game-content archives. Recording is intentionally not reproducible:
SDL/X11/Mesa clock calls affect faketime's call count and ping-derived demo bytes.

## Sanitizer known failures

`tests/known-bugs.txt` contains regexes for UBSan diagnostics exercised by the
sanitizer unit job. Each diagnostic must match; every listed pattern must occur.
A matching error prints `known, tracked in #31`. An unknown error, an ASan crash,
a nonzero subprocess exit, a changed unit golden, or a disappeared known error
fails. The test still runs and prints its diagnostics; this is not suppression.
A #31 fix removes its entry and updates the policy self-check if applicable.
The existing bot smoke also runs under GCC UBSan (`runtime --sanitize`), comparing
the same content goldens and treating every unsuppressed diagnostic as fatal.
Both compile and link steps enable UBSan. This uses the original GCC runtime
baseline; Clang's function check reads metadata before the generated JIT entry,
which can fall outside its mmap region. A diagnostic relink without that check
only in vm_x86.cpp passes both smoke goldens; no CI flag or suppression changed.
The transition JIT is removed in #2. Unit ASan/UBSan coverage remains required;
the ASan runtime experiment with faketime timed out before producing output.
The unit driver also initializes and frees real zlib state through its default
allocator callbacks, under the existing sanitizer modes. It reads no content and
reuses the unit allocation stubs; the ordinary unit golden stays unchanged.
The JPEG table-index check also lives in the unit driver. It compiles the actual
vendor routine and its probe as C, checking legal AC/DC table destinations and
expected index errors without file loading.
Pointer comparisons run separately from UBSan: combining both instruments Clang's
generated pointer-overflow checks and reports invalid pairs in otherwise valid
pointer increments. Both runs compare the same unit golden; neither replaces the
other. The pointer run catches extensionless names in `FS_AllowedExtension` before
the #31 fix, with `ASAN_OPTIONS=detect_invalid_pointer_pairs=2`.
PNG chunk alignment and JPEG table-index reproducers are already recorded in
issue #31 and `docs/cpp-port-notes.md`; they are not exercised by the unit driver.

The inherited `tools/port/ubsan.supp` remains unchanged. Leak checks are disabled
because the inherited isolated test allocator retains hunk allocations to exit.
Omit `--known-bugs` for fatal-on-first-error UBSan behavior.

## Explicit golden regeneration

Only an intentional issue-scoped change may replace affected goldens. Initial
OpenArena and demo baselines are established in #3; accepted Quake 3 baselines
must not be regenerated for infrastructure edits.

```
python3 tests/run.py unit --regenerate
python3 tests/run.py differential --regenerate
python3 tests/run.py runtime --regenerate
python3 tests/demo.py --record-fixtures
python3 tests/demo.py --regenerate
```

`demo.py --record-fixtures` explicitly records one new demo per map and replaces
frame goldens; `--regenerate` alone replaces frame goldens using existing demos.
Initial Mesa profiles can also be established from reviewed hosted artifacts,
without ever running regeneration in CI:

```
gh run download RUN_ID -R msetaro/aftershock -n runtime-diagnostics -D /tmp/replay-evidence
python3 tests/frames.py --output /tmp/replay-evidence/aftershock-demo-tests --content openarena --regenerate
```

Review the job's fixture hashes and all screenshots first. The evidence checker
requires both repetitions, both renderer identities, one Mesa version, and three
changing samples per map. This explicit local command hashes the downloaded TGA
files itself; it does not trust a hash manifest supplied by CI. Each initial
profile and its run/source provenance must be explained in the PR.
Add the same `--content openarena --data /tmp/aftershock-openarena-baseoa` arguments
to select that content set. Review demo logs/screenshots and explain every changed
hash or gameplay event in the PR. All golden writes are rejected when `CI` is set;
CI compares committed outputs and never regenerates them.

### Native C transition (#2, in progress)

The pinned GPL C imports and provenance are in `docs/native-game-import.json`.
These Linux x86_64 commands build the native modules and compare their shared ABI
layouts against the engine; installed Quake 3 content is required for runtime/replay:

```
python3 tests/native.py
python3 tests/native.py --cc clang --cxx clang++ --output /tmp/native-clang
python3 tests/run.py runtime --game-code native
python3 tests/demo.py --game-code native
```

The native ABI uses binary32 literals and rounds host math results to float, as
the QVM compiler does; `bg_lib.c` preserves the QVM random sequence. Temporary DLL
entry points marshal pointer-width words until static integration removes them.
Replay uses the same committed demos/frame hashes. Smoke removes only module load
metadata, build date and bot-skill printf padding before comparing the accepted QVM
log. Gameplay text is retained. The OpenArena comparison also removes its VM-only
magic/version and jump-table compilation metadata. Both Quake 3 maps and both-renderer
replay pass after #31 PR #51 fixed the engine's uninitialized movement result.
Native parity commands reject regeneration; the accepted QVM default is unchanged.

Hosted native parity uses the pinned OpenArena B52 C source and the #31 patches:

```
python3 tests/native.py --content openarena
python3 tests/run.py runtime --game-code native --content openarena --data /tmp/aftershock-openarena-baseoa
python3 tests/demo.py --game-code native --content openarena --data /tmp/aftershock-openarena-baseoa
```

The runtime job runs both commands against the existing OpenArena goldens. Source is
exported from revision 331464ca396d80e91cf9be273588f2b5f4b7afc8 into the test output;
its original GPL notices remain. Only native ABI entry/call adaptation is generated;
reviewed bug patches remain separate. Original module lists select the base q3_ui
sources, and bg_lib preserves QVM random/sort behavior. This external dependency
builds as C with GCC or Clang; --game-language c++ applies to the imported Q3 port.
Its 28 identical structure sizes and three offsets are compared to the engine,
plus the consumed 140-byte refEntity prefix (OpenArena appends 36 eye-vector bytes).
No native game content is downloaded or committed.

### Bot movement result regression

`python3 tests/bot_move.py` calls the production `BotMoveToGoal` early return with
two poisoned output buffers and checks all result fields. It links the dedicated
server objects with a test entry point, so no content is required. `--cc`, `--cxx`
and `--output` select the same compiler matrix as the other unit checks.

`python3 tests/native_dispatch.py` checks native engine calls with zero through
three arguments, including zero-filled unused slots and balanced call depth. It
uses the production VM_Call body and requires no game content. The same compiler
and output options as bot_move.py apply.

`python3 tests/teamleader.py` compiles both imported GPL C team-leader paths with
Clang's bounds diagnostics as errors. It uses the imported bot-state declarations
and native ABI header. No game content or external header checkout is required.
The original #31 failing test used pinned GPL headers before #2 imported them.

Native C++ port checks use `python3 tests/native.py --language c++` and append
`--game-language c++` to native runtime/demo commands. C remains the transitional
reference (the default). Both languages compare the same 29 layouts and three
offsets with the engine; module links reject unresolved symbols. GCC and Clang
use explicit binary32 source literals without compiler-specific literal flags. For
Clang C++ only, bg_lib.c is
compiled separately with __NO_INLINE__ to avoid glibc's conflicting inline atof
definition; this header setting preserves the Clang C object byte-for-byte and
does not disable the optimizer's inlining. Other translation units keep their
original standard-library headers and calls.

`python3 tests/native_shared.py` compares the real shared native C and C++ functions
on the same host: 4,096 angle/vector/normalization/inverse-square-root cases (including
zero and quadrant angles), plus all nonzero byte values through Q_strlwr/Q_strupr in
the C locale. It compares raw result words, requires no assets or golden writes,
and accepts --cc/--cxx/--output. Both unit compiler jobs run it. The C++ native build
pins the C library feature set to the C99 reference, avoiding C23 scanf/strtol
redirection from the C++ compiler's default _GNU_SOURCE.

`python3 tests/openarena_strings.py` verifies the OpenArena native CI dependency's
case-sensitive name comparison (missing names and single argument evaluation) and
in-place extension stripping (model suffix paths, bounded truncation, empty input
and capacity one). It fetches pinned public source from OpenArena/gamecode
revision 331464ca396d80e91cf9be273588f2b5f4b7afc8 when the source cache is absent,
then applies the name-comparison and extension patches in tests/patches to its output
directory. The extension probe links the actual q_shared.c helper.
No game content is fetched by this check. --cc, --source and --output select the
compiler/cache/output. Both unit compiler jobs run name comparisons under UBSan and
extension stripping under ASan. The source patches are for #2's native OpenArena
configuration; the existing QVM fixtures remain unchanged. Original GPL notices remain in the fetched headers.
