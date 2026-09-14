# Modernization checkpoint

Integration: `modernization`. Issue branches: `issue/<number>-<slug>`, one bug per
#31 PR and one warning class per #8 PR. Merge commits only after gates/self-review.
Never push main, force-push, rewrite history, or touch port-evidence. Stop after #8
and a design-only `docs/design/rhi.md` for #6; no #6/#7 implementation.

## Next action

Active work: `issue/2-native-game`, draft PR #50. #31 team-leader bounds PR #53
merged as f908cd8e81556a8292bb9bcd841389942e8fdb8e after regression 34909591046
and full build 34909591164 attempt 2 passed on 0ff62c30. Merged-tree regression
34910099630 passed. #52 merged as 99f3b2b5; its merged-tree regression
34908936239 passed. This merge brings #52/#53 into #2, preserves all three native
CI probes, and resolves the two add/add GPL files to the reviewed #53 versions.
Their manifest dispositions now reference #53; original import hashes are retained.

Next: verify #53's merged-tree regression, then run permanent GCC/Clang C native
smoke and fixed-demo replay. Complete OpenArena native support, then the T1–T25
C++ port/gates, static calls and VM/JIT removal. No C++ source port or VM removal
has started. Accepted goldens remain unchanged on #2. Keep newly found bugs in
separate #31 failing-test-first PRs.


#3 is complete (PR #33, merged-tree regression 34867621821 passed). The Huffman
alignment fix merged as PR #36 / bb4474db after regression 34868566671 and full
build 34868566674 passed; its merged-tree run 34869117306 passed.

Filesystem PR #37 merged as 73108eab after regression 34877286456 and full build
34877286443 passed; merged-tree regression 34877819880 passed.

Download PR #38 merged as 02b16def after regression 34878247137 and full build
34878247337 passed; merged-tree regression 34878892522 passed.

ALSA PR #39 merged as 811c6f7a after regression 34879358567 and full build
34879358584 passed; merged-tree regression 34879983585 passed.

Curl PR #40 merged as 998f21c3 after regression 34880567812 and full build
34880567796 passed; merged-tree regression 34881337473 passed.

ZIP PR #41 merged as 2a9436fe after regression 34885119593 and full build
34885119668 passed; merged-tree regression 34885592975 passed.

VM PR #42 merged as 62dad842 after regression 34886047536 and full build
34886047431 passed; merged-tree regression 34886485915 passed.

zlib callback PR #43 merged as fed755d6 after regression 34886950022 and full
build 34886949860 passed; merged-tree regression 34887353903 passed.

Extension output PR #44 merged as bdef99f4 after regression 34887856044 and full
build 34887856072 passed; merged-tree regression 34888464336 passed.

AAS PR #45 merged as 481d008a after regression 34889484418 and full build
34889484412 passed; merged-tree regression 34889992769 passed.

PNG PR #46 merged as f46f48c7 after regression 34890358367 and full build
34890358307 passed; merged-tree regression 34890928632 passed.

JPEG PR #47 merged as 9a7c2625 after regression 34891606879 and full build
34891606634 passed on source 01f2dd40. Its merged-tree regression 34892331846 passed. The disposition audit accounts for all twelve defects; CMake remains #5.
#31 is complete; all twelve fixes are merged and linked in the closed notes ledger.

#1 merged as PR #48 / 95b418b0 after regression 34892972996 and full build
34892972994 passed on e0013d90; merged-tree regression 34893585998 passed.

#2 exact GPL C import is committed/pushed on issue/2-native-game: b3ef1acd plus
checkpoint a937d710. Its 125 imported files and four retained ABI headers have
SHA256 provenance. Native preflight found an LP64 Q_rsqrt overread; no fix was made
on #2. The source is not wired into the engine yet.

Native math PR #49 merged as 2018564f after regression 34894597080 and full build
34894597081 passed on source 152cc6e2. Its merged-tree regression 34895239211 passed. #31 is closed again.

Native transition branch: `issue/2-native-game` (draft PR #50, checkpoint f4653398). Merging modernization retains the original
GPL import commit b3ef1acd and resolves the q_math add/add to the reviewed #49 fix.
The progress conflict is resolved to the latest #31 evidence plus this #2 state.
No history rewritten. Sources still compile only in temporary native preflight,
not the permanent engine build. Original headers match seven shared ABI sizes and
three offsets. Temporary native game/cgame/UI modules compile as C after pointer
entry-point adaptation and linking original bg_lib support; all 36 q3dm17 gameplay
events match the accepted QVM log. Full log differences are implementation-loading
metadata, compile date and bot-skill printf padding. Both-renderer fixed-demo
native preflight repeats identically but fails all accepted frame hashes: differences
range from 2 pixels to 931 pixels per sample. No fixture/golden changes. Added the
required ui_shared.h header verbatim with provenance (126 imported files now).
Temporary native ABI adaptation now passes every accepted frame on both maps and
renderers: binary32 literals plus float results at math calls reproduce the QVM
compiler model (lcc/src/bytecode.c declares float/double/long double all size 4).
GCC uses -fsingle-precision-constant; Clang accepts -cl-single-precision-constant
(the GCC spelling is ignored by Clang, verified with a literal-type assertion).
This is native ABI compatibility, not a new simulation algorithm: original FP
expressions and all accepted fixtures/goldens remain unchanged. Evidence:
/tmp/aftershock-native-demo-math-boundary.log, original frame hash b38004b1.
Permanent C native build/layout/smoke/replay commands are under development.
All 29 shared ABI sizes/alignments and three offsets match with GCC and Clang.
Native syscall arguments now use intptr_t words and explicit unused tail words,
matching the transitional DLL host's fixed vararg reads. The default QVM smoke
still passes both accepted goldens. Native q3dm17 passes the normalized accepted
log; native q3dm7 repeats but adds a Major chat line at the end, so parity fails.
The gate retains that failure and all gameplay text. Temporary engine tracing
shows both modes receive seed 140, run 1,494 frames, and finish at 74,900 ms.
Player states match through 42,150 ms; the first bot movement input difference
then precedes the differing state at 42,200 ms and syscall sequence at 42,300 ms.
Trace edits live only under /tmp, outside this branch's engine sources.
Permanent native demo replay now passes all accepted frames on both maps and both
renderers (/tmp/aftershock-native-demo-permanent.log, hash b38004b1). The server
trace identifies the blocker: BotMoveToGoal initializes only its first six fields;
the obstacle early return leaves movedir/weapon/ideal_viewangles untouched.
At 42,150 ms both modes pass identical movement state and goal, return blocked by
entity 167 with flags 32, then BotAIBlocked reads stale movedir. QVM has old stack
coordinates while native has different stack contents, causing different avoidance.
This is an existing engine bug, not native arithmetic drift. No inline #2 fix.
Current branch: `issue/31-bot-move-result`, based on modernization. #2 is safely
checkpointed/pushed as f4653398 and draft PR #50. #31 is reopened.
Next: a separate #31 failing-test-first PR initializes the
complete movement result and explains any resulting golden changes. After it
merges, resume permanent native smoke/replay parity and C++/static integration. No VM/JIT
removal before full native parity. Keep all new bugs in separate #31 PRs.

Clang runtime observation classified: VM_CallCompiled's instrumented indirect
call reads metadata at codeBase-8 before entering JIT code; the mmap allocation
starts at codeBase and has no preceding metadata. Disassembly confirms that load.
A temporary relink with only vm_x86.o built with -fno-sanitize=function passes both
original Q3 smoke goldens; all other UBSan instrumentation remains. No source or
CI flag/suppression changes were made. This is an instrumentation/JIT compatibility
limit of the transition oracle; #2 removes that JIT. The allocator callback bug
is independently covered by the permanent Clang unit test. The ASan/faketime
experiment still timed out before output and is not a claimed runtime gate.

## Issue status and remaining sequence

| Order | Issue | Status / required work |
|---|---|---|
| 1 | #3 regression suite | Complete: merged PR #33, merged-tree regression passed. |
| 2 | #31 bugs | Huffman merged (#36, upstream #424); filesystem merged (#37, upstream #425); download URL merged (#38, upstream #426); ALSA merged (#39, upstream #427); curl va_start merged (#40, port-specific); ZIP alignment merged (#41, upstream #428); VM alignment merged (#42, upstream #429); zlib callbacks merged (#43, upstream #430); extension output merged (#44, upstream #431); AAS missing candidate merged (#45, upstream #432); PNG header alignment merged (#46, upstream #433); JPEG table index merged (#47, upstream #434); thirteen fixes merged through #49; merged-tree regression 34895239211 passed. Movement result (#51, upstream #435) and native dispatch (#52, upstream #436) also merged; active follow-up is GPL team-leader name bounds. Each fix needs failing-before/passing-after evidence, affected golden regeneration explained, removal of its expectation/suppression, upstream PR if not port-specific. Read docs/cpp-port-notes.md and issue #31. |
| 3 | #1 error model | Complete: merged #48, merged-tree regression 34893585998 passed. |
| 4 | #2 native game | In progress: exact C import and native preflight; prove QVM/native bot-smoke and fixed-demo parity; port with catalog T1–T25 and gates; static native modules, then remove VMs/JITs. Explicitly ends Quake 3 mod compatibility. |
| 5 | #4 boundaries | Hash-verified directory moves, include/OS-access CI checks, docs/subsystems.md; rename cpp-port-notes.md to docs/bugs.md. |
| 6 | #5 CMake | Repair as primary, object parity before removing Makefile; generated MSVC projects, 64-bit little-endian only. |
| 7 | #8 code rules | Warning class per PR; one codegen-identical tree-wide clang-format commit; tidy subsets; fixed-width wire/file types and layout traits; release-identical Q_ASSERT. |
| 8 | #6 design only | Write docs/design/rhi.md after #8, then stop. |

## Rulings in force

- The user's 2026-09-14 continuation supersedes the older roadmap order and handoff.
  Preserve fixed-timestep simulation, prediction/snapshots, cvars/pk3, arena/POD data,
  no per-frame allocation. No simulation FP restructuring except tested #31 fixes.
- C++20, no exceptions/RTTI, longjmp/trivial core lifetimes. Existing VM/dlopen paths
  are the transition oracle only. Match surrounding style until #8.
- Local system packages must not be installed. Each CI job installs prerequisites.
  Proprietary paks stay outside git and uploads. Missing content/tools fail tests.
- Network coverage is finished. This thread ran `python3 tests/network.py --max-error 0`
  exactly once: exit 1, `FAIL: prediction bound exceeded`, measured 8.875 against zero.
  Do not modify or rerun that harness in this continuation.
- Normal demo checks replay fixed fixtures; recording is intentionally not reproducible.
  Golden creation/replacement is explicit, reviewed, and forbidden in CI.
- Engine/vendor bug fixes belong only in individual #31 PRs. No production source
  changed in #3. The independent scope adjustment on issue #3 remains in force.

## #3 acceptance evidence

Source/test/golden head: **98096f9710b29fb99464ac2464b356b7a28799cc**.

- Regression workflow **34866966536: success**. GCC, Clang/libc++, aarch64/mingw,
  sanitizer expectations, OpenArena collision/both-map smoke, and both real software
  renderer replay gates pass. The independently assigned job is non-blocking per
  the issue's scope ruling.
- Full build workflow **34866966514: success**, including Linux/macOS, MSVC x64/ARM64
  Debug/Release and Windows mingw. Skipped release-publishing jobs are inapplicable.
- Local GCC/Clang unit hashes and active Q_rsqrt one-ULP controls pass. Original
  accepted unit/collision/Quake 3 smoke goldens are byte-identical to d754d683.
  Local collision, both smoke maps, and final fixed-demo replays pass again.
- The sanitizer unit run reports HuffmanGetSymbol alignment as `known, tracked in #31`
  while comparing all thirteen groups. Unknown diagnostics, ASan failures, and stale
  expectations fail. `tests/check_known_bugs.py` verifies the classification policy.
  PNG alignment and JPEG table-index defects/reproducers are already on #31 and in
  docs/cpp-port-notes.md; they are not exercised by the unit driver.
- OpenArena uses oa_dm1/oa_dm7, Sarge/Beret, fs_game=baseoa, networking disabled and
  900 waits for combat. Hosted collision/smoke match locally generated goldens.
  Ubuntu paks contain native-module markers; tests/openarena.py adds official GPL
  oaxB52 QVMs with a pinned SHA256. Source/license/provisioning are in tests/README.md.
- Review rejected initial q3dm17 idle-player death-screen samples; final Quake 3
  fixtures follow a bot. Review also corrected the inherited opengl1 Make value to
  opengl: previous two-renderer claims were incorrect. Both renderer identities are
  now asserted; tests/check_frames.py rejects mislabeled logs and unequal repeats.
- Exact frame profiles use Mesa 26.0.8 locally and Mesa 25.2.8 on Ubuntu 24.04.
  No pixel tolerance or fallback. The hosted profile was generated LOCALLY with
  tests/frames.py --regenerate after visual review of run **34866295338** on
  **145939aa**. All 24 saved frames (12 samples, two repeats), renderer identities,
  and fixed fixture hashes were checked. CI never generated a golden.
- Golden-writing modes reject CI. AGENTS.md lists permanent tests/ commands.

## Self-review

#3 contains only tests/CI/docs and initial public-content/demo fixtures. No engine
or vendor changes, new core destructors, allocations, OS calls, simulation FP edits,
or layout changes. Original accepted goldens and port-evidence remain untouched.
README documents prerequisites, both content sets, provenance, exact profiles,
known-failure policy, and explicit fixture/evidence commands. Issue #3 and PR #33
record the measurements and corrected renderer finding.

## Local recovery paths

#3 worktree: `/home/matt/.t3/worktrees/aftershock/t3code-b3a8e505`.
The supplied workspace was at the integration baseline when this thread began.
`/tmp/aftershock-openarena-baseoa` is staged public content; original downloaded
packages were extracted under `/tmp/aftershock-openarena`, never installed.
`/tmp/aftershock-demo-tests` and `/tmp/aftershock-oa-demo` contain final local replay
evidence. `/tmp/aftershock-ci-runtime4/aftershock-demo-tests` contains reviewed hosted
baseline evidence. These paths are disposable; source and README commands suffice.

## #31 Huffman validation

Fatal Clang ASan/UBSan unit run failed before the fix at huffman_static.cpp:206,
exit 1; the same command passes afterward with no diagnostic/expectation.
Production GCC assembly and symbols pass the existing port gates before/after.
The explicit unit and differential regeneration commands produce zero golden
diff. Local q3dm17/q3dm7 smoke and both-renderer fixed-demo replay pass unchanged.
An upstream C regression tests every symbol at 32 bit offsets: sanitizer fails
before, passes after, and GCC C assembly is also identical. Upstream PR: https://github.com/ec-/Quake3e/pull/424. Fork PR #36 merged after full CI and self-review; merged-tree regression passed.

## #31 filesystem validation

The permanent `--sanitize --pointer-compare` command fails before the fix with
NULL versus filename+3, and passes afterward with the original unit golden.
Separate ASan/UBSan and ASan/pointer-comparison runs retain both checks: combining
them under Clang 21 instruments generated pointer-overflow comparisons in COM_ParseExt
(disassembly shows `__sanitizer_ptr_cmp(pointer, -3)`). No suppression/expectation
covered this bug. GCC/Clang/libc++ units, the one-ULP control, both sanitizer modes,
collision, both-map smoke and both-renderer replay pass. Explicit unit/collision
regeneration changes no golden. Symbol gate passes; normalized per-function assembly
changes only FS_AllowedExtension among 99 functions, as expected for the new guard.
Upstream C test fails before and passes after: https://github.com/ec-/Quake3e/pull/425.
Fork: https://github.com/msetaro/aftershock/pull/37. Regression/full build runs above
include public-content runtime and all platform legs. The caller audit's separate
Sys_LoadLibrary uninitialized diagnostic pointer is recorded in notes and issue #31.

## #31 download validation

Permanent `python3 tests/download.py` fails before on the trailing-slash base
(`maps//map%20name.pk3`), then passes with GCC and Clang/libc++ after the one-line
guarded last-character check. `%1` substitution, escaping and empty-base behavior
are retained. Explicit regeneration created only tests/golden/download.txt.
Existing unit/one-ULP, collision, smoke and replay gates pass. One local smoke
attempt overlapped replay and hit an occupied UDP port; serial smoke passed both
original map goldens. Run those local runtime gates serially. No golden changed
to accommodate the collision. The hosted runtime URL check uses existing libcurl
packages; begin/cleanup run without a transfer.
Upstream C fix/test: https://github.com/ec-/Quake3e/pull/426.
Fork PR: https://github.com/msetaro/aftershock/pull/38.

## #31 ALSA validation

Permanent callback assignments reject both original void(void) signatures. GCC and
Clang/libc++ pass after the pthread-compatible signatures/direct registration/NULL
returns. Real ALSA null output receives positive MMAP/DIRECT submissions and both
threads join within the timeout; no physical device or fabricated audio backend.
The dynamic-ALSA production object builds. Explicit unit/collision regeneration
produces no golden diff; serial both-map smoke and both-renderer replay pass unchanged.
Upstream C test/fix: https://github.com/ec-/Quake3e/pull/427.
Fork PR: https://github.com/msetaro/aftershock/pull/39. CI runs are above.

## #31 curl varargs validation

The permanent Clang test fails before at va_start and passes after changing the last
named argument to int and retaining a CURLoption local. Removed -Wno-varargs.
GCC/Clang local-file tests verify long/pointer/offset forwarding, body suppression,
private-data identity, bounded partial output on size-limit rejection and exact
returned bytes. No network transfer. Explicit URL regeneration changes no golden.
Normalized GCC codegen/symbol gates pass; the internal mangled name changes with
its parameter type, and all callers are in cl_curl.cpp. Unit/one-ULP, collision,
serial both-map smoke and both-renderer replay pass unchanged.
Fork PR: https://github.com/msetaro/aftershock/pull/40. CI runs are above.

## #31 ZIP validation

The existing GCC UBSan bot smoke fails with the original ZIP object and passes
with CopyLittleLong through a signed int temporary. Removed only the ZIP alignment
suppression. Q3 and OpenArena both-map goldens pass unchanged. Explicit unit/collision
regeneration produces no golden diff; normal serial smoke and fixed-demo replay
pass. GCC production symbol gate passes. Codegen gate reports only stack slots
64/68 exchanged with their matching sign-extending consumers; field destinations
and operations are unchanged. This reviewed difference is acceptable under #31's
codegen ruling; the gate is not weakened or reported as identical.
Upstream C UBSan startup on valid installed paks fails before and exits cleanly
after: https://github.com/ec-/Quake3e/pull/428. No new content or loader target.
Self-review: one packed-read bug, shared function covers all callers; no simulation
FP edit, layout change, allocation, destructor or new engine OS call. Runtime
sanitizer coverage reuses the existing smoke runner and compares existing goldens.
Regression 34885119593 passed on source 09e7cc96, including hosted GCC UBSan smoke.
Full build 34885119668 passed on the same source. Final checkpoint changes only
documentation; self-review passes and no goldens changed.

Next bug reproduction is ready without a source change: the same GCC runtime
with UBSAN_OPTIONS=halt_on_error=1 (no suppressions) exits 1 at vm.cpp:1181.
Evidence: /tmp/aftershock-vm-before-runtime.log. Follow with its own branch/PR.

## #31 VM validation

Permanent GCC UBSan runtime fails before at vm.cpp:1181, passes after the packed
read uses CopyLittleLong through int32_t. Removed the final alignment suppression;
tools/port/ubsan.supp is empty. All Q3/OA smoke goldens, normal serial smoke and
both-renderer replay pass unchanged. Explicit unit/collision regeneration has no
golden diff. Production symbols and all 26 function-section bytes match. Text
codegen differs only in the compiler switch-table label CSWTCH.89/90; unchanged
instructions/data reviewed acceptable, no gate edits. Upstream C reproducer
fails before and passes after: https://github.com/ec-/Quake3e/pull/429.
Self-review: one packed-read bug in the shared caller path; no instruction format,
FP, JIT behavior, allocation, destructor, layout or engine OS changes. No new
known-bug entry or test target; existing runtime CI is the permanent regression.
Regression 34886047536 and full build 34886047431 pass on source f320dee6.
Final checkpoint changes documentation only. Self-review passes.

## #31 zlib callback validation

Permanent Clang ASan/UBSan unit fails before on zcalloc's byte-pointer callback
type; correct void-pointer signatures and direct registration pass initialization
and cleanup. Separate pointer sanitizer mode also passes. The helper is linked
into the existing unit driver and uses its allocator stubs, with no content input.
No new runner/target or expectation/suppression. Explicit unit/collision golden
regeneration produces no diff; one-ULP negative control, normal/GCC UBSan smoke
and both-renderer replay pass. Production normalized codegen/symbol gates pass;
only private callback mangled names change. All callers are in unzip.cpp.
Upstream C regression fails before and passes after:
https://github.com/ec-/Quake3e/pull/430. Self-review: one callback ABI bug; no
allocation arithmetic, per-frame allocation, layout, FP, destructor or engine
OS-access changes. Regression 34886950022 and full build 34886949860 pass
on source 74b15fd6. Final checkpoint is documentation only; self-review passes.

## #31 extension output validation

Both platform diagnostic callers receive a defined extension on true returns:
the shared function now initializes its output before classification, using an
empty string for names without a dot. Versioned .so and rejected-extension strings
remain unchanged; the other callers only consume output on false returns.
Existing unit assertions fail before and pass after for empty, extensionless,
trailing-dot, ordinary, .so.N and pk3 cases. No library is loaded by the test.
Clang sanitizer/pointer checks, one-ULP control, collision, smoke and replay pass.
Explicit unit/collision regeneration changes no golden. Symbols pass; only
FS_AllowedExtension changes bytes among 99 functions. The assembly text also
renames CSWTCH.612/613 in FS_Seek, whose bytes are unchanged. Reviewed expected
codegen difference; no gate weakening. Upstream C fix/test:
https://github.com/ec-/Quake3e/pull/431. Self-review: shared out-parameter bug only;
no extension policy, OS access, allocation, destructor, FP or layout changes.
Regression 34887856044 and full build 34887856072 pass on source 8fd57208.
Final checkpoint changes documentation only; self-review passes.

Next AAS reproduction: GCC UBSan compile of be_aas_reach.cpp with its existing
-Wno-maybe-uninitialized exception overridden by -Werror=maybe-uninitialized fails
on beststart at line 2196. The function is AAS_Reachability_Jump (the earlier note
incorrectly called it JumpArea). A temporary source copy with a bestdist==999999
return before VectorMiddle compiles cleanly; engine source is unchanged so far.
The Makefile exception explicitly names this one defect and can be removed in
its own #31 PR. Evidence: /tmp/aftershock-aas-warning-before.log and
/tmp/aftershock-aas-guard-check.log.

## #31 AAS candidate validation

The existing runtime --sanitize build fails before on the recorded beststart
warning once its specific Makefile exception is removed. AAS_Reachability_Jump
now returns false when bestdist retains its initial sentinel, before midpoint
reads. The edge selector only replaces bestdist when it supplies endpoints; no
candidate arithmetic was rewritten. Upstream C compilation likewise fails before
and passes after: https://github.com/ec-/Quake3e/pull/432.
Explicit unit/collision regeneration has no golden diff. Final Q3 and OA UBSan
both-map smoke, normal serial smoke and both-renderer replay pass unchanged.
The production symbol/codegen gates are not identical: only jump/grapple functions
change, with an added private VectorLength body and no direct sqrtf import.
Register/stack/inlining changes were reviewed; actual emitted VectorLength matches
a libm oracle for four million finite-input vectors across all four rounding modes.
No source FP expression change or gate weakening. Self-review: one missing-candidate
bug, existing compile/runtime regression, no layout/OS/allocation/destructor change.
Regression 34889484418 and full build 34889484412 pass on source b5e31b8c.
Final checkpoint is documentation only; self-review passes.

## #31 PNG header validation

The existing demo build fails before on the new chunk-header alignment assertion
(4 instead of 1). A scoped packing pragma gives this wire type byte alignment;
its eight-byte size, two uint32_t fields and BigLong conversions stay unchanged.
All six buffered header-read sites share the type. GCC production codegen/symbol
gates pass identically; both-renderer fixed replay preserves every frame hash.
Explicit unit/collision regeneration has no golden diff; both-map smoke passes.
Upstream C layout assertions fail before and pass after with GCC and Clang:
https://github.com/ec-/Quake3e/pull/433. No expected-failure entry or suppression
covered this type. Self-review: one alignment bug; persistent layout assertions
in the existing renderer build, no new content/recording, FP, allocation, OS-access
or destructor changes. Regression 34890358367 and full build 34890358307 pass
on source becd4b27. Final checkpoint changes documentation only; self-review passes.

## #31 JPEG table-index validation

The permanent Clang sanitizer unit fails before at index 5 outside JHUFF_TBL *[4].
get_dht now chooses the AC/DC base array first and applies the index after its
existing validation. All legal slot destinations and expected error code/index
values pass, with the routine and probe compiled as C. Pointer mode also passes.
Explicit unit/collision regeneration has no golden diff; one-ULP control, normal
smoke and fixed-demo replay pass unchanged. Symbols pass; only get_dht changes
normalized assembly among 15 functions, reviewed as the expected pointer-lifetime
change. No gate weakening. Upstream C test fails before and passes after:
https://github.com/ec-/Quake3e/pull/434. Known-bugs has no entries, and ubsan.supp
is empty. Self-review: one vendor bounds bug, existing unit runner, no file loading,
FP, layout, engine OS-access, allocation or destructor change. Regression
34891606879 and full build 34891606634 pass on source 01f2dd40. Final checkpoint
changes documentation only; self-review passes.

## #1 acceptance evidence

Source e0013d9085f56ddd4d726a15e5c5a724dc9272f3 passed regression 34892972996 and
full build 34892972994. Local Clang 21 and hosted Clang analysis pass 140 engine
translation units using 356 compilation commands across both renderer configurations.
Controls reject seven owning objects, accept trivial/defaulted objects and pointers,
and verify core inclusion/platform exclusion. Clang's AST `destroyed` annotation
comes directly from VarDecl::needsDestruction; the controls detect format drift.
Section 11 records the retained longjmp rationale, wrapper restriction, and inactive
preprocessor-branch/self-review limitation. AGENTS.md and README document the command.
No engine source or golden changed. Final self-review passes; docs checkpoint only.

## #31 native math acceptance evidence

Source 152cc6e294a37772c0d942fb1ce69dadb1d57c9d passed regression 34894597080 and
full build 34894597081. GCC/Clang optimized and ASan C checks pass all eight words
from the unmodified 32-bit SSE C executable. Explicit unit/collision regeneration
has zero diff. Symbols pass; only Q_rsqrt changes normalized assembly among 47
functions, with unchanged FP arithmetic order. No expectation/suppression existed.
Self-review: one LP64 native word-width defect, three prerequisite GPL source/header
imports, no production engine/FP/layout/OS/allocation/destructor change. This C
math dependency is test-only until #2. Final checkpoint changes documentation only.

Independent temporary #2 preflight: seven shared ABI sizes and three offsets match
between original GPL C headers and engine C++ headers (/tmp/aftershock-native-layout-*.txt).
Base-game native smoke with original bg_lib support reproduces all 36 accepted
q3dm17 gameplay events. Full text differs only in module-loading metadata, build
date and bot-skill padding (custom VM printf vs native libc). This is preliminary
single-map evidence, not completed parity. Temporary UI compiles; cgame additionally
requires upstream code/ui/ui_shared.h, an include omitted from the initial #2 import.
Do not remove VM/JIT paths or change accepted fixtures before full native parity.

## #31 movement result validation

The poisoned-output production-engine test fails before (3993d575) and passes after
for GCC and Clang/libc++. The upstream C test fails before with GCC and passes after
with GCC and Clang: https://github.com/ec-/Quake3e/pull/435. BotMoveToGoal now zeroes all 52 bytes before lookup/early returns;
no FP expression, layout, allocation, OS access or destructor change. Symbols pass;
only BotMoveToGoal differs among 33 assembly functions: additional zero stores and
register allocation changes, reviewed as expected for this tested bug fix.

Explicit unit/collision regeneration preserves their hashes (8d44421d / 9674cd22).
q3dm17 is unchanged. q3dm7 changes only by Major's two-line chat (one say event),
now hash 028fba42; native and QVM match after implementation metadata normalization.
OpenArena obstacle avoidance no longer depends on stale caller memory: oa_dm1 has
70 rather than 75 Item events, still four kills, one rather than two say events
(hash 9ca81956); oa_dm7 has 58 rather than 71 Item events, five rather than two kills,
and two rather than one say events (5a511a91). Both maps repeat identically before
these explicit golden writes. No recording/frame golden changes. Fixed replay
passes both maps/renderers for Quake 3 (b38004b1) and OpenArena (5b89d338).
No expected-failure entry or UBSan suppression covered uninitialized movement output.
Regression 34900480717 and full build 34900480656 pass on source
fc49615d4be82ff41110f6d521ee6945dd376739. GCC UBSan Quake 3 smoke also passes both
updated goldens. Self-review: one #31 initialization defect, production-body test
first, explained golden changes only, unchanged file/wire layout and FP expressions,
no new engine OS calls, non-trivial destructors or allocations. Issue updated;
upstream #435 open. This final checkpoint changes documentation only.

#2 preparation only: OpenArena oaxB52 source tag resolves to
331464ca396d80e91cf9be273588f2b5f4b7afc8, matching the release used by hosted QVM
fixtures. Clone is /tmp/aftershock-oa-native-source; no native OA build or code change
yet. GCC and Clang both accept their binary32 literal flags in C++20 as well as C.
Temporary C++ compilation of the base game lists expected enum/pointer/constness,
FOFS pointer-to-int and old-style definition conversions; no C++ source port begun.

## #31 native dispatch validation

The test at 6d4710b4 uses the actual production VM_Call and a native entry stub;
counts 3, 0, 1, 2 check every delivered argument, return value and restored call
depth. Clang 21 crashes before at the zero-count call. After initializing unused
slots, GCC and Clang/libc++ pass. The native Clang module from #2 then reproduces
both QVM Quake 3 logs through the transition driver (6dad7c18 / a15c9c91 normalized).
Default QVM bot smoke and both-map/both-renderer fixed replay remain unchanged.
Explicit unit/collision regeneration has zero diff. Symbols pass; only VM_Call
changes among 26 assembly functions: zero stores and native-path control flow;
no source FP, layout, OS, allocation or destructor changes. No known-bug entry or
suppression covered the native uninitialized arguments. Regression 34908517245 and
full build 34908517199 pass on source 3201b7fa8babcd54be0129fac3c0d0ab99349dcf.
Self-review: one shared dispatch initialization bug, failing test first, no unrelated
refactoring, unchanged file/wire layout, FP expressions, allocation, OS access and
trivial lifetime rules. Issue updated, upstream #436 open. Final checkpoint docs only.

Native dispatch upstream C fix/test: https://github.com/ec-/Quake3e/pull/436.

Next #31 prerequisite investigation: Clang -std=gnu99 -O2 -Werror=array-bounds
-fsyntax-only on the original pinned GPL ai_cmd.c and ai_team.c independently
rejects both index-32 teamleader writes. Other writes already use ClientName or
Q_strncpyz. These game files are absent from ec-/Quake3e, and pinned OpenArena
already terminates at sizeof(teamleader)-1. Keep this native import defect scoped
to its own #31 PR; do not introduce unrelated game imports upstream.

## #31 team-leader bounds validation

Test-first commit fc665341 imports only code/game/ai_cmd.c and ai_team.c from GPL
revision dbe4ddb10315479fc00086f08e25d968b4b43c49, retaining their notices. Full native
integration remains #2. `python3 tests/teamleader.py` checks the actual C functions
against the pinned, clean original bot-state headers with Clang bounds errors.
Both original index-32 writes fail; both bounded-copy replacements pass. The test
fetches public source headers when absent, never game assets. CI runs it on Clang.
Original source SHA256s and reproducer are in cpp-port-notes.md.

GCC -O2 -DNDEBUG defined-symbol comparison passes. Of 43 ai_cmd and 22 ai_team
functions, only BotMatch_StartTeamLeaderShip and BotTeamAI change normalized assembly:
the existing Q_strncpyz call replaces strncpy plus the out-of-bounds byte store;
the first function also reallocates one register. No FP instruction changes.
No expected-failure entry or UBSan suppression covered this compile-time failure.

Explicit unit/collision regeneration produces zero golden diff (8d44421d /
9674cd22). Gameplay/frame fixtures are unaffected by these test-only prerequisite
imports; the existing CI runtime gates remain required.

PR #53 source 0ff62c302c96e00f929f4537e7bc8559805f2c29 passed regression
34909591046. Full build 34909591164 compiled macOS release successfully but its
artifact upload timed out at CreateArtifact (ETIMEDOUT); retry only the failed job
after the remaining build job finishes. This is not a source/build failure and
is not counted as a passing full-build gate. The original GPL ai_team.c ends in
a blank line, preserved byte-for-byte in the prerequisite import; the fix itself
has no whitespace-only changes.

Full build 34909591164 attempt 2 passed on the same source; only the failed macOS
job was rerun. PR #53 self-review: one #31 bug, exact two-file prerequisite import,
only two bounded-copy changes; no new OS access, non-trivial lifetime, allocation,
wire/file layout or FP expression changes. Defined-symbol/codegen review and the
failing-before/passing-after test pass. Goldens/fixtures remain unchanged; no
expectation or suppression applies. This final checkpoint changes documentation only.

#2 resumed at merge 383c53e0. Permanent Clang C native smoke passes both Q3 maps
(6dad7c18 / a15c9c91 normalized accepted logs); Clang C native fixed replay passes
both maps/renderers with the unchanged frame hash b38004b1 and original fixtures.
Commands use --game-code native --cc clang --cxx 'clang++ -stdlib=libc++'; logs:
/tmp/aftershock-native-runtime-clang-final.log and
/tmp/aftershock-native-demo-clang-final.log. GCC parity was already measured before
this bounds-only fix; the unused team paths are now covered by tests/teamleader.py.
OpenArena C native preflight is confined to /tmp/aftershock-oa-native-work from the
pinned source: transitional intptr_t syscall words, entry signatures and the same
binary32 ABI header. No repository OA source import or golden changes yet.

OpenArena native preflight builds game/cgame C modules but oa_dm1 stops before bot
startup: SP_func_door passes NULL ent->targetname through strequals to libc strcmp.
GDB confirms __strcmp_avx2 -> SP_func_door -> G_CallSpawn -> G_InitGame. Reproducer:
/tmp/aftershock-oa-native-smoke.py and /tmp/aftershock-oa-native-gdb.py (logs alongside).
The original macro is code/qcommon/q_shared.h:712; nullable targetname also reaches
it in three g_main.c paths. This is an external OpenArena game-source #31 item;
no patch has been made. Quake 3 native compiler/replay parity is complete, so its
C++ catalog port can proceed independently while OA remains a failing prerequisite.
The permanent native OA build and UI source mapping are not yet implemented.

## #2 C++ compatibility deviations

Baseline 3306d55d compiles 103 native C release objects (module-local shared sources
included) with GCC -O2 -DNDEBUG and the established native ABI flags. Hash manifest:
/tmp/aftershock-native-c-object-gate/before/sha256.json. This precedes all C++ edits.
Two necessary syntax adaptations outside T1–T25 are isolated in their own commit:
- FOFS uses `(int)offsetof(gentity_t, x)` plus stddef.h instead of narrowing a pointer
  expression directly to int, rejected by 64-bit C++. The int field representation
  and every measured offset stay the same; no entity layout change.
- Three bg_lib sort helper definitions use prototype parameter lists with their
  original types, replacing K&R definitions that C++ cannot parse. Bodies unchanged.
All 103 C release objects remain byte-identical after these changes (zero changed
SHA256s), including the field table and sort code. Evidence command:
python3 /tmp/aftershock-native-c-object-gate.py deviations. No simulation expression
or golden change; no new algorithm or bug fix. Ordinary catalog casts/renames follow
in separate commits. C++ syntax preflight initially reports 50 of 100 module TUs
failing; bg_lib adds old-style-definition errors. No permissive flags are enabled.
