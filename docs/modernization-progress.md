# Modernization checkpoint

Integration branch: `modernization`; issue branches: `issue/<number>-<slug>`. Strict C++20 port is merged in main (PR #32). Never push main, force-push, rewrite history, or alter port-evidence. Work stops after #8 and a design-only note for #6; #6/#7 implementation is excluded.

## Next action

Start #3 on issue/3-regression-suite: inspect and reuse tools/port, inventory installed tools and content, establish explicit golden generation and read-only comparison, then add runtime/demo/network/fuzz/sanitizer/CI coverage. Do not merge until the issue's gates and self-review pass; record exact blockers rather than weaken acceptance.

## Issue status

| Order | Issue | Status | Outcome / next step |
|---|---|---|---|
| 1 | #3 regression suite | todo | Permanent differential hashes, q3dm17/q3dm7 smoke/demo goldens, software-rendered frames, sanitizers, network simulation, fuzzers, compiler matrix, one-ULP negative control. |
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

Pending #3 baseline. Existing port evidence remains reachable through docs/cpp-port-progress.md and the untouched orphan port-evidence branch. New regression goldens will be generated only through an explicit documented command that CI never invokes.

## Blockers

None established yet; tool/data inventory is next. Missing prerequisites will be recorded here and in the affected issue, with partial work preserved on its issue branch rather than falsely marked complete.
