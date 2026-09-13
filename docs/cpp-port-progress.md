# C++20 port checkpoint

Base: `8a7e8ed2`; branch: `t3code/port-engine-to-cpp20`. Work is incomplete.

## Next action

Phase 1: next `code/botlib/be_aas_bspq3.c`. Resume there; do not redo done files. Blocked files remain unchanged; final gates/rename remain pending.

## Phases

- [ ] Phase 0: harness artifacts/checksum/gate controls/CI/notes complete; formatting <3% requirement unmet (DEVIATION recorded)
- [ ] Phase 1: qcommon
- [ ] Phase 1: server
- [ ] Phase 1: botlib
- [ ] Phase 1: unix + sdl
- [ ] Phase 1: client
- [ ] Phase 1: renderercommon
- [ ] Phase 1: renderer
- [ ] Phase 1: renderervk
- [ ] Phase 1: win32
- [ ] Phase 2: external C linkage boundaries
- [ ] Phase 3: content-free engine renames and build-list updates
- [ ] Final G1-G8, differential/runtime/sanitizer checks

## Decisions

- Read AGENTS.md and the entire plan before work. The existing t3code branch is used as explicitly requested; no main pushes, force pushes, or history edits.
- Plan section 7 supersedes early CMake instructions: GNU Make only; CMake remains untouched. Section 9 defers vcxproj lists to rename and makes MSVC CI-only.
- Phase 0 makes no edits under code/. Catalog transformations apply to phase 1; harness infrastructure is expressly authorized by phase 0.
- Headers are tracked individually and verified through their consuming translation units. Inactive architecture sources remain tracked and cannot be called verified based on the native build.
- T2 will use `(qboolean)( expression )` consistently in each module.
- All engine .c/.h outside the six excluded directories appear below. Game/UI sources outside shared headers/bg scope are tracked as done with an exclusion reason, not ported.
- Checksum reproducibility uses `SOURCE_DATE_EPOCH=1789257600` for both C baselines: unix_main embeds `__TIME__` and common embeds `__DATE__`. No source or C flags changed to remove those values.
- Missing tools: Xvfb and x86_64-w64-mingw32-gcc not found on PATH. No packages will be installed. gcc, g++, clang, clang-format, clang-tidy, pahole, bear are present.
- Gate G4 differences are advisory and recorded for human review; they do not count as a passed identical-codegen check.

## Evidence and reproduction

- Baseline full C build: PASS, 295 objects. First Makefile checksum comparison: PASS, all 295 objects byte-identical. Final Makefile checksum check: PASS, 295/295 identical.
- Reproduction: `SOURCE_DATE_EPOCH=1789257600 make -B -j$(nproc) BUILD_DIR=/tmp/aftershock-cpp-port/oracle`, run on base and modified Makefile in the same worktree. Hash every `.o` relative to the build root. Persistent baseline manifest: `tools/port/evidence/phase0-c.sha256`. Local manifests: `/tmp/aftershock-cpp-port/{before,after}.sha256`; logs: `{baseline,after}.log`.
- Baseline warning probes: `SOURCE_DATE_EPOCH=1789257600 make -k -j$(nproc) BUILD_DIR=/tmp/aftershock-cpp-port/c-warnings CFLAGS=-Wextra`; C++ counterpart uses `BUILD_CXX=1 BUILD_DIR=/tmp/aftershock-cpp-port/cxx-warnings CXX_FROZEN_WARNINGS='-Wall -Wextra'` (no permissive mode). C++ probe failed as expected with 3489 errors including cascades; this is not a completed port.
- Warning counts below count diagnostic lines from engine paths (exclude vendored paths), with duplicate client/ded/renderer compilations counted separately. Frozen suppressions: sign-compare 544 C / 532 C++; unused-parameter 171 / 173; missing-field-initializers 100 / 595; implicit-fallthrough 39 / 39; ignored-qualifiers 4 / 4; type-limits 2 / 2.
- Unsuppressed observed classes: C discarded-qualifiers 4 and old-style-declaration 4; C++ literal-suffix 57, write-strings 243, unused-function 9, register 10, parentheses 48, deprecated-enum-float-conversion 42, switch 2, extra 1. Catalog-fixable warnings must be transformed. Warnings cascading from parse errors will be reassessed after fixing the cause.
- Single-object strict C++ builds of `ded/md4.o` and `ded/q_math.o`: PASS. Use `make BUILD_CXX=1 BUILD_DIR=/tmp/aftershock-cpp-port/single /tmp/aftershock-cpp-port/single/release-linux-x86_64/ded/md4.o` (likewise q_math).
- Local artifacts: `/tmp/aftershock-cpp-port`; persistent evidence follows in `tools/port/` and this checkpoint.

## Harness status

- Corrected direct-object proxy timestamp bug: existing top-level object targets previously skipped recursion, so source freshness was not checked. Added a phony prerequisite only to proxy rules; recursive Make still uses normal dependencies. Revalidated every retained edit through e00a079d: forced full C rebuild PASS, SHA256 295/295 equal to original oracle; 110 native strict release/debug client/ded/renderer object targets PASS. Prior per-file checksum statements are now independently confirmed by this forced rebuild. Commands: `SOURCE_DATE_EPOCH=1789257600 make -B -j20 BUILD_DIR=/tmp/aftershock-cpp-port/oracle`; hash against committed manifest; strict target log `/tmp/aftershock-cpp-port/recheck-strict.log`.

- Qcommon assessed completely: 7 blocked source/header entries remain; module is not green. Retained source edits passed G8 review (134 replaced lines across 14 files, 0.425%, through 9277ddcb); later vm_interpreted adds one T1 cast. Full C client/renderers and dedicated builds PASS after 74ee31d9 using the fixed epoch. Native strict release/debug contexts checked; JIT blockers prevent the complete C++ link/runtime. Continuing server per stuck rules.

- `tools/port/math_gate.sh`: standalone reproducible G5 math differential, currently FAIL as recorded under blocked files. It does not modify engine sources.

- md5 has a nested union type declaration repeated in C++ pahole output. Use pahole `-M` (data members only) to compare actual stored layout, retaining embedded union offsets and sizes; md5 is added to gate controls.

- G3 local-static numbering: C emits cv.N while C++ mangles function scope with no suffix. Strip compiler numeric suffixes on both sides and retain the complete multiset and nm linkage kinds; source locals remain counted.

- cm_load exposed compiler-generated C `__func__.N` arrays versus C++ `.LC` strings in G3. Ignore compiler function-name string symbols symmetrically with existing compiler labels; G4 still compares their contents. This is harness normalization, not a source change.

- C dedicated smoke: exit 0 with requested q3dm17/Sarge/Major/wait-300 command. C ASan/UBSan smoke: exit 0, no ASan error, three known unaligned loads (unzip.c:1523/:1524, vm.c:1181). `tools/port/ubsan.supp` seeds function-scoped alignment exclusions; verification pending. No source fix.
- Sanitizer reproduction: `SOURCE_DATE_EPOCH=1789257600 make -j$(nproc) BUILD_CLIENT=0 BUILD_DIR=/tmp/aftershock-cpp-port/sanitize CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' LDFLAGS='-fsanitize=address,undefined -lm -ldl'`; run its `release-linux-x86_64/quake3e.ded.x64 +set dedicated 1 +set sv_pure 0 +set com_logfile 0 +map q3dm17 +addbot sarge 3 +addbot major 3 +wait 300 +quit` with `ASAN_OPTIONS=detect_leaks=0`. Logs: `/tmp/aftershock-cpp-port/{sanitize-build,runtime-sanitize,runtime-c}.log`.
- G8: reviewer verified checksum proof, layout positive/negative controls, symbol boundary correction, and CI references. Direct renderer2-target issue fixed in 7e34499d and rechecked locally. Artifact compiler currently native Linux x86_64; other architectures must not be marked verified by it.

- G8 reviewer found direct rend2 object targets bypassed USE_OPENGL2=0. Disabled those pattern rules in C++ mode; full C mode remains unchanged.

- G8 reviewer found G3 demangling hid missing extern-C on GetRefAPI. Fixed by preserving raw names for the plan-listed loader/assembly exports; added a negative control. JIT-specific call targets still require phase-2 source inventory.

- CI: removed windows-msys32; added non-failing ubuntu-cxx-probe with compiler error count, make status, and log artifact. Dependent archive jobs now use existing windows-msys gcc artifacts; release output filenames remain the same. Existing C CI legs retained.

- `.clang-tidy`: only signed-char-misuse, narrowing-conversions, suspicious-string-compare, and portability-simd-intrinsics; changed-line use only, no modernization checks.

- `tools/port/selfcheck.sh`: PASS, positive controls and deliberate layout-offset, symbol-linkage, and instruction mutations all detected.

- Entry point: `tools/port/codegen_gate.sh C-artifact CXX-artifact`; returns nonzero and prints FAIL plus unified diff on differences.

- Entry point: `tools/port/symbol_gate.sh C-artifact CXX-artifact`; returns nonzero and prints FAIL plus unified diff on differences.

- Entry point: `tools/port/layout_gate.sh C-artifact CXX-artifact`; returns nonzero and prints FAIL plus unified diff on differences.

- Makefile: committed, C checksum proof PASS (295 objects), strict single objects PASS; frozen warning counts above.
- `tools/port/compile_pair.py ded/md4.o /tmp/aftershock-cpp-port/gates` (likewise q_math) emits debug objects, -O2 assembly, and exact compiler commands using Makefile flags.
- G2/G3 on q_math/md4: PASS. G4 md4 PASS, q_math advisory FAIL retained below.
- Gate implementation excludes DWARF records by declaration provenance (engine-only) and fails on missing DWARF/empty engine layouts. Symbols retain nm kind/linkage; only labels and compiler clone numbering normalize. Assembly retains instructions/constants.
- Initial clang-format whole-file trial: cvar.c 549/2141 changed lines (25.642%); cl_main.c 982/5120 (19.180%). Threshold unmet; source files untouched. Surrounding source mixes styles that a global formatter cannot preserve exactly. Sixteen configurations tested; best results and disposition are listed under Deviations. This is not a <3% pass.

## Deviations

- `DEVIATION: freeze observed legacy string-literal warnings`: add `-Wno-write-strings` (243 engine diagnostics in the original C++ probe) to the frozen list. Plan section 11 explicitly identifies this class as historical baseline noise tolerated by the C build. The original list mistakenly excluded it, which would force noncatalog signature changes in VM_Indent. Reviewer confirmed that freezing this observed class fits phase 0; the separate deviation documents changing an already-frozen list. No engine behavior, C flags, or -fpermissive policy changes. Other noncatalog hard errors remain blocked.

- `DEVIATION: preserve source style despite formatter threshold`: after 16 real formatter trials varying declaration alignment, array/cast spaces and operand alignment, best whole-file change counts are cvar.c 547/2141 (25.549%) and cl_main.c 914/5120 (17.852%). The requested <3% full-file threshold is unmet. Existing files mix tab alignment, braces and expression spacing. A global clang-format configuration cannot encode every surrounding line's style; disabling formatting to claim 0% would be a false pass. Keep the closest id-style configuration, use changed-line output only as an advisory review aid, and manually preserve surrounding style as the authoritative plan requires. No engine file was reformatted. Phase 0's threshold remains a documented limitation; all other harness work and subsequent independently verifiable source work continue unattended.

## Codegen differences

- `code/server/sv_world.c`: G4 advisory FAIL; full diff `tools/port/evidence/sv_world.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/sv_world.o /tmp/aftershock-cpp-port/sv_world` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/server/sv_snapshot.c`: G4 advisory FAIL; full diff `tools/port/evidence/sv_snapshot.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/sv_snapshot.o /tmp/aftershock-cpp-port/sv_snapshot` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/server/sv_net_chan.c`: G4 advisory FAIL; full diff `tools/port/evidence/sv_net_chan.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/sv_net_chan.o /tmp/aftershock-cpp-port/sv_net_chan` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/server/sv_main.c`: G4 advisory FAIL; full diff `tools/port/evidence/sv_main.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/sv_main.o /tmp/aftershock-cpp-port/sv_main` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/server/sv_init.c`: G4 advisory FAIL; full diff `tools/port/evidence/sv_init.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/sv_init.o /tmp/aftershock-cpp-port/sv_init` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/server/sv_game.c`: G4 advisory FAIL; full diff `tools/port/evidence/sv_game.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/sv_game.o /tmp/aftershock-cpp-port/sv_game` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/server/sv_filter.c`: G4 advisory FAIL; full diff `tools/port/evidence/sv_filter.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/sv_filter.o /tmp/aftershock-cpp-port/sv_filter` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/server/sv_ccmds.c`: G4 advisory FAIL; full diff `tools/port/evidence/sv_ccmds.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/sv_ccmds.o /tmp/aftershock-cpp-port/sv_ccmds` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/server/sv_bot.c`: G4 advisory FAIL; full diff `tools/port/evidence/sv_bot.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/sv_bot.o /tmp/aftershock-cpp-port/sv_bot` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/vm_interpreted.c`: G4 advisory FAIL; full diff `tools/port/evidence/vm_interpreted.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/qvm/vm_interpreted.o /tmp/aftershock-cpp-port/vm_interpreted` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/common.c`: G4 advisory FAIL; full diff `tools/port/evidence/common.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/common.o /tmp/aftershock-cpp-port/common` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/vm.c`: G4 advisory FAIL; full diff `tools/port/evidence/vm.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/qvm/vm.o /tmp/aftershock-cpp-port/vm` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/unzip.c`: G4 advisory FAIL; full diff `tools/port/evidence/unzip.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/unzip.o /tmp/aftershock-cpp-port/unzip` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/files.c`: G4 advisory FAIL; full diff `tools/port/evidence/files.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/files.o /tmp/aftershock-cpp-port/files` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/common.c`: G4 advisory FAIL; full diff `tools/port/evidence/common.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/common.o /tmp/aftershock-cpp-port/common` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/puff.c`: G4 advisory FAIL; full diff `tools/port/evidence/puff.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/puff.o /tmp/aftershock-cpp-port/puff` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/msg.c`: G4 advisory FAIL; full diff `tools/port/evidence/msg.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/msg.o /tmp/aftershock-cpp-port/msg` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/q_shared.c`: G4 advisory FAIL; full diff `tools/port/evidence/q_shared.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/q_shared.o /tmp/aftershock-cpp-port/q_shared` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/net_ip.c`: G4 advisory FAIL; full diff `tools/port/evidence/net_ip.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/net_ip.o /tmp/aftershock-cpp-port/net_ip` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/net_chan.c`: G4 advisory FAIL; full diff `tools/port/evidence/net_chan.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/net_chan.o /tmp/aftershock-cpp-port/net_chan` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/cvar.c`: G4 advisory FAIL; full diff `tools/port/evidence/cvar.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/cvar.o /tmp/aftershock-cpp-port/cvar` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/cmd.c`: G4 advisory FAIL; full diff `tools/port/evidence/cmd.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/cmd.o /tmp/aftershock-cpp-port/cmd` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/huffman.c`: G4 advisory FAIL; full diff `tools/port/evidence/huffman.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/huffman.o /tmp/aftershock-cpp-port/huffman` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/md5.c`: G4 advisory FAIL; full diff `tools/port/evidence/md5.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/md5.o /tmp/aftershock-cpp-port/md5` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/keys.c`: G4 advisory FAIL; full diff `tools/port/evidence/keys.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/keys.o /tmp/aftershock-cpp-port/keys` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/history.c`: G4 advisory FAIL; full diff `tools/port/evidence/history.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/history.o /tmp/aftershock-cpp-port/history` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/cm_trace.c`: G4 advisory FAIL; full diff `tools/port/evidence/cm_trace.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/cm_trace.o /tmp/aftershock-cpp-port/cm_trace` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/cm_patch.c`: G4 advisory FAIL; full diff `tools/port/evidence/cm_patch.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/cm_patch.o /tmp/aftershock-cpp-port/cm_patch` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `cm_load.c`: G4 advisory FAIL; compiler function-name string placement and resulting label/section differences. Full diff: `tools/port/evidence/cm_load.codegen.diff.gz` (`gzip -dc` to inspect). C SHA256 unchanged; G2/G3 PASS. Reproduce with `python3 tools/port/compile_pair.py ded/cm_load.o /tmp/aftershock-cpp-port/cm_load`, then each `tools/port/*_gate.sh` on the emitted `.c.o/.cxx.o` (G2/G3) or `.c.s/.cxx.s` (G4). 28 T1 casts, 28/1059 changed lines, no floating-point expression changed.

- `q_math.c`: G4 FAIL (advisory) on unchanged source, gcc/g++ 15.2, actual release Makefile flags plus `-O2 -S`. C uses double `sincos`, C++ selects `sincosf`; additional overload-related instruction differences exist. This is a potential semantic difference, not merely labels. Full diff: `tools/port/evidence/q_math.codegen.diff`. No floating-point expression was changed. G5 now confirms differing results; file is blocked; a double-argument cast is not in T1-T17 and must not be silently introduced.
- The full q_math G4 diff is committed without engine changes and must remain visible to human review.
- `md4.c`: G4 PASS, normalized assembly identical.

## Blocked files

- `code/server/tlds.h`: Unchanged initializer fragment; sole consumer sv_client.c is blocked, so complete-object C++/G2/G3 verification is unavailable.

- `code/server/sv_rankings.c`: At :25 missing rankings/1.0/gr/grapi.h SDK (legacy backslash include); SDK absent and source has no Makefile object rule. Cannot compile either C oracle or C++ or run gates; left unchanged.

- `code/server/sv_client.c`: At :402 C++ strstr(const char*, ...) returns const char*, assigned to writable str then sprintf writes through it. Removing const from cmd or casting away const is outside T8 (which permits adding const for literal pointers); source retained unchanged. Seven other T1/T2/T3 diagnostics remain.

- `code/qcommon/vm_optimize.h`: depends on blocked JIT consumers; unchanged.

- `code/qcommon/vm_powerpc.c`: Native syntax probe: :1727/:2579 need T1; :2558/:2561/:2564/:2567 pass function pointers to const void*, outside T1. No PPC64 cross compiler/sysroot verified; unmodified, target unverified.

- `code/qcommon/vm_armv7l.c`: Native syntax probe: :1223/:1931 need T1; :1978 __clear_cache undeclared; eight host-width overflow diagnostics from 32-bit instruction constants. No ARM cross compiler/sysroot verified; unmodified, target unverified.

- `code/qcommon/vm_aarch64.c`: Native syntax probe: :1583/:2290 need T1; :2348 __clear_cache undeclared. No aarch64 cross compiler/sysroot verified; cannot prove C object equivalence or target gates. Unmodified, target unverified.

- `code/qcommon/vm_x86.c`: At :3498 mov_rx_ptr(R_SYSCALL, vm->systemCall) converts syscall_t function pointer to const void*. T1 covers the reverse direction only; no catalog remedy. A T3 cast is also needed at :4323. No source edit retained.


- `code/qcommon/q_math.c`: confirmed semantic difference, not only G4 noise. `tools/port/math_gate.sh` compares 10,000 fixed inputs. C / C++ hashes: RotatePointAroundVector `056104dc` / `220b9dd8`; vectoangles `6f225d24` / `0929b283`; AngleVectors (fed preceding output) `0691ca72` / `84895270`; Q_rsqrt `301a8708` / `301a8708`. Float math overloads replace C double promotion. No allowed T1-T17 transformation restores double arithmetic here; source remains untouched. Full-port behavior equivalence and rename are blocked.

- `code/qcommon/huffman_static.c`: G3 `R HuffmanDecoderTable` -> `r HuffmanDecoderTable`. No existing declaration to move (whole-tree search found definition and internal use only). Adding `extern` to a const object is outside T1-T17; unchanged source retained per stuck rule.

## Bugs

`docs/cpp-port-notes.md` exists. No engine bug fixes; math overload hazard and previously documented CMake defects are recorded.

## Per-file status

| File | Status | Evidence / reason |
|---|---|---|
| `code/asm/qasm.h` | todo | Pending module pass. |
| `code/botlib/aasfile.h` | todo | Pending module pass. |
| `code/botlib/be_aas.h` | todo | Pending module pass. |
| `code/botlib/be_aas_bsp.h` | todo | Pending module pass. |
| `code/botlib/be_aas_bspq3.c` | todo | Pending module pass. |
| `code/botlib/be_aas_cluster.c` | todo | Pending module pass. |
| `code/botlib/be_aas_cluster.h` | todo | Pending module pass. |
| `code/botlib/be_aas_debug.c` | todo | Pending module pass. |
| `code/botlib/be_aas_debug.h` | todo | Pending module pass. |
| `code/botlib/be_aas_def.h` | todo | Pending module pass. |
| `code/botlib/be_aas_entity.c` | todo | Pending module pass. |
| `code/botlib/be_aas_entity.h` | todo | Pending module pass. |
| `code/botlib/be_aas_file.c` | todo | Pending module pass. |
| `code/botlib/be_aas_file.h` | todo | Pending module pass. |
| `code/botlib/be_aas_funcs.h` | todo | Pending module pass. |
| `code/botlib/be_aas_main.c` | todo | Pending module pass. |
| `code/botlib/be_aas_main.h` | todo | Pending module pass. |
| `code/botlib/be_aas_move.c` | todo | Pending module pass. |
| `code/botlib/be_aas_move.h` | todo | Pending module pass. |
| `code/botlib/be_aas_optimize.c` | todo | Pending module pass. |
| `code/botlib/be_aas_optimize.h` | todo | Pending module pass. |
| `code/botlib/be_aas_reach.c` | todo | Pending module pass. |
| `code/botlib/be_aas_reach.h` | todo | Pending module pass. |
| `code/botlib/be_aas_route.c` | todo | Pending module pass. |
| `code/botlib/be_aas_route.h` | todo | Pending module pass. |
| `code/botlib/be_aas_routealt.c` | todo | Pending module pass. |
| `code/botlib/be_aas_routealt.h` | todo | Pending module pass. |
| `code/botlib/be_aas_sample.c` | todo | Pending module pass. |
| `code/botlib/be_aas_sample.h` | todo | Pending module pass. |
| `code/botlib/be_ai_char.c` | todo | Pending module pass. |
| `code/botlib/be_ai_char.h` | todo | Pending module pass. |
| `code/botlib/be_ai_chat.c` | todo | Pending module pass. |
| `code/botlib/be_ai_chat.h` | todo | Pending module pass. |
| `code/botlib/be_ai_gen.c` | todo | Pending module pass. |
| `code/botlib/be_ai_gen.h` | todo | Pending module pass. |
| `code/botlib/be_ai_goal.c` | todo | Pending module pass. |
| `code/botlib/be_ai_goal.h` | todo | Pending module pass. |
| `code/botlib/be_ai_move.c` | todo | Pending module pass. |
| `code/botlib/be_ai_move.h` | todo | Pending module pass. |
| `code/botlib/be_ai_weap.c` | todo | Pending module pass. |
| `code/botlib/be_ai_weap.h` | todo | Pending module pass. |
| `code/botlib/be_ai_weight.c` | todo | Pending module pass. |
| `code/botlib/be_ai_weight.h` | todo | Pending module pass. |
| `code/botlib/be_ea.c` | todo | Pending module pass. |
| `code/botlib/be_ea.h` | todo | Pending module pass. |
| `code/botlib/be_interface.c` | todo | Pending module pass. |
| `code/botlib/be_interface.h` | todo | Pending module pass. |
| `code/botlib/botlib.h` | todo | Pending module pass. |
| `code/botlib/l_crc.c` | todo | Pending module pass. |
| `code/botlib/l_crc.h` | todo | Pending module pass. |
| `code/botlib/l_libvar.c` | todo | Pending module pass. |
| `code/botlib/l_libvar.h` | todo | Pending module pass. |
| `code/botlib/l_log.c` | todo | Pending module pass. |
| `code/botlib/l_log.h` | todo | Pending module pass. |
| `code/botlib/l_memory.c` | todo | Pending module pass. |
| `code/botlib/l_memory.h` | todo | Pending module pass. |
| `code/botlib/l_precomp.c` | todo | Pending module pass. |
| `code/botlib/l_precomp.h` | todo | Pending module pass. |
| `code/botlib/l_script.c` | todo | Pending module pass. |
| `code/botlib/l_script.h` | todo | Pending module pass. |
| `code/botlib/l_struct.c` | todo | Pending module pass. |
| `code/botlib/l_struct.h` | todo | Pending module pass. |
| `code/botlib/l_utils.h` | todo | Pending module pass. |
| `code/cgame/cg_public.h` | todo | Pending module pass. |
| `code/client/cl_avi.c` | todo | Pending module pass. |
| `code/client/cl_cgame.c` | todo | Pending module pass. |
| `code/client/cl_cin.c` | todo | Pending module pass. |
| `code/client/cl_console.c` | todo | Pending module pass. |
| `code/client/cl_curl.c` | todo | Pending module pass. |
| `code/client/cl_curl.h` | todo | Pending module pass. |
| `code/client/cl_input.c` | todo | Pending module pass. |
| `code/client/cl_jpeg.c` | todo | Pending module pass. |
| `code/client/cl_keys.c` | todo | Pending module pass. |
| `code/client/cl_main.c` | todo | Pending module pass. |
| `code/client/cl_net_chan.c` | todo | Pending module pass. |
| `code/client/cl_parse.c` | todo | Pending module pass. |
| `code/client/cl_scrn.c` | todo | Pending module pass. |
| `code/client/cl_ui.c` | todo | Pending module pass. |
| `code/client/client.h` | todo | Pending module pass. |
| `code/client/keycodes.h` | todo | Pending module pass. |
| `code/client/keys.h` | todo | Pending module pass. |
| `code/client/snd_adpcm.c` | todo | Pending module pass. |
| `code/client/snd_codec.c` | todo | Pending module pass. |
| `code/client/snd_codec.h` | todo | Pending module pass. |
| `code/client/snd_codec_ogg.c` | todo | Pending module pass. |
| `code/client/snd_codec_wav.c` | todo | Pending module pass. |
| `code/client/snd_dma.c` | todo | Pending module pass. |
| `code/client/snd_local.h` | todo | Pending module pass. |
| `code/client/snd_main.c` | todo | Pending module pass. |
| `code/client/snd_mem.c` | todo | Pending module pass. |
| `code/client/snd_mix.c` | todo | Pending module pass. |
| `code/client/snd_public.h` | todo | Pending module pass. |
| `code/client/snd_wavelet.c` | todo | Pending module pass. |
| `code/game/bg_public.h` | todo | Pending module pass. |
| `code/game/g_public.h` | todo | Pending module pass. |
| `code/qcommon/cm_load.c` | done | T1: 28; native ded C SHA256 unchanged (df42e0cabff475c22ebf8383e443cfe34d558abf817baf6f5cd8ad0c96700017); strict C++/G2/G3 PASS; G4 advisory diff retained. |
| `code/qcommon/cm_local.h` | done | T1-T17: 0; unchanged header checked via cm_load.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_patch.c` | done | T1: 3, T2: 6, T3: 3; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_patch.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/cm_patch.h` | done | T1-T17: 0; unchanged header checked via cm_patch.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_polylib.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_polylib.o); G4 PASS. |
| `code/qcommon/cm_polylib.h` | done | T1-T17: 0; unchanged header checked via cm_polylib.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_public.h` | done | T1-T17: 0; unchanged header checked via cm_load.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_test.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_test.o); G4 PASS. |
| `code/qcommon/cm_trace.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_trace.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/cmd.c` | done | T1: 1, T2: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cmd.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/common.c` | done | T1: 5, T2: 2, T3: 1, T15: 29; client/debug variants checked; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/common.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/cvar.c` | done | T2: 2, T3: 4 (cast compound-assignment result); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cvar.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/files.c` | done | T1: 13, T2: 2; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/files.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/history.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/history.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/huffman.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/huffman.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/huffman_static.c` | blocked | Compiles unchanged, but G3: HuffmanDecoderTable changes R (external) to r (static) under C++. Restoring extern const linkage is outside T1-T17; no source edit applied. |
| `code/qcommon/json.h` | done | Excluded: whole-tree include search finds only renderer2/tr_bsp.c; implementation is solely for excluded renderer2. Left unchanged. |
| `code/qcommon/keys.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/keys.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/md4.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/md4.o); G4 PASS. |
| `code/qcommon/md5.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/md5.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/msg.c` | done | T16: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/msg.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/net_chan.c` | done | T1: 1, T4: 1 identifier (12 occurrences); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/net_chan.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/net_ip.c` | done | T1: 2, T2: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/net_ip.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/puff.c` | done | T1-T17: 0 (already compatible); 3 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/puff.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/puff.h` | done | T1-T17: 0; unchanged header checked via puff.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/q_math.c` | blocked | G2/G3 PASS, but G5 fixed-input hashes differ in RotatePointAroundVector and vectoangles; AngleVectors chain differs too. C++ float overloads change results; double-argument casts are outside T1-T17. No source changes. |
| `code/qcommon/q_platform.h` | done | T1-T17: 0; unchanged header checked via md4.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/q_shared.c` | done | T1: 4, T2: 3; 4 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/q_shared.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/q_shared.h` | done | T1-T17: 0; unchanged header checked via q_shared.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/qcommon.h` | done | T1-T17: 0; unchanged header checked via md4.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/qfiles.h` | done | T1-T17: 0; unchanged header checked via cm_load.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/surfaceflags.h` | done | T1-T17: 0; unchanged header checked via cm_load.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/unzip.c` | done | T1: 10, T14: 5; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/unzip.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/unzip.h` | done | T1-T17: 0; unchanged header checked via unzip.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/vm.c` | done | T1: 5; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/qvm/vm.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/vm_aarch64.c` | blocked | Native syntax probe: :1583/:2290 need T1; :2348 __clear_cache undeclared. No aarch64 cross compiler/sysroot verified; cannot prove C object equivalence or target gates. Unmodified, target unverified. |
| `code/qcommon/vm_armv7l.c` | blocked | Native syntax probe: :1223/:1931 need T1; :1978 __clear_cache undeclared; eight host-width overflow diagnostics from 32-bit instruction constants. No ARM cross compiler/sysroot verified; unmodified, target unverified. |
| `code/qcommon/vm_interpreted.c` | done | T1: 1; literal retained under frozen warning policy; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/qvm/vm_interpreted.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/vm_local.h` | done | T1-T17: 0; unchanged header checked via vm.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/vm_optimize.h` | blocked | Unchanged; all consuming JIT translation units are blocked/unverified, so complete-object gates cannot verify this header. |
| `code/qcommon/vm_powerpc.c` | blocked | Native syntax probe: :1727/:2579 need T1; :2558/:2561/:2564/:2567 pass function pointers to const void*, outside T1. No PPC64 cross compiler/sysroot verified; unmodified, target unverified. |
| `code/qcommon/vm_x86.c` | blocked | At :3498 mov_rx_ptr(R_SYSCALL, vm->systemCall) converts syscall_t function pointer to const void*. T1 covers the reverse direction only; no catalog remedy. A T3 cast is also needed at :4323. No source edit retained. |
| `code/renderer/iqm.h` | todo | Pending module pass. |
| `code/renderer/qgl.h` | todo | Pending module pass. |
| `code/renderer/tr_animation.c` | todo | Pending module pass. |
| `code/renderer/tr_arb.c` | todo | Pending module pass. |
| `code/renderer/tr_backend.c` | todo | Pending module pass. |
| `code/renderer/tr_bsp.c` | todo | Pending module pass. |
| `code/renderer/tr_cmds.c` | todo | Pending module pass. |
| `code/renderer/tr_common.h` | todo | Pending module pass. |
| `code/renderer/tr_curve.c` | todo | Pending module pass. |
| `code/renderer/tr_flares.c` | todo | Pending module pass. |
| `code/renderer/tr_image.c` | todo | Pending module pass. |
| `code/renderer/tr_init.c` | todo | Pending module pass. |
| `code/renderer/tr_light.c` | todo | Pending module pass. |
| `code/renderer/tr_local.h` | todo | Pending module pass. |
| `code/renderer/tr_main.c` | todo | Pending module pass. |
| `code/renderer/tr_marks.c` | todo | Pending module pass. |
| `code/renderer/tr_mesh.c` | todo | Pending module pass. |
| `code/renderer/tr_model.c` | todo | Pending module pass. |
| `code/renderer/tr_model_iqm.c` | todo | Pending module pass. |
| `code/renderer/tr_scene.c` | todo | Pending module pass. |
| `code/renderer/tr_shade.c` | todo | Pending module pass. |
| `code/renderer/tr_shade_calc.c` | todo | Pending module pass. |
| `code/renderer/tr_shader.c` | todo | Pending module pass. |
| `code/renderer/tr_shadows.c` | todo | Pending module pass. |
| `code/renderer/tr_sky.c` | todo | Pending module pass. |
| `code/renderer/tr_surface.c` | todo | Pending module pass. |
| `code/renderer/tr_vbo.c` | todo | Pending module pass. |
| `code/renderer/tr_world.c` | todo | Pending module pass. |
| `code/renderercommon/tr_font.c` | todo | Pending module pass. |
| `code/renderercommon/tr_image_bmp.c` | todo | Pending module pass. |
| `code/renderercommon/tr_image_jpg.c` | todo | Pending module pass. |
| `code/renderercommon/tr_image_pcx.c` | todo | Pending module pass. |
| `code/renderercommon/tr_image_png.c` | todo | Pending module pass. |
| `code/renderercommon/tr_image_tga.c` | todo | Pending module pass. |
| `code/renderercommon/tr_noise.c` | todo | Pending module pass. |
| `code/renderercommon/tr_public.h` | todo | Pending module pass. |
| `code/renderercommon/tr_types.h` | todo | Pending module pass. |
| `code/renderercommon/vulkan/vk_platform.h` | todo | Pending module pass. |
| `code/renderercommon/vulkan/vulkan.h` | todo | Pending module pass. |
| `code/renderercommon/vulkan/vulkan_core.h` | todo | Pending module pass. |
| `code/renderercommon/vulkan/vulkan_win32.h` | todo | Pending module pass. |
| `code/renderercommon/vulkan/vulkan_xlib.h` | todo | Pending module pass. |
| `code/renderercommon/vulkan/vulkan_xlib_xrandr.h` | todo | Pending module pass. |
| `code/renderervk/iqm.h` | todo | Pending module pass. |
| `code/renderervk/shaders/bin2hex.c` | todo | Pending module pass. |
| `code/renderervk/shaders/spirv/shader_data.c` | todo | Pending module pass. |
| `code/renderervk/tr_animation.c` | todo | Pending module pass. |
| `code/renderervk/tr_backend.c` | todo | Pending module pass. |
| `code/renderervk/tr_bsp.c` | todo | Pending module pass. |
| `code/renderervk/tr_cmds.c` | todo | Pending module pass. |
| `code/renderervk/tr_common.h` | todo | Pending module pass. |
| `code/renderervk/tr_curve.c` | todo | Pending module pass. |
| `code/renderervk/tr_image.c` | todo | Pending module pass. |
| `code/renderervk/tr_init.c` | todo | Pending module pass. |
| `code/renderervk/tr_light.c` | todo | Pending module pass. |
| `code/renderervk/tr_local.h` | todo | Pending module pass. |
| `code/renderervk/tr_main.c` | todo | Pending module pass. |
| `code/renderervk/tr_marks.c` | todo | Pending module pass. |
| `code/renderervk/tr_mesh.c` | todo | Pending module pass. |
| `code/renderervk/tr_model.c` | todo | Pending module pass. |
| `code/renderervk/tr_model_iqm.c` | todo | Pending module pass. |
| `code/renderervk/tr_scene.c` | todo | Pending module pass. |
| `code/renderervk/tr_shade.c` | todo | Pending module pass. |
| `code/renderervk/tr_shade_calc.c` | todo | Pending module pass. |
| `code/renderervk/tr_shader.c` | todo | Pending module pass. |
| `code/renderervk/tr_shadows.c` | todo | Pending module pass. |
| `code/renderervk/tr_sky.c` | todo | Pending module pass. |
| `code/renderervk/tr_surface.c` | todo | Pending module pass. |
| `code/renderervk/tr_world.c` | todo | Pending module pass. |
| `code/renderervk/vk.c` | todo | Pending module pass. |
| `code/renderervk/vk.h` | todo | Pending module pass. |
| `code/renderervk/vk_flares.c` | todo | Pending module pass. |
| `code/renderervk/vk_vbo.c` | todo | Pending module pass. |
| `code/sdl/sdl_gamma.c` | todo | Pending module pass. |
| `code/sdl/sdl_glimp.c` | todo | Pending module pass. |
| `code/sdl/sdl_glw.h` | todo | Pending module pass. |
| `code/sdl/sdl_icon.h` | todo | Pending module pass. |
| `code/sdl/sdl_input.c` | todo | Pending module pass. |
| `code/sdl/sdl_snd.c` | todo | Pending module pass. |
| `code/server/server.h` | done | T1-T17: 0; unchanged header verified through server consumers, strict native release/debug and G2/G3 PASS. |
| `code/server/sv_bot.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_bot.o); G4 advisory FAIL, full diff retained. |
| `code/server/sv_ccmds.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_ccmds.o); G4 advisory FAIL, full diff retained. |
| `code/server/sv_client.c` | blocked | At :402 C++ strstr(const char*, ...) returns const char*, assigned to writable str then sprintf writes through it. Removing const from cmd or casting away const is outside T8 (which permits adding const for literal pointers); source retained unchanged. Seven other T1/T2/T3 diagnostics remain. |
| `code/server/sv_filter.c` | done | T3: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_filter.o); G4 advisory FAIL, full diff retained. |
| `code/server/sv_game.c` | done | T1: 199, T3: 7; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_game.o); G4 advisory FAIL, full diff retained. |
| `code/server/sv_init.c` | done | T1: 4; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_init.o); G4 advisory FAIL, full diff retained. |
| `code/server/sv_main.c` | done | T3: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_main.o); G4 advisory FAIL, full diff retained. |
| `code/server/sv_net_chan.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_net_chan.o); G4 advisory FAIL, full diff retained. |
| `code/server/sv_rankings.c` | blocked | At :25 missing rankings/1.0/gr/grapi.h SDK (legacy backslash include); SDK absent and source has no Makefile object rule. Cannot compile either C oracle or C++ or run gates; left unchanged. |
| `code/server/sv_snapshot.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_snapshot.o); G4 advisory FAIL, full diff retained. |
| `code/server/sv_world.c` | done | T3: 2 (includes bitwise assignment result); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_world.o); G4 advisory FAIL, full diff retained. |
| `code/server/tlds.h` | blocked | Unchanged initializer fragment; sole consumer sv_client.c is blocked, so complete-object C++/G2/G3 verification is unavailable. |
| `code/ui/ui_public.h` | todo | Pending module pass. |
| `code/unix/linux_glimp.c` | todo | Pending module pass. |
| `code/unix/linux_joystick.c` | todo | Pending module pass. |
| `code/unix/linux_local.h` | todo | Pending module pass. |
| `code/unix/linux_qgl.c` | todo | Pending module pass. |
| `code/unix/linux_qvk.c` | todo | Pending module pass. |
| `code/unix/linux_signals.c` | todo | Pending module pass. |
| `code/unix/linux_snd.c` | todo | Pending module pass. |
| `code/unix/unix_glw.h` | todo | Pending module pass. |
| `code/unix/unix_main.c` | todo | Pending module pass. |
| `code/unix/unix_shared.c` | todo | Pending module pass. |
| `code/unix/x11_dga.c` | todo | Pending module pass. |
| `code/unix/x11_randr.c` | todo | Pending module pass. |
| `code/unix/x11_vidmode.c` | todo | Pending module pass. |
| `code/win32/glw_win.h` | todo | Pending module pass. |
| `code/win32/resource.h` | todo | Pending module pass. |
| `code/win32/win_gamma.c` | todo | Pending module pass. |
| `code/win32/win_glimp.c` | todo | Pending module pass. |
| `code/win32/win_input.c` | todo | Pending module pass. |
| `code/win32/win_local.h` | todo | Pending module pass. |
| `code/win32/win_main.c` | todo | Pending module pass. |
| `code/win32/win_minimize.c` | todo | Pending module pass. |
| `code/win32/win_qgl.c` | todo | Pending module pass. |
| `code/win32/win_qvk.c` | todo | Pending module pass. |
| `code/win32/win_shared.c` | todo | Pending module pass. |
| `code/win32/win_snd.c` | todo | Pending module pass. |
| `code/win32/win_syscon.c` | todo | Pending module pass. |
| `code/win32/win_wndproc.c` | todo | Pending module pass. |
