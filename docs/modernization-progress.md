# Modernization checkpoint

Integration branch: `modernization`; issue branches: `issue/<number>-<slug>`. Strict C++20 port is merged in main (PR #32). Never push main, force-push, rewrite history, or alter port-evidence. Work stops after #8 and a design-only note for #6; #6/#7 implementation is excluded.

## Next action

Continue #3 on issue/3-regression-suite: deterministic software-rendered demo recording/replay, network/fuzz coverage and CI; preserve the existing HuffmanGetSymbol sanitizer failure for a separate #31 fix. Unit/differential/runtime goldens and explicit regeneration are implemented; do not merge incomplete acceptance.

## New-thread handoff (2026-09-14)

All implementation through `069c2249` is committed and pushed to `origin/issue/3-regression-suite`. Draft PR [#33](https://github.com/msetaro/aftershock/pull/33) targets `modernization`; leave it draft until acceptance passes. This checkpoint-only commit follows that implementation. The user requested a pause for a new thread, not a scope change. No production engine or vendored source has been modified during #3.

Resume by reading this file, AGENTS.md, plan sections 10/11 and the specified issues. Do not restart the port or regenerate accepted goldens. Concrete remaining steps:

1. Add the missing BSP, MD3, IQM and pk3 robustness targets to `tests/fuzz/run.py`, reusing `tests/fuzz/common.h`, `files.h`, and real loader implementations. Existing targets are parse/msg/tga/png/jpeg. BSP investigation stopped before edits: `CM_ClearMap` is declared in cm_public.h; loader cvar stubs will need `Cvar_Get` and `Cvar_SetDescription` with persistent trivial storage.
2. Run `python3 tests/network.py --max-error 0` to verify the prediction-bound negative control. The normal in-process test already passes; do not restore the superseded UDP experiment.
3. Investigate the remaining repeated demo-byte mismatch in `tests/demo.py`. The checksumFeed fixture and corrected `fixedtime` cvar are committed; no complete demo/frame golden is accepted. Do not waive scoreboard ping or unexplained packet differences.
4. Resolve or explicitly checkpoint CI prerequisites and the sanitizer bugs, keeping fixes in individual #31 PRs. Follow the user's stuck rule if #3 cannot be completed; do not merge a failing draft or silently skip checks. All later modernization issues remain outstanding as listed below.

Verified CI on the implementation commit: [existing build workflow](https://github.com/msetaro/aftershock/actions/runs/34858816371) **passed**; [new regression workflow](https://github.com/msetaro/aftershock/actions/runs/34858816443) **failed**. GCC unit/one-ULP, parser fuzz and TGA fuzz passed. Sanitizer/MSG, PNG and JPEG jobs reproduce the recorded bugs. Hosted clang/libc++ fails with `/usr/bin/ld: cannot find -lc++`; cross jobs lack the requested compilers; runtime stops at missing faketime, with game-asset provisioning also unresolved. Local tools are available as recorded below. No system packages were installed and no runner was provisioned.

Local scratch evidence is useful but disposable; it is not required to recover the source:

- `/tmp/modernization-ci-failed.log`: downloaded regression failures; reproduce with `gh run view 34858816443 -R msetaro/aftershock --log-failed`.
- `/tmp/aftershock-demo-tests/`: differing recordings, record/replay logs and builds; `/tmp/modernization-demos.log`: most recent mismatch. Temporary decoder `/tmp/modernization-snapshotdump.cpp` and `/tmp/demo7-snap-{0,1}.txt` showed the scoreboard ping discrepancy. These diagnostic files are not suite deliverables.
- `/tmp/aftershock-loopback-tests/results.json`: passing impairment measurements; `/tmp/aftershock-fuzz/`: seeded corpora/crashes. Recreate using the committed tests commands if missing.

Full runnable commands and explicit regeneration rules are in `tests/README.md`. Preserve `port-evidence` unchanged. No policy/tool rejection was observed in this session; the recorded blockers are reproducible test failures or missing CI prerequisites.

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
- No system package installation. Proprietary game paks remain outside git; CI content provisioning must be explicit and cannot silently skip required tests.
- One issue per PR against modernization, with separate PRs for individual #31 bugs / #8 warning classes as explicitly requested. Merge commits only after required gates and scope self-review pass.

## Measurements and reproducible commands

Initial #3 baseline: 13 asset-free groups (the 12 inherited groups plus explicit wire/file layout) pass GCC and Clang/libc++; full G5 adds the original collision sweep for 14 groups total. vector_math remains 5c00b4de. q3dm17 and q3dm7 bot traces repeat identically and have committed goldens; tests/README.md gives commands. Test sources reuse the port driver as C++ with the same test stubs, without requiring a historical C checkout. One-ULP negative control targets the active GCC SSE return; its initial fallback-only mutant correctly revealed that it had not changed the active implementation, and was corrected. Existing port evidence remains reachable through docs/cpp-port-progress.md and the untouched orphan port-evidence branch. New regression goldens will be generated only through an explicit documented command that CI never invokes.

## Blockers

1. #3 sanitizer run finds existing HuffmanGetSymbol unaligned uint32_t read at huffman_static.cpp:206 during MSG roundtrip; recorded in #31 and cpp-port-notes.md, no added suppression or engine change. Reproducer: python3 tests/run.py unit --cc clang --cxx "clang++ -stdlib=libc++" --sanitize --output /tmp/aftershock-san-tests.
2. #3 hosted runtime CI cannot access required q3dm17/q3dm7 paks: gh secret list is empty and gh api repos/msetaro/aftershock/actions/runners returns total_count 0. Local licensed assets exist; do not upload paks or count missing-data jobs as passing.
3. ffmpeg and glslangValidator are absent; native TGA screenshot support avoids needing ffmpeg for fixed-frame hashing. No packages installed. GCC, Clang/libc++, both requested cross compilers, faketime, Xvfb and Mesa lvp ICD are present.

## #3 continuation measurements

- Actual in-process loopback is implemented in tests/network.py with test-only GNU link wrappers at NET_GetLoopPacket, NET_Init and Com_Frame. The original client/server, prediction QVM, serializers and loopback send code remain intact; net_enabled=0 is asserted. The test queues POD packet copies with seeded 20–140 ms virtual delay, 5% loss, and reordering. One run received 397/356, delivered 369/330, dropped 51 and reordered 124 packets; maximum reported correction 8.875 world units is below the scenario's explicit 64-unit bound. This is a scenario bound, not a guarantee for arbitrary maps/connections. Reproduce: python3 tests/network.py. The earlier uncommitted two-process UDP experiment was superseded by the user's no-real-network instruction and is not part of the suite.
- Parser libFuzzer smoke passes 1,000 runs; seeded TGA smoke passes 1,000 runs. MSG reproduces HuffmanGetSymbol alignment UB. Seeded PNG finds PNG_ChunkHeader alignment at tr_image_png.cpp:467; seeded JPEG finds an out-of-range Huffman table address formed before checking its index at libjpeg/jdmarker.c:508. No suppression or source fix added. Reproduce with python3 tests/fuzz/run.py TARGET --runs 1000 (parse/msg/tga/png/jpeg); full seed/run commands and crash artifacts are kept under /tmp/aftershock-fuzz. BSP/MD3/IQM/pk3 entry points remain to implement.
- Demo investigation: static Vulkan and OpenGL builds pass; software devices are forced with LIBGL_ALWAYS_SOFTWARE=1, GALLIUM_DRIVER=llvmpipe, LP_NUM_THREADS=1 and Mesa's lvp ICD. Two recorded q3dm17 demos replayed identical sampled TGA frames with both renderers in the initial trial. A test-only /dev/urandom fixture pins checksumFeed, whose four random bytes caused an initial recording difference. Later repeated recordings still differ: decoded q3dm7 playerstate was identical but the recorded scoreboard ping differed (413/414), and other packet-byte differences remain unexplained. No byte difference is waived and no complete demo golden is accepted yet. Use tests/demo.py --regenerate only for an explicitly reviewed baseline; it stops on a repeat mismatch. The engine's fixed-step cvar is named fixedtime, not com_fixedtime; corrected the runner after reading Com_ModifyMsec.
- The regression workflow is being assembled with mandatory checks; it never regenerates goldens or installs packages. Existing build.yml supplies MSVC coverage. Neither the new workflow nor #3 is claimed green before the open sanitizer/demo/asset/toolchain requirements pass.

## Current self-review

Scope remains #3 tests/CI and discovered-bug records. No engine or vendor source changed, no core destructor/OS call/per-frame allocation added. The new test wrapper owns a fixed POD queue outside production code. Golden changes only establish the initial unchanged-engine baseline and add explicit layout coverage. AGENTS.md Verification commands will change only after #3 lands.
