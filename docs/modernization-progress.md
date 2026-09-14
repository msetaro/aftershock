# Modernization checkpoint

Integration: `modernization`. Issue branches: `issue/<number>-<slug>`, one bug per
#31 PR and one warning class per #8 PR. Merge commits only after gates/self-review.
Never push main, force-push, rewrite history, or touch port-evidence. Stop after #8
and a design-only `docs/design/rhi.md` for #6; no #6/#7 implementation.

## Next action

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

Current branch: `issue/31-zlib-callbacks`. Added an allocator lifecycle check to
the existing unit driver, using real private inflate initialization/cleanup and
existing allocation stubs, without reading any content. Clang's standalone
ASan/UBSan and permanent unit reproductions fail on the original zcalloc type.
Correct void-pointer signatures and direct assignments pass the unit sanitizer
and pointer checks. Upstream C fails before and passes after:
https://github.com/ec-/Quake3e/pull/430. Production codegen/symbol gates pass;
unit/collision regeneration, normal smoke, GCC UBSan smoke and fixed-demo replay
retain all goldens. No allocation behavior change. PR #43 is open at source
74b15fd6; regression 34886950022 and build 34886949860 passed. Self-review passes.
Next: mark ready, merge, verify merged-tree regression, then fix the filesystem
extension out-parameter on issue/31-extension-output.

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
| 2 | #31 bugs | Huffman merged (#36, upstream #424); filesystem merged (#37, upstream #425); download URL merged (#38, upstream #426); ALSA merged (#39, upstream #427); curl va_start merged (#40, port-specific); ZIP alignment merged (#41, upstream #428); VM alignment merged (#42, upstream #429); zlib callbacks in progress. Each fix needs failing-before/passing-after evidence, affected golden regeneration explained, removal of its expectation/suppression, upstream PR if not port-specific. Read docs/cpp-port-notes.md and issue #31. |
| 3 | #1 error model | Decided: retain longjmp; record rationale in plan section 11 and enforce trivial engine destructors in CI. |
| 4 | #2 native game | Import GPL 1.32 game sources as C; prove QVM/native bot-smoke and fixed-demo parity; port with catalog T1–T25 and gates; static native modules, then remove VMs/JITs. Explicitly ends Quake 3 mod compatibility. |
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
