# Modernization checkpoint

Integration branch: `modernization`; issue branches: `issue/<number>-<slug>`. Strict C++20 port is merged in main (PR #32). Never push main, force-push, rewrite history, or alter port-evidence. Work stops after #8 and a design-only note for #6; #6/#7 implementation is excluded.

## Next action

Finish #3 on `issue/3-regression-suite` / draft PR #33. Validate fixed-demo replay
and the new OpenArena baselines locally and on hosted CI, finish documentation and
AGENTS.md verification commands, then mark ready and merge with a merge commit only
after gates/self-review. Preserve accepted Quake 3 unit/collision/smoke goldens.
The user's 2026-09-14 continuation rulings supersede the previous handoff.

## Current continuation (2026-09-14)

- Resumed checkpoint d754d683 in the existing issue worktree
  `/home/matt/.t3/worktrees/aftershock/t3code-b3a8e505` (the supplied workspace was
  at integration baseline 8183d969). Production sources remain unchanged.
- Ran the required network negative control exactly once:
  `python3 tests/network.py --max-error 0` exited 1 with
  `FAIL: prediction bound exceeded`; measured correction 8.875, bound 0.
  Existing normal network coverage is finished; do not rerun or modify it here.
- CI may install its own dependencies. This machine must not install packages.
  OpenArena packages were downloaded and extracted only under
  `/tmp/aftershock-openarena`; no system installation. Hosted runtime will install
  `openarena-data`, faketime, Xvfb and Mesa, use `fs_game baseoa`, oa_dm1/oa_dm7,
  and separate `tests/golden/openarena` outputs. Quake 3 paks stay outside git.
- Demo recording repeatability is not an acceptance condition. Each map is recorded
  once behind `--record-fixtures`; ordinary runs replay its committed .dm_68 twice
  per software renderer and compare sampled frame hashes. Initial q3dm17/q3dm7
  fixtures both replay identically on Vulkan and OpenGL1; review/commit pending.
- `tests/known-bugs.txt` enumerates diagnostics exercised by the sanitizer unit job.
  The run reports the existing Huffman alignment error as `known, tracked in #31`
  and still compares all thirteen unit groups. Unknown diagnostics, ASan failures,
  or a missing expected diagnostic fail. The initial local run passes this policy.
  PNG chunk alignment and JPEG table-index defects already have reproducer records
  in both #31 and docs/cpp-port-notes.md; fixes remain separate #31 PRs.
- Runtime asset and compiler provisioning edits are in progress. Hosted status is
  not yet green; #3 remains draft. Full sequence below remains outstanding.

## Issue status

| Order | Issue | Status | Outcome / next step |
|---|---|---|---|
| 1 | #3 regression suite | in progress | Permanent differential hashes, q3dm17/q3dm7 smoke/demo goldens, software-rendered frames, sanitizers, network simulation, fuzzers, compiler matrix, one-ULP negative control. |
| 2 | #31 recorded bugs | todo | One failing-test-first bug fix and upstream PR per eligible bug; remove repaired suppressions. |
| 3 | #1 error model | decided; implementation todo | Option 1: retain longjmp, trivial engine lifetimes, RAII only at safe platform/GPU boundaries; enforce in CI. |
| 4 | #2 native game | decided; implementation todo | GPL 1.32 game source import as C, QVM/native parity, catalog port, static modules, remove QVM; ends Quake 3 mod compatibility. |
| 5 | #4 boundaries | todo | Hash-preserving directory moves, include/OS checks, subsystem docs, bugs ledger rename. |
| 6 | #5 CMake | decided; implementation todo | Repair CMake, prove object parity before deleting Make; generated MSVC projects; 64-bit little-endian only. |
| 7 | #8 code rules | todo | Warning-class PRs, codegen-identical format commit, tidy, wire/file layout and fixed-width checks, release-identical Q_ASSERT. |
| 8 | #6 design note only | todo | docs/design/rhi.md; no RHI or ImGui implementation. |

## Decisions

- User's explicit sequence supersedes the older tracking-issue ordering. Read AGENTS.md on origin/modernization, plan sections 10/11, and issues #25, #1–#8, #31 before starting.
- Existing QVM/dlopen paths remain only as the required regression/parity oracle until their sequenced retirement; do not add new JIT/dlopen usage. No simulation-affecting floating-point edits except tested #31 fixes.
- Match surrounding style until #8; no tree-wide formatting before its dedicated verified commit.
- Tests may own their resources outside the engine; engine core gains no non-trivial destruction, OS calls, or per-frame allocation.
- No local system package installation; CI installs prerequisites in each job. Proprietary paks remain outside git; missing content fails the test.
- One issue per PR against modernization, with separate PRs for individual #31 bugs / #8 warning classes as explicitly requested. Merge commits only after required gates and scope self-review pass.

## Accepted baseline

Thirteen asset-free groups pass GCC and Clang/libc++; full differential adds the
original q3dm17 collision sweep. vector_math remains 5c00b4de. q3dm17/q3dm7 bot traces
have accepted repeated goldens. The one-ULP Q_rsqrt negative control rejects a
mutant of the active GCC SSE return. Port evidence is unchanged and remains
reachable via docs/cpp-port-progress.md and the orphan port-evidence branch.

## Self-review / outstanding verification

No engine/vendor change, allocation, destructor, OS call or simulation FP edit.
Only initial new-content/demo baselines may be generated in #3. Existing accepted
goldens must stay byte-identical. Still required: review demo fixtures/screenshots,
OpenArena checks, expected-failure policy negative checks, hosted workflow including
merged tree, documentation, issue measurements and ready/merge PR #33.

### Local acceptance measurements before hosted CI

GCC/Clang unit goldens and one-ULP controls pass. Known-bug classifier rejects both
unknown diagnostics and disappearance of its expected error; the sanitizer run
reports Huffman alignment and matches all thirteen hashes. OpenArena oa_dm1 and
oa_dm7 smoke both repeat and match new goldens, with networking disabled, CPU label
normalized, and 900 waits to exercise combat. The first temporary-content prefix
normalization was corrected before acceptance (replace HOME before DATA).

OpenArena apt data contain native-module markers instead of QVM bytecode. The
staging helper adds checksum-pinned official oaxB52 GPL QVMs (README has source and
license details); all downloaded content stays in /tmp. OpenArena demo frames match
on a second complete replay run. Quake 3 initial idle-player recordings were
rejected during visual review because q3dm17 showed only a death screen. Replacement
recordings wait for connection, then spectate a bot; screenshot review now shows
active gameplay. Final Q3 fixture hashes: q3dm17
f5a407f330241906e1e5055f225ce7966ac1b390b2cd6e3b937513fcb21b71ad;
q3dm7 ca6950d578ccf17dfcb9bd0f72388783a2901029e6770f0494b1bc130fbe91b2.
Each replays identical frames twice per renderer. No previously accepted golden,
production source, or port-evidence was changed. Hosted verification is next.

Hosted run 34865213291 on 358c0fec passed GCC, Clang, both cross builds, sanitizers,
and OpenArena collision/both smoke goldens. Runtime stopped compiling the client:
its job lacked SDL development prerequisites installed by the separate build
workflow. Added libsdl2-dev to the runtime job and exposed build logs on failure;
replay on hosted CI remains to verify. Final local Quake 3 collision/smoke/replay
comparison also passed without changing accepted baselines.

Follow-up 34865406267 passed the same non-client gates and identified the next
missing client prerequisite explicitly: curl/curl.h. Added the curl development
package and Mesa headers used by the existing supported Linux build. No test or
golden waiver; the hosted replay must actually execute before merge.

### Cross-host renderer investigation (acceptance still pending)

Run 34865647962 on 75e505b3 reached both-map replay: every repeated pair matched,
but hosted Mesa 25.2.8 differed from local Mesa 26.0.8 by small pixel values (oa_dm1
frame100 maximum channel delta 2; oa_dm7 maximum 8). Collision and smoke goldens
matched across hosts. The full existing build matrix passed on 75e505b3.

Review found a preexisting #3 harness configuration error: Make expects
RENDERER_DEFAULT=opengl, but demo.py passed opengl1, leaving Vulkan selected. Earlier
claims of two-renderer coverage were incorrect. Fixing the test configuration and
asserting GL_RENDERER/VK_RENDERER in logs; production source remains unchanged.
Current demos remain fixed; only the initial, still-unaccepted frame baselines are
being corrected with explicit commands. Exact hashes are now separated by Mesa
version; unknown versions fail until their repeated evidence is reviewed. No pixel
tolerance or golden fallback is introduced. The saved-evidence checker enables a
local explicit regeneration command after reviewing a hosted artifact; CI itself
never regenerates goldens. Next: validate real OpenGL1, review local/hosted frames,
commit the new initial profiles, and rerun all gates before ready/merge.

Hosted evidence run 34866295338 on 145939aa passes both content oracles and all
replay-pair checks with real OpenGL1 and Vulkan. It fails only because the initial
Mesa 25.2.8 profile is not yet committed. Downloaded runtime-diagnostics, reviewed
all twelve sampled screenshots and renderer logs, then ran the documented LOCAL
command `python3 tests/frames.py --output /tmp/aftershock-ci-runtime4/aftershock-demo-tests
--content openarena --regenerate`. The command verifies all twenty-four screenshots
(two repeats) and writes the new exact-hash profile; CI never regenerates anything.
The fixed demo hashes match those committed before the run. Local Mesa 26.0.8
Quake 3/OpenArena replay passes again with real OpenGL1, and runnable negative
checks reject wrong-renderer logs and unequal repeats. Full build workflow
34866295171 passed. Next: commit this reviewed initial hosted profile, require
current-head CI green, update the checkpoint/issue, ready/merge #33, and verify the
merged-tree workflow before starting #31.
