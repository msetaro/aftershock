# C++ port notes — bug dispositions

The strict port made no engine bug fixes. Modernization #31 dispositions are recorded below.

## Current disposition

The original twelve engine/vendor defects are closed with merged tested fixes
below; final regression 34892331846 passed on 9a7c2625. #2 native import preflight
found an additional LP64 game-math defect; its separate fix is merged PR #49. Merged-tree regression 34895239211 passed
on 2018564f. #31 movement-result fix is merged as PR #51 / 5ecf43e5 with merged-tree regression
34900871236 passed. #31 remains the tracker for subsequently found bugs. The formatter capacity
defects below are fixed in #99/#100; native dispatch and team-leader fixes are
recorded in their later entries. `tests/known-bugs.txt` has no active entries and
`tools/port/ubsan.supp` is empty.

## Native pure-server connection rejected after VM removal (#31)

An ordinary native client connecting to a password-protected sv_pure=1 server
loads the map and static cgame, then is dropped before ClientBegin. Found during
#28 local match packaging using installed OpenArena plus the owned level; logs
are ~/.cache/aftershock-modernization/match-runtime.log and
/tmp/aftershock-match-runtime/{client,server}.log. The server still requires pak
checksums for vm/cgame.qvm and vm/ui.qvm. Native modules do not load/reference these
files, so the client pure list has no corresponding entries. This is independent
of the owned level or password authorization.

`python3 tests/native_pure.py` compiles the real filesystem implementation and
fails on the first missing native-marker assertion on merge 6a3cb22d, before any
fix (native-pure-before.log). Preserve actual content checksum verification; the
native module code/ABI is provided by the executable/protocol agreement, not QVM
pak files. This new functional bug has no sanitizer suppression/expected-failure
entry. The correction uses explicit zero native-module markers while retaining all actual
content checks. GCC and Clang/libc++ filesystem probes pass; tests/native_pure_runtime.py
fails on the pre-fix #27 binaries and passes with both Q3 and OA after the fix.
Unit golden 8d44421d is unchanged; no accepted fixture is regenerated. Full hosted
PR/integration acceptance remains pending on issue/31-native-pure.

## Weapon loopback wall deadline and cleanup (#151)

#11 merged-tree runtime 35534705876 failed in the test harness on a slower software
renderer. Frame-counted input outlasted the fixed 45-second wall deadline despite
1147/1147 matching weapon/animation acknowledgements, both switches and a completed
grenade. The subsequent five-second graceful shutdown timeout masked the scenario
failure and skipped proxy/server cleanup until the outer limit. This is test
infrastructure; no engine defect, accepted-golden change or sanitizer entry is involved.

Existing failure: runtime job 106141530168 and its runtime-diagnostics artifact.
The lower-FPS real-client control now completes in 54.0 seconds: 301/301 shots,
22 hits, 49 uncompensated differences, and 1511/1511 weapon/animation comparisons.
Its 200 ms median view age includes the deliberate 50 ms frame interval and stays
inside the configured rewind window. The original 100-FPS bound remains 180 ms.
The scenario gets 120 seconds inside a 240-second outer limit; all hit/state checks
are retained. `tests/netcode_cleanup.py` first failed on the absent stop_client
helper (netcode-cleanup-before.log), and now verifies forced kill/reaping when a
private child ignores SIGTERM. Full CI/integration acceptance remains pending in
https://github.com/msetaro/aftershock/issues/151 .

| Defect | Fork fix | Upstream |
|---|---|---|
| Huffman packed read | [#36](https://github.com/msetaro/aftershock/pull/36) | [#424](https://github.com/ec-/Quake3e/pull/424) |
| Extension NULL comparison | [#37](https://github.com/msetaro/aftershock/pull/37) | [#425](https://github.com/ec-/Quake3e/pull/425) |
| Download trailing slash | [#38](https://github.com/msetaro/aftershock/pull/38) | [#426](https://github.com/ec-/Quake3e/pull/426) |
| ALSA thread signatures | [#39](https://github.com/msetaro/aftershock/pull/39) | [#427](https://github.com/ec-/Quake3e/pull/427) |
| curl va_start enum | [#40](https://github.com/msetaro/aftershock/pull/40) | Port-specific |
| ZIP packed fields | [#41](https://github.com/msetaro/aftershock/pull/41) | [#428](https://github.com/ec-/Quake3e/pull/428) |
| VM packed operands | [#42](https://github.com/msetaro/aftershock/pull/42) | [#429](https://github.com/ec-/Quake3e/pull/429) |
| zlib allocator callbacks | [#43](https://github.com/msetaro/aftershock/pull/43) | [#430](https://github.com/ec-/Quake3e/pull/430) |
| Extension diagnostic output | [#44](https://github.com/msetaro/aftershock/pull/44) | [#431](https://github.com/ec-/Quake3e/pull/431) |
| AAS missing jump candidate | [#45](https://github.com/msetaro/aftershock/pull/45) | [#432](https://github.com/ec-/Quake3e/pull/432) |
| PNG chunk-header alignment | [#46](https://github.com/msetaro/aftershock/pull/46) | [#433](https://github.com/ec-/Quake3e/pull/433) |
| JPEG table index | [#47](https://github.com/msetaro/aftershock/pull/47) | [#434](https://github.com/ec-/Quake3e/pull/434) |

CMake defects belong to #5. T21 resolved the math-overload compatibility hazard.
Clock-dependent smoke/recording observations are verification limitations handled
by the documented smoke clock and fixed-demo replay. The Clang JIT instrumentation
and ASan/faketime observations below are verification limits, not passing gates;
#2 removes the transitional JIT. The additional native-math defect is assigned to #31 below.

## Vulkan image acquisition status found during #6

Source audit at 2b43a0bb found that `vk_begin_frame` in
`engine/renderervk/vk.cpp` accepts every nonnegative `vkAcquireNextImageKHR`
result and sets `swapchain_image_acquired`. `VK_TIMEOUT` and `VK_NOT_READY`
do not supply an acquired image; only success/suboptimal results do. See the
[Vulkan acquisition contract](https://docs.vulkan.org/refpages/latest/refpages/source/vkAcquireNextImageKHR.html).
The permanent `python3 tests/vulkan_acquire.py` executes the real `vk_begin_frame`
with controlled GPU callbacks and stops at command recording; it creates no GPU
or window. On integration e82eb43b, success/suboptimal cases pass, but timeout and
not-ready cases both exit 1 because command recording is reached without an image
(expected: error callback exit 42 with acquired state false). Evidence:
~/.cache/aftershock-modernization/vulkan-acquire-before.log. No actual driver
failure has been observed in normal replay. This separate test-first #31 PR must
pass before #6's frame lifecycle gate is complete.
No fix belongs in the RHI extraction PR, and no golden changes are anticipated
for successful rendering.

Test-first ea17a6ba is followed by the single-condition correction: only
VK_SUCCESS/VK_SUBOPTIMAL_KHR permit acquisition. Timeout/not-ready use the existing
fatal acquisition-error path; the existing out-of-date retry remains. Both GCC and
Clang/libc++ now pass all four cases, and fixed-demo video-restart replay retains
b38004b1. No active known-bug/suppression entry exists for this new defect and none
is added; no accepted golden or fixture regeneration is warranted. PR #141 merged as 61401e17 after head 7382d9af passed full build 35484485400
and regression 35484485349, with AGENTS self-review. The fix enters #6 through
that integration merge; merged-tree regression 35484895454 passed.

## Formatter capacity defects found during #8

Two formatter defects reproduced while inventorying Apple deprecations for #8; fixes will be separate test-first #31 PRs, only in msetaro/aftershock.

1. Both engine/qcommon/q_shared.cpp and game/bg/q_shared.cpp call unbounded vsprintf into Com_sprintf's 32,000-byte temporary before checking its result. A 32,000-character string plus its terminator writes past the temporary; both real implementations exit 1 with ASan stack-buffer-overflow before their existing Com_Error guard.
2. Both va implementations use two 32,000-byte static slots with unbounded vsprintf. Formatting that same string twice reaches the second slot and exits 1 with ASan global-buffer-overflow in both implementations.

Reproduction uses the real engine helper compiled as C++20 and the GPL helper compiled as C99, -O1 -g -fsanitize=address -fno-omit-frame-pointer -ffunction-sections -fdata-sections, linker --gc-sections, and only stub Com_Error/Com_Printf. The probe uses a static 32001-byte NUL-terminated input filled with 32000 x characters. It calls Com_sprintf(output, sizeof(output), "%s", input), or va("%s", input) twice. Com_Error's stub exits 42, so ASan exit 1 establishes the overflow precedes the guard. No engine fixes or accepted golden changes yet.

Artifacts: ~/.cache/aftershock-modernization/format-capacity.cpp, format-capacity-{engine,game}-{sprintf,va}.log, format-capacity-results.json. Permanent small regression commands will be added test-first in the respective #31 branches. The ordinary regression suite currently does not exercise these boundaries.

Com_sprintf test-first commit b3c44459 adds `python3 tests/format.py`: the old
implementation fails with ASan before its guard, while normal text, destination
truncation and in-place formatting pass. Bounding the shared temporary with
Q_vsnprintf (engine) / vsnprintf (native game) makes all six GCC/Clang C/C++
variants pass under ASan/UBSan. PR #99 merged 1a20f502 after build 35457958368 and regression 35457958406 passed.
Fixed Q3 replay retains b38004b1. The va defect is separate and still unfixed; its
permanent test extension preserves valid lengths/rotation and reproduces global
overflow in engine C++, game C and game C++. No accepted golden changes.

The va test-first commit 9150f6e9 extends `tests/format.py` with valid output,
two-slot rotation and the boundary reproducer. Both real implementations fail
under ASan before the fix. Bounded Q_vsnprintf/vsnprintf calls now reject output
that does not fit the selected slot. Decision: report ERR_FATAL, consistent with
Com_sprintf, rather than returning truncated filenames/commands. All six GCC/Clang
engine C++ and native C/C++ ASan+UBSan variants pass; hosted gates/merge remain
complete: PR #100 merged a3191a24 after build 35458501955 and regression
35458501948 passed. Fixed Q3 replay retains b38004b1. No accepted fixtures or
goldens changed.

## Historical observations and validation

The entries below preserve what was known at each checkpoint. Statements such as
“unchanged”, “pending” and “suppressed” describe that checkpoint; the disposition
table above and subsequent validation entries give the current state.

- Upstream CMake defects already documented in cpp-port-plan.md section 7 remain untouched.
- Phase 0 found that C++ math-header overload resolution changes unchanged q_math.c
  from double `sincos` to float `sincosf`. This is a port compatibility hazard, not
  a defect in the C oracle. The exact G4 diff and disposition are tracked in
  cpp-port-progress.md. The accepted T21 catalog now pins these C math calls.

- C ASan/UBSan baseline (gcc 15.2, q3dm17, Sarge/Major): unaligned int
  loads in unzip.c:1523 and :1524 and vm.c:1181. These are existing byte-packed
  file/VM access patterns; leave them unchanged per the plan. Runtime exited 0,
  no ASan error reported. Leak checking was disabled for this initial baseline.
  Narrow function alignment suppressions are seeded in tools/port/ubsan.supp;
  suppression effectiveness was confirmed by the subsequent C runtime smoke (exit 0, no sanitizer diagnostics).

- The q_math hazard is confirmed by `tools/port/math_gate.sh`: fixed-input hashes
  differed before T21; both math gates now pass, including vector_math 5c00b4de.
  The unchanged C implementation remains the oracle.

- The exact unattended bot smoke is nondeterministic for two runs of the same C binary: Item events differ because engine/game initialization uses wall-clock seeds. This is a verification limitation, not a new engine bug; no timing or seed behavior changed. The full repeat diff is retained in tools/port/evidence/c-runtime-repeat.diff.

- linux_snd.c passes void(void) thread procedures through void* to pthread_create, whose callback type is void*(*)(void*). This preexisting signature mismatch remains; T1 casts wrap the existing conversion without changing thread bodies or signatures. C object hash remains identical.

- cl_curl.c Com_DL_Begin tests dl->URL[strlen(dl->URL)] against slash. That index is the terminating NUL, so the slash append always executes when percent-1 URL replacement fails. Recorded during constness review; source unchanged.

- FS_AllowedExtension compares the result of strrchr relationally to fileName + 3 before checking it for NULL. Inputs without an extension reach a relational comparison involving NULL; this existing undefined pointer comparison was noticed while preparing G5. No source fix.

- The prescribed libfaketime increment procedure resolves the earlier unfaked bot-smoke repeat limitation: two warmed C logs are now byte-identical. No engine timing or seed code changed.
- Clang C++ warns that qcurl_easy_setopt_warn uses a CURLoption enum as the last named va_start argument (cl_curl.c:264); the enum undergoes default argument promotion. This preexisting source pattern is left intact; no public signature or varargs logic is changed.

- Sanitized GCC build warns that AAS_Reachability_JumpArea beststart may be uninitialized at be_aas_reach.cpp:2196. The original C sanitizer build reports the same warning. If the nested qualifying-face/edge loops never supply a candidate, VectorMiddle reads beststart before the later bestdist check. Recorded without initialization/control-flow changes.

- Modernization #3 differential driver under Clang ASan/UBSan reports an unaligned
  `const uint32_t` load in `HuffmanGetSymbol` at huffman_static.cpp:206 when reading the
  existing MSG roundtrip fixture (buffer offset one). Reproduce:
  `python3 tests/run.py unit --cc clang --cxx 'clang++ -stdlib=libc++' --sanitize --output /tmp/aftershock-san-tests`.
  Existing tools/port/ubsan.supp does not cover this function; no new suppression or
  engine fix was added. Repair belongs in a separate #31 failing-test-first PR.

- Modernization #3 seeded PNG fuzzing reports `PNG_ChunkHeader` member access on a
  byte-packed chunk at tr_image_png.cpp:467 (the valid 1x1 RGBA seed already triggers
  it). Reproduce: `python3 tests/fuzz/run.py png --runs 1000`. No inline fix.
- Modernization #3 JPEG fuzzing reports a Huffman-table index outside the four-entry
  array at libjpeg/jdmarker.c:508: get_dht forms the table-element address before
  rejecting an invalid index. Reproduce: `python3 tests/fuzz/run.py jpeg --runs 1000`;
  observed crash SHA1 1868dc3f9cb6875028fd4f795a1d596b87706c16 under /tmp/aftershock-fuzz/jpeg.
  Vendor code remains untouched; a dedicated #31 test/fix/upstream PR is required.

- #31 Huffman alignment fix: `HuffmanGetSymbol` now copies the packed word with
  memcpy before applying the unchanged shifts/mask. The fatal sanitizer unit
  reproducer failed before and passes after. GCC production assembly/symbols and
  regenerated unit/differential goldens are identical; both-map bot logs and
  fixed-demo frames remain identical. Removed its known-bugs entry; it never had
  a UBSan suppression. Upstream C regression covers 256 symbols at 32 bit offsets.
  Upstream C fix/test: https://github.com/ec-/Quake3e/pull/424.

  Huffman fork fix merged: https://github.com/msetaro/aftershock/pull/36.

- #31 filesystem extension validation: extensionless names (`"no_extension"`,
  `"a"`, `""`) and short dotted names (`"."`, `".x"`) now have permanent assertions.
  Reproducer: `python3 tests/run.py unit --cc clang --cxx clang++ --sanitize
  --pointer-compare --output /tmp/tests-pointers`. Before: ASan invalid-pointer-pair
  in FS_AllowedExtension, NULL versus filename+3, exit 1. After moving the NULL
  check first and using `(e - fileName) >= 3`: pass with unchanged unit hash.
  Separate UBSan and pointer-comparison runs avoid combined-instrumentation
  interference seen with Clang 21 while retaining both checks in CI. Upstream C
  fix and failing-then-passing test: https://github.com/ec-/Quake3e/pull/425.
  Fork PR #37 passed regression 34877286456 and full build 34877286443.

- #31 caller audit finding (separate fix pending): Unix and Windows Sys_LoadLibrary
  pass an uninitialized local `ext` to Com_Error when FS_AllowedExtension returns
  true. The latter only writes `ext` when rejecting an executable extension.
  Reproducer path: Sys_LoadLibrary with an extensionless or non-executable name
  reaches the error formatter with an indeterminate `%s` pointer. Recorded from
  call-flow inspection, not executed; no loader test or source fix in this PR.

- #31 download URL separator fix: Com_DL_Begin indexed the terminating NUL instead
  of the last character. `python3 tests/download.py` fails before on a trailing-slash
  base producing a double separator and passes after the guarded last-character
  check. GCC/Clang test calls real begin/cleanup with libcurl, but never performs
  a transfer. New golden `tests/golden/download.txt` records bases with/without `/`,
  percent-1 replacement/escaping and the preserved empty-base result. No existing
  gameplay or frame golden changes. Caller audit: CL_Download is the sole caller
  and already rejects empty configured URLs; the shared fix still guards empty input.
  Upstream C test/fix: https://github.com/ec-/Quake3e/pull/426.
  Local unit/negative-control, collision, smoke and replay gates pass unchanged.
  Fork PR #38 passed regression 34878247137 and full build 34878247337.

- #31 ALSA callback fix: both native audio thread procedures now have the required
  `void *(*)(void *)` signature; pthread_create no longer needs casts. Returning NULL
  replaces the explicit pthread_exit call and its unused import. Reproducer:
  `python3 tests/audio.py` fails compilation before and passes after with GCC and
  Clang/libc++. The real ALSA null sink receives positive MMAP/DIRECT submissions,
  then both threads join. No physical device is needed. Caller audit finds only
  the two setup_ALSA registrations. The dynamic-ALSA production object also builds.
  Upstream C test/fix: https://github.com/ec-/Quake3e/pull/427. Unit/collision
  regeneration produces no golden diff; both-map smoke and both-renderer replay pass.
  Fork PR #39 passed regression 34879358567 and full build 34879358584.

- #31 curl va_start fix: the last named argument is now int; a CURLoption local
  preserves the forwarding logic. `python3 tests/download.py --cc clang --cxx
  'clang++ -stdlib=libc++' --output /tmp/tests-curl` fails before on -Werror=varargs
  and passes after, including a local-file transfer verifying long/pointer/offset
  options. Removed the Makefile's -Wno-varargs. URL golden is unchanged.
  Correction to the original classification: Clang C/C11 accepts the upstream
  source, and a type probe finds CURLoption compatible with its promoted C type
  (unsigned int). C++ distinguishes the enum and rejects va_start. This is a port
  defect on our toolchains; no failing upstream C test or upstream PR is claimed.
  The pre-C++26 parameter restriction is described in WG21 P2537R2:
  https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2537r2.html.
  GCC production codegen and symbol gates are identical. Unit/one-ULP, collision,
  serial both-map smoke and both-renderer replay pass unchanged. All callers are
  within cl_curl.cpp; no public header declaration or wire layout changes.
  Fork PR #40 passed regression 34880567812 and full build 34880567796.
  The normalized symbol gate passes; the internal C++ mangled name changes with
  the parameter type, and all callers rebuild in the same translation unit.

- #31 ZIP packed reads: removed unzlocal_GetCurrentFileInfoInternal's alignment
  suppression. `python3 tests/run.py runtime --sanitize --output /tmp/tests-runtime-ubsan`
  is the existing valid-content bot smoke under GCC UBSan. The original ZIP object
  fails at the unaligned int load; CopyLittleLong through a signed temporary passes
  both original Q3 map goldens. Keeping the temporary signed preserves the original
  conversion to the wider uLong fields. All archive iteration paths use the same
  shared function (file-info queries and first/next/position navigation). Both OA
  smoke maps and normal unit/collision/smoke/replay gates also pass unchanged.
  Symbol gate passes; GCC assembly only exchanges stack slots 64/68 and matching
  sign-extending consumers, reviewed acceptable under #31. No gate weakening.
  Upstream C reproducer fails before and passes after:
  https://github.com/ec-/Quake3e/pull/428. Fork PR #41 passed regression
  34885119593 and full build 34885119668 on source 09e7cc96.

- Additional #31 finding during runtime expansion: Clang UBSan reports zcalloc
  and zcfree calls through incompatible function-pointer types. Their private
  signatures use byte pointers while z_stream callbacks use void pointers.
  Reproduce after the ZIP alignment fix with `python3 tests/run.py runtime --cc clang
  --cxx clang++ --sanitize --output /tmp/tests-clang-runtime`. The error occurs
  during ordinary valid-pak startup. A recovery survey reports both callback kinds
  before a later SIGSEGV at legacy QVM startup (cause not yet isolated). No callback
  fix or new suppression in the ZIP PR; the GCC runtime baseline and existing
  Clang unit sanitizer checks remain the required gates. The ASan/faketime runtime
  experiment timed out before output; it is not treated as an engine failure or pass.

- #31 VM packed operand fix: the existing `python3 tests/run.py runtime --sanitize`
  fails before at VM_LoadInstructions with its alignment suppression removed.
  CopyLittleLong into an int32_t temporary passes the original Q3/OA map goldens;
  tools/port/ubsan.supp is now empty. The interpreter and x86/aarch64/armv7/powerpc
  compilers all call this shared function. GCC production symbols pass and all
  26 function-section byte sequences are identical. The text codegen gate differs
  only in the compiler-generated switch-table name CSWTCH.89/90, with unchanged
  instructions and table data; no gate weakening. Upstream C startup on a valid
  qagame QVM fails before and passes after (only its separate ZIP bug suppressed).
  Upstream fix: https://github.com/ec-/Quake3e/pull/429. Explicit unit/collision
  regeneration has no golden diff; normal smoke and fixed-demo replay pass unchanged.
  Fork PR #42 passed regression 34886047536 and full build 34886047431.

- #31 zlib callback fix: zcalloc/zcfree now use the z_stream void-pointer types,
  with direct assignments and unchanged allocation/free behavior. The existing
  unit driver initializes/frees real inflate state through its allocator stubs,
  without content input. Permanent Clang ASan/UBSan test fails before and passes
  after; pointer-check mode also passes. No expectation or suppression was added.
  GCC normalized production codegen/symbol gates pass (private callback mangled
  names change). Explicit unit/collision regeneration has no golden diff; normal
  and GCC UBSan smoke and fixed-demo replay pass unchanged. Upstream C test/fix:
  https://github.com/ec-/Quake3e/pull/430. The separate Clang QVM startup observation
  remains unclassified; this fix resolves the callback diagnostics only.

- Clang QVM crash classification: instrumented VM_CallCompiled emits a function
  signature probe at codeBase-8 before the indirect JIT entry call. VM_Alloc_Compiled
  supplies a fresh mmap region beginning at codeBase, without preceding sanitizer
  metadata. Disassembly confirms the probe. Temporary relink with only vm_x86.o
  built using -fno-sanitize=function passes both original Q3 smoke goldens; every
  other UBSan check remains. Evidence: /tmp/aftershock-clang-jit-check.py and .log.
  This is a transition-JIT instrumentation compatibility limitation, addressed by
  JIT removal in #2. No engine fix, CI flag change or suppression was added.
  Callback fork PR #43 passed regression 34886950022 and full build 34886949860.

- #31 extension diagnostic fix: FS_AllowedExtension now always writes a supplied
  extension output, using an empty string when there is no dot. This repairs both
  Sys_LoadLibrary diagnostics in the shared function. Versioned .so and rejected
  suffix output/classification stay unchanged. Existing unit assertions fail before
  and pass after; no library is loaded by the test. Clang sanitizer/pointer checks,
  one-ULP control, collision, smoke and replay pass. Explicit unit/collision
  regeneration has no golden diff. Symbols pass; only FS_AllowedExtension changes
  bytes among 99 functions (the text diff also renames an FS_Seek switch-table label).
  Upstream C regression/fix: https://github.com/ec-/Quake3e/pull/431.
  Extension fork PR #44 passed regression 34887856044 and full build 34887856072.

- #31 AAS missing jump candidate: the correct function name is
  AAS_Reachability_Jump, not the earlier JumpArea wording. If bestdist retains
  its initial 999999 sentinel, return false before calculating midpoints from
  unset endpoint vectors. Candidate arithmetic is unchanged. Removed the Makefile
  GCC sanitizer warning exception that explicitly names this defect. The existing
  runtime --sanitize build fails before on beststart and passes after, then compares
  original smoke goldens. Upstream C compilation reproduces the same warning/error.
  Production codegen/symbol gates differ: jump/grapple functions use an outlined
  VectorLength helper, with register/stack changes and removal of the direct sqrtf
  import. Source expressions are unchanged. The actual emitted helper matches a
  libm reference for 4 million finite-input cases across four rounding modes:
  /tmp/aftershock-aas-length-check.cpp and .log. Differences reviewed acceptable
  for this tested bug fix; no gate weakening or identical-codegen claim.
  Upstream C fix: https://github.com/ec-/Quake3e/pull/432. Unit/collision
  regeneration changes no golden; final Q3/OA UBSan smoke, normal smoke and
  fixed-demo replay pass unchanged.
  Fork PR #45 passed regression 34889484418 and full build 34889484412.

- #31 PNG chunk header alignment: chunks follow variable-length data, so the
  shared header type needs byte alignment. Scoped packing fixes all six buffered
  header reads in FindChunk, DecompressIDATs and R_LoadPNG. Its two uint32_t fields,
  eight-byte size and BigLong conversions are unchanged. Size/alignment assertions
  in the existing renderer build fail before at alignment 4 instead of 1, then
  pass after. Upstream C compile-only assertions likewise fail before and pass
  after with GCC and Clang. Production GCC codegen/symbol gates and both-renderer
  fixed-demo frame hashes are identical. No new fixture recording.
  Upstream C fix: https://github.com/ec-/Quake3e/pull/433. Explicit unit/collision
  regeneration has no golden diff, and normal both-map smoke passes.
  Fork PR #46 passed regression 34890358367 and full build 34890358307.

- #31 JPEG Huffman table index: get_dht now selects the AC/DC array before the
  existing index check and adds the index only afterward. The permanent Clang
  sanitizer unit fails before at index 5 outside JHUFF_TBL *[4], then passes after.
  The actual routine and probe compile as C inside the existing unit driver; all
  legal table slots, expected index errors and table destinations are checked.
  No file loading or new runner. Error codes/index normalization stay unchanged.
  Pointer checks also pass. Explicit unit/collision regeneration has no golden
  diff; one-ULP control, smoke and replay pass unchanged. Production symbols pass;
  only get_dht changes normalized assembly among 15 functions. No gate weakening,
  known-bug entry or suppression. Upstream C test also fails before and passes after.
  Upstream C regression/fix: https://github.com/ec-/Quake3e/pull/434.

- #31 native game math word (found while preparing #2): the official GPL 1.32
  Q_rsqrt reads a four-byte float through long*, which is eight bytes on LP64.
  A direct Clang -O2 call returns 4 for input 4; -O0 ASan reports an eight-byte
  stack overread. The permanent `python3 tests/native_math.py --cc clang` fails
  before (optimized test SIGSEGV) and passes after; GCC also passes both modes.
  Use int32_t and memcpy for the bit transfers; all FP expressions stay unchanged.
  Eight expected result words were measured from the unmodified function in a
  freestanding 32-bit SSE C executable. No accepted engine/gameplay/frame golden
  is regenerated by this test. No known-bug entry or suppression covered it.

  This PR imports only the native shared-math dependency and its two required
  headers, ahead of #2's larger exact import. Engine qcommon math is unaffected;
  the new C source is compiled only by its regression until #2 links native game
  code. GPL source: https://github.com/id-Software/Quake-III-Arena/tree/dbe4ddb10315479fc00086f08e25d968b4b43c49
  (`code/game/q_math.c`, `q_shared.h`, `surfaceflags.h`). All original notices are
  retained. This is LP64 native import compatibility work; ec-/Quake3e has no
  corresponding game source, and its engine Q_rsqrt already uses a four-byte word.
  Original `q_math.c` SHA256: `571cffb6357b3f34fb4a5c000e22d7108f2f3380a49b55f8a993f0b0ce0ba239`.
  Original `q_shared.h` SHA256: `9083a35790991b674bc58c3800b068a9a978898508c5fb08123ea52e1dc8597a`.
  Original `surfaceflags.h` SHA256: `f05993c571858f3bb86cdcb9121d1748351377746916b5db0e28312bdb3b6722`.

  Explicit unit/collision regeneration produces zero golden diff. GCC -O2
  -DNDEBUG symbol comparison passes; only Q_rsqrt changes normalized assembly
  among 47 functions. Its load/shift use the intended 32-bit word; all FP
  arithmetic instructions keep their order. No codegen gate weakening or blanket exception.

  Fork PR #49: regression 34894597080 and full build 34894597081 passed on
  152cc6e2. Final checkpoint is documentation only; self-review passes.

## #31 movement result initialization (merged #51, found during #2)

`BotMoveToGoal` in code/botlib/be_ai_move.cpp clears only failure, type, blocked,
blockentity, traveltype and flags. Invalid-state/no-goal and obstacle returns leave
weapon, movedir and ideal_viewangles from caller storage. The imported GPL game's
`BotAIBlocked` reads movedir unconditionally on a blocked result for avoidance.
Reproducer: `python3 tests/run.py runtime --game-code native` on #2's C ABI branch.
q3dm17 matches; q3dm7 adds an end-of-match Major chat line. Temporary engine tracing
finds identical player states through 42,150 ms, identical movement state/goal, and
a blocked obstacle result for entity 167 (flags 32) with different stale movedir
words. QVM/native then choose different avoidance. The trace writes only after
shutdown; both runs use seed 140 and 1,494 frames ending at 74,900 ms.
Separate #31 fix branch issue/31-bot-move-result: the permanent poisoned-output
probe fails at commit 3993d575, then passes after all 52 result bytes are initialized.
GCC/Clang C++ and upstream C probes pass; upstream PR: https://github.com/ec-/Quake3e/pull/435. q3dm7 adds only Major's chat; both OpenArena
bot logs change due to corrected obstacle avoidance and repeat identically. All
fixed-demo frames remain unchanged. See modernization-progress.md for counts/hashes.
No fix or accepted golden change was made on #2. No known-bug entry/suppression covered it.

## #31 native dispatch argument initialization (merged #52, found during #2)

`VM_Call`'s native branch fills only nargs slots of args[3], but always reads all
three for entryPoint. Zero/one/two-argument calls therefore read uninitialized ints.
`python3 tests/run.py runtime --game-code native --cc clang --cxx 'clang++ -stdlib=libc++'`
loads qagame then catches SIGSEGV before bot startup. GDB identifies VM_Call's copy
loop; Clang 21 release assembly has no zero-count exit in this branch and overwrites
the stack on GAME_CONSOLE_COMMAND (nargs=0). GCC native smoke passes by accident.
Separate branch issue/31-native-dispatch adds the production-body stub regression
first (6d4710b4, Clang SIGSEGV), then initializes unused slots. GCC/Clang pass counts
0–3 and Clang native bot smoke matches both QVM maps. Upstream C also fails before
and passes after. No golden changes; no fix made on #2.

## #31 team-leader name termination (open, found during #2)

Clang's C build diagnoses `bs->teamleader[sizeof(bs->teamleader)] = '\0'` in
BotMatch_StartTeamLeaderShip (ai_cmd.c:1311) and BotTeamAI (ai_team.c:1963).
The field has 32 bytes; both write index 32 after strncpy and can leave index 31
unterminated. Reproducer discovery: `python3 tests/native.py --cc clang`, diagnostics
in the game.log output. Separate #31 branch issue/31-teamleader-name commits the failing compile check
first as fc665341. `python3 tests/teamleader.py` fails on both original functions
with Clang's array-bounds diagnostic against the real pinned GPL bot-state type;
after both sites use existing Q_strncpyz, it passes. No inline #2 fix.

Only these two C files are prerequisite imports from the original GPL revision
[dbe4ddb10315479fc00086f08e25d968b4b43c49](https://github.com/id-Software/Quake-III-Arena/tree/dbe4ddb10315479fc00086f08e25d968b4b43c49).
Original SHA256 ai_cmd.c: a999dc8d989afac9d108ca5ce693d6c43c37ed8df0ad294446ab63b030ef907d.
Original SHA256 ai_team.c: 6c72ba41c01f181ae4f7ffe29be4c2962d4cee12327f01f883dc1f8fe9384165.
The test fetches this pinned checkout's headers when absent and checks their revision
and cleanliness. #2 integrates the complete native modules. All original notices
remain. ec-/Quake3e has no corresponding game files; importing unrelated game code
there would not be an upstream engine fix. No expectation or suppression applies.

Native dispatch upstream C fix/test: https://github.com/ec-/Quake3e/pull/436.

Team-leader fix PR #53 source 0ff62c30 passed regression 34909591046 and full build
34909591164 (attempt 2 after an artifact-service timeout). Self-review passes.
Defined symbols are unchanged and only the two affected functions change codegen;
unit/collision regeneration has zero golden diff. No expectation/suppression applies.

## #31 OpenArena native door target comparison (merged #54, found during #2)

Pinned OpenArena oaxB52 source 331464ca396d80e91cf9be273588f2b5f4b7afc8 defines
strequals(s1,s2) as strcmp(s1,s2)==0 in code/qcommon/q_shared.h:712. SP_func_door
(g_mover.c:966) passes a nullable ent->targetname; native oa_dm1 startup crashes in
libc strcmp. Three g_main.c elimination-target paths also pass nullable targetname.
Temporary C native ABI preflight plus /tmp/aftershock-oa-native-smoke.py reproduces;
/tmp/aftershock-oa-native-gdb.py shows strcmp -> SP_func_door -> G_CallSpawn ->
G_SpawnGEntityFromSpawnVars -> G_InitGame. No patch, golden edit, suppression or
claimed OA native pass. These external game sources are absent from ec-/Quake3e;
resolve in a separate #31 PR if retained for the native CI content configuration.

Separate branch issue/31-openarena-target commits test 43a3dac3 first; UBSan reports
a NULL argument in the actual pinned header's strcmp expansion. The scoped source
patch replaces the macro with an inline helper that checks both names and evaluates
arguments once. `python3 tests/openarena_strings.py` passes with GCC/Clang after;
16 name pairs, case sensitivity and argument evaluation are checked. Patch lives
under tests/patches for #2's native OpenArena CI dependency; no engine source change,
expected-failure entry or suppression. No corresponding ec-/Quake3e source exists.

Native OA startup now completes and both bot smoke logs match their accepted
gameplay after #2's QVM rand/sort library is linked (the original OA native build
uses libc rand). Only VM loading metadata is normalized; no golden change. All 13
caller translation units retain their symbol sets; 19 functions gain null guards
and related compiler branch/register changes. Full source patch remains isolated
from #2's build adaptation. No corresponding engine source exists upstream.

PR #54 source 07ea4fe3 passed regression 34913347473/full build 34913347479;
self-review passes. Native OA replay preflight matches oa_dm7 but not oa_dm1;
that outstanding #2 compatibility investigation is not claimed as a fixed bug
or passing frame gate. The source helper test and both native bot smoke logs pass.

### OpenArena in-place extension overlap (#31)

Pinned B52 code/qcommon/q_shared.c:COM_StripExtension copies out onto itself through
Q_strncpyz/strncpy. Native model suffix paths become truncated on this libc; the
fixed oa_dm1 replay loses part of the grenade-launcher model. Callers include cgame
weapon registration and both UI weapon previews; separate-buffer UI filename paths
use the same helper. Engine/ec-/Quake3e already handles equal pointers, so there is
no corresponding upstream engine fix.

Test-first 02f74cd3: `python3 tests/openarena_strings.py` links actual pinned source
and ASan reports strncpy-param-overlap. The source patch avoids the identical-pointer
copy, retains bounded termination and routes invalid parameters through the existing
checks. GCC and Clang pass after. All 58 function symbols match; only this helper's
assembly changes. Temporary native OA fixed replay matches both maps/renderers with
no golden change. The existing name-comparison UBSan regression still runs. PR #55 source d1e58dcb
passed regression 34914627444 and full build 34914627413. GCC and Clang native OA
smoke/replay pass both maps and renderers; final guarded patch frame hash is
5b89d338. Self-review passes; no expectation or suppression is introduced.

### OpenArena empty extension output (#31)

The combined extension probe also exposed a distinct stack-buffer underflow: empty
output makes length -1 and `if (length)` writes out[-1]. Empty input and output
capacity one reproduce it under ASan. Test-first c5a2ab4c adds both forms, in place and into a separate buffer, and ASan
fails in COM_StripExtension. The one-condition patch requires a positive index.
GCC/Clang pass after. All 58 symbols match; the sole assembly change removes the
branch allowing the negative-index store. The overlap fix is already merged as
PR #55 / b051c915. No corresponding ec-/Quake3e change applies: its engine helper
uses a different implementation and handles empty strings already.

PR #56 source a1f04017 passed regression 34915071172/full build 34915071248.
Clang native OA bot smoke and fixed replay match both maps and renderers after
this patch; no accepted golden change. Self-review passes.

### Native UI negative weapon sentinel (#31)

UI_DrawPlayer and UI_PlayerInfo_SetInfo compare weapon_t values with -1, while the
GPL UI stores that sentinel in playerInfo_t.pendingWeapon and takes it through the
SetInfo enum parameter. Native C++ enum conversion/load cannot represent that value:
Clang UBSan reports load of 4294967295, invalid for weapon_t. The original C interface
used the integer bit pattern. Reproducer: compile /tmp/aftershock-ui-sentinel.cpp
against the #2 header with Clang -O2 -fsanitize=undefined -fno-sanitize-recover=all,
then run with -1; it fails at the field read. Controls_UpdateModel supplies the
integer sentinel; model/settings callers supply ordinary weapon constants.

Fix only in a separate #31 PR: use signed integer storage/parameter for the two
sentinel-bearing values, preserving normal weapon fields and the enum definition.
No corresponding ec-/Quake3e UI implementation exists. Test-first 63f86e7b adds `python3 tests/ui_weapon.py`; UBSan fails at the actual
state field read. The separate #31 fix uses int for the pending field and setter
input. GCC/Clang/libc++ pass the sentinel, normal weapon, signature and layout checks.
State size 1128, pending offset 1076 and timer offset 1080 remain unchanged. C symbol
sets match for all 11 functions; three functions load the same {-1,0} pair from a
constant instead of an immediate. No floating-point expressions change.

Prerequisite imports retain GPL notices from id-Software/Quake-III-Arena at
revision dbe4ddb10315479fc00086f08e25d968b4b43c49, all verbatim in 63f86e7b:
- code/q3_ui/ui_local.h -> code/ui/ui_local.h: a6646ebf728fa741c1638a630e8d8c3e6979e75218108831b48e53490418a807
- code/q3_ui/ui_players.c -> code/ui/ui_players.c: 6e7c12e92ec1858f3e5508dfe48f98f3ace8c67a53882ca4836b414692843cc1
- code/q3_ui/keycodes.h -> code/ui/keycodes.h: dd0f7c5cba444a3399ca70684d1292d82aec5094b607cf222ad67aa50b5342df
- code/cgame/tr_types.h: 6ce0e5cfd49d0ec6ca6907e0c80985b41c9b2963958ce3583bd6a3918ca16dd0

The native integration/C++ catalog changes remain on #2, including the necessary
cast adaptation when these signed types are merged. No engine upstream UI source
exists to receive this native-port-specific fix. No expectation/suppression added.

PR #57 source 630ae8e1 passed regression 34923921313/full build 34923921329.
A temporary complete Clang C++ UI module with this fix passes real SetInfo calls
for clearing a pending change, queuing a valid weapon and the new-model sentinel
path. Self-review passes; accepted goldens and fixtures remain unchanged.

### Native team-voter reset overruns adjacent state (#31)

CalculateRanks in the original GPL g_main.c clears TEAM_NUM_TEAMS (4) entries of
numteamVotingClients[2], overwriting spawning and numSpawnVars. Red/blue consumers
use only indices 0/1. All callers route through this reset: client lifecycle,
combat scores, tournament updates and team scores/status; CheckTeamVote consumes
these counts. The optimized #2 C/C++ warning inventory exposed both invalid writes.

`python3 tests/team_voters.py` calls the real function under UBSan, failing at index
2 before the fix. It checks zero-client reset, human red/blue counts, bot exclusion
and preservation of seeded adjacent fields. Only the two unrelated end-level notification symbols are weakened with objcopy
and replaced by test stubs; the actual rank calculation remains intact. Fix the bound to the actual array length in this #31 PR;
no floating-point expression, wire layout or accepted golden needs to change.

Three verbatim prerequisites retain GPL notices from id-Software/Quake-III-Arena
at dbe4ddb10315479fc00086f08e25d968b4b43c49:
- code/game/g_main.c: fdc9abc73283c57a27e25c15fbcac7cc7b63d0a82d6fe9ce8f8af8252548ee4a
- code/game/g_local.h: de98d3c7212f026650cf581baf102908c359667eb55f1bc65ef5c5b8819283e0
- code/game/g_team.h: 0df64a2d49ce05fc5cb569792ee4d2fffdd93db6ba1fee106a2a11613a16d9bb
No corresponding game implementation exists in ec-/Quake3e. The #2 native port is
parked at bb869f79, with its source adaptation and unchanged replay verified.

Test-first 10ed8eb4 and notification-isolation 5c5217f5 reproduce index 2 on both
GCC/Clang. The array-length fix passes both. All G2 layouts and G3 symbols match;
only CalculateRanks changes assembly among 39 functions. No unrelated source fix.

PR #58 source/test head 9e1f9411 passed regression 34925252301 and full build
34925252282. Temporary full native C++ GCC/Clang game modules with the fix match
both accepted Q3 bot logs, with repeated identical runs. Explicit unit/collision/
Q3 runtime golden regeneration is byte-identical. Self-review passes; no expected
bug entry or UBSan suppression was needed, and no accepted golden changes.

### CTF initialization indexes an invalid flag status (#31)

Team_InitGame sets both flag statuses to -1, then Team_SetFlagStatus updates red
and formats both values. The still-invalid blue status indexes ctfFlagStatusRemap
out of bounds; C++ also rejects the invalid enum. `python3 tests/team_flags.py`
calls the real function and fails UBSan at index 4294967295 in the original C.
It checks base-game/one-flag initialization, valid pickup/drop updates, duplicate
update elimination and reinitialization. SaveRegisteredItems calls initialization;
all other setters (dropped flags, reset and pickup) already pass valid statuses.

Fix initialization in this separate #31 PR: keep valid zeroed at-base states and
publish the complete initial configstring directly, including one-flag mode.
No corresponding ec-/Quake3e game implementation exists. The single verbatim GPL
prerequisite code/game/g_team.c is from id-Software/Quake-III-Arena at revision
dbe4ddb10315479fc00086f08e25d968b4b43c49, SHA256
d004609c19db6949e3d4fe3d3a2d911fbb10f2d7f04249fd13218fb0aa928182.
Its GPL notice remains intact. Native C++ integration stays on #2.

Test-first a6c34e5b fails at the invalid C table index. The initialization fix
passes GCC/Clang in base and missionpack modes. G2 layouts/G3 symbols remain
identical; only Team_InitGame changes assembly (36 base/46 missionpack functions).
The initial configstring is now complete on its first publication, with no invalid
intermediate state. Subsequent setter behavior is unchanged.

PR #59 source 622ae3af passed regression 34926290647/full build 34926290657.
Temporary complete #2 GCC/Clang C++ flag checks also pass UBSan. Unit/collision/
Q3 runtime explicit regeneration is byte-identical. Self-review passes; no expected
bug entry, suppression or accepted golden changed.

### Native bot command floats narrow outside byte range (#31)

The full native C++ UBSan q3dm17 smoke reports -6280.11 outside signed char range
at BotInputToUserCommand (ai_main.c:877). All three float movement expressions
assign directly into signed command bytes. BotUpdateInput is the sole caller.
The native instruction sequence already truncates to int then stores the low byte,
but the direct float-to-byte source conversion is undefined outside byte range.

`python3 tests/bot_command.py` calls the real conversion with controlled horizontal
and vertical AngleVectors bases. Its explicit expected bytes cover sign, endpoints,
fractional truncation and modulo storage; float-cast-overflow fails at 254 before
the fix. A shared helper stages unchanged pinned GPL headers for this and the
existing team-leader check. The sole verbatim source prerequisite ai_main.c retains
its GPL notice from id-Software/Quake-III-Arena revision
dbe4ddb10315479fc00086f08e25d968b4b43c49, SHA256
e969a606253b2d0aa69e9dc1c46981ef3a2ed352140ee3ce637cf456fc23497e.

Fix all three components in this separate #31 PR by making the intermediate int
conversion explicit around the whole expression. No clamping or FP arithmetic
restructuring. No corresponding ec-/Quake3e game implementation exists. Native
port integration remains #2.

Test-first 02a9ddeb fails at 254 on both GCC/Clang. All 18 expected-byte cases pass
after; both compilers produce byte-identical C release objects and identical G2,
G3 and G4 output. No floating-point expression or command representation changes.
Artifacts: /tmp/aftershock-bot-command-gates.py and /tmp/aftershock-bot-command-gates.

PR #60 source 0366fa06 passed regression 34927341670/full build 34927341749.
Temporary complete #2 native C++ UBSan smoke now passes both Q3 maps with identical
repeats and accepted logs. Explicit unit/collision/Q3 runtime regeneration gives
no diff. Self-review passes; no suppression, expected-bug entry or accepted golden
changes. The full native runtime reproducer is /tmp/aftershock-bot-command-native.py.

## Native info-string overlap (#31, found during #2 static preflight)

Original GPL Info_RemoveKey and Info_RemoveKey_Big use strcpy(start, s) when
removing a pair. The source suffix overlaps its destination. Native DLL map-change
preflight exposed corrupted userinfo; ASan confirms strcpy-param-overlap directly
in both helpers. `python3 tests/native_info.py --variant small` and `--variant big`
exercise seven valid-string cases each. GCC and Clang fail before the fix.

The sole verbatim prerequisite import code/game/q_shared.c is from
id-Software/Quake-III-Arena dbe4ddb10315479fc00086f08e25d968b4b43c49, SHA256
a8ddd2b1093ee69df7d0826aad75180ac2ff27bb45f25389e4313efeb51c4be2.
Its GPL notice is retained. Caller audit includes both Info_SetValueForKey variants
whose callers include game/UI code on #2. The engine/upstream helper already uses memmove;
there is no additional ec-/Quake3e fix to submit. Static module reset requirements
remain separate #2 integration work; no simulation FP edits belong to this fix.

Test-first e5fd1033 fails with strcpy-param-overlap for both helpers on GCC/Clang.
The fix replaces only the two copies with memmove, including the suffix terminator.
All 14 cases pass in C and in temporary full-port C++ under both compilers' ASan.
G2 layouts stay identical. G3 adds only the expected undefined memmove reference;
G4 changes only the two removal routines (GCC splits them into .part.0 bodies).
No floating-point code changes. Artifacts: /tmp/aftershock-native-info-gates.py and
/tmp/aftershock-native-info-gates. Explicit unit/collision/Q3 runtime regeneration
is byte-identical. No suppression or expected-bug entry is used.

The complete #2 restart/map-change comparison now matches the fixed native DLL
reference twice at dd1fe5c3be6e1133ce2305819f8f1dffbe51e8917d9258292e035fec1aa23a79.
That comparison also includes the separate scratch #2 bot-state reset and explicit
module memmove binding; neither integration change is part of this #31 PR.

PR #61 source cdcbb7df passed regression 34930264125 and full build 34930264136.
Native C++ smoke and fixed replay also match both Q3 maps/renderers. Self-review
passes with no unrelated edits, allocations, OS calls or non-trivial lifetimes.
The reviewed two-line source fix is the only difference from the GPL prerequisite.

## OpenArena native allocator alignment (#31)

Pinned gamecode 331464ca396d80e91cf9be273588f2b5f4b7afc8 bg_alloc.c rounds block
sizes to 32 bytes but returns the payload after a four-byte int size header.
Native pointer-bearing structures require eight-byte alignment; the char-array
pool also has no explicit pointer-alignment guarantee. #2 static OA runtime
UBSan reports bot_state_t member access at ai_main.c:1210. Its parked reproducer
is python3 tests/run.py runtime --sanitize --content openarena
--data /tmp/aftershock-openarena-baseoa --output /tmp/aftershock-native-integrated-oa-ubsan
on issue/2-native-game checkpoint 956eebfa.

python3 tests/openarena_alloc.py stages the exact public allocator/headers and
checks real gentity_t access plus payload alignment, free/reuse and preservation
of live allocations. GCC and Clang fail before the patch, with UBSan's misaligned
member access. No assets are required. All callers route through BG_Alloc/BG_Free;
BG_CanAlloc must use the same padded header size. No FP expression change is needed.
No suppression/known-bug entry applies. ec-/Quake3e lacks this external game
allocator; there is no applicable engine upstream patch.

Test-first f576a3d2 fails with GCC and Clang. The fix uses a union size header
aligned like a pointer; allocation, capacity checks and free use its common size.
A pool union guarantees freeMemNode_t alignment without changing pool capacity.
On native 64-bit this pads the header from four to eight bytes; 32-bit QVM pointer
alignment/header size remains four. This is private allocator bookkeeping, with
no wire/file layout or floating-point change. Callers remain unchanged.

Focused ASan/UBSan passes both compilers, including allocation/free/reuse and live
payload checks. G3 symbols remain identical; G2 adds only private allocHeader_u,
with existing layouts unchanged. Of six functions, G4 changes only BG_CanAlloc,
BG_Alloc and BG_Free. Artifacts: /tmp/aftershock-oa-allocation-gates. Explicit
unit/collision/Q3 runtime golden regeneration has zero diff. Full #2 static OA
sanitizer smoke passes both maps with unchanged accepted bot logs, using the patched
C object and original engine objects (/tmp/aftershock-oa-allocation-static).

PR #62 source d7fb120b passed regression 34934306507 and full build 34934306523.
Explicit OpenArena QVM runtime regeneration also has zero golden diff. Self-review
passes: one alignment cause, shared allocation/free/capacity paths audited, no FP
or wire-layout change, no added allocation/OS/non-trivial lifetime, no suppression.

## OpenArena freeing from an empty free list (#31, separate from alignment)

BG_Free unconditionally writes freeHead->prev after putting a released block at
the head. If allocations completely consumed the pool, freeHead is NULL. A small
reproducer fills it through BG_CanAlloc(16)/BG_Alloc(16), then frees one block;
UBSan reports member access within null pointer at bg_alloc.c:168 after the
alignment patch. Files: /tmp/aftershock-openarena-full-pool.c and .log. This is a
separate original allocator bug, not fixed by #62; test-first commit 2a4e4aad
covers it. No game content is required and no FP expression is involved.

The permanent openarena_alloc check now fills through BG_CanAlloc/BG_Alloc,
frees the complete pool and repeats its full allocation count. GCC/Clang fail
at the first free before this separate fix. Alignment PR #62 is merged as
0c3ef426; it deliberately did not alter this free-list transition.

The separate patch guards the previous head's backlink when the list is empty.
The new block still becomes the head, and the non-empty transition is unchanged.
GCC/Clang ASan/UBSan pass full allocation/free/reallocation and existing live-data
checks. Layouts and symbols are identical; only BG_Free changes codegen, out of
six allocator functions. Artifacts: /tmp/aftershock-oa-free-gates. No expected-bug
entry or suppression applies, and ec-/Quake3e has no corresponding OA allocator.

Full static OA UBSan smoke passes both accepted map logs with the patched C
object and unchanged #2 engine objects: /tmp/aftershock-oa-free-static. Unit
golden remains 8d44421d; no golden or fixture change is required.

PR #63 source be9a9bc3 passed regression 34935436665 and full build 34935436703.
Self-review passes with one guarded backlink, unchanged layouts/symbols and no
FP, OS, allocation or non-trivial lifetime changes.

#63 explicit unit/collision golden regeneration is byte-identical; static OA
smoke also matches both accepted bot logs. No golden/fixture change.

## MinGW SDL without curl: Windows headers (#31)

The optional PLATFORM=mingw64 ARCH=x86_64 USE_SDL=1 USE_CURL=0 configuration
fails before native integration: sdl_glimp.cpp's clipboard helper lacks Windows
types/functions, and sdl_gamma.cpp includes windows.h inside GLimp_SetGamma,
where SDK extern-C declarations are invalid in C++. Curl's transitive headers
hide this in other configurations. #2 changes neither SDL source. The native
Windows CI configuration uses USE_SDL=0 and builds successfully.

Reproduce with make -B -k PLATFORM=mingw64 ARCH=x86_64 USE_CURL=0 USE_SDL=1
BUILD_DIR=/tmp/aftershock-native-platform-mingw followed by the two object targets:
/tmp/aftershock-native-platform-mingw/release-mingw64-x86_64/client/sdl_glimp.o
/tmp/aftershock-native-platform-mingw/release-mingw64-x86_64/client/sdl_gamma.o
Diagnostics: /tmp/aftershock-mingw-sdl-headers.log. Record a separate failing-build
then passing-build #31 fix; no suppression or expected-runtime-failure entry.

Test-first 73fb26b8 adds the actual two-object compile to the MinGW CI leg. Both
fail before and pass after explicit file-scope Windows includes; the old include
inside GLimp_SetGamma is removed. G3 symbols and G4 codegen for both objects match
the build with Windows headers supplied explicitly before the old source. The
compared release assembly excludes debug metadata and LTO representation; production
compilation retains LTO. Artifacts: /tmp/aftershock-sdl-header-gates and its driver.
The patch changes no function body, type layout, FP operation or OS call.

Original upstream C f694bbbc also fails both objects with curl disabled and passes
the same header fix. Logs: /tmp/aftershock-upstream-sdl-before.log and
/tmp/aftershock-upstream-sdl-after/build.log. This is not confined to the C++ port;
an upstream PR is applicable. No known-bug entry or suppression covers build errors.

Explicit unit/collision golden regeneration is byte-identical after this header fix.

Upstream PR: https://github.com/ec-/Quake3e/pull/439 (C source 88524c13).
Aftershock PR #64 source e38d9335 passed regression 34937896536 and full build
34937896427. Self-review passes: explicit platform header dependencies, identical
symbols/codegen, no executable/type/FP/OS-call/allocation/lifetime changes.

## Affinity expression operators (#31, found during #8)

`engine/platform/sys_runtime.cpp::parseAffinityMask` saves neither `+` nor `-`
before recursively parsing its operand. Its switch examines the character after
the operand instead. On source 470d44da, valid masks `1+2` and `3-1` produce 1
and 3 instead of 3 and 2. `1+2-1` produces 1 instead of 2. Both the public
Sys_ApplyAffinityMask path and recursive expressions use this helper.

Reproducer `/tmp/aftershock-affinity-observe.cpp` includes the real sys_runtime.cpp
and calls the private helper with those literals, without applying CPU affinity.
Compile with `g++ -std=c++20 -fno-exceptions -fno-rtti -O2 -ffunction-sections
-fdata-sections /tmp/aftershock-affinity-observe.cpp -Wl,--gc-sections -lm
-o /tmp/aftershock-affinity-observe`, then run that binary. Add a permanent failing
unit check and fix in its own #31 PR; no fix is included in warning PR #67.

The permanent test-first commit e84a1f6e fails on the actual helper and public
apply path. Saving the operator before consuming its operand fixes all 16 valid
cases under GCC/Clang UBSan. Both common.cpp callers (initialization and cvar
updates) use the same public path. No affinity OS call occurs in the test.
Upstream C f694bbbc reproduces the same failures through Com_SetAffinityMask and
passes the same source fix under GCC/Clang; an upstream PR is applicable.
Upstream PR: https://github.com/ec-/Quake3e/pull/440 (C fix edee6fef).
Aftershock fix: 75828328, after failing test e84a1f6e.
Explicit unit/collision regeneration is byte-identical (8d44421d / 9674cd22).
The hex-sentinel issue below is deliberately unchanged in this operator fix.

## Affinity hexadecimal sentinel (#31, found during #8)

The same helper assigns signed `hex_code`'s -1 sentinel to uint64_t `v` before
checking `>= 0`. The check is always true. The observation probe above reports
`0xZ` as UINT64_MAX; `0x1` correctly reports 1. A bare prefix also advances beyond
its terminator and needs a bounded regression in the separate fix. This is a
second root cause; keep its test/fix separate from expression-operator handling.
The existing -Wtype-limits diagnostic identifies the exact condition. No fix or
new expected-failure entry is included in #67; these are recorded #31 follow-ups.

Hex test-first 985a3f1f adds two cases to the existing affinity test and enables
ASan with UBSan. It confirms 0xZ becomes UINT64_MAX and bare 0x reads beyond its
terminator. The fix keeps hex_code's return in signed int until it is validated,
then widens valid digits to uint64_t. All 18 expressions and the intercepted public
apply path pass GCC/Clang ASan+UBSan. Upstream C f694bbbc independently fails and
passes the same hex-only fix with ten cases; its unrelated operator bug remains
outside that upstream branch. No expectation/suppression entry applies.

Hex source e25cf588 follows failing test 985a3f1f. Upstream C fix 38238103 is
https://github.com/ec-/Quake3e/pull/441, based independently on upstream main.
Nine production object comparisons show only parseAffinityMask changes, with no
function additions/removals and identical unrelated instructions/relocations.
Explicit unit/collision golden regeneration is byte-identical (8d44421d/9674cd22).
Runtime/hosted gates and self-review remain before this separate fix merges.

- #31 bot chat unmatched-variable sentinel (found during #8 warning review):
  `bot_matchvariable_t.offset` is plain char in both engine and imported game
  declarations. `BotFindMatch` stores -1 for missing variables, but unsigned-char
  targets read it as 255. Both `BotMatchVariable` and `BotExpandChatMessage` then
  treat it as present. Reproducer: `python3 tests/chat_offset.py`; signed-char
  passes and unsigned-char returns Q instead of an empty string from both paths.
  The bounded probe uses real template matching and both consumers, with a byte
  at string[255] and a one-byte length. It also checks the mirrored game type and
  unchanged 8/328-byte layouts under ASan/UBSan. No file loading or game assets.
  Test-first 36410f00; fix c58e2751 makes both declarations signed char. PR #71
  passes GCC/Clang ASan/UBSan under both defaults and preserves x86 instructions.
  ARM64 changes only offset consumers. Unit/collision regeneration, Q3 smoke and
  fixed replay are unchanged. Upstream C has the same failing/passing evidence:
  https://github.com/ec-/Quake3e/pull/442 (98691272). No existing expectation or
  suppression applies. Offsets above 127 are outside this fix.

- #8 conditional-source review found a latent C++ build limitation in the retained
  MISSIONPACK path: `CG_VoiceChat` assigns `atoi` directly to enum `qboolean` at
  `game/cgame/cg_servercmds.cpp:936`. Reproduce with
  `g++ -std=c++20 -fno-exceptions -fno-rtti -include game/bg/native_abi_public.h
  -DCGAME -DMISSIONPACK -c game/cgame/cg_servercmds.cpp -o /tmp/missionpack.o`.
  The unchanged baseline fails before the unused-constant guard move; before/after
  preprocessed MISSIONPACK output is identical. Current CMake presets do not enable
  MISSIONPACK. Decision: retain this as a #31 item if that configuration is enabled;
  do not expand #8 into unsupported gameplay configuration work. No supported build
  gate is skipped and no engine fix is folded into the warning PR.

## UI skill conversion before validation (#31, found during #8)

The real UI_SPSkillMenu_SkillEvent callback casts g_spSkill to int before
checking it. A large finite value (1e38) is accepted by Q_atof and causes
float-cast-overflow; NaN/Inf are filtered by Q_atof and are not the reproducer.
Sibling UI readers in ui_splevel.cpp, ui_gameinfo.cpp and ui_addbots.cpp also
convert before validation. ui_spskill.cpp initialization already clamps first.

Bounded reproducer: include game/ui/ui_spskill.cpp in a standalone translation
unit; stub trap_Cvar_VariableValue to return volatile float 1e38f, stub
trap_Cvar_SetValue and trap_S_StartLocalSound, and define color_red/color_white.
Invoke UI_SPSkillMenu_SkillEvent with menucommon_s.id=ID_EASY and QM_ACTIVATED.
Compile with -DCOM_TRAP_GETVALUE=700 -ffunction-sections -fdata-sections
-fsanitize=undefined,float-cast-overflow -fno-sanitize-recover=all and
-Wl,--gc-sections. GCC and Clang both fail at ui_spskill.cpp:114 before any fix.
The exact local probe is /tmp/aftershock-ui-skill-probe.cpp; logs are
/tmp/aftershock-ui-skill-before{,-gcc}.log. Add a permanent test first and fix all
UI skill readers in a separate #31 PR, preserving their distinct range policies.
This is imported game UI code, which ec-/Quake3e does not contain. No applicable
engine upstream PR, expected-failure entry or suppression exists for this bug.

UI skill test-first cea0a882 fails in both the real callback and score writer
under GCC/Clang float-cast-overflow. Fix 2475e0d2 shares a bounded UI_GetSkill
reader across all five consumers; 0/6 remain invalid sentinels while valid
fractions still truncate as before. Both sanitized paths pass; native C/C++
layout/symbol gates pass. Unit/collision regeneration, Q3 smoke and fixed replay
are unchanged. No expectation/suppression entry applies. Hosted gates pending.

## Team-message formatter result (#31, found during #8)

PrintMsg uses an unbounded formatter and compares its returned length with `>`
instead of accounting for the terminator at the capacity boundary. The test-first
`python3 tests/team_message.py` invokes the real function with small ordinary text,
substitutes formatter return values, and captures dispatch/error routing. It never
makes an oversized write. The pre-fix full-capacity result incorrectly dispatches
and exits 0 instead of taking the existing error path (exit 42 in the probe).
Valid text, fitting-result and formatter-error controls behave as expected.

Fix: pass the actual capacity to vsnprintf, finish va_end, then reject a
negative result or a required length at/above capacity. Test-first commit 1ef998b0
records the failure before fix e8752ef1. GCC and Clang/libc++ pass the base and
MISSIONPACK checks under ASan/UBSan. Preserve quote replacement,
broadcast routing and the existing PrintMsg overrun error. No suppression or
expected-UBSan entry is needed for this contract check; no golden regeneration.

## Native diagnostic output capacity (#31)

Twelve active native diagnostic formatters in g_main, cg_main, ui_atoms and ai_main
do not pass their output capacity to the formatting library. A small-text capacity
and routing probe fails all twelve pre-fix contracts; a cached bounded candidate
passes 24 GCC/Clang ASan/UBSan checks. Evidence is native-diagnostic-before.json and
native-diagnostic-preview/{changes,results}.json in the modernization cache.
`python3 tests/native_diagnostics.py` is the committed small-text capacity/routing
contract test; the initial game print case fails as expected before the fix.
Test-first commit a3652169 records the failing contract. The fix uses standard
vsnprintf with the existing buffer/remainder capacity for all twelve calls,
retaining error routing and the seven-byte log prefix offset. Diagnostics truncate
to those capacities; valid text remains unchanged. Fix f3facd3a passes all twelve
checks with both GCC and Clang/libc++ under ASan/UBSan.
The PrintMsg fix merged separately as PR #128. The two unused native parser diagnostics
are a separate #8 deletion. All work remains in msetaro/aftershock.

## Fixed: IQM model allocation accounting (#31, discovered during #9)

`R_LoadIQM` allocates its runtime block without updating `model_t::dataSize`.
`modellist` and the developer model inspector consequently report zero bytes for
valid loaded IQM models, even though their geometry/poses render correctly.
This behavior is present before #9; no fix is included in the asset feature PR.
Reproducer: cook tests/assets/cook-character/assets.json with tools/cook, load
models/character.iqm through the Animation inspector, then run `modellist`.
The line reports `0 : (0) models/character.iqm` and the inspector reports 62 frames,
0 model bytes. Source: engine/render/tr_model_iqm.cpp's allocation and
engine/render/tr_model.cpp's R_Modellist_f. Evidence is in cook-ui-check.log and
/tmp/aftershock-cook-runtime-test/client.log. Add a failing allocation/accounting
check and fix in its own #31 PR after #9 supplies the owned fixture.

Fixed on issue/31-iqm-accounting after #9 merge c195f798. Test-first 85563345
compares the native allocator request with model_t::dataSize and fails at its
initial zero value. R_LoadIQM now assigns its one block's size for both hunk and
owned storage; twelve replacements verify that accounting never accumulates.
The size must fit the existing signed 32-bit reporting field before explicit
conversion; MSVC caught the initially implicit narrowing.
`python3 tests/cook.py` passes with GCC and Clang/libc++; the fixed Quake 3 demo
projection remains 43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4.
No golden changes are needed because only reporting metadata changes. This is
not a sanitizer finding and has no expected-failure or UBSan suppression entry.
Merged in [PR #146](https://github.com/msetaro/aftershock/pull/146), 3d104d0c;
integration regression 35508970698 passed.

## Fixed: IQM rotated nonuniform joint scale (#31, discovered during #9)

`JointToMatrix` in engine/render/tr_model_iqm.cpp multiplies rotation rows by
scale. A native joint with quaternion `(0,0,sqrt(0.5),sqrt(0.5))`, translation
zero and scale `(2,1,1)` maps vertex `(1,0,0)` to `(0,1,0)`; scale-then-rotate
requires `(0,2,0)`. This predates #9. A small production-function check in
`~/.cache/aftershock-modernization/iqm-scale-review.cpp` exits 1 and records
`rotated scaled X: 0.000000 1.000000 0.000000; expected 0 2 0`. Compile it with
C++20, USE_VULKAN_API, function/data sections and linker --gc-sections.

The new cooker explicitly rejects this incompatible joint scale/rotation
combination until a separate #31 test-first fix. Uniform and compatible
axis-aligned joint scales remain supported, as do baked static transforms.
After #9, fix this separately from the allocation accounting bug, remove the
cooker diagnostic and prove native/glTF pose agreement. No engine matrix
arithmetic is changed by #9.


Fixed on issue/31-iqm-joint-scale: JointToMatrix now multiplies matrix columns
by the matching local-axis scale, preserving scale-then-rotate order for both
bind loading and animated poses. Test-first commits 13f999df/e2cd9e50 fail on
the first analytical point before the fix; `python3 tests/iqm_scale.py` now passes
under GCC and Clang/libc++ with UBSan. It covers all axes, signed/nonuniform/unit
scales, inverse products and an owned cooked glTF pose whose native vertices must
match independent analytical coordinates. The cooker can remove its temporary
rejection and reuse ordinary TRS decomposition.

Full cooker checks pass on both compilers; fixed Quake 3 replays retain projection
43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4. No accepted golden
is regenerated: the corrected nonuniform case is newly tested analytically and
existing replay frames remain identical. There is no known-bugs/UBSan suppression
entry for this numerical correctness bug. It remains separate from accounting.
Merged in [PR #147](https://github.com/msetaro/aftershock/pull/147), 7f4d43a7;
integration regression 35510058541 passed.

## Open: main build publication lacks write permission (#158)

Main baseline 81a0f9dc passes all compiler jobs but build run 35545534633 fails
in create-testing job 106171225739 with `Resource not accessible by integration`
when creating refs/tags/latest. The job lacks contents-write permission; the
legacy action then attempts a rolling-tag update. Reproducer: the main push build
that became active when modernization was retired. Log: main-baseline-publish.log.

The isolated #158 infrastructure fix uses job-scoped write permissions and the
installed gh CLI to publish immutable per-commit build tags, verifying any
existing target before retrying assets. `python3 tests/publish_build.py` is an
offline fake-CLI contract; it fails before the publisher is implemented. No
engine bug, accepted golden, sanitizer suppression or external publication.
