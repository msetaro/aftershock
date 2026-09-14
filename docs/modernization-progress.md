# Modernization checkpoint

Integration branch: `modernization`; issue branches: `issue/<number>-<slug>`. Strict C++20 port is merged in main (PR #32). Never push main, force-push, rewrite history, or alter port-evidence. Work stops after #8 and a design-only note for #6; #6/#7 implementation is excluded.

## Next action

Continue #3 on issue/3-regression-suite: deterministic software-rendered demo recording/replay, network/fuzz coverage and CI; preserve the existing Huff_Decode sanitizer failure for a separate #31 fix. Unit/differential/runtime goldens and explicit regeneration are implemented; do not merge incomplete acceptance.

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

Initial #3 baseline: 12 asset-free groups pass GCC and Clang/libc++; full G5 adds the original collision sweep. vector_math remains 5c00b4de. q3dm17 and q3dm7 bot traces repeat identically and have committed goldens; tests/README.md gives commands. Test sources reuse the port driver as C++ with the same test stubs, without requiring a historical C checkout. One-ULP negative control targets the active GCC SSE return; its initial fallback-only mutant correctly revealed that it had not changed the active implementation, and was corrected. Existing port evidence remains reachable through docs/cpp-port-progress.md and the untouched orphan port-evidence branch. New regression goldens will be generated only through an explicit documented command that CI never invokes.

## Blockers

1. #3 sanitizer run finds existing Huff_Decode unaligned uint32_t read at huffman_static.cpp:206 during MSG roundtrip; recorded in #31 and cpp-port-notes.md, no added suppression or engine change. Reproducer: python3 tests/run.py unit --cc clang --cxx "clang++ -stdlib=libc++" --sanitize --output /tmp/aftershock-san-tests.
2. #3 hosted runtime CI cannot access required q3dm17/q3dm7 paks: gh secret list is empty and gh api repos/msetaro/aftershock/actions/runners returns total_count 0. Local licensed assets exist; do not upload paks or count missing-data jobs as passing.
3. ffmpeg and glslangValidator are absent; native TGA screenshot support avoids needing ffmpeg for fixed-frame hashing. No packages installed. GCC, Clang/libc++, both requested cross compilers, faketime, Xvfb and Mesa lvp ICD are present.
