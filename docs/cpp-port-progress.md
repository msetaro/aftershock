# C++20 port checkpoint

Base: `8a7e8ed2`; branch: `t3code/port-engine-to-cpp20`. Work is incomplete.

## Next action

Phase 1: renderervk; next `code/client/snd_mix.c`. Resume there; do not redo files marked done. Harness formatter threshold remains an explicit deviation; final gates/rename are not authorized by a partial native pass.

## Phases

- [ ] Phase 0: harness artifacts/checksum/gate controls/CI/notes complete; formatting <3% requirement unmet (DEVIATION recorded)
- [ ] Phase 1: qcommon
- [ ] Phase 1: server
- [x] Phase 1: botlib (native per-file compilation/G2/G3 complete; integrated runtime and final matrix pending)
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

- Windows G8 review complete: 14 T1 + 8 T2 casts in nine files; redundant same-type cast removed in 289755ce. Host full/default and dedicated C builds PASS. All engine source/header rows assessed; 32-bit qasm preprocessing also blocked by missing multilib headers.
- Phase 2 begins with guarded Q_EXTERN_C macro (empty in C), Q_setjmp_c/Q_longjmp_c prototypes. Full C rebuild PASS and 295/295 original hashes unchanged; md4 consumer strict C++/G2/G3 PASS. Windows declarations remain target-unverified.

- Windows inspection follows the user's explicit missing-MinGW exception: retain only catalog casts/keyword fixes whose source types can be established, commit each file, mark blocked/unverified because no C oracle, C++ build or G2/G3 can run. Known noncatalog issues remain in place. No system packages installed.

- Vulkan uses the same atomic T4 sequencing as OpenGL: 242 code-token occurrences across 13 files; comments/strings unchanged. Full C rebuild and all 295 original hashes PASS (`renderervk-t4-c.log`), same-source header G2/G3 PASS through rendv/tr_marks.o. Remaining per-file casts/gates remain pending.

- G8 confirms renderer keyword rename exactly changes only code tokens. T3 covers local nonvolatile enum compound arithmetic in tr_arb: preserve +2/+1 offsets and unused standalone increment result; add cast of original promoted integer result, as with prior enum bitwise assignments.

- Renderer T4 prerequisite is atomic across tr_local.h and all 14 source consumers (243 identifier occurrences; 202 replaced lines). Per-file commit sequencing cannot keep C green while a shared field declaration and its uses disagree; plan T4 requires every use updated. Reviewer approves coherent prerequisite, then individual completion commits. Comments/strings stay unchanged. Full C rebuild passes and all 295 original object hashes match (`renderer-t4-c.log`); same-source header G2/G3 pass through client/linux_signals.o. Whole-tree reference search retained at `/tmp/aftershock-cpp-port/or-whole-tree.txt`; renderervk/renderer2 have separate own types and are not consumers of this header.

- renderercommon/tr_font.c is verified in the supported default BUILD_FREETYPE-disabled configuration. freetype2 development metadata/headers are unavailable; dormant BUILD_FREETYPE body remains unverified and unchanged. No package installation.

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

- G8 Windows correction: removed a redundant same-type HMODULE cast at win_main GetModuleHandleExA lookup; only three actual void-pointer/allocator casts remain. win_gamma local hKernel32 is HANDLE/void*, so its cast remains T1.

- Vulkan assessed: 22 successful linked-engine sources and four headers pass original C hashes, strict native release/debug, G2/G3; tr_init deferred to phase 2, vk.c and its generated shader_data.c blocked by const linkage. Full default/ded C PASS (`renderervk-{full,ded}-c.log`). G8 approves through shader089a5011 and identical no-op T4 consumers; vk_vbo6T1 checked locally. Clang++21 syntax: 141 saved completed engine commands PASS; one standalone utility skipped by that probe and separately gcc/g++ checked. bin2hex reproduction: compile unchanged source with gcc and g++ -x c++ -std=c++20 -fno-exceptions -fno-rtti, both -O2 -Wall -Wextra -Werror -Wno-sign-compare; feed bytes(range(256))+bytes([0,255,1]) in ordinary and +append output modes, outputs identical.

- OpenGL renderer: 24/25 sources and all four headers completed with original C hashes, strict native release/debug and G2/G3 PASS. tr_init is deferred to phase-2 GetRefAPI T5 (cast attempt reverted). Full default, dedicated and non-SDL C builds PASS (`renderer-{full,ded,nosdl}-c.log`). G8 approves retained edits through shader f50e7609; no-op files and vbo T1/T2 checked locally. Clang++21 syntax probe now 119 completed native sources PASS. G4 advisory diffs are retained with module-prefixed filenames to avoid colliding with Vulkan counterparts.

- Renderercommon: all seven native C sources and public/types headers pass strict release/debug OpenGL/Vulkan object builds, original C hashes, G2/G3. Five G4 PASS; tr_image_tga/tr_noise advisory diffs retained. G8 approves 23 T1 casts across five files. Full default/ded C PASS (`renderercommon-{full,ded}-c.log`). Generated Khronos headers remain unchanged; native Xlib/Xrandr C/C++ fixture G2/G3 PASS, Windows header unverified (missing cross-toolchain). Fixture reproduction: define VK_USE_PLATFORM_XLIB_KHR and VK_USE_PLATFORM_XLIB_XRANDR_EXT, include code/renderercommon/vulkan/vulkan.h, define int port_vulkan_header_check; compile with gcc/g++ -I. -g -fno-eliminate-unused-debug-types (C++ adds -x c++ -std=c++20 -Wall -Wextra -Werror -fno-exceptions -fno-rtti), then layout/symbol gates. Local `/tmp/aftershock-cpp-port/vulkan-headers/`.

- G7 clang++21 syntax probe: 88 completed native sources PASS after five T16 field-offset macro casts (be_ai_goal 1, be_ai_weap 2, msg 2). Each cast preserves the existing address expression and int initializer member. Exact saved Makefile commands use clang++ with -fsyntax-only; `/tmp/aftershock-cpp-port/clang-probe.log`. This is not a full clang link/build gate. G8 reviewer independently approves these followups.

- Client assessed: 19 source files and seven local plus four shared ABI headers done; three sources blocked (cl_curl, cl_main, snd_codec_ogg). Default C and dedicated C full builds PASS; non-SDL full C PASS; all 19 successful non-SDL C objects match original hashes and 38 release/debug C++ targets PASS. Logs `/tmp/aftershock-cpp-port/client-{full,ded,full-nosdl}-c.log` and `client-nosdl-{0,1}.log`. G8 reviewer approves all retained T1/T2/T3/T15 edits and unchanged blockers through eab18e91. C++ runtime cannot link while compile blockers remain.

- Unix/SDL assessed: successful files have unchanged C hashes, strict native release/debug and G2/G3 PASS; five source/header blockers remain after linux_signals was cleared by renderer T4. Default SDL C, dedicated C and non-SDL C full builds PASS (`/tmp/aftershock-cpp-port/platform-{full,ded,nosdl}-c.log`). Dormant joystick explicitly checked with its feature flag. Logged preexisting ALSA pthread callback signature mismatch; no fix. Next client.

- Dormant linux_joystick.c has a generic Make object rule but is absent from linked object lists. Verify its real body using USE_SDL=0 CFLAGS=-DUSE_JOYSTICK; source equals base byte-for-byte. Added this explicit-feature C baseline object to nosdl-c.sha256 (298 standard +1 dormant). No feature enabled in supported builds.

- G2 named nested records: pahole -M alone omitted C++ download_s::func_s while C exposed func_s at file scope. Added native --show_private_classes to compare nested member records on both sides. Positive named-nested control and same-size member-swap negative control PASS; full gate selfcheck PASS; all 63 existing artifact pairs G2 PASS after this correction. No source type/layout change.

- Botlib: all 28 native source files and 34 headers assessed; strict release/debug client/ded, C SHA256 and G2/G3 PASS per file; G4 advisory diffs retained. Full default/ded C PASS. G8 passes retained casts/keyword rename (37 replaced lines across 8 files including final be_ai_move T1). Integrated C++ runtime remains blocked by qcommon/server; no full-engine equivalence claim.
- Unix/non-SDL baseline captured before any platform-source edits: 298 C objects, `SOURCE_DATE_EPOCH=1789257600 make -j20 USE_SDL=0 BUILD_DIR=/tmp/aftershock-cpp-port/oracle-nosdl`; manifest `tools/port/evidence/nosdl-c.sha256`. Non-SDL per-file checks use this oracle and pass USE_SDL=0 to compile_pair.py.

- Runtime reproducibility blocker confirmed: two executions of the same rebuilt C server with the exact requested q3dm17/two-bot/wait-300 arguments both exit 0 but differ in Item events, beyond timestamps/PIDs. Persistent diff: `tools/port/evidence/c-runtime-repeat.diff`. C seeds use time(NULL), Com_Milliseconds(), and GAME_INIT receives Com_Milliseconds() (common:5055, sv_init:529, sv_game:1061). Therefore the literal console-diff criterion is nondeterministic even before C++; no engine seed/timing behavior changed. C++ runtime still blocked by build failures; deterministic harness control remains needed for a meaningful comparison.
- Suppression check: same sanitizer baseline with `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=suppressions=$PWD/tools/port/ubsan.supp` exits 0 with no runtime-error or sanitizer diagnostic. Log: `/tmp/aftershock-cpp-port/runtime-sanitize-suppressed.log`. These two function-scoped alignment suppressions are effective on the tested C smoke.

- Server assessed: 9 native source files strict release/debug and G2/G3 PASS; sv_client.c, sv_rankings.c and tlds.h blocked. Full default C and dedicated C PASS (logs `/tmp/aftershock-cpp-port/server-{full,ded}-c.log`). G8 reviewer independently verified sv_game 199 T1 +7 T3 and sv_world 2 T3; all casts preserve original expressions. sv_game requires 116/1139 changed lines because almost every syscall passes untyped VM arguments; density is necessary at the ABI dispatch boundary, not cleanup. Module remains incomplete due blockers; proceeding botlib.

- Proxy follow-up: limit forced proxy rules to explicitly requested object goals via static patterns. A broad forced %.o rule triggered GNU Make implicit .d.o rebuild attempts when dependency files existed; normal default/ded C builds now PASS again. Strict completed-file matrix rechecked.

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

- `DEVIATION: freeze observed parenthesized-declarator warnings`: add -Wno-parentheses, 48 engine diagnostics observed in the original C++ probe (24 per client/ded be_ai_move.c). C accepts the unchanged macro declaration `bot_moveresult_t (x) = ...`; removing parentheses is outside T1-T17 and unnecessary to preserve behavior. G8 reviewer recommended the phase-0 baseline-warning policy already used for write-strings. C flags, source macro, ABI and expressions remain unchanged; hard conversion errors remain enabled. Reopen be_ai_move.c for its single T1 allocator cast.

- `DEVIATION: freeze observed legacy string-literal warnings`: add `-Wno-write-strings` (243 engine diagnostics in the original C++ probe) to the frozen list. Plan section 11 explicitly identifies this class as historical baseline noise tolerated by the C build. The original list mistakenly excluded it, which would force noncatalog signature changes in VM_Indent. Reviewer confirmed that freezing this observed class fits phase 0; the separate deviation documents changing an already-frozen list. No engine behavior, C flags, or -fpermissive policy changes. Other noncatalog hard errors remain blocked.

- `DEVIATION: preserve source style despite formatter threshold`: after 16 real formatter trials varying declaration alignment, array/cast spaces and operand alignment, best whole-file change counts are cvar.c 547/2141 (25.549%) and cl_main.c 914/5120 (17.852%). The requested <3% full-file threshold is unmet. Existing files mix tab alignment, braces and expression spacing. A global clang-format configuration cannot encode every surrounding line's style; disabling formatting to claim 0% would be a false pass. Keep the closest id-style configuration, use changed-line output only as an advisory review aid, and manually preserve surrounding style as the authoritative plan requires. No engine file was reformatted. Phase 0's threshold remains a documented limitation; all other harness work and subsequent independently verifiable source work continue unattended.

## Codegen differences

- `code/renderervk/tr_init.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_init.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_init.o /tmp/aftershock-cpp-port/renderervk-tr_init ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_init.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_init.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_init.o /tmp/aftershock-cpp-port/renderer-tr_init ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/vk_vbo.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-vk_vbo.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/vk_vbo.o /tmp/aftershock-cpp-port/renderervk-vk_vbo ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/vk_flares.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-vk_flares.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/vk_flares.o /tmp/aftershock-cpp-port/renderervk-vk_flares ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_world.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_world.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_world.o /tmp/aftershock-cpp-port/renderervk-tr_world ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_surface.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_surface.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_surface.o /tmp/aftershock-cpp-port/renderervk-tr_surface ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_sky.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_sky.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_sky.o /tmp/aftershock-cpp-port/renderervk-tr_sky ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_shadows.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_shadows.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_shadows.o /tmp/aftershock-cpp-port/renderervk-tr_shadows ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_shader.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_shader.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_shader.o /tmp/aftershock-cpp-port/renderervk-tr_shader ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_shade_calc.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_shade_calc.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_shade_calc.o /tmp/aftershock-cpp-port/renderervk-tr_shade_calc ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_shade.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_shade.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_shade.o /tmp/aftershock-cpp-port/renderervk-tr_shade ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_scene.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_scene.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_scene.o /tmp/aftershock-cpp-port/renderervk-tr_scene ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_model_iqm.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_model_iqm.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_model_iqm.o /tmp/aftershock-cpp-port/renderervk-tr_model_iqm ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_model.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_model.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_model.o /tmp/aftershock-cpp-port/renderervk-tr_model ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_mesh.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_mesh.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_mesh.o /tmp/aftershock-cpp-port/renderervk-tr_mesh ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_main.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_main.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_main.o /tmp/aftershock-cpp-port/renderervk-tr_main ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_light.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_light.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_light.o /tmp/aftershock-cpp-port/renderervk-tr_light ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_image.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_image.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_image.o /tmp/aftershock-cpp-port/renderervk-tr_image ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_curve.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_curve.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_curve.o /tmp/aftershock-cpp-port/renderervk-tr_curve ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_cmds.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_cmds.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_cmds.o /tmp/aftershock-cpp-port/renderervk-tr_cmds ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_bsp.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_bsp.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_bsp.o /tmp/aftershock-cpp-port/renderervk-tr_bsp ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_backend.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_backend.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_backend.o /tmp/aftershock-cpp-port/renderervk-tr_backend ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderervk/tr_animation.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderervk-tr_animation.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rendv/tr_animation.o /tmp/aftershock-cpp-port/renderervk-tr_animation ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_world.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_world.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_world.o /tmp/aftershock-cpp-port/renderer-tr_world ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_vbo.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_vbo.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_vbo.o /tmp/aftershock-cpp-port/renderer-tr_vbo ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_vbo.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_vbo.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_vbo.o /tmp/aftershock-cpp-port/renderer-tr_vbo ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_surface.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_surface.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_surface.o /tmp/aftershock-cpp-port/renderer-tr_surface ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_sky.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_sky.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_sky.o /tmp/aftershock-cpp-port/renderer-tr_sky ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_shadows.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_shadows.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_shadows.o /tmp/aftershock-cpp-port/renderer-tr_shadows ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_shader.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_shader.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_shader.o /tmp/aftershock-cpp-port/renderer-tr_shader ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_shade_calc.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_shade_calc.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_shade_calc.o /tmp/aftershock-cpp-port/renderer-tr_shade_calc ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_shade.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_shade.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_shade.o /tmp/aftershock-cpp-port/renderer-tr_shade ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_scene.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_scene.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_scene.o /tmp/aftershock-cpp-port/renderer-tr_scene ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_model_iqm.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_model_iqm.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_model_iqm.o /tmp/aftershock-cpp-port/renderer-tr_model_iqm ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_model.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_model.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_model.o /tmp/aftershock-cpp-port/renderer-tr_model ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_mesh.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_mesh.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_mesh.o /tmp/aftershock-cpp-port/renderer-tr_mesh ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_main.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_main.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_main.o /tmp/aftershock-cpp-port/renderer-tr_main ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_light.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_light.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_light.o /tmp/aftershock-cpp-port/renderer-tr_light ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_image.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_image.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_image.o /tmp/aftershock-cpp-port/renderer-tr_image ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_flares.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_flares.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_flares.o /tmp/aftershock-cpp-port/renderer-tr_flares ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_curve.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_curve.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_curve.o /tmp/aftershock-cpp-port/renderer-tr_curve ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_cmds.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_cmds.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_cmds.o /tmp/aftershock-cpp-port/renderer-tr_cmds ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_bsp.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_bsp.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_bsp.o /tmp/aftershock-cpp-port/renderer-tr_bsp ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_backend.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_backend.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_backend.o /tmp/aftershock-cpp-port/renderer-tr_backend ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_arb.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_arb.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_arb.o /tmp/aftershock-cpp-port/renderer-tr_arb ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_animation.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_animation.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_animation.o /tmp/aftershock-cpp-port/renderer-tr_animation ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderercommon/tr_noise.c`: G4 advisory FAIL; full diff `tools/port/evidence/tr_noise.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_noise.o /tmp/aftershock-cpp-port/tr_noise ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderercommon/tr_image_tga.c`: G4 advisory FAIL; full diff `tools/port/evidence/tr_image_tga.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_image_tga.o /tmp/aftershock-cpp-port/tr_image_tga ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/msg.c`: G4 advisory FAIL; full diff `tools/port/evidence/msg.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/msg.o /tmp/aftershock-cpp-port/msg ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/snd_mix.c`: G4 advisory FAIL; full diff `tools/port/evidence/snd_mix.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/snd_mix.o /tmp/aftershock-cpp-port/snd_mix ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/snd_mem.c`: G4 advisory FAIL; full diff `tools/port/evidence/snd_mem.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/snd_mem.o /tmp/aftershock-cpp-port/snd_mem ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/snd_dma.c`: G4 advisory FAIL; full diff `tools/port/evidence/snd_dma.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/snd_dma.o /tmp/aftershock-cpp-port/snd_dma ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/cl_ui.c`: G4 advisory FAIL; full diff `tools/port/evidence/cl_ui.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/cl_ui.o /tmp/aftershock-cpp-port/cl_ui ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/cl_scrn.c`: G4 advisory FAIL; full diff `tools/port/evidence/cl_scrn.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/cl_scrn.o /tmp/aftershock-cpp-port/cl_scrn ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/cl_parse.c`: G4 advisory FAIL; full diff `tools/port/evidence/cl_parse.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/cl_parse.o /tmp/aftershock-cpp-port/cl_parse ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/cl_net_chan.c`: G4 advisory FAIL; full diff `tools/port/evidence/cl_net_chan.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/cl_net_chan.o /tmp/aftershock-cpp-port/cl_net_chan ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/cl_keys.c`: G4 advisory FAIL; full diff `tools/port/evidence/cl_keys.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/cl_keys.o /tmp/aftershock-cpp-port/cl_keys ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/cl_jpeg.c`: G4 advisory FAIL; full diff `tools/port/evidence/cl_jpeg.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/cl_jpeg.o /tmp/aftershock-cpp-port/cl_jpeg ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/cl_input.c`: G4 advisory FAIL; full diff `tools/port/evidence/cl_input.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/cl_input.o /tmp/aftershock-cpp-port/cl_input ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/cl_console.c`: G4 advisory FAIL; full diff `tools/port/evidence/cl_console.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/cl_console.o /tmp/aftershock-cpp-port/cl_console ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/cl_cin.c`: G4 advisory FAIL; full diff `tools/port/evidence/cl_cin.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/cl_cin.o /tmp/aftershock-cpp-port/cl_cin ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/cl_cgame.c`: G4 advisory FAIL; full diff `tools/port/evidence/cl_cgame.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/cl_cgame.o /tmp/aftershock-cpp-port/cl_cgame ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/cl_avi.c`: G4 advisory FAIL; full diff `tools/port/evidence/cl_avi.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/cl_avi.o /tmp/aftershock-cpp-port/cl_avi ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/unix/unix_main.c`: G4 advisory FAIL; full diff `tools/port/evidence/unix_main.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/unix_main.o /tmp/aftershock-cpp-port/unix_main ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/unix/linux_snd.c`: G4 advisory FAIL; full diff `tools/port/evidence/linux_snd.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/linux_snd.o /tmp/aftershock-cpp-port/linux_snd USE_SDL=0` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/unix/linux_qgl.c`: G4 advisory FAIL; full diff `tools/port/evidence/linux_qgl.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/linux_qgl.o /tmp/aftershock-cpp-port/linux_qgl USE_SDL=0` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/unix/linux_joystick.c`: G4 advisory FAIL; full diff `tools/port/evidence/linux_joystick.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/linux_joystick.o /tmp/aftershock-cpp-port/linux_joystick USE_SDL=0 CFLAGS=-DUSE_JOYSTICK` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/unix/linux_glimp.c`: G4 advisory FAIL; full diff `tools/port/evidence/linux_glimp.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/linux_glimp.o /tmp/aftershock-cpp-port/linux_glimp USE_SDL=0` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/be_ai_move.c`: G4 advisory FAIL; full diff `tools/port/evidence/be_ai_move.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/be_ai_move.o /tmp/aftershock-cpp-port/be_ai_move` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/l_struct.c`: G4 advisory FAIL; full diff `tools/port/evidence/l_struct.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/l_struct.o /tmp/aftershock-cpp-port/l_struct` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/l_script.c`: G4 advisory FAIL; full diff `tools/port/evidence/l_script.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/l_script.o /tmp/aftershock-cpp-port/l_script` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/l_precomp.c`: G4 advisory FAIL; full diff `tools/port/evidence/l_precomp.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/l_precomp.o /tmp/aftershock-cpp-port/l_precomp` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/l_memory.c`: G4 advisory FAIL; full diff `tools/port/evidence/l_memory.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/l_memory.o /tmp/aftershock-cpp-port/l_memory` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/be_ai_chat.c`: G4 advisory FAIL; full diff `tools/port/evidence/be_ai_chat.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/be_ai_chat.o /tmp/aftershock-cpp-port/be_ai_chat` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/be_ai_char.c`: G4 advisory FAIL; full diff `tools/port/evidence/be_ai_char.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/be_ai_char.o /tmp/aftershock-cpp-port/be_ai_char` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/be_aas_sample.c`: G4 advisory FAIL; full diff `tools/port/evidence/be_aas_sample.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/be_aas_sample.o /tmp/aftershock-cpp-port/be_aas_sample` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/be_aas_routealt.c`: G4 advisory FAIL; full diff `tools/port/evidence/be_aas_routealt.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/be_aas_routealt.o /tmp/aftershock-cpp-port/be_aas_routealt` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/be_aas_route.c`: G4 advisory FAIL; full diff `tools/port/evidence/be_aas_route.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/be_aas_route.o /tmp/aftershock-cpp-port/be_aas_route` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/be_aas_reach.c`: G4 advisory FAIL; full diff `tools/port/evidence/be_aas_reach.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/be_aas_reach.o /tmp/aftershock-cpp-port/be_aas_reach` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/be_aas_move.c`: G4 advisory FAIL; full diff `tools/port/evidence/be_aas_move.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/be_aas_move.o /tmp/aftershock-cpp-port/be_aas_move` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/be_aas_file.c`: G4 advisory FAIL; full diff `tools/port/evidence/be_aas_file.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/be_aas_file.o /tmp/aftershock-cpp-port/be_aas_file` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/be_aas_debug.c`: G4 advisory FAIL; full diff `tools/port/evidence/be_aas_debug.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/be_aas_debug.o /tmp/aftershock-cpp-port/be_aas_debug` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/be_aas_cluster.c`: G4 advisory FAIL; full diff `tools/port/evidence/be_aas_cluster.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/be_aas_cluster.o /tmp/aftershock-cpp-port/be_aas_cluster` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/be_aas_bspq3.c`: G4 advisory FAIL; full diff `tools/port/evidence/be_aas_bspq3.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/be_aas_bspq3.o /tmp/aftershock-cpp-port/be_aas_bspq3` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

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

- `code/asm/qasm.h`: Unchanged assembly preprocessing header. Explicit gcc -m32 assembler check fails because 32-bit libc bits/wordsize.h is unavailable through q_platform.h/endian.h; no target gates or package installation.

- `code/win32/win_local.h`: Unverified (no MinGW/Windows SDK). Unchanged header inspected; no required catalog transformation identified, target consuming-object gates unavailable.

- `code/win32/resource.h`: Unverified (no MinGW/Windows SDK). Unchanged header inspected; no required catalog transformation identified, target consuming-object gates unavailable.

- `code/win32/glw_win.h`: Unverified (no MinGW/Windows SDK). Unchanged header inspected; no required catalog transformation identified, target consuming-object gates unavailable.

- `code/win32/win_wndproc.c`: Unverified (no MinGW/Windows SDK). T1: 2 clipboard-pointer casts, T2: 2 boolean-toggle casts by inspection; target gates unavailable.

- `code/win32/win_syscon.c`: Unverified (no MinGW/Windows SDK). T2: 1 boolean-toggle cast by inspection; target C/C++ builds and G1-G4 unavailable.

- `code/win32/win_snd.c`: Unverified (no MinGW/Windows SDK). T1: 2 casts around existing void* loader casts, WINAPI preserved. WASAPI explicitly uses C lpVtbl interfaces/REFIID pointer arguments; C++ SDK interface selection requires uncataloged changes. Target gates unavailable.

- `code/win32/win_shared.c`: Unverified (no MinGW/Windows SDK). No obvious catalog edits; unchanged. Optional USE_PROFILES calls FARPROC with arguments despite C++ zero-argument type, requiring an uncataloged function-pointer signature cast. Target gates unavailable.

- `code/win32/win_qvk.c`: Unverified (no MinGW/Windows SDK). T1: 3 library/proc casts by inspection. VK_GetInstanceProcAddr still returns a function pointer as void*, outside T1. Target gates unavailable.

- `code/win32/win_qgl.c`: Unverified (no MinGW/Windows SDK). T1: 2 source sites (library handle and proc macro with APIENTRY preserved). GL_GetProcAddress still assigns a function pointer to void*, outside T1. Target gates unavailable.

- `code/win32/win_minimize.c`: Unverified (no MinGW/Windows SDK). Inspection found no required catalog transformations in key-token table/parser; unchanged, target gates unavailable.

- `code/win32/win_main.c`: Unverified (no MinGW/Windows SDK). T1: 3 allocator/void-handle casts by inspection. Sys_LoadFunction still assigns FARPROC to void*, outside T1; C checksum and G1-G4 unavailable.

- `code/win32/win_input.c`: Unverified (no MinGW/Windows SDK). T2: 1 boolean return cast by inspection. DirectInput SDK interface macros and optional joystick/MIDI paths require target compile; C checksum and G1-G4 unavailable.

- `code/win32/win_glimp.c`: Unverified (no MinGW/Windows SDK). T1: 1 allocator cast, T2: 4 boolean-expression casts by inspection; C checksum and G1-G4 unavailable.

- `code/win32/win_gamma.c`: Unverified (no MinGW/Windows SDK). T1: 1 HANDLE/void-pointer to HMODULE argument cast by inspection; all gamma behavior unchanged. C checksum and G1-G4 unavailable.

- `code/renderervk/shaders/spirv/shader_data.c`: Unchanged generated initializer included by vk.c: 74 const arrays lose external linkage in C++; G3 FAIL, no existing extern declarations. See vk.c blocker.

- `code/renderervk/vk.c`: Catalog casts compile with unchanged C hash/G2 PASS, but G3 reports 74 generated const shader arrays changing external R to internal r. No existing extern declarations to move; adding new declarations is outside catalog. Attempt reverted. Evidence tools/port/evidence/vulkan-shader-linkage.diff.



- `code/renderercommon/vulkan/vulkan_win32.h`: Unverified: unchanged generated Khronos Windows header; no MinGW cross-compiler/Windows SDK available.

- `code/client/snd_codec_ogg.c`: G3 fails: const S_OGG_Callbacks changes external D to internal d in C++; no prior extern declaration exists. Restoring const-object external linkage is outside T1-T17. Reverted three T1 casts; source unchanged.

- `code/client/cl_main.c`: At :864 strrchr(const char*arg) assigned to read-only local char*ext_test; adding const is behavior-preserving but outside T8 literal string-constant scope (G8 reviewed). Remaining T1/T2/T3 diagnostics retained; source unchanged.

- `code/client/cl_curl.c`: At :967 strrchr(const char*localName) assigned to char*s. Local pointer is read-only and adding const would preserve behavior, but T8 is explicitly string-literal constness; this library-overload const propagation is outside literal catalog scope (G8 reviewed). 35 T1 sites also pending; source unchanged.

- `code/sdl/sdl_icon.h`: Unchanged image initializer; sole consuming translation unit sdl_glimp.c blocked, so complete-object C++ gates unavailable.

- `code/sdl/sdl_input.c`: Nested anonymous enum inside consoleKey_s (:119) scopes QUAKE_KEY/CHARACTER in C++; uses at :158/:163/:185/:190 no longer resolve. Qualifying names or restructuring the enum is outside T1-T17. Integer-to-keyNum_t T3 diagnostics also remain; source unchanged.

- `code/sdl/sdl_glimp.c`: At :763 returns PFN_vkVoidFunction as void*: function-pointer-to-object-pointer conversion is outside T1. Eight additional T1/T3 diagnostics remain; source unchanged.

- `code/unix/unix_shared.c`: At :22 _GNU_SOURCE is redefined: source defines it empty, g++ predefines it as 1. Adding an ifndef guard or changing macro value is outside T1-T17; diagnostic has no named -W class to freeze. T1 char** allocation at :236 also pending; source unchanged.


- `code/unix/linux_qvk.c`: At :76 returns PFN_vkVoidFunction (function pointer) as void*. Explicit function-pointer-to-object-pointer conversion is outside T1; source unchanged.


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
| `code/asm/qasm.h` | blocked | Unchanged assembly preprocessing header. Explicit gcc -m32 assembler check fails because 32-bit libc bits/wordsize.h is unavailable through q_platform.h/endian.h; no target gates or package installation. |
| `code/botlib/aasfile.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_bsp.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_bspq3.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_bspq3.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/be_aas_cluster.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_cluster.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/be_aas_cluster.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_debug.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_debug.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/be_aas_debug.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_def.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_entity.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_entity.o); G4 PASS. |
| `code/botlib/be_aas_entity.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_file.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_file.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/be_aas_file.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_funcs.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_main.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_main.o); G4 PASS. |
| `code/botlib/be_aas_main.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_move.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_move.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/be_aas_move.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_optimize.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_optimize.o); G4 PASS. |
| `code/botlib/be_aas_optimize.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_reach.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_reach.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/be_aas_reach.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_route.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_route.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/be_aas_route.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_routealt.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_routealt.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/be_aas_routealt.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_sample.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_sample.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/be_aas_sample.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_char.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_char.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/be_ai_char.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_char.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_chat.c` | done | T1: 4; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_chat.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/be_ai_chat.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_chat.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_gen.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_gen.o); G4 PASS. |
| `code/botlib/be_ai_gen.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_gen.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_goal.c` | done | T1: 1, T16: 1 macro site (8 expanded casts); clang narrowing follow-up; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_goal.o, default); G4 PASS. |
| `code/botlib/be_ai_goal.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_goal.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_move.c` | done | T1: 1; declaration macro retained under frozen warning policy; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_move.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/be_ai_move.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_goal.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_weap.c` | done | T1: 1, T16: 2 macro sites (36 expanded casts); clang narrowing follow-up; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_weap.o, default); G4 PASS. |
| `code/botlib/be_ai_weap.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_weap.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ai_weight.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_weight.o); G4 PASS. |
| `code/botlib/be_ai_weight.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_goal.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_ea.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ea.o); G4 PASS. |
| `code/botlib/be_ea.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_ai_chat.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_interface.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_interface.o); G4 PASS. |
| `code/botlib/be_interface.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_debug.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/botlib.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_crc.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_crc.o); G4 PASS. |
| `code/botlib/l_crc.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_route.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_libvar.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_libvar.o); G4 PASS. |
| `code/botlib/l_libvar.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_cluster.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_log.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_log.o); G4 PASS. |
| `code/botlib/l_log.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_cluster.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_memory.c` | done | T1: 2; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_memory.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/l_memory.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_precomp.c` | done | T1: 3 (one in inactive LoadSourceMemory), T4: 1 field (10 occurrences); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_precomp.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/l_precomp.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_script.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_script.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/l_script.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_struct.c` | done | T3: 14; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_struct.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/l_struct.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_utils.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_entity.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/cgame/cg_public.h` | done | T1-T17: 0; unchanged shared ABI header verified through cl_cgame.c actual dependency and native strict builds/G2/G3. |
| `code/client/cl_avi.c` | done | T1: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_avi.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_cgame.c` | done | T1: 116, T2: 1, T3: 6; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_cgame.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_cin.c` | done | T1: 2, T2: 5; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_cin.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_console.c` | done | T1: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_console.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_curl.c` | blocked | At :967 strrchr(const char*localName) assigned to char*s. Local pointer is read-only and adding const would preserve behavior, but T8 is explicitly string-literal constness; this library-overload const propagation is outside literal catalog scope (G8 reviewed). 35 T1 sites also pending; source unchanged. |
| `code/client/cl_curl.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/cl_input.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_input.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_jpeg.c` | done | T1: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_jpeg.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_keys.c` | done | T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_keys.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_main.c` | blocked | At :864 strrchr(const char*arg) assigned to read-only local char*ext_test; adding const is behavior-preserving but outside T8 literal string-constant scope (G8 reviewed). Remaining T1/T2/T3 diagnostics retained; source unchanged. |
| `code/client/cl_net_chan.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_net_chan.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_parse.c` | done | T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_parse.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_scrn.c` | done | T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_scrn.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_ui.c` | done | T1: 80, T2: 1, T3: 8; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_ui.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/client.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/keycodes.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/keys.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/snd_adpcm.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_adpcm.o, default); G4 PASS. |
| `code/client/snd_codec.c` | done | T1: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_codec.o, default); G4 PASS. |
| `code/client/snd_codec.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/snd_codec.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/snd_codec_ogg.c` | blocked | G3 fails: const S_OGG_Callbacks changes external D to internal d in C++; no prior extern declaration exists. Restoring const-object external linkage is outside T1-T17. Reverted three T1 casts; source unchanged. |
| `code/client/snd_codec_wav.c` | done | T1: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_codec_wav.o, default); G4 PASS. |
| `code/client/snd_dma.c` | done | T1: 1, T15: 2 (non-SDL branch); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_dma.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/snd_local.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/snd_main.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_main.o, default); G4 PASS. |
| `code/client/snd_mem.c` | done | T1: 4; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_mem.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/snd_mix.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_mix.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/snd_public.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/snd_wavelet.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_wavelet.o, default); G4 PASS. |
| `code/game/bg_public.h` | done | T1-T17: 0; unchanged shared ABI header verified through sv_game.c actual dependency and native strict builds/G2/G3. |
| `code/game/g_public.h` | done | T1-T17: 0; unchanged shared ABI header verified through sv_game.c actual dependency and native strict builds/G2/G3. |
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
| `code/qcommon/msg.c` | done | T16: 3 sites (1 mask, 99 expanded field-offset casts); clang narrowing follow-up; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/msg.o, default); G4 advisory FAIL, full diff retained. |
| `code/qcommon/net_chan.c` | done | T1: 1, T4: 1 identifier (12 occurrences); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/net_chan.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/net_ip.c` | done | T1: 2, T2: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/net_ip.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/puff.c` | done | T1-T17: 0 (already compatible); 3 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/puff.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/puff.h` | done | T1-T17: 0; unchanged header checked via puff.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/q_math.c` | blocked | G2/G3 PASS, but G5 fixed-input hashes differ in RotatePointAroundVector and vectoangles; AngleVectors chain differs too. C++ float overloads change results; double-argument casts are outside T1-T17. No source changes. |
| `code/qcommon/q_platform.h` | done | T1-T17: 0; unchanged header checked via md4.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/q_shared.c` | done | T1: 4, T2: 3; 4 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/q_shared.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/q_shared.h` | done | T5: guarded Q_EXTERN_C macro plus 2 Windows assembly prototypes; native md4 consumer G2/G3 PASS and all 295 C hashes unchanged; Windows branch unverified. |
| `code/qcommon/qcommon.h` | done | T5: 3 native DLL function typedefs and 2 assembly FPU prototypes; vm_interpreted consumer C hash unchanged, strict C++/G2/G3 PASS. 32-bit FPU branch unverified. |
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
| `code/renderer/iqm.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderer/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderer/qgl.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderer/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderer/tr_animation.c` | done | T1: 3, T2: 1, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_animation.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_arb.c` | done | T4: 3 occurrences (prerequisite), T2: 5, T3: 4, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_arb.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_backend.c` | done | T4: 9 occurrences (prerequisite), T1: 4, T2: 2, T3: 3, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_backend.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_bsp.c` | done | T1: 42, T3: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_bsp.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_cmds.c` | done | T1: 11; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_cmds.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_common.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderer/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderer/tr_curve.c` | done | T1: 3; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_curve.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_flares.c` | done | T4: 3 occurrences (prerequisite), T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_flares.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_image.c` | done | T1: 9, T2: 2, T3: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_init.c` | done | T1: 10, T2: 1, T3: 1, T5: 2 conditional definitions; phase-2 GetRefAPI blocker resolved; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_init.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_light.c` | done | T4: 10 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_light.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_local.h` | done | T4: 5 occurrences (prerequisite); actual consumer code/renderer/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderer/tr_main.c` | done | T4: 113 occurrences (prerequisite), T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_main.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_marks.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_marks.o, default); G4 PASS. |
| `code/renderer/tr_mesh.c` | done | T4: 4 occurrences (prerequisite), T1: 4, T2: 1, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_mesh.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_model.c` | done | T1: 5; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_model.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_model_iqm.c` | done | T1: 4, T2: 2, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_model_iqm.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_scene.c` | done | T4: 4 occurrences (prerequisite), T1: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_scene.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_shade.c` | done | T4: 1 occurrence (prerequisite), T3: 1, T17: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_shade.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_shade_calc.c` | done | T4: 50 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_shade_calc.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_shader.c` | done | T1: 5, T3: 16, T16: 1, T17: 9; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_shader.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_shadows.c` | done | T4: 4 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_shadows.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_sky.c` | done | T4: 7 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_sky.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_surface.c` | done | T4: 25 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_surface.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_vbo.c` | done | T4: 3 occurrences (prerequisite), T1: 7, T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_vbo.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_world.c` | done | T4: 2 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_world.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderercommon/tr_font.c` | done | T1: 1; dormant BUILD_FREETYPE body unverified (missing dependency); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_font.o, default); G4 PASS. |
| `code/renderercommon/tr_image_bmp.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image_bmp.o, default); G4 PASS. |
| `code/renderercommon/tr_image_jpg.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image_jpg.o, default); G4 PASS. |
| `code/renderercommon/tr_image_pcx.c` | done | T1: 2; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image_pcx.o, default); G4 PASS. |
| `code/renderercommon/tr_image_png.c` | done | T1: 18; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image_png.o, default); G4 PASS. |
| `code/renderercommon/tr_image_tga.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image_tga.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderercommon/tr_noise.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_noise.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderercommon/tr_public.h` | done | T5: GetRefAPI typedef and static-build prototype; tr_marks consumer C hash unchanged and strict C++/G2/G3 PASS. |
| `code/renderercommon/tr_types.h` | done | T1-T17: 0; unchanged header verified through code/renderercommon/tr_font.c, strict native builds/G2/G3 PASS. |
| `code/renderercommon/vulkan/vk_platform.h` | done | T1-T17: 0; unchanged generated Khronos header; C and strict C++20 Xlib/Xrandr header fixture G2/G3 PASS. |
| `code/renderercommon/vulkan/vulkan.h` | done | T1-T17: 0; unchanged generated Khronos header; C and strict C++20 Xlib/Xrandr header fixture G2/G3 PASS. |
| `code/renderercommon/vulkan/vulkan_core.h` | done | T1-T17: 0; unchanged generated Khronos header; C and strict C++20 Xlib/Xrandr header fixture G2/G3 PASS. |
| `code/renderercommon/vulkan/vulkan_win32.h` | blocked | Unverified: unchanged generated Khronos Windows header; no MinGW cross-compiler/Windows SDK available. |
| `code/renderercommon/vulkan/vulkan_xlib.h` | done | T1-T17: 0; unchanged generated Khronos header; C and strict C++20 Xlib/Xrandr header fixture G2/G3 PASS. |
| `code/renderercommon/vulkan/vulkan_xlib_xrandr.h` | done | T1-T17: 0; unchanged generated Khronos header; C and strict C++20 Xlib/Xrandr header fixture G2/G3 PASS. |
| `code/renderervk/iqm.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderervk/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderervk/shaders/bin2hex.c` | done | Unchanged standalone build utility, not linked engine code; gcc/g++ strict native -O2 compile PASS, fixed 259-byte input and append output byte-identical. Engine layout gate not applicable (no engine records). |
| `code/renderervk/shaders/spirv/shader_data.c` | blocked | Unchanged generated initializer included by vk.c: 74 const arrays lose external linkage in C++; G3 FAIL, no existing extern declarations. See vk.c blocker. |
| `code/renderervk/tr_animation.c` | done | T1: 3, T2: 1, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_animation.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_backend.c` | done | T4: 11 occurrences (prerequisite), T1: 4, T3: 3, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_backend.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_bsp.c` | done | T1: 42, T3: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_bsp.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_cmds.c` | done | T1: 6; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_cmds.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_common.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderervk/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderervk/tr_curve.c` | done | T1: 3; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_curve.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_image.c` | done | T1: 8, T2: 1, T3: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_image.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_init.c` | done | T1: 5, T5: 2 conditional definitions; phase-2 GetRefAPI blocker resolved; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_init.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_light.c` | done | T4: 10 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_light.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_local.h` | done | T4: 5 occurrences (prerequisite); actual consumer code/renderervk/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderervk/tr_main.c` | done | T4: 113 occurrences (prerequisite), T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_main.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_marks.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_marks.o, default); G4 PASS. |
| `code/renderervk/tr_mesh.c` | done | T4: 4 occurrences (prerequisite), T1: 4, T2: 1, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_mesh.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_model.c` | done | T1: 5; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_model.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_model_iqm.c` | done | T1: 4, T2: 2, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_model_iqm.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_scene.c` | done | T4: 4 occurrences (prerequisite), T1: 2, T3: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_scene.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_shade.c` | done | T4: 4 occurrences (prerequisite), T2: 1, T17: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_shade.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_shade_calc.c` | done | T4: 50 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_shade_calc.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_shader.c` | done | T1: 5, T3: 17, T16: 1, T17: 9; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_shader.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_shadows.c` | done | T4: 4 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_shadows.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_sky.c` | done | T4: 7 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_sky.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_surface.c` | done | T4: 25 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_surface.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/tr_world.c` | done | T4: 2 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/tr_world.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/vk.c` | blocked | Catalog casts compile with unchanged C hash/G2 PASS, but G3 reports 74 generated const shader arrays changing external R to internal r. No existing extern declarations to move; adding new declarations is outside catalog. Attempt reverted. Evidence tools/port/evidence/vulkan-shader-linkage.diff. |
| `code/renderervk/vk.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderervk/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderervk/vk_flares.c` | done | T4: 3 occurrences (prerequisite); no further transformations; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/vk_flares.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderervk/vk_vbo.c` | done | T1: 6; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rendv/vk_vbo.o, default); G4 advisory FAIL, full diff retained. |
| `code/sdl/sdl_gamma.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/sdl_gamma.o, default); G4 PASS. |
| `code/sdl/sdl_glimp.c` | blocked | At :763 returns PFN_vkVoidFunction as void*: function-pointer-to-object-pointer conversion is outside T1. Eight additional T1/T3 diagnostics remain; source unchanged. |
| `code/sdl/sdl_glw.h` | done | T1-T17: 0; unchanged header verified through sdl_gamma.c; native strict builds and G2/G3 PASS. |
| `code/sdl/sdl_icon.h` | blocked | Unchanged image initializer; sole consuming translation unit sdl_glimp.c blocked, so complete-object C++ gates unavailable. |
| `code/sdl/sdl_input.c` | blocked | Nested anonymous enum inside consoleKey_s (:119) scopes QUAKE_KEY/CHARACTER in C++; uses at :158/:163/:185/:190 no longer resolve. Qualifying names or restructuring the enum is outside T1-T17. Integer-to-keyNum_t T3 diagnostics also remain; source unchanged. |
| `code/sdl/sdl_snd.c` | done | T1: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/sdl_snd.o, default); G4 PASS. |
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
| `code/ui/ui_public.h` | done | T1-T17: 0; unchanged shared ABI header verified through cl_ui.c actual dependency and native strict builds/G2/G3. |
| `code/unix/linux_glimp.c` | done | T1: 2 sites (4 expanded casts), T2: 3, T3: 2, T4: 1 identifier (3 occurrences); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/linux_glimp.o, nosdl); G4 advisory FAIL, full diff retained. |
| `code/unix/linux_joystick.c` | done | T1-T17: 0; dormant USE_JOYSTICK body explicitly compiled; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/linux_joystick.o, nosdl); G4 advisory FAIL, full diff retained. |
| `code/unix/linux_local.h` | done | T1-T17: 0; unchanged header verified through unix_main.c and linux_glimp.c; native strict builds and G2/G3 PASS. |
| `code/unix/linux_qgl.c` | done | T1: 1 macro site (6 expanded casts); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/linux_qgl.o, nosdl); G4 advisory FAIL, full diff retained. |
| `code/unix/linux_qvk.c` | blocked | At :76 returns PFN_vkVoidFunction (function pointer) as void*. Explicit function-pointer-to-object-pointer conversion is outside T1; source unchanged. |
| `code/unix/linux_signals.c` | done | T1-T17: 0; renderer header T4 prerequisite resolves prior blocker; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/linux_signals.o, default); G4 PASS. |
| `code/unix/linux_snd.c` | done | T1: 5; original thread-function casts retained inside typed casts; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/linux_snd.o, nosdl); G4 advisory FAIL, full diff retained. |
| `code/unix/unix_glw.h` | done | T1-T17: 0; unchanged header verified through non-SDL linux_glimp.c, linux_qgl.c and X11 extensions; G2/G3 PASS. |
| `code/unix/unix_main.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/unix_main.o, default); G4 advisory FAIL, full diff retained. |
| `code/unix/unix_shared.c` | blocked | At :22 _GNU_SOURCE is redefined: source defines it empty, g++ predefines it as 1. Adding an ifndef guard or changing macro value is outside T1-T17; diagnostic has no named -W class to freeze. T1 char** allocation at :236 also pending; source unchanged. |
| `code/unix/x11_dga.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/x11_dga.o, nosdl); G4 PASS. |
| `code/unix/x11_randr.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/x11_randr.o, nosdl); G4 PASS. |
| `code/unix/x11_vidmode.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/x11_vidmode.o, nosdl); G4 PASS. |
| `code/win32/glw_win.h` | blocked | Unverified (no MinGW/Windows SDK). Unchanged header inspected; no required catalog transformation identified, target consuming-object gates unavailable. |
| `code/win32/resource.h` | blocked | Unverified (no MinGW/Windows SDK). Unchanged header inspected; no required catalog transformation identified, target consuming-object gates unavailable. |
| `code/win32/win_gamma.c` | blocked | Unverified (no MinGW/Windows SDK). T1: 1 HANDLE/void-pointer to HMODULE argument cast by inspection; all gamma behavior unchanged. C checksum and G1-G4 unavailable. |
| `code/win32/win_glimp.c` | blocked | Unverified (no MinGW/Windows SDK). T1: 1 allocator cast, T2: 4 boolean-expression casts by inspection; C checksum and G1-G4 unavailable. |
| `code/win32/win_input.c` | blocked | Unverified (no MinGW/Windows SDK). T2: 1 boolean return cast by inspection. DirectInput SDK interface macros and optional joystick/MIDI paths require target compile; C checksum and G1-G4 unavailable. |
| `code/win32/win_local.h` | blocked | Unverified (no MinGW/Windows SDK). Unchanged header inspected; no required catalog transformation identified, target consuming-object gates unavailable. |
| `code/win32/win_main.c` | blocked | Unverified (no MinGW/Windows SDK). T1: 3 allocator/void-handle casts by inspection. Sys_LoadFunction still assigns FARPROC to void*, outside T1; C checksum and G1-G4 unavailable. |
| `code/win32/win_minimize.c` | blocked | Unverified (no MinGW/Windows SDK). Inspection found no required catalog transformations in key-token table/parser; unchanged, target gates unavailable. |
| `code/win32/win_qgl.c` | blocked | Unverified (no MinGW/Windows SDK). T1: 2 source sites (library handle and proc macro with APIENTRY preserved). GL_GetProcAddress still assigns a function pointer to void*, outside T1. Target gates unavailable. |
| `code/win32/win_qvk.c` | blocked | Unverified (no MinGW/Windows SDK). T1: 3 library/proc casts by inspection. VK_GetInstanceProcAddr still returns a function pointer as void*, outside T1. Target gates unavailable. |
| `code/win32/win_shared.c` | blocked | Unverified (no MinGW/Windows SDK). No obvious catalog edits; unchanged. Optional USE_PROFILES calls FARPROC with arguments despite C++ zero-argument type, requiring an uncataloged function-pointer signature cast. Target gates unavailable. |
| `code/win32/win_snd.c` | blocked | Unverified (no MinGW/Windows SDK). T1: 2 casts around existing void* loader casts, WINAPI preserved. WASAPI explicitly uses C lpVtbl interfaces/REFIID pointer arguments; C++ SDK interface selection requires uncataloged changes. Target gates unavailable. |
| `code/win32/win_syscon.c` | blocked | Unverified (no MinGW/Windows SDK). T2: 1 boolean-toggle cast by inspection; target C/C++ builds and G1-G4 unavailable. |
| `code/win32/win_wndproc.c` | blocked | Unverified (no MinGW/Windows SDK). T1: 2 clipboard-pointer casts, T2: 2 boolean-toggle casts by inspection; target gates unavailable. |
