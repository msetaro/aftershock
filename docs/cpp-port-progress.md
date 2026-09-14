# C++20 port checkpoint

Base: `8a7e8ed2`; branch: `t3code/port-engine-to-cpp20`.

The port has resumed under the accepted T1–T23 catalog and revised gates. Prior blockers below are historical diagnoses pending revalidation, not unresolved requests for guidance. `q_math.c` now passes C hashes, strict C++, G2/G3 and G4 after T21. Remaining work: tree-wide math conversions, native blockers, full links/runtime, cross-target verification, T5 review, rename and CI. No files have been renamed. The old aggregate results below are retained until the replacement sweep completes.

## Next action

Continuation: renderer; next `code/renderer/tr_main.c` under amended T1-T23. Preserve completed transformations; continue native math, remaining native blockers, cross targets, T5 review, then rename only after native gates/runtime pass.

## Phase checklist

- [ ] Phase 0 fully satisfies all criteria: all six artifacts exist, checksums and gate controls pass; formatter is now advisory; per-file whitespace checks are enforced.
- [ ] Phase 1 qcommon: assessed, seven blocked entries.
- [ ] Phase 1 server: assessed, three blocked entries.
- [x] Phase 1 botlib: native per-file strict builds/G2/G3 and C builds pass; integrated runtime blocked elsewhere.
- [ ] Phase 1 unix + sdl: assessed, five blocked entries.
- [ ] Phase 1 client: assessed, three blocked entries.
- [ ] Phase 1 renderercommon: native consumers pass; Windows Vulkan header unverified.
- [x] Phase 1 renderer: all native objects pass; C++ OpenGL shared renderer builds and loads.
- [ ] Phase 1 renderervk: assessed; vk.c/generated shader data blocked by const linkage.
- [ ] Phase 1 win32: all files inspected; all target verification unavailable.
- [ ] Phase 2 complete: available native boundaries pass; foreign-target verification and removal of unnecessary internal callback linkage remain.
- [ ] Phase 3: intentionally not started because prerequisites fail.
- [x] Final available G1-G8 checks executed and evidence recorded; failures are not presented as passes.

## Module inventory

Counts include headers and the standalone Vulkan shader utility. “Done” is per-file/native-context status, not a claim that the module or complete engine runs as C++.

| Module | Done | Blocked |
|---|---:|---:|
| asm | 0 | 1 |
| botlib | 62 | 0 |
| cgame | 1 | 0 |
| client | 26 | 3 |
| game | 2 | 0 |
| qcommon | 35 | 7 |
| renderer | 28 | 0 |
| renderercommon | 14 | 1 |
| renderervk | 28 | 2 |
| sdl | 3 | 3 |
| server | 10 | 3 |
| ui | 1 | 0 |
| unix | 11 | 2 |
| win32 | 0 | 14 |

## Verification results

| Check | Final result | Persistent evidence |
|---|---|---|
| C oracle | Full client/renderers and dedicated PASS; 295/295 original object SHA256 hashes unchanged | `tools/port/evidence/phase0-c.sha256`, `final-c-checksums.log` |
| G1/G7 compiler matrix | Eight C configurations PASS; eight C++ configurations FAIL in blocked files | `tools/port/evidence/build-matrix-results.json`, `build-matrix.log`, all 16 `*.build.log.gz` |
| G2/G3 current completed files | 143 native TUs PASS; one standalone utility separately verified | `tools/port/evidence/completed-gates.json`, `final-native-gates.log.gz` |
| Gate controls | Positive and negative controls PASS, including nested layout, changed static linkage, missing export/assembly linkage and optimized JIT clone | `tools/port/evidence/final-selfcheck.log` |
| G4 | 33 completed-source PASS; 110 advisory differences retained; blocked q_math also differs | Complete list below |
| G5 | 12 groups match; vector math FAIL | `tools/port/evidence/g5-differential.log` |
| G6 C smoke | Final requested q3dm17/two-bot/wait-300 run exits 0 | `tools/port/evidence/final-runtime-c.log.gz` |
| G6 C++ runtime | Blocked by full builds; no equivalent-runtime claim | Blocked list and compiler matrix |
| C sanitizer smoke | ASan/UBSan exits 0 with no diagnostics using documented alignment suppressions; leak detection disabled | `tools/port/evidence/final-runtime-sanitize.log.gz`, `tools/port/ubsan.supp` |
| G7 clang-tidy | 99 changed native sources checked, zero tool/compile failures, **236 narrowing findings**; nine Windows sources skipped | `tools/port/evidence/static-summary.txt`, `static-warnings.json`, `clang-tidy.log.gz` |
| G8 | Reviewer found no uncatalog engine-source hunks; final harness findings fixed and regression checked | Decisions and per-file transformation counts below |

C matrix coverage: GCC release/debug with SDL and without SDL, plus static OpenGL and Vulkan; Clang release SDL and debug non-SDL. Default dlopen configurations build client, dedicated and both renderers. Each has a C++ counterpart. C++ compiler-error counts are GCC 143/104/143/104/95/130 and Clang 105/77 respectively (diagnostics include repeated contexts; Clang stops after its error limit). Failed linking includes const-linkage and semantic blockers beyond these compiler counts.

The G5 test uses one GCC-compiled C driver for both object sets. Test-only objcopy aliases normalize unambiguous global symbol spelling without changing visibility, instructions or data; the production G3 gate is separate. Allocation/log stubs and a linker-wrapped read isolate the tested behavior. This is not a test of the engine memory allocator or full filesystem/runtime. It covers COM_ParseExt, Info, Q_str/va/Com_sprintf, command tokenization, cvars, FS paths, basic MSG roundtrip, 256 usercmd/entity/playerstate delta roundtrips, adaptive Huffman, numeric/loopback NET_StringToAdr, 1,000 CM_BoxTrace sweeps with both collisions and misses, and Q_rsqrt/Q_fabs. Those twelve groups match. Vector math hashes are C `5c00b4de`, C++ `31ee774f`: unchanged C++ math headers choose float overloads instead of C double promotion. The original standalone math probe also records per-function differences.

Collision data comes from the installed user-owned `~/.q3a/baseq3/pak0.pk3` member `maps/q3dm17.bsp`, extracted only under /tmp. BSP SHA256: `ee1394417b06d92f705088150d7609d6b9f55f5796a9a7626bfe7982e8fc94e8`. No archive or game data is committed. This uses existing licensed data instead of bundling a BSP.

The exact bot-smoke console-diff criterion is nondeterministic even for two runs of the same C binary: Item events differ, beyond timestamps/PIDs. Evidence: `tools/port/evidence/c-runtime-repeat.diff`. Existing seeds use time(NULL), Com_Milliseconds(), and the GAME_INIT time argument (common.c, sv_init.c, sv_game.c). No seed or timing behavior changed. Full C++ runtime/snapshot comparison remains blocked by compilation; client timedemo/image checks also lack a display/Xvfb.

## Reproduction commands

Run from this worktree on native Linux x86_64. No system packages were installed. GCC 15.2, Clang 21, pahole, clang-format and clang-tidy are present. MinGW, ARM/PPC cross toolchains/sysroots, 32-bit libc headers, FreeType development headers, and Xvfb/display are unavailable.

```sh
port_repo=$PWD
export SOURCE_DATE_EPOCH=1789257600
make -j20 BUILD_DIR=/tmp/aftershock-cpp-port/oracle
make -j20 BUILD_CLIENT=0 BUILD_DIR=/tmp/aftershock-cpp-port/oracle
(cd /tmp/aftershock-cpp-port/oracle && sha256sum -c "$port_repo/tools/port/evidence/phase0-c.sha256")

tools/port/selfcheck.sh
python3 tools/port/completed_gates.py /tmp/aftershock-cpp-port/final-gates
python3 tools/port/build_matrix.py /tmp/aftershock-cpp-port/matrix
python3 tools/port/static_gate.py /tmp/aftershock-cpp-port/static
python3 tools/port/differential_gate.py /tmp/aftershock-cpp-port/differential
tools/port/math_gate.sh
```

Expected status: selfcheck/completed gates/static execution return 0 (static warnings remain); build_matrix/differential/math return 1 while the listed blockers remain. Matrix runs allocate fresh build roots; exact commands from the recorded first fresh run are retained in build-matrix-results.json. Completed gates always rebuild their current source pairs. Single-object mode:

```sh
make BUILD_CXX=1 BUILD_DIR=/tmp/aftershock-cpp-port/single /tmp/aftershock-cpp-port/single/release-linux-x86_64/ded/md4.o
python3 tools/port/compile_pair.py ded/md4.o /tmp/aftershock-cpp-port/md4
# Likewise ded/q_math.o; q_math layout/symbol pass but codegen/math differ.
tools/port/layout_gate.sh /tmp/aftershock-cpp-port/md4/md4.c.o /tmp/aftershock-cpp-port/md4/md4.cxx.o
tools/port/symbol_gate.sh /tmp/aftershock-cpp-port/md4/md4.c.o /tmp/aftershock-cpp-port/md4/md4.cxx.o
tools/port/codegen_gate.sh /tmp/aftershock-cpp-port/md4/md4.c.s /tmp/aftershock-cpp-port/md4/md4.cxx.s
```

Renderer loading (overall make exits 2 from unrelated blockers, but the OpenGL shared object builds):

```sh
SOURCE_DATE_EPOCH=1789257600 make -k -j20 BUILD_CXX=1 BUILD_DIR=/tmp/aftershock-cpp-port/strict
python3 -c 'import ctypes; lib=ctypes.CDLL("/tmp/aftershock-cpp-port/strict/release-linux-x86_64/quake3e_opengl_x86_64.so"); print(bool(lib.GetRefAPI))'
```

`True` confirms RTLD_NOW loading and GetRefAPI resolution; this is not graphical initialization or an integrated C++ client run.

Runtime and sanitizer reproduction:

```sh
timeout 60 /tmp/aftershock-cpp-port/oracle/release-linux-x86_64/quake3e.ded.x64 +set dedicated 1 +set sv_pure 0 +set com_logfile 0 +map q3dm17 +addbot sarge 3 +addbot major 3 +wait 300 +quit
SOURCE_DATE_EPOCH=1789257600 make -j20 BUILD_CLIENT=0 BUILD_DIR=/tmp/aftershock-cpp-port/sanitize CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' LDFLAGS='-fsanitize=address,undefined -lm -ldl'
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=suppressions=$PWD/tools/port/ubsan.supp timeout 60 /tmp/aftershock-cpp-port/sanitize/release-linux-x86_64/quake3e.ded.x64 +set dedicated 1 +set sv_pure 0 +set com_logfile 0 +map q3dm17 +addbot sarge 3 +addbot major 3 +wait 300 +quit
```

Both final runtime commands returned 0; the suppressed sanitizer run emitted no diagnostics. Without suppressions, the C baseline reported the intentional unaligned unzip/VM loads recorded in the bug notes.

## Decisions and harness details

- Revised G3 uses separately emitted *.sym.o objects at -O2 -fno-builtin -D__NO_CTYPE=1 -U_FORTIFY_SOURCE, identically for C/C++. Reviewer measured these controls against all 33 failures: nine libc/header implementation differences disappear; 24 math differences remain. Each control is needed on this host. Float sin/sinf negative control still fails, pinned-double and integer-sin positive controls pass. Production C hashes, G2 objects, G4 assembly and G5 remain unchanged. No undefined symbols are ignored. Cross-target applicability will be tested with target gates.

- Continuation G3 now compares all undefined references after demangling; raw name requirements follow clarified T5. Reran all 143 previously completed pairs: 33 failures, with full diffs in tools/port/evidence/expanded-symbols-initial.json. These include expected float math references and independently observed libc header/optimizer substitutions; neither is silently ignored. A G3-only probe configuration is being evaluated to compare source linkage without optimizer-created libc call differences; G4/G5 will keep the production optimization settings.

- Read AGENTS.md and the full plan before port work. GNU Make only: plan section 7 supersedes early CMake references, and section 9 defers vcxproj lists to rename. Existing t3code worktree/branch used; per-file commits and module pushes, no main push, force push, history rewrite, source rename, vendor or renderer2 port.
- All 257 engine .c/.h outside the excluded vendor/renderer2 directories are in the table; final filesystem inventory found zero missing/stale rows. There are no bg implementation files in this checkout, only the four shared cgame/game/ui headers listed.
- Make BUILD_CXX=1 uses C++20, no exceptions/RTTI and no permissive mode, filters C-only flags, keeps vendored C and assembly unchanged, and skips renderer2 even for direct objects. Explicit object proxies support make -k and current source dependencies. The initial proxy freshness bug was corrected and independently revalidated with a forced C rebuild: all 295 original hashes matched; final full rebuild confirms them again. SOURCE_DATE_EPOCH preserves __DATE__/__TIME__ checksums without source edits.
- Frozen flags: -Wall -Wextra -Werror, with only observed -Wno-sign-compare, -Wno-unused-parameter, -Wno-missing-field-initializers, -Wno-implicit-fallthrough, -Wno-ignored-qualifiers, -Wno-type-limits, -Wno-write-strings and -Wno-parentheses. Original engine diagnostic counts (C/C++, repeated object contexts included): sign-compare 544/532, unused-parameter 171/173, missing-field-initializers 100/595, implicit-fallthrough 39/39, ignored-qualifiers 4/4, type-limits 2/2. C++ write-strings 243 and parentheses 48 are the two explicit later frozen-list deviations. No -fpermissive enabled.
- G2 requires real engine DWARF records, compares sizes/members/offsets/alignment with pahole -a -A -I -M --show_private_classes, and preserves nested/union layouts. Compiler/system/vendor records are excluded by declaration provenance, not by intersecting record sets. G3 preserves symbol-kind multisets and raw enumerated ABI names, including undefined assembly references and optimized JIT clones; local static/compiler numbering is normalized. The GNU private CPUID_EX helper is distinct from the MSVC external assembly entry. All normalization has positive/negative controls.
- T2 consistently casts the original Boolean expression to qboolean. T3 compound enum operations cast the original promoted result without changing offsets/operands or evaluating expressions twice. T8 is limited to literal constness; library-overload const propagation remains blocked. No floating-point expression was restructured.
- Shared renderer keyword changes require atomic T4 prerequisites to keep C valid: OpenGL 243 code-token occurrences across its header and 14 sources; Vulkan 242 across 13 files. Comments/strings unchanged, all consumers updated, 295 original C hashes reconfirmed after each prerequisite. Per-file work then resumed. No speculative cleanup.
- Windows inspection retained 14 T1 + 8 T2 casts across nine files, plus T5 GPU exports; G8 removed one redundant same-type cast. All Windows entries remain blocked/unverified without MinGW/SDK. Optional FreeType body is unchanged/unverified without its dev headers; native default tr_font is verified. 32-bit qasm preprocessing fails through endian.h because bits/wordsize.h is unavailable.
- Phase 2 Q_EXTERN_C is empty in C, extern "C" in C++. Function typedefs/prototypes and GetRefAPI definitions are annotated; guarded linkage blocks preserve definitions and static storage for sound globals and native/JIT callbacks. Enumerated Windows assembly prototypes are retained but target-unverified. Never use Q_EXTERN_C static, which is invalid C++; never turn a global definition into a mere extern declaration.
- G8 reviewed the final 113-file engine diff (1,310 insertions / 1,274 deletions, about 0.5% of engine C/header lines). Every retained hunk maps to T1-T17; no behavior or FP restructuring found. The reviewer identified a gate clone-name blind spot and analysis-reporting ambiguity; both were fixed and verified. All 143 native symbol pairs were rerun after the strengthened gate, with zero failures.

## Deviations

- `DEVIATION: freeze observed parenthesized-declarator warnings`: add -Wno-parentheses, 48 engine diagnostics observed in the original C++ probe (24 per client/ded be_ai_move.c). C accepts the unchanged macro declaration `bot_moveresult_t (x) = ...`; removing parentheses is outside T1-T17 and unnecessary to preserve behavior. G8 reviewer recommended the phase-0 baseline-warning policy already used for write-strings. C flags, source macro, ABI and expressions remain unchanged; hard conversion errors remain enabled. be_ai_move.c subsequently passed its per-file gates.

- `DEVIATION: freeze observed legacy string-literal warnings`: add `-Wno-write-strings` (243 engine diagnostics in the original C++ probe) to the frozen list. Plan section 11 explicitly identifies this class as historical baseline noise tolerated by the C build. The original list mistakenly excluded it, which would force noncatalog signature changes in VM_Indent. Reviewer confirmed that freezing this observed class fits phase 0; the separate deviation documents changing an already-frozen list. No engine behavior, C flags, or -fpermissive policy changes. Other noncatalog hard errors remain blocked.

- `DEVIATION: preserve source style despite formatter threshold`: after 16 real formatter trials varying declaration alignment, array/cast spaces and operand alignment, best whole-file change counts are cvar.c 547/2141 (25.549%) and cl_main.c 914/5120 (17.852%). The requested <3% full-file threshold is unmet. Existing files mix tab alignment, braces and expression spacing. A global clang-format configuration cannot encode every surrounding line's style; disabling formatting to claim 0% would be a false pass. Keep the closest id-style configuration, use changed-line output only as an advisory review aid, and manually preserve surrounding style as the authoritative plan requires. No engine file was reformatted. Phase 0's threshold remains a documented limitation; all other harness work and subsequent independently verifiable source work continue unattended.


## Codegen differences

- `code/renderer/tr_light.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_light.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_light.o /tmp/aftershock-cpp-port/renderer-tr_light ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_flares.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_flares.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_flares.o /tmp/aftershock-cpp-port/renderer-tr_flares ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_bsp.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_bsp.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_bsp.o /tmp/aftershock-cpp-port/renderer-tr_bsp ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderer/tr_arb.c`: G4 advisory FAIL; full diff `tools/port/evidence/renderer-tr_arb.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_arb.o /tmp/aftershock-cpp-port/renderer-tr_arb ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/renderercommon/tr_noise.c`: G4 advisory FAIL; full diff `tools/port/evidence/tr_noise.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py rend1/tr_noise.o /tmp/aftershock-cpp-port/tr_noise ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/cl_ui.c`: G4 advisory FAIL; full diff `tools/port/evidence/cl_ui.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/cl_ui.o /tmp/aftershock-cpp-port/cl_ui ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/cl_input.c`: G4 advisory FAIL; full diff `tools/port/evidence/cl_input.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/cl_input.o /tmp/aftershock-cpp-port/cl_input ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/cl_cgame.c`: G4 advisory FAIL; full diff `tools/port/evidence/cl_cgame.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/cl_cgame.o /tmp/aftershock-cpp-port/cl_cgame ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/client/cl_avi.c`: G4 advisory FAIL; full diff `tools/port/evidence/cl_avi.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py client/cl_avi.o /tmp/aftershock-cpp-port/cl_avi ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/l_precomp.c`: G4 advisory FAIL; full diff `tools/port/evidence/l_precomp.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/l_precomp.o /tmp/aftershock-cpp-port/l_precomp ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/be_ai_move.c`: G4 advisory FAIL; full diff `tools/port/evidence/be_ai_move.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/be_ai_move.o /tmp/aftershock-cpp-port/be_ai_move ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/botlib/be_aas_reach.c`: G4 advisory FAIL; full diff `tools/port/evidence/be_aas_reach.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/be_aas_reach.o /tmp/aftershock-cpp-port/be_aas_reach ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/server/sv_game.c`: G4 advisory FAIL; full diff `tools/port/evidence/sv_game.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/sv_game.o /tmp/aftershock-cpp-port/sv_game ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/cvar.c`: G4 advisory FAIL; full diff `tools/port/evidence/cvar.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/cvar.o /tmp/aftershock-cpp-port/cvar ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/cm_trace.c`: G4 advisory FAIL; full diff `tools/port/evidence/cm_trace.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/cm_trace.o /tmp/aftershock-cpp-port/cm_trace ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

- `code/qcommon/cm_patch.c`: G4 advisory FAIL; full diff `tools/port/evidence/cm_patch.codegen.diff.gz` (`gzip -dc`). C objects unchanged, G2/G3 PASS. Reproduce: `python3 tools/port/compile_pair.py ded/cm_patch.o /tmp/aftershock-cpp-port/cm_patch ` then the three gate entry points on emitted objects/assembly. Diff requires human review; no identical C++ behavior claim.

Final current-header sweep: 143 compiled native TUs pass G2/G3; 33 G4 PASS, 110 advisory differences. The standalone bin2hex utility is tested separately. G4 differences require human review and do not establish C++ behavioral equivalence. In addition, blocked q_math.c has its complete 730-line phase-0 diff in `tools/port/evidence/q_math.codegen.diff`; G5 proves a behavioral difference there.

Reproduce all current completed sources with `python3 tools/port/completed_gates.py /tmp/aftershock-cpp-port/final-gates` (expected exit 0 for compilation/G2/G3; G4 remains advisory). It always rebuilds pairs from current sources using the actual recorded Make contexts. Per-file object/variant and statuses: `tools/port/evidence/completed-gates.json`. For one file: `python3 tools/port/compile_pair.py OBJECT OUTPUT [MAKE_VARIABLE=value ...]`, then `tools/port/{layout,symbol}_gate.sh OUTPUT/STEM.c.o OUTPUT/STEM.cxx.o` and `tools/port/codegen_gate.sh OUTPUT/STEM.c.s OUTPUT/STEM.cxx.s`.

Every nonempty completed-source G4 diff is listed below. Read each with `gzip -dc PATH`.

| Source | Complete advisory diff |
|---|---|
| `code/botlib/be_aas_bspq3.c` | `tools/port/evidence/be_aas_bspq3.codegen.diff.gz` |
| `code/botlib/be_aas_cluster.c` | `tools/port/evidence/be_aas_cluster.codegen.diff.gz` |
| `code/botlib/be_aas_debug.c` | `tools/port/evidence/be_aas_debug.codegen.diff.gz` |
| `code/botlib/be_aas_file.c` | `tools/port/evidence/be_aas_file.codegen.diff.gz` |
| `code/botlib/be_aas_move.c` | done | T1-T17: 0 (already compatible); T21/T22: 3 argument casts at 3 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_move.o, default); G4 PASS. |
| `code/botlib/be_aas_reach.c` | done | T1-T17: 0 (already compatible); T21/T22: 14 argument casts at 14 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_reach.o, default); G4 advisory FAIL, full diff retained. |
| `code/botlib/be_aas_route.c` | `tools/port/evidence/be_aas_route.codegen.diff.gz` |
| `code/botlib/be_aas_routealt.c` | `tools/port/evidence/be_aas_routealt.codegen.diff.gz` |
| `code/botlib/be_aas_sample.c` | `tools/port/evidence/be_aas_sample.codegen.diff.gz` |
| `code/botlib/be_ai_char.c` | `tools/port/evidence/be_ai_char.codegen.diff.gz` |
| `code/botlib/be_ai_chat.c` | `tools/port/evidence/be_ai_chat.codegen.diff.gz` |
| `code/botlib/be_ai_move.c` | done | T1: 1; T21/T22: 12 argument casts at 12 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_move.o, default); G4 advisory FAIL, full diff retained. |
| `code/botlib/l_memory.c` | `tools/port/evidence/l_memory.codegen.diff.gz` |
| `code/botlib/l_precomp.c` | `tools/port/evidence/l_precomp.codegen.diff.gz` |
| `code/botlib/l_script.c` | `tools/port/evidence/l_script.codegen.diff.gz` |
| `code/botlib/l_struct.c` | `tools/port/evidence/l_struct.codegen.diff.gz` |
| `code/client/cl_avi.c` | `tools/port/evidence/cl_avi.codegen.diff.gz` |
| `code/client/cl_cgame.c` | `tools/port/evidence/cl_cgame.codegen.diff.gz` |
| `code/client/cl_cin.c` | `tools/port/evidence/cl_cin.codegen.diff.gz` |
| `code/client/cl_console.c` | `tools/port/evidence/cl_console.codegen.diff.gz` |
| `code/client/cl_input.c` | `tools/port/evidence/cl_input.codegen.diff.gz` |
| `code/client/cl_jpeg.c` | `tools/port/evidence/cl_jpeg.codegen.diff.gz` |
| `code/client/cl_keys.c` | `tools/port/evidence/cl_keys.codegen.diff.gz` |
| `code/client/cl_net_chan.c` | `tools/port/evidence/cl_net_chan.codegen.diff.gz` |
| `code/client/cl_parse.c` | `tools/port/evidence/cl_parse.codegen.diff.gz` |
| `code/client/cl_scrn.c` | `tools/port/evidence/cl_scrn.codegen.diff.gz` |
| `code/client/cl_ui.c` | `tools/port/evidence/cl_ui.codegen.diff.gz` |
| `code/client/snd_dma.c` | `tools/port/evidence/snd_dma.codegen.diff.gz` |
| `code/client/snd_mem.c` | `tools/port/evidence/snd_mem.codegen.diff.gz` |
| `code/client/snd_mix.c` | `tools/port/evidence/snd_mix.codegen.diff.gz` |
| `code/qcommon/cm_load.c` | `tools/port/evidence/cm_load.codegen.diff.gz` |
| `code/qcommon/cm_patch.c` | done | T1: 3, T2: 6, T3: 3; T21/T22: 14 argument casts at 14 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_patch.o, default); G4 advisory FAIL, full diff retained. |
| `code/qcommon/cm_trace.c` | done | T1-T17: 0 (already compatible); T21/T22: 6 argument casts at 6 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_trace.o, default); G4 advisory FAIL, full diff retained. |
| `code/qcommon/cmd.c` | `tools/port/evidence/cmd.codegen.diff.gz` |
| `code/qcommon/common.c` | `tools/port/evidence/common.codegen.diff.gz` |
| `code/qcommon/cvar.c` | done | T2: 2, T3: 4 (cast compound-assignment result); T21/T22: 2 argument casts at 2 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cvar.o, default); G4 advisory FAIL, full diff retained. |
| `code/qcommon/files.c` | `tools/port/evidence/files.codegen.diff.gz` |
| `code/qcommon/history.c` | `tools/port/evidence/history.codegen.diff.gz` |
| `code/qcommon/huffman.c` | `tools/port/evidence/huffman.codegen.diff.gz` |
| `code/qcommon/keys.c` | `tools/port/evidence/keys.codegen.diff.gz` |
| `code/qcommon/md5.c` | `tools/port/evidence/md5.codegen.diff.gz` |
| `code/qcommon/msg.c` | `tools/port/evidence/msg.codegen.diff.gz` |
| `code/qcommon/net_chan.c` | `tools/port/evidence/net_chan.codegen.diff.gz` |
| `code/qcommon/net_ip.c` | `tools/port/evidence/net_ip.codegen.diff.gz` |
| `code/qcommon/puff.c` | `tools/port/evidence/puff.codegen.diff.gz` |
| `code/qcommon/q_shared.c` | `tools/port/evidence/q_shared.codegen.diff.gz` |
| `code/qcommon/unzip.c` | `tools/port/evidence/unzip.codegen.diff.gz` |
| `code/qcommon/vm.c` | `tools/port/evidence/vm.codegen.diff.gz` |
| `code/qcommon/vm_interpreted.c` | `tools/port/evidence/vm_interpreted.codegen.diff.gz` |
| `code/renderer/tr_animation.c` | `tools/port/evidence/renderer-tr_animation.codegen.diff.gz` |
| `code/renderer/tr_arb.c` | `tools/port/evidence/renderer-tr_arb.codegen.diff.gz` |
| `code/renderer/tr_backend.c` | `tools/port/evidence/renderer-tr_backend.codegen.diff.gz` |
| `code/renderer/tr_bsp.c` | `tools/port/evidence/renderer-tr_bsp.codegen.diff.gz` |
| `code/renderer/tr_cmds.c` | `tools/port/evidence/renderer-tr_cmds.codegen.diff.gz` |
| `code/renderer/tr_curve.c` | `tools/port/evidence/renderer-tr_curve.codegen.diff.gz` |
| `code/renderer/tr_flares.c` | `tools/port/evidence/renderer-tr_flares.codegen.diff.gz` |
| `code/renderer/tr_image.c` | `tools/port/evidence/renderer-tr_image.codegen.diff.gz` |
| `code/renderer/tr_init.c` | `tools/port/evidence/renderer-tr_init.codegen.diff.gz` |
| `code/renderer/tr_light.c` | `tools/port/evidence/renderer-tr_light.codegen.diff.gz` |
| `code/renderer/tr_main.c` | `tools/port/evidence/renderer-tr_main.codegen.diff.gz` |
| `code/renderer/tr_mesh.c` | `tools/port/evidence/renderer-tr_mesh.codegen.diff.gz` |
| `code/renderer/tr_model.c` | `tools/port/evidence/renderer-tr_model.codegen.diff.gz` |
| `code/renderer/tr_model_iqm.c` | `tools/port/evidence/renderer-tr_model_iqm.codegen.diff.gz` |
| `code/renderer/tr_scene.c` | `tools/port/evidence/renderer-tr_scene.codegen.diff.gz` |
| `code/renderer/tr_shade.c` | `tools/port/evidence/renderer-tr_shade.codegen.diff.gz` |
| `code/renderer/tr_shade_calc.c` | `tools/port/evidence/renderer-tr_shade_calc.codegen.diff.gz` |
| `code/renderer/tr_shader.c` | `tools/port/evidence/renderer-tr_shader.codegen.diff.gz` |
| `code/renderer/tr_shadows.c` | `tools/port/evidence/renderer-tr_shadows.codegen.diff.gz` |
| `code/renderer/tr_sky.c` | `tools/port/evidence/renderer-tr_sky.codegen.diff.gz` |
| `code/renderer/tr_surface.c` | `tools/port/evidence/renderer-tr_surface.codegen.diff.gz` |
| `code/renderer/tr_vbo.c` | `tools/port/evidence/renderer-tr_vbo.codegen.diff.gz` |
| `code/renderer/tr_world.c` | `tools/port/evidence/renderer-tr_world.codegen.diff.gz` |
| `code/renderercommon/tr_image_tga.c` | `tools/port/evidence/tr_image_tga.codegen.diff.gz` |
| `code/renderercommon/tr_noise.c` | `tools/port/evidence/tr_noise.codegen.diff.gz` |
| `code/renderervk/tr_animation.c` | `tools/port/evidence/renderervk-tr_animation.codegen.diff.gz` |
| `code/renderervk/tr_backend.c` | `tools/port/evidence/renderervk-tr_backend.codegen.diff.gz` |
| `code/renderervk/tr_bsp.c` | `tools/port/evidence/renderervk-tr_bsp.codegen.diff.gz` |
| `code/renderervk/tr_cmds.c` | `tools/port/evidence/renderervk-tr_cmds.codegen.diff.gz` |
| `code/renderervk/tr_curve.c` | `tools/port/evidence/renderervk-tr_curve.codegen.diff.gz` |
| `code/renderervk/tr_image.c` | `tools/port/evidence/renderervk-tr_image.codegen.diff.gz` |
| `code/renderervk/tr_init.c` | `tools/port/evidence/renderervk-tr_init.codegen.diff.gz` |
| `code/renderervk/tr_light.c` | `tools/port/evidence/renderervk-tr_light.codegen.diff.gz` |
| `code/renderervk/tr_main.c` | `tools/port/evidence/renderervk-tr_main.codegen.diff.gz` |
| `code/renderervk/tr_mesh.c` | `tools/port/evidence/renderervk-tr_mesh.codegen.diff.gz` |
| `code/renderervk/tr_model.c` | `tools/port/evidence/renderervk-tr_model.codegen.diff.gz` |
| `code/renderervk/tr_model_iqm.c` | `tools/port/evidence/renderervk-tr_model_iqm.codegen.diff.gz` |
| `code/renderervk/tr_scene.c` | `tools/port/evidence/renderervk-tr_scene.codegen.diff.gz` |
| `code/renderervk/tr_shade.c` | `tools/port/evidence/renderervk-tr_shade.codegen.diff.gz` |
| `code/renderervk/tr_shade_calc.c` | `tools/port/evidence/renderervk-tr_shade_calc.codegen.diff.gz` |
| `code/renderervk/tr_shader.c` | `tools/port/evidence/renderervk-tr_shader.codegen.diff.gz` |
| `code/renderervk/tr_shadows.c` | `tools/port/evidence/renderervk-tr_shadows.codegen.diff.gz` |
| `code/renderervk/tr_sky.c` | `tools/port/evidence/renderervk-tr_sky.codegen.diff.gz` |
| `code/renderervk/tr_surface.c` | `tools/port/evidence/renderervk-tr_surface.codegen.diff.gz` |
| `code/renderervk/tr_world.c` | `tools/port/evidence/renderervk-tr_world.codegen.diff.gz` |
| `code/renderervk/vk_flares.c` | `tools/port/evidence/renderervk-vk_flares.codegen.diff.gz` |
| `code/renderervk/vk_vbo.c` | `tools/port/evidence/renderervk-vk_vbo.codegen.diff.gz` |
| `code/server/sv_bot.c` | `tools/port/evidence/sv_bot.codegen.diff.gz` |
| `code/server/sv_ccmds.c` | `tools/port/evidence/sv_ccmds.codegen.diff.gz` |
| `code/server/sv_filter.c` | `tools/port/evidence/sv_filter.codegen.diff.gz` |
| `code/server/sv_game.c` | done | T1: 199, T3: 7; T21/T22: 7 argument casts at 6 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_game.o, default); G4 advisory FAIL, full diff retained. |
| `code/server/sv_init.c` | `tools/port/evidence/sv_init.codegen.diff.gz` |
| `code/server/sv_main.c` | `tools/port/evidence/sv_main.codegen.diff.gz` |
| `code/server/sv_net_chan.c` | `tools/port/evidence/sv_net_chan.codegen.diff.gz` |
| `code/server/sv_snapshot.c` | `tools/port/evidence/sv_snapshot.codegen.diff.gz` |
| `code/server/sv_world.c` | `tools/port/evidence/sv_world.codegen.diff.gz` |
| `code/unix/linux_glimp.c` | `tools/port/evidence/linux_glimp.codegen.diff.gz` |
| `code/unix/linux_joystick.c` | `tools/port/evidence/linux_joystick.codegen.diff.gz` |
| `code/unix/linux_qgl.c` | `tools/port/evidence/linux_qgl.codegen.diff.gz` |
| `code/unix/linux_snd.c` | `tools/port/evidence/linux_snd.codegen.diff.gz` |
| `code/unix/unix_main.c` | `tools/port/evidence/unix_main.codegen.diff.gz` |

## Blocked files

These are the exact unresolved file statuses, including missing verification. Codegen differences alone are advisory and are not used to mark otherwise completed files blocked. The additional non-file blocker is the formatter threshold documented above.

- `code/asm/qasm.h`: Unchanged assembly preprocessing header. Explicit gcc -m32 assembler check fails because 32-bit libc bits/wordsize.h is unavailable through q_platform.h/endian.h; no target gates or package installation.
- `code/client/cl_curl.c`: At :967 strrchr(const char*localName) assigned to char*s. Local pointer is read-only and adding const would preserve behavior, but T8 is explicitly string-literal constness; this library-overload const propagation is outside literal catalog scope (G8 reviewed). 35 T1 sites also pending; source unchanged.
- `code/client/cl_main.c`: At :864 strrchr(const char*arg) assigned to read-only local char*ext_test; adding const is behavior-preserving but outside T8 literal string-constant scope (G8 reviewed). Remaining T1/T2/T3 diagnostics retained; source unchanged.
- `code/client/snd_codec_ogg.c`: G3 fails: const S_OGG_Callbacks changes external D to internal d in C++; no prior extern declaration exists. Restoring const-object external linkage is outside T1-T17. Reverted three T1 casts; source unchanged.
- `code/qcommon/huffman_static.c`: Compiles unchanged, but G3: HuffmanDecoderTable changes R (external) to r (static) under C++. Restoring extern const linkage is outside T1-T17; no source edit applied.
- `code/qcommon/q_math.c`: G2/G3 PASS, but G5 fixed-input hashes differ in RotatePointAroundVector and vectoangles; AngleVectors chain differs too. C++ float overloads change results; double-argument casts are outside T1-T17. No source changes.
- `code/qcommon/vm_aarch64.c`: Native syntax probe: :1583/:2290 need T1; :2348 __clear_cache undeclared. No aarch64 cross compiler/sysroot verified; cannot prove C object equivalence or target gates. Unmodified, target unverified.
- `code/qcommon/vm_armv7l.c`: Native syntax probe: :1223/:1931 need T1; :1978 __clear_cache undeclared; eight host-width overflow diagnostics from 32-bit instruction constants. No ARM cross compiler/sysroot verified; unmodified, target unverified.
- `code/qcommon/vm_optimize.h`: Unchanged; all consuming JIT translation units are blocked/unverified, so complete-object gates cannot verify this header.
- `code/qcommon/vm_powerpc.c`: Native syntax probe: :1727/:2579 need T1; :2558/:2561/:2564/:2567 pass function pointers to const void*, outside T1. No PPC64 cross compiler/sysroot verified; unmodified, target unverified.
- `code/qcommon/vm_x86.c`: At :3498 mov_rx_ptr(R_SYSCALL, vm->systemCall) converts syscall_t function pointer to const void*. T1 covers the reverse direction only; no catalog remedy. A T3 cast is also needed at :4323. No source edit retained.
- `code/renderercommon/vulkan/vulkan_win32.h`: Unverified: unchanged generated Khronos Windows header; no MinGW cross-compiler/Windows SDK available.
- `code/renderervk/shaders/spirv/shader_data.c`: Unchanged generated initializer included by vk.c: 74 const arrays lose external linkage in C++; G3 FAIL, no existing extern declarations. See vk.c blocker.
- `code/renderervk/vk.c`: Catalog casts compile with unchanged C hash/G2 PASS, but G3 reports 74 generated const shader arrays changing external R to internal r. No existing extern declarations to move; adding new declarations is outside catalog. Attempt reverted. Evidence tools/port/evidence/vulkan-shader-linkage.diff.
- `code/sdl/sdl_glimp.c`: At :763 returns PFN_vkVoidFunction as void*: function-pointer-to-object-pointer conversion is outside T1. Eight additional T1/T3 diagnostics remain; source unchanged.
- `code/sdl/sdl_icon.h`: Unchanged image initializer; sole consuming translation unit sdl_glimp.c blocked, so complete-object C++ gates unavailable.
- `code/sdl/sdl_input.c`: Nested anonymous enum inside consoleKey_s (:119) scopes QUAKE_KEY/CHARACTER in C++; uses at :158/:163/:185/:190 no longer resolve. Qualifying names or restructuring the enum is outside T1-T17. Integer-to-keyNum_t T3 diagnostics also remain; source unchanged.
- `code/server/sv_client.c`: At :402 C++ strstr(const char*, ...) returns const char*, assigned to writable str then sprintf writes through it. Removing const from cmd or casting away const is outside T8 (which permits adding const for literal pointers); source retained unchanged. Seven other T1/T2/T3 diagnostics remain.
- `code/server/sv_rankings.c`: At :25 missing rankings/1.0/gr/grapi.h SDK (legacy backslash include); SDK absent and source has no Makefile object rule. Cannot compile either C oracle or C++ or run gates; left unchanged.
- `code/server/tlds.h`: Unchanged initializer fragment; sole consumer sv_client.c is blocked, so complete-object C++/G2/G3 verification is unavailable.
- `code/unix/linux_qvk.c`: At :76 returns PFN_vkVoidFunction (function pointer) as void*. Explicit function-pointer-to-object-pointer conversion is outside T1; source unchanged.
- `code/unix/unix_shared.c`: At :22 _GNU_SOURCE is redefined: source defines it empty, g++ predefines it as 1. Adding an ifndef guard or changing macro value is outside T1-T17; diagnostic has no named -W class to freeze. T1 char** allocation at :236 also pending; source unchanged.
- `code/win32/glw_win.h`: Unverified (no MinGW/Windows SDK). Unchanged header inspected; no required catalog transformation identified, target consuming-object gates unavailable.
- `code/win32/resource.h`: Unverified (no MinGW/Windows SDK). Unchanged header inspected; no required catalog transformation identified, target consuming-object gates unavailable.
- `code/win32/win_gamma.c`: Unverified (no MinGW/Windows SDK). T1: 1 HANDLE/void-pointer to HMODULE argument cast by inspection; all gamma behavior unchanged. C checksum and G1-G4 unavailable.
- `code/win32/win_glimp.c`: Unverified (no MinGW/Windows SDK). T1: 1, T2: 4; T5: conditional C-linkage block for 2 GPU exports, preserves definitions/initializers. Target checksum/G1-G4 unavailable.
- `code/win32/win_input.c`: Unverified (no MinGW/Windows SDK). T2: 1 boolean return cast by inspection. DirectInput SDK interface macros and optional joystick/MIDI paths require target compile; C checksum and G1-G4 unavailable.
- `code/win32/win_local.h`: Unverified (no MinGW/Windows SDK). Unchanged header inspected; no required catalog transformation identified, target consuming-object gates unavailable.
- `code/win32/win_main.c`: Unverified (no MinGW/Windows SDK). T1: 3 allocator/void-handle casts by inspection. Sys_LoadFunction still assigns FARPROC to void*, outside T1; C checksum and G1-G4 unavailable.
- `code/win32/win_minimize.c`: Unverified (no MinGW/Windows SDK). Inspection found no required catalog transformations in key-token table/parser; unchanged, target gates unavailable.
- `code/win32/win_qgl.c`: Unverified (no MinGW/Windows SDK). T1: 2 source sites (library handle and proc macro with APIENTRY preserved). GL_GetProcAddress still assigns a function pointer to void*, outside T1. Target gates unavailable.
- `code/win32/win_qvk.c`: Unverified (no MinGW/Windows SDK). T1: 3 library/proc casts by inspection. VK_GetInstanceProcAddr still returns a function pointer as void*, outside T1. Target gates unavailable.
- `code/win32/win_shared.c`: Unverified (no MinGW/Windows SDK). No obvious catalog edits; unchanged. Optional USE_PROFILES calls FARPROC with arguments despite C++ zero-argument type, requiring an uncataloged function-pointer signature cast. Target gates unavailable.
- `code/win32/win_snd.c`: Unverified (no MinGW/Windows SDK). T1: 2 casts around existing void* loader casts, WINAPI preserved. WASAPI explicitly uses C lpVtbl interfaces/REFIID pointer arguments; C++ SDK interface selection requires uncataloged changes. Target gates unavailable.
- `code/win32/win_syscon.c`: Unverified (no MinGW/Windows SDK). T2: 1 boolean-toggle cast by inspection; target C/C++ builds and G1-G4 unavailable.
- `code/win32/win_wndproc.c`: Unverified (no MinGW/Windows SDK). T1: 2 clipboard-pointer casts, T2: 2 boolean-toggle casts by inspection; target gates unavailable.

## Bugs and compatibility hazards recorded, not fixed

Full notes: `docs/cpp-port-notes.md`.

- Existing CMake defects; GNU Make remains the supported build.
- Unchanged q_math C++ float overloads alter numerical results (confirmed G5 blocker).
- Intentional unaligned unzip.c/VM file-buffer loads; narrow sanitizer suppressions verified.
- Same-C bot runs differ due existing time-dependent seeds, limiting literal runtime log comparison.
- linux_snd thread callback signature mismatch is preserved; casts retain the original conversion.
- cl_curl slash test indexes the URL terminator and therefore always appends in that branch.
- FS_AllowedExtension compares a possible NULL strrchr result relationally before its NULL check.

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
| `code/botlib/be_aas_move.c` | done | T1-T17: 0 (already compatible); T21/T22: 3 argument casts at 3 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_move.o, default); G4 PASS. |
| `code/botlib/be_aas_move.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_optimize.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_optimize.o); G4 PASS. |
| `code/botlib/be_aas_optimize.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/be_aas_reach.c` | done | T1-T17: 0 (already compatible); T21/T22: 14 argument casts at 14 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_aas_reach.o, default); G4 advisory FAIL, full diff retained. |
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
| `code/botlib/be_ai_move.c` | done | T1: 1; T21/T22: 12 argument casts at 12 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/be_ai_move.o, default); G4 advisory FAIL, full diff retained. |
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
| `code/botlib/l_precomp.c` | done | T1: 3 (one in inactive LoadSourceMemory), T4: 1 field (10 occurrences); T21/T22: 3 argument casts at 3 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_precomp.o, default); G4 advisory FAIL, full diff retained. |
| `code/botlib/l_precomp.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_script.c` | done | T1-T17: 0 (already compatible); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_script.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/l_script.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_struct.c` | done | T3: 14; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/l_struct.o); G4 advisory FAIL, full diff retained. |
| `code/botlib/l_struct.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_bspq3.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/botlib/l_utils.h` | done | T1-T17: 0; unchanged header verified in actual preprocessor dependencies of code/botlib/be_aas_entity.c; native strict C++ and G2/G3 PASS. Inactive BSPC/MEMDEBUG branches are outside native matrix. |
| `code/cgame/cg_public.h` | done | T1-T17: 0; unchanged shared ABI header verified through cl_cgame.c actual dependency and native strict builds/G2/G3. |
| `code/client/cl_avi.c` | done | T1: 2; T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_avi.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_cgame.c` | done | T1: 116, T2: 1, T3: 6; T21/T22: 7 argument casts at 6 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_cgame.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_cin.c` | done | T1: 2, T2: 5; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_cin.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_console.c` | done | T1: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_console.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_curl.c` | blocked | At :967 strrchr(const char*localName) assigned to char*s. Local pointer is read-only and adding const would preserve behavior, but T8 is explicitly string-literal constness; this library-overload const propagation is outside literal catalog scope (G8 reviewed). 35 T1 sites also pending; source unchanged. |
| `code/client/cl_curl.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/cl_input.c` | done | T1-T17: 0 (already compatible); T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_input.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_jpeg.c` | done | T1: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_jpeg.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_keys.c` | done | T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_keys.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_main.c` | blocked | At :864 strrchr(const char*arg) assigned to read-only local char*ext_test; adding const is behavior-preserving but outside T8 literal string-constant scope (G8 reviewed). Remaining T1/T2/T3 diagnostics retained; source unchanged. |
| `code/client/cl_net_chan.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_net_chan.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_parse.c` | done | T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_parse.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_scrn.c` | done | T2: 1; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_scrn.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/cl_ui.c` | done | T1: 80, T2: 1, T3: 8; T21/T22: 7 argument casts at 6 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/cl_ui.o, default); G4 advisory FAIL, full diff retained. |
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
| `code/client/snd_mix.c` | done | T5: C linkage block for 3 assembly globals plus 5 assembly declarations/conditional definitions; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_mix.o, default); G4 advisory FAIL, full diff retained. |
| `code/client/snd_public.h` | done | T1-T17: 0; unchanged header in actual dependencies of code/client/cl_avi.c; native GCC strict builds and G2/G3 PASS. |
| `code/client/snd_wavelet.c` | done | T1-T17: 0 (already compatible); 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (client/snd_wavelet.o, default); G4 PASS. |
| `code/game/bg_public.h` | done | T1-T17: 0; unchanged shared ABI header verified through sv_game.c actual dependency and native strict builds/G2/G3. |
| `code/game/g_public.h` | done | T1-T17: 0; unchanged shared ABI header verified through sv_game.c actual dependency and native strict builds/G2/G3. |
| `code/qcommon/cm_load.c` | done | T1: 28; native ded C SHA256 unchanged (df42e0cabff475c22ebf8383e443cfe34d558abf817baf6f5cd8ad0c96700017); strict C++/G2/G3 PASS (ded/cm_load.o, default); G4 advisory diff retained. |
| `code/qcommon/cm_local.h` | done | T1-T17: 0; unchanged header checked via cm_load.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_patch.c` | done | T1: 3, T2: 6, T3: 3; T21/T22: 14 argument casts at 14 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_patch.o, default); G4 advisory FAIL, full diff retained. |
| `code/qcommon/cm_patch.h` | done | T1-T17: 0; unchanged header checked via cm_patch.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_polylib.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_polylib.o); G4 PASS. |
| `code/qcommon/cm_polylib.h` | done | T1-T17: 0; unchanged header checked via cm_polylib.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_public.h` | done | T1-T17: 0; unchanged header checked via cm_load.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/cm_test.c` | done | T1: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_test.o); G4 PASS. |
| `code/qcommon/cm_trace.c` | done | T1-T17: 0 (already compatible); T21/T22: 6 argument casts at 6 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cm_trace.o, default); G4 advisory FAIL, full diff retained. |
| `code/qcommon/cmd.c` | done | T1: 1, T2: 1; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cmd.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/common.c` | done | T1: 5, T2: 2, T3: 1, T15: 29; T5: 2 conditional MSVC CPUID_EX declarations/definitions (target-unverified); 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/common.o, default); G4 advisory FAIL, full diff retained. |
| `code/qcommon/cvar.c` | done | T2: 2, T3: 4 (cast compound-assignment result); T21/T22: 2 argument casts at 2 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/cvar.o, default); G4 advisory FAIL, full diff retained. |
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
| `code/qcommon/q_math.c` | done | T21: 20 argument casts at 18 calls, redundant cast correction; 4 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/q_math.o, default); G4 PASS. |
| `code/qcommon/q_platform.h` | done | T1-T17: 0; unchanged header checked via md4.c native objects, G2/G3 PASS. Platform-specific branches await target matrix. |
| `code/qcommon/q_shared.c` | done | T1: 4, T2: 3; 4 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/q_shared.o); G4 advisory FAIL, full diff retained. |
| `code/qcommon/q_shared.h` | done | T5: guarded Q_EXTERN_C macro plus 2 Windows assembly prototypes; native md4 consumer G2/G3 PASS and all 295 C hashes unchanged; Windows branch unverified. |
| `code/qcommon/qcommon.h` | done | T5: 4 native/JIT function typedefs and 2 assembly FPU prototypes; native consumer strict C++/G2/G3 PASS, full C and all 295 original hashes PASS; 32-bit FPU branch unverified. |
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
| `code/renderer/tr_arb.c` | done | T4: 3 occurrences (prerequisite), T2: 5, T3: 4, T17: 2; T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_arb.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_backend.c` | done | T4: 9 occurrences (prerequisite), T1: 4, T2: 2, T3: 3, T17: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_backend.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_bsp.c` | done | T1: 42, T3: 2; T21/T22: 94 argument casts at 94 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_bsp.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_cmds.c` | done | T1: 11; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_cmds.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_common.h` | done | T1-T17: 0; unchanged header; actual consumer code/renderer/tr_animation.c strict native builds/G2/G3 PASS. |
| `code/renderer/tr_curve.c` | done | T1: 3; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_curve.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_flares.c` | done | T4: 3 occurrences (prerequisite), T2: 1; T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_flares.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_image.c` | done | T1: 9, T2: 2, T3: 2; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_image.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_init.c` | done | T1: 10, T2: 1, T3: 1, T5: 2 conditional definitions; phase-2 GetRefAPI blocker resolved; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_init.o, default); G4 advisory FAIL, full diff retained. |
| `code/renderer/tr_light.c` | done | T4: 10 occurrences (prerequisite); T21/T22: 1 argument casts at 1 calls; 1 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_light.o, default); G4 advisory FAIL, full diff retained. |
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
| `code/renderercommon/tr_noise.c` | done | T1-T17: 0 (already compatible); T21/T22: 3 argument casts at 3 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (rend1/tr_noise.o, default); G4 advisory FAIL, full diff retained. |
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
| `code/server/sv_game.c` | done | T1: 199, T3: 7; T21/T22: 7 argument casts at 6 calls; 2 C object SHA256s unchanged; strict C++/G2/G3 PASS (ded/sv_game.o, default); G4 advisory FAIL, full diff retained. |
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
| `code/win32/win_glimp.c` | blocked | Unverified (no MinGW/Windows SDK). T1: 1, T2: 4; T5: conditional C-linkage block for 2 GPU exports, preserves definitions/initializers. Target checksum/G1-G4 unavailable. |
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

Continuation correction: the first q_math T21 edit contained seven redundant nested casts from an AST inventory that also selected explicit casts. Removed those redundant casts in a new commit without rewriting history; the scanner now selects only implicit conversions. The intended 20 argument casts at 18 calls remain.
