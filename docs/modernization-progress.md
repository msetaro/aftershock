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
