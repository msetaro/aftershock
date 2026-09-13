# C++20 port checkpoint

Base: `8a7e8ed2`; branch: `t3code/port-engine-to-cpp20`. Work is incomplete.

## Next action

Phase 0: verify gate negative controls, finish formatter/tidy/CI and notes; investigate formatter threshold before declaring phase 0 complete.

## Phases

- [ ] Phase 0: harness, checksum proof, warning inventory, gates, formatting, CI, notes
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
- Reproduction: `SOURCE_DATE_EPOCH=1789257600 make -B -j$(nproc) BUILD_DIR=/tmp/aftershock-cpp-port/oracle`, run on base and modified Makefile in the same worktree. Hash every `.o` relative to the build root. Manifests: `/tmp/aftershock-cpp-port/{before,after}.sha256`; logs: `{baseline,after}.log`.
- Baseline warning probes: `SOURCE_DATE_EPOCH=1789257600 make -k -j$(nproc) BUILD_DIR=/tmp/aftershock-cpp-port/c-warnings CFLAGS=-Wextra`; C++ counterpart uses `BUILD_CXX=1 BUILD_DIR=/tmp/aftershock-cpp-port/cxx-warnings CXX_FROZEN_WARNINGS='-Wall -Wextra'` (no permissive mode). C++ probe failed as expected with 3489 errors including cascades; this is not a completed port.
- Warning counts below count diagnostic lines from engine paths (exclude vendored paths), with duplicate client/ded/renderer compilations counted separately. Frozen suppressions: sign-compare 544 C / 532 C++; unused-parameter 171 / 173; missing-field-initializers 100 / 595; implicit-fallthrough 39 / 39; ignored-qualifiers 4 / 4; type-limits 2 / 2.
- Unsuppressed observed classes: C discarded-qualifiers 4 and old-style-declaration 4; C++ literal-suffix 57, write-strings 243, unused-function 9, register 10, parentheses 48, deprecated-enum-float-conversion 42, switch 2, extra 1. Catalog-fixable warnings must be transformed. Warnings cascading from parse errors will be reassessed after fixing the cause.
- Single-object strict C++ builds of `ded/md4.o` and `ded/q_math.o`: PASS. Use `make BUILD_CXX=1 BUILD_DIR=/tmp/aftershock-cpp-port/single /tmp/aftershock-cpp-port/single/release-linux-x86_64/ded/md4.o` (likewise q_math).
- Local artifacts: `/tmp/aftershock-cpp-port`; persistent evidence follows in `tools/port/` and this checkpoint.

## Harness status

- `.clang-tidy`: only signed-char-misuse, narrowing-conversions, suspicious-string-compare, and portability-simd-intrinsics; changed-line use only, no modernization checks.

- `tools/port/selfcheck.sh`: PASS, positive controls and deliberate layout-offset, symbol-linkage, and instruction mutations all detected.

- Entry point: `tools/port/codegen_gate.sh C-artifact CXX-artifact`; returns nonzero and prints FAIL plus unified diff on differences.

- Entry point: `tools/port/symbol_gate.sh C-artifact CXX-artifact`; returns nonzero and prints FAIL plus unified diff on differences.

- Entry point: `tools/port/layout_gate.sh C-artifact CXX-artifact`; returns nonzero and prints FAIL plus unified diff on differences.

- Makefile: committed, C checksum proof PASS (295 objects), strict single objects PASS; frozen warning counts above.
- `tools/port/compile_pair.py ded/md4.o /tmp/aftershock-cpp-port/gates` (likewise q_math) emits debug objects, -O2 assembly, and exact compiler commands using Makefile flags.
- G2/G3 on q_math/md4: PASS. G4 md4 PASS, q_math advisory FAIL retained below.
- Gate implementation excludes DWARF records by declaration provenance (engine-only) and fails on missing DWARF/empty engine layouts. Symbols retain nm kind/linkage; only labels and compiler clone numbering normalize. Assembly retains instructions/constants.
- Initial clang-format whole-file trial: cvar.c 549/2141 changed lines (25.642%); cl_main.c 982/5120 (19.180%). Threshold unmet; source files untouched. Surrounding source mixes styles that a global formatter cannot preserve exactly. Further tuning pending; this is not a phase-0 pass.

## Deviations

None.

## Codegen differences

- `q_math.c`: G4 FAIL (advisory) on unchanged source, gcc/g++ 15.2, actual release Makefile flags plus `-O2 -S`. C uses double `sincos`, C++ selects `sincosf`; additional overload-related instruction differences exist. This is a potential semantic difference, not merely labels. Full diff: `tools/port/evidence/q_math.codegen.diff` (pending commit). No floating-point expression was changed. Phase 1 must assess this before marking the file done; a double-argument cast is not in T1-T17 and must not be silently introduced.
- `md4.c`: G4 PASS, normalized assembly identical.

## Blocked files

None assessed yet.

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
| `code/qcommon/cm_load.c` | todo | Pending module pass. |
| `code/qcommon/cm_local.h` | todo | Pending module pass. |
| `code/qcommon/cm_patch.c` | todo | Pending module pass. |
| `code/qcommon/cm_patch.h` | todo | Pending module pass. |
| `code/qcommon/cm_polylib.c` | todo | Pending module pass. |
| `code/qcommon/cm_polylib.h` | todo | Pending module pass. |
| `code/qcommon/cm_public.h` | todo | Pending module pass. |
| `code/qcommon/cm_test.c` | todo | Pending module pass. |
| `code/qcommon/cm_trace.c` | todo | Pending module pass. |
| `code/qcommon/cmd.c` | todo | Pending module pass. |
| `code/qcommon/common.c` | todo | Pending module pass. |
| `code/qcommon/cvar.c` | todo | Pending module pass. |
| `code/qcommon/files.c` | todo | Pending module pass. |
| `code/qcommon/history.c` | todo | Pending module pass. |
| `code/qcommon/huffman.c` | todo | Pending module pass. |
| `code/qcommon/huffman_static.c` | todo | Pending module pass. |
| `code/qcommon/json.h` | todo | Pending module pass. |
| `code/qcommon/keys.c` | todo | Pending module pass. |
| `code/qcommon/md4.c` | todo | Pending module pass. |
| `code/qcommon/md5.c` | todo | Pending module pass. |
| `code/qcommon/msg.c` | todo | Pending module pass. |
| `code/qcommon/net_chan.c` | todo | Pending module pass. |
| `code/qcommon/net_ip.c` | todo | Pending module pass. |
| `code/qcommon/puff.c` | todo | Pending module pass. |
| `code/qcommon/puff.h` | todo | Pending module pass. |
| `code/qcommon/q_math.c` | todo | Pending module pass. |
| `code/qcommon/q_platform.h` | todo | Pending module pass. |
| `code/qcommon/q_shared.c` | todo | Pending module pass. |
| `code/qcommon/q_shared.h` | todo | Pending module pass. |
| `code/qcommon/qcommon.h` | todo | Pending module pass. |
| `code/qcommon/qfiles.h` | todo | Pending module pass. |
| `code/qcommon/surfaceflags.h` | todo | Pending module pass. |
| `code/qcommon/unzip.c` | todo | Pending module pass. |
| `code/qcommon/unzip.h` | todo | Pending module pass. |
| `code/qcommon/vm.c` | todo | Pending module pass. |
| `code/qcommon/vm_aarch64.c` | todo | Pending module pass. |
| `code/qcommon/vm_armv7l.c` | todo | Pending module pass. |
| `code/qcommon/vm_interpreted.c` | todo | Pending module pass. |
| `code/qcommon/vm_local.h` | todo | Pending module pass. |
| `code/qcommon/vm_optimize.h` | todo | Pending module pass. |
| `code/qcommon/vm_powerpc.c` | todo | Pending module pass. |
| `code/qcommon/vm_x86.c` | todo | Pending module pass. |
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
| `code/server/server.h` | todo | Pending module pass. |
| `code/server/sv_bot.c` | todo | Pending module pass. |
| `code/server/sv_ccmds.c` | todo | Pending module pass. |
| `code/server/sv_client.c` | todo | Pending module pass. |
| `code/server/sv_filter.c` | todo | Pending module pass. |
| `code/server/sv_game.c` | todo | Pending module pass. |
| `code/server/sv_init.c` | todo | Pending module pass. |
| `code/server/sv_main.c` | todo | Pending module pass. |
| `code/server/sv_net_chan.c` | todo | Pending module pass. |
| `code/server/sv_rankings.c` | todo | Pending module pass. |
| `code/server/sv_snapshot.c` | todo | Pending module pass. |
| `code/server/sv_world.c` | todo | Pending module pass. |
| `code/server/tlds.h` | todo | Pending module pass. |
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
