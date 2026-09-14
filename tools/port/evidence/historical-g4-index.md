## Every retained current G4 difference

G4 is advisory. These are complete normalized -O2 C/C++ assembly diffs, not accepted evidence of identical behavior. The matching G2/G3 comparisons pass. Open each with `gzip -dc tools/port/evidence/<artifact>`. Reproduce with `python3 tools/port/compile_pair.py <Object> /tmp/port-pair <Make variables>` and `tools/port/codegen_gate.sh` on its .c.s/.cxx.s pair. Linux joystick is an empty default translation unit; both native inventories explicitly enable USE_JOYSTICK for its meaningful body gates. Its unchanged default C object hash is checked separately with the full manifest. Historical superseded diffs remain in git/evidence for audit (notably the original q_math failure); current q_math is G4/G5 PASS.

| Context | Object | Make variables | Full diff artifact under tools/port/evidence |
|---|---|---|---|
| native-tu | `ded/be_aas_bspq3.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_bspq3.diff.gz` |
| native-tu | `ded/be_aas_cluster.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_cluster.diff.gz` |
| native-tu | `ded/be_aas_debug.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_debug.diff.gz` |
| native-tu | `ded/be_aas_file.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_file.diff.gz` |
| native-tu | `ded/be_aas_reach.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_reach.diff.gz` |
| native-tu | `ded/be_aas_route.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_route.diff.gz` |
| native-tu | `ded/be_aas_routealt.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_routealt.diff.gz` |
| native-tu | `ded/be_aas_sample.o` | `defaults` | `current-native-tu-botlib-ded-be_aas_sample.diff.gz` |
| native-tu | `ded/be_ai_char.o` | `defaults` | `current-native-tu-botlib-ded-be_ai_char.diff.gz` |
| native-tu | `ded/be_ai_chat.o` | `defaults` | `current-native-tu-botlib-ded-be_ai_chat.diff.gz` |
| native-tu | `ded/be_ai_move.o` | `defaults` | `current-native-tu-botlib-ded-be_ai_move.diff.gz` |
| native-tu | `ded/l_memory.o` | `defaults` | `current-native-tu-botlib-ded-l_memory.diff.gz` |
| native-tu | `ded/l_precomp.o` | `defaults` | `current-native-tu-botlib-ded-l_precomp.diff.gz` |
| native-tu | `ded/l_script.o` | `defaults` | `current-native-tu-botlib-ded-l_script.diff.gz` |
| native-tu | `ded/l_struct.o` | `defaults` | `current-native-tu-botlib-ded-l_struct.diff.gz` |
| native-tu | `client/cl_avi.o` | `defaults` | `current-native-tu-client-client-cl_avi.diff.gz` |
| native-tu | `client/cl_cgame.o` | `defaults` | `current-native-tu-client-client-cl_cgame.diff.gz` |
| native-tu | `client/cl_cin.o` | `defaults` | `current-native-tu-client-client-cl_cin.diff.gz` |
| native-tu | `client/cl_console.o` | `defaults` | `current-native-tu-client-client-cl_console.diff.gz` |
| native-tu | `client/cl_curl.o` | `defaults` | `current-native-tu-client-client-cl_curl.diff.gz` |
| native-tu | `client/cl_input.o` | `defaults` | `current-native-tu-client-client-cl_input.diff.gz` |
| native-tu | `client/cl_jpeg.o` | `defaults` | `current-native-tu-client-client-cl_jpeg.diff.gz` |
| native-tu | `client/cl_keys.o` | `defaults` | `current-native-tu-client-client-cl_keys.diff.gz` |
| native-tu | `client/cl_main.o` | `defaults` | `current-native-tu-client-client-cl_main.diff.gz` |
| native-tu | `client/cl_net_chan.o` | `defaults` | `current-native-tu-client-client-cl_net_chan.diff.gz` |
| native-tu | `client/cl_parse.o` | `defaults` | `current-native-tu-client-client-cl_parse.diff.gz` |
| native-tu | `client/cl_scrn.o` | `defaults` | `current-native-tu-client-client-cl_scrn.diff.gz` |
| native-tu | `client/cl_ui.o` | `defaults` | `current-native-tu-client-client-cl_ui.diff.gz` |
| native-tu | `client/snd_dma.o` | `defaults` | `current-native-tu-client-client-snd_dma.diff.gz` |
| native-tu | `client/snd_mem.o` | `defaults` | `current-native-tu-client-client-snd_mem.diff.gz` |
| native-tu | `client/snd_mix.o` | `defaults` | `current-native-tu-client-client-snd_mix.diff.gz` |
| native-tu | `ded/cm_load.o` | `defaults` | `current-native-tu-qcommon-ded-cm_load.diff.gz` |
| native-tu | `ded/cm_patch.o` | `defaults` | `current-native-tu-qcommon-ded-cm_patch.diff.gz` |
| native-tu | `ded/cm_trace.o` | `defaults` | `current-native-tu-qcommon-ded-cm_trace.diff.gz` |
| native-tu | `ded/cmd.o` | `defaults` | `current-native-tu-qcommon-ded-cmd.diff.gz` |
| native-tu | `ded/common.o` | `defaults` | `current-native-tu-qcommon-ded-common.diff.gz` |
| native-tu | `ded/cvar.o` | `defaults` | `current-native-tu-qcommon-ded-cvar.diff.gz` |
| native-tu | `ded/files.o` | `defaults` | `current-native-tu-qcommon-ded-files.diff.gz` |
| native-tu | `ded/history.o` | `defaults` | `current-native-tu-qcommon-ded-history.diff.gz` |
| native-tu | `ded/huffman.o` | `defaults` | `current-native-tu-qcommon-ded-huffman.diff.gz` |
| native-tu | `ded/keys.o` | `defaults` | `current-native-tu-qcommon-ded-keys.diff.gz` |
| native-tu | `ded/md5.o` | `defaults` | `current-native-tu-qcommon-ded-md5.diff.gz` |
| native-tu | `ded/msg.o` | `defaults` | `current-native-tu-qcommon-ded-msg.diff.gz` |
| native-tu | `ded/net_chan.o` | `defaults` | `current-native-tu-qcommon-ded-net_chan.diff.gz` |
| native-tu | `ded/net_ip.o` | `defaults` | `current-native-tu-qcommon-ded-net_ip.diff.gz` |
| native-tu | `client/puff.o` | `defaults` | `current-native-tu-qcommon-client-puff.diff.gz` |
| native-tu | `ded/q_shared.o` | `defaults` | `current-native-tu-qcommon-ded-q_shared.diff.gz` |
| native-tu | `ded/unzip.o` | `defaults` | `current-native-tu-qcommon-ded-unzip.diff.gz` |
| native-tu | `ded/qvm/vm.o` | `defaults` | `current-native-tu-qcommon-ded-qvm-vm.diff.gz` |
| native-tu | `ded/qvm/vm_interpreted.o` | `defaults` | `current-native-tu-qcommon-ded-qvm-vm_interpreted.diff.gz` |
| native-tu | `ded/qvm/vm_x86.o` | `defaults` | `current-native-tu-qcommon-ded-qvm-vm_x86.diff.gz` |
| native-tu | `rend1/tr_animation.o` | `defaults` | `current-native-tu-renderer-rend1-tr_animation.diff.gz` |
| native-tu | `rend1/tr_arb.o` | `defaults` | `current-native-tu-renderer-rend1-tr_arb.diff.gz` |
| native-tu | `rend1/tr_backend.o` | `defaults` | `current-native-tu-renderer-rend1-tr_backend.diff.gz` |
| native-tu | `rend1/tr_bsp.o` | `defaults` | `current-native-tu-renderer-rend1-tr_bsp.diff.gz` |
| native-tu | `rend1/tr_cmds.o` | `defaults` | `current-native-tu-renderer-rend1-tr_cmds.diff.gz` |
| native-tu | `rend1/tr_curve.o` | `defaults` | `current-native-tu-renderer-rend1-tr_curve.diff.gz` |
| native-tu | `rend1/tr_flares.o` | `defaults` | `current-native-tu-renderer-rend1-tr_flares.diff.gz` |
| native-tu | `rend1/tr_image.o` | `defaults` | `current-native-tu-renderer-rend1-tr_image.diff.gz` |
| native-tu | `rend1/tr_init.o` | `defaults` | `current-native-tu-renderer-rend1-tr_init.diff.gz` |
| native-tu | `rend1/tr_light.o` | `defaults` | `current-native-tu-renderer-rend1-tr_light.diff.gz` |
| native-tu | `rend1/tr_main.o` | `defaults` | `current-native-tu-renderer-rend1-tr_main.diff.gz` |
| native-tu | `rend1/tr_mesh.o` | `defaults` | `current-native-tu-renderer-rend1-tr_mesh.diff.gz` |
| native-tu | `rend1/tr_model.o` | `defaults` | `current-native-tu-renderer-rend1-tr_model.diff.gz` |
| native-tu | `rend1/tr_model_iqm.o` | `defaults` | `current-native-tu-renderer-rend1-tr_model_iqm.diff.gz` |
| native-tu | `rend1/tr_scene.o` | `defaults` | `current-native-tu-renderer-rend1-tr_scene.diff.gz` |
| native-tu | `rend1/tr_shade.o` | `defaults` | `current-native-tu-renderer-rend1-tr_shade.diff.gz` |
| native-tu | `rend1/tr_shade_calc.o` | `defaults` | `current-native-tu-renderer-rend1-tr_shade_calc.diff.gz` |
| native-tu | `rend1/tr_shader.o` | `defaults` | `current-native-tu-renderer-rend1-tr_shader.diff.gz` |
| native-tu | `rend1/tr_shadows.o` | `defaults` | `current-native-tu-renderer-rend1-tr_shadows.diff.gz` |
| native-tu | `rend1/tr_sky.o` | `defaults` | `current-native-tu-renderer-rend1-tr_sky.diff.gz` |
| native-tu | `rend1/tr_surface.o` | `defaults` | `current-native-tu-renderer-rend1-tr_surface.diff.gz` |
| native-tu | `rend1/tr_vbo.o` | `defaults` | `current-native-tu-renderer-rend1-tr_vbo.diff.gz` |
| native-tu | `rend1/tr_world.o` | `defaults` | `current-native-tu-renderer-rend1-tr_world.diff.gz` |
| native-tu | `rend1/tr_image_tga.o` | `defaults` | `current-native-tu-renderercommon-rend1-tr_image_tga.diff.gz` |
| native-tu | `rend1/tr_noise.o` | `defaults` | `current-native-tu-renderercommon-rend1-tr_noise.diff.gz` |
| native-tu | `rendv/tr_animation.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_animation.diff.gz` |
| native-tu | `rendv/tr_backend.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_backend.diff.gz` |
| native-tu | `rendv/tr_bsp.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_bsp.diff.gz` |
| native-tu | `rendv/tr_cmds.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_cmds.diff.gz` |
| native-tu | `rendv/tr_curve.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_curve.diff.gz` |
| native-tu | `rendv/tr_image.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_image.diff.gz` |
| native-tu | `rendv/tr_init.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_init.diff.gz` |
| native-tu | `rendv/tr_light.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_light.diff.gz` |
| native-tu | `rendv/tr_main.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_main.diff.gz` |
| native-tu | `rendv/tr_mesh.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_mesh.diff.gz` |
| native-tu | `rendv/tr_model.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_model.diff.gz` |
| native-tu | `rendv/tr_model_iqm.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_model_iqm.diff.gz` |
| native-tu | `rendv/tr_scene.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_scene.diff.gz` |
| native-tu | `rendv/tr_shade.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_shade.diff.gz` |
| native-tu | `rendv/tr_shade_calc.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_shade_calc.diff.gz` |
| native-tu | `rendv/tr_shader.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_shader.diff.gz` |
| native-tu | `rendv/tr_shadows.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_shadows.diff.gz` |
| native-tu | `rendv/tr_sky.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_sky.diff.gz` |
| native-tu | `rendv/tr_surface.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_surface.diff.gz` |
| native-tu | `rendv/tr_world.o` | `defaults` | `current-native-tu-renderervk-rendv-tr_world.diff.gz` |
| native-tu | `rendv/vk.o` | `defaults` | `current-native-tu-renderervk-rendv-vk.diff.gz` |
| native-tu | `rendv/vk_flares.o` | `defaults` | `current-native-tu-renderervk-rendv-vk_flares.diff.gz` |
| native-tu | `rendv/vk_vbo.o` | `defaults` | `current-native-tu-renderervk-rendv-vk_vbo.diff.gz` |
| native-tu | `client/sdl_input.o` | `defaults` | `current-native-tu-sdl-client-sdl_input.diff.gz` |
| native-tu | `ded/sv_bot.o` | `defaults` | `current-native-tu-server-ded-sv_bot.diff.gz` |
| native-tu | `ded/sv_ccmds.o` | `defaults` | `current-native-tu-server-ded-sv_ccmds.diff.gz` |
| native-tu | `ded/sv_filter.o` | `defaults` | `current-native-tu-server-ded-sv_filter.diff.gz` |
| native-tu | `ded/sv_game.o` | `defaults` | `current-native-tu-server-ded-sv_game.diff.gz` |
| native-tu | `ded/sv_init.o` | `defaults` | `current-native-tu-server-ded-sv_init.diff.gz` |
| native-tu | `ded/sv_main.o` | `defaults` | `current-native-tu-server-ded-sv_main.diff.gz` |
| native-tu | `ded/sv_net_chan.o` | `defaults` | `current-native-tu-server-ded-sv_net_chan.diff.gz` |
| native-tu | `ded/sv_snapshot.o` | `defaults` | `current-native-tu-server-ded-sv_snapshot.diff.gz` |
| native-tu | `ded/sv_world.o` | `defaults` | `current-native-tu-server-ded-sv_world.diff.gz` |
| native-tu | `client/linux_glimp.o` | `USE_SDL=0` | `current-native-tu-unix-client-linux_glimp.diff.gz` |
| native-tu | `client/linux_joystick.o` | `USE_SDL=0 CFLAGS=-DUSE_JOYSTICK` | `current-native-tu-unix-client-linux_joystick.diff.gz` |
| native-tu | `client/linux_qgl.o` | `USE_SDL=0` | `current-native-tu-unix-client-linux_qgl.diff.gz` |
| native-tu | `client/linux_snd.o` | `USE_SDL=0` | `current-native-tu-unix-client-linux_snd.diff.gz` |
| native-tu | `ded/unix_main.o` | `defaults` | `current-native-tu-unix-ded-unix_main.diff.gz` |
| native-tu | `ded/unix_shared.o` | `defaults` | `current-native-tu-unix-ded-unix_shared.diff.gz` |
| mingw64 | `client/be_aas_bspq3.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_bspq3.diff.gz` |
| mingw64 | `client/be_aas_cluster.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_cluster.diff.gz` |
| mingw64 | `client/be_aas_debug.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_debug.diff.gz` |
| mingw64 | `client/be_aas_entity.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_entity.diff.gz` |
| mingw64 | `client/be_aas_file.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_file.diff.gz` |
| mingw64 | `client/be_aas_move.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_move.diff.gz` |
| mingw64 | `client/be_aas_optimize.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_optimize.diff.gz` |
| mingw64 | `client/be_aas_reach.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_reach.diff.gz` |
| mingw64 | `client/be_aas_route.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_route.diff.gz` |
| mingw64 | `client/be_aas_sample.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_aas_sample.diff.gz` |
| mingw64 | `client/be_ai_char.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_ai_char.diff.gz` |
| mingw64 | `client/be_ai_chat.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_ai_chat.diff.gz` |
| mingw64 | `client/be_ai_goal.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_ai_goal.diff.gz` |
| mingw64 | `client/be_ai_move.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-be_ai_move.diff.gz` |
| mingw64 | `client/cl_avi.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_avi.diff.gz` |
| mingw64 | `client/cl_cgame.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_cgame.diff.gz` |
| mingw64 | `client/cl_console.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_console.diff.gz` |
| mingw64 | `client/cl_curl.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_curl.diff.gz` |
| mingw64 | `client/cl_input.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_input.diff.gz` |
| mingw64 | `client/cl_jpeg.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_jpeg.diff.gz` |
| mingw64 | `client/cl_keys.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_keys.diff.gz` |
| mingw64 | `client/cl_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_main.diff.gz` |
| mingw64 | `client/cl_net_chan.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_net_chan.diff.gz` |
| mingw64 | `client/cl_parse.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_parse.diff.gz` |
| mingw64 | `client/cl_scrn.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_scrn.diff.gz` |
| mingw64 | `client/cl_ui.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cl_ui.diff.gz` |
| mingw64 | `client/cm_load.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cm_load.diff.gz` |
| mingw64 | `client/cm_patch.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cm_patch.diff.gz` |
| mingw64 | `client/cm_test.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cm_test.diff.gz` |
| mingw64 | `client/cm_trace.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cm_trace.diff.gz` |
| mingw64 | `client/cmd.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cmd.diff.gz` |
| mingw64 | `client/common.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-common.diff.gz` |
| mingw64 | `client/cvar.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-cvar.diff.gz` |
| mingw64 | `client/files.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-files.diff.gz` |
| mingw64 | `client/history.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-history.diff.gz` |
| mingw64 | `client/huffman.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-huffman.diff.gz` |
| mingw64 | `client/keys.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-keys.diff.gz` |
| mingw64 | `client/l_memory.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-l_memory.diff.gz` |
| mingw64 | `client/l_precomp.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-l_precomp.diff.gz` |
| mingw64 | `client/l_script.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-l_script.diff.gz` |
| mingw64 | `client/l_struct.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-l_struct.diff.gz` |
| mingw64 | `client/md5.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-md5.diff.gz` |
| mingw64 | `client/msg.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-msg.diff.gz` |
| mingw64 | `client/net_chan.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-net_chan.diff.gz` |
| mingw64 | `client/net_ip.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-net_ip.diff.gz` |
| mingw64 | `client/puff.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-puff.diff.gz` |
| mingw64 | `client/q_math.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-q_math.diff.gz` |
| mingw64 | `client/q_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-q_shared.diff.gz` |
| mingw64 | `client/qvm/vm.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-qvm-vm.diff.gz` |
| mingw64 | `client/qvm/vm_interpreted.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-qvm-vm_interpreted.diff.gz` |
| mingw64 | `client/qvm/vm_x86.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-qvm-vm_x86.diff.gz` |
| mingw64 | `client/snd_dma.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-snd_dma.diff.gz` |
| mingw64 | `client/snd_mem.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-snd_mem.diff.gz` |
| mingw64 | `client/snd_mix.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-snd_mix.diff.gz` |
| mingw64 | `client/sv_bot.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_bot.diff.gz` |
| mingw64 | `client/sv_ccmds.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_ccmds.diff.gz` |
| mingw64 | `client/sv_filter.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_filter.diff.gz` |
| mingw64 | `client/sv_game.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_game.diff.gz` |
| mingw64 | `client/sv_init.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_init.diff.gz` |
| mingw64 | `client/sv_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_main.diff.gz` |
| mingw64 | `client/sv_net_chan.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_net_chan.diff.gz` |
| mingw64 | `client/sv_snapshot.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_snapshot.diff.gz` |
| mingw64 | `client/sv_world.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-sv_world.diff.gz` |
| mingw64 | `client/unzip.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-unzip.diff.gz` |
| mingw64 | `client/win_glimp.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-win_glimp.diff.gz` |
| mingw64 | `client/win_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-win_main.diff.gz` |
| mingw64 | `client/win_minimize.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-win_minimize.diff.gz` |
| mingw64 | `client/win_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-win_shared.diff.gz` |
| mingw64 | `client/win_syscon.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-win_syscon.diff.gz` |
| mingw64 | `client/win_wndproc.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-client-win_wndproc.diff.gz` |
| mingw64 | `ded/be_aas_bspq3.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_bspq3.diff.gz` |
| mingw64 | `ded/be_aas_cluster.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_cluster.diff.gz` |
| mingw64 | `ded/be_aas_debug.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_debug.diff.gz` |
| mingw64 | `ded/be_aas_entity.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_entity.diff.gz` |
| mingw64 | `ded/be_aas_file.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_file.diff.gz` |
| mingw64 | `ded/be_aas_move.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_move.diff.gz` |
| mingw64 | `ded/be_aas_optimize.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_optimize.diff.gz` |
| mingw64 | `ded/be_aas_reach.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_reach.diff.gz` |
| mingw64 | `ded/be_aas_route.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_route.diff.gz` |
| mingw64 | `ded/be_aas_sample.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_aas_sample.diff.gz` |
| mingw64 | `ded/be_ai_char.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_ai_char.diff.gz` |
| mingw64 | `ded/be_ai_chat.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_ai_chat.diff.gz` |
| mingw64 | `ded/be_ai_goal.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_ai_goal.diff.gz` |
| mingw64 | `ded/be_ai_move.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-be_ai_move.diff.gz` |
| mingw64 | `ded/cm_load.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-cm_load.diff.gz` |
| mingw64 | `ded/cm_patch.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-cm_patch.diff.gz` |
| mingw64 | `ded/cm_test.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-cm_test.diff.gz` |
| mingw64 | `ded/cm_trace.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-cm_trace.diff.gz` |
| mingw64 | `ded/cmd.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-cmd.diff.gz` |
| mingw64 | `ded/common.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-common.diff.gz` |
| mingw64 | `ded/cvar.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-cvar.diff.gz` |
| mingw64 | `ded/files.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-files.diff.gz` |
| mingw64 | `ded/history.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-history.diff.gz` |
| mingw64 | `ded/huffman.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-huffman.diff.gz` |
| mingw64 | `ded/keys.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-keys.diff.gz` |
| mingw64 | `ded/l_memory.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-l_memory.diff.gz` |
| mingw64 | `ded/l_precomp.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-l_precomp.diff.gz` |
| mingw64 | `ded/l_script.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-l_script.diff.gz` |
| mingw64 | `ded/l_struct.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-l_struct.diff.gz` |
| mingw64 | `ded/md5.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-md5.diff.gz` |
| mingw64 | `ded/msg.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-msg.diff.gz` |
| mingw64 | `ded/net_chan.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-net_chan.diff.gz` |
| mingw64 | `ded/net_ip.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-net_ip.diff.gz` |
| mingw64 | `ded/q_math.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-q_math.diff.gz` |
| mingw64 | `ded/q_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-q_shared.diff.gz` |
| mingw64 | `ded/qvm/vm.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-qvm-vm.diff.gz` |
| mingw64 | `ded/qvm/vm_interpreted.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-qvm-vm_interpreted.diff.gz` |
| mingw64 | `ded/qvm/vm_x86.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-qvm-vm_x86.diff.gz` |
| mingw64 | `ded/sv_bot.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_bot.diff.gz` |
| mingw64 | `ded/sv_ccmds.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_ccmds.diff.gz` |
| mingw64 | `ded/sv_filter.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_filter.diff.gz` |
| mingw64 | `ded/sv_game.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_game.diff.gz` |
| mingw64 | `ded/sv_init.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_init.diff.gz` |
| mingw64 | `ded/sv_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_main.diff.gz` |
| mingw64 | `ded/sv_net_chan.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_net_chan.diff.gz` |
| mingw64 | `ded/sv_snapshot.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_snapshot.diff.gz` |
| mingw64 | `ded/sv_world.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-sv_world.diff.gz` |
| mingw64 | `ded/unzip.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-unzip.diff.gz` |
| mingw64 | `ded/win_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-win_main.diff.gz` |
| mingw64 | `ded/win_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-win_shared.diff.gz` |
| mingw64 | `ded/win_syscon.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-ded-win_syscon.diff.gz` |
| mingw64 | `rend1/puff.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-puff.diff.gz` |
| mingw64 | `rend1/q_math.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-q_math.diff.gz` |
| mingw64 | `rend1/q_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-q_shared.diff.gz` |
| mingw64 | `rend1/tr_animation.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_animation.diff.gz` |
| mingw64 | `rend1/tr_arb.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_arb.diff.gz` |
| mingw64 | `rend1/tr_backend.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_backend.diff.gz` |
| mingw64 | `rend1/tr_bsp.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_bsp.diff.gz` |
| mingw64 | `rend1/tr_cmds.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_cmds.diff.gz` |
| mingw64 | `rend1/tr_curve.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_curve.diff.gz` |
| mingw64 | `rend1/tr_flares.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_flares.diff.gz` |
| mingw64 | `rend1/tr_image.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_image.diff.gz` |
| mingw64 | `rend1/tr_image_tga.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_image_tga.diff.gz` |
| mingw64 | `rend1/tr_init.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_init.diff.gz` |
| mingw64 | `rend1/tr_light.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_light.diff.gz` |
| mingw64 | `rend1/tr_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_main.diff.gz` |
| mingw64 | `rend1/tr_marks.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_marks.diff.gz` |
| mingw64 | `rend1/tr_mesh.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_mesh.diff.gz` |
| mingw64 | `rend1/tr_model.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_model.diff.gz` |
| mingw64 | `rend1/tr_model_iqm.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_model_iqm.diff.gz` |
| mingw64 | `rend1/tr_scene.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_scene.diff.gz` |
| mingw64 | `rend1/tr_shade.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_shade.diff.gz` |
| mingw64 | `rend1/tr_shade_calc.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_shade_calc.diff.gz` |
| mingw64 | `rend1/tr_shader.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_shader.diff.gz` |
| mingw64 | `rend1/tr_shadows.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_shadows.diff.gz` |
| mingw64 | `rend1/tr_sky.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_sky.diff.gz` |
| mingw64 | `rend1/tr_surface.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_surface.diff.gz` |
| mingw64 | `rend1/tr_vbo.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_vbo.diff.gz` |
| mingw64 | `rend1/tr_world.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rend1-tr_world.diff.gz` |
| mingw64 | `rendv/puff.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-puff.diff.gz` |
| mingw64 | `rendv/q_math.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-q_math.diff.gz` |
| mingw64 | `rendv/q_shared.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-q_shared.diff.gz` |
| mingw64 | `rendv/tr_animation.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_animation.diff.gz` |
| mingw64 | `rendv/tr_backend.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_backend.diff.gz` |
| mingw64 | `rendv/tr_bsp.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_bsp.diff.gz` |
| mingw64 | `rendv/tr_cmds.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_cmds.diff.gz` |
| mingw64 | `rendv/tr_curve.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_curve.diff.gz` |
| mingw64 | `rendv/tr_image.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_image.diff.gz` |
| mingw64 | `rendv/tr_image_tga.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_image_tga.diff.gz` |
| mingw64 | `rendv/tr_init.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_init.diff.gz` |
| mingw64 | `rendv/tr_light.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_light.diff.gz` |
| mingw64 | `rendv/tr_main.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_main.diff.gz` |
| mingw64 | `rendv/tr_marks.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_marks.diff.gz` |
| mingw64 | `rendv/tr_mesh.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_mesh.diff.gz` |
| mingw64 | `rendv/tr_model.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_model.diff.gz` |
| mingw64 | `rendv/tr_model_iqm.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_model_iqm.diff.gz` |
| mingw64 | `rendv/tr_scene.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_scene.diff.gz` |
| mingw64 | `rendv/tr_shade.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_shade.diff.gz` |
| mingw64 | `rendv/tr_shade_calc.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_shade_calc.diff.gz` |
| mingw64 | `rendv/tr_shader.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_shader.diff.gz` |
| mingw64 | `rendv/tr_shadows.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_shadows.diff.gz` |
| mingw64 | `rendv/tr_sky.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_sky.diff.gz` |
| mingw64 | `rendv/tr_surface.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_surface.diff.gz` |
| mingw64 | `rendv/tr_world.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-tr_world.diff.gz` |
| mingw64 | `rendv/vk.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-vk.diff.gz` |
| mingw64 | `rendv/vk_flares.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-vk_flares.diff.gz` |
| mingw64 | `rendv/vk_vbo.o` | `PLATFORM=mingw64 ARCH=x86_64 USE_SDL=0 OPTIMIZE=-O2 -ffast-math -fno-lto` | `current-cross-mingw64-rendv-vk_vbo.diff.gz` |
| aarch64 | `ded/be_aas_bspq3.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_bspq3.diff.gz` |
| aarch64 | `ded/be_aas_cluster.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_cluster.diff.gz` |
| aarch64 | `ded/be_aas_debug.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_debug.diff.gz` |
| aarch64 | `ded/be_aas_file.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_file.diff.gz` |
| aarch64 | `ded/be_aas_reach.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_reach.diff.gz` |
| aarch64 | `ded/be_aas_route.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_route.diff.gz` |
| aarch64 | `ded/be_aas_routealt.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_routealt.diff.gz` |
| aarch64 | `ded/be_aas_sample.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_aas_sample.diff.gz` |
| aarch64 | `ded/be_ai_char.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_ai_char.diff.gz` |
| aarch64 | `ded/be_ai_chat.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_ai_chat.diff.gz` |
| aarch64 | `ded/be_ai_move.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-be_ai_move.diff.gz` |
| aarch64 | `ded/cm_load.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-cm_load.diff.gz` |
| aarch64 | `ded/cm_patch.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-cm_patch.diff.gz` |
| aarch64 | `ded/cm_trace.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-cm_trace.diff.gz` |
| aarch64 | `ded/cmd.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-cmd.diff.gz` |
| aarch64 | `ded/common.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-common.diff.gz` |
| aarch64 | `ded/cvar.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-cvar.diff.gz` |
| aarch64 | `ded/files.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-files.diff.gz` |
| aarch64 | `ded/history.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-history.diff.gz` |
| aarch64 | `ded/huffman.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-huffman.diff.gz` |
| aarch64 | `ded/keys.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-keys.diff.gz` |
| aarch64 | `ded/l_memory.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-l_memory.diff.gz` |
| aarch64 | `ded/l_precomp.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-l_precomp.diff.gz` |
| aarch64 | `ded/l_script.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-l_script.diff.gz` |
| aarch64 | `ded/l_struct.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-l_struct.diff.gz` |
| aarch64 | `ded/md5.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-md5.diff.gz` |
| aarch64 | `ded/msg.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-msg.diff.gz` |
| aarch64 | `ded/net_chan.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-net_chan.diff.gz` |
| aarch64 | `ded/net_ip.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-net_ip.diff.gz` |
| aarch64 | `ded/q_shared.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-q_shared.diff.gz` |
| aarch64 | `ded/qvm/vm.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-qvm-vm.diff.gz` |
| aarch64 | `ded/qvm/vm_aarch64.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-qvm-vm_aarch64.diff.gz` |
| aarch64 | `ded/qvm/vm_interpreted.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-qvm-vm_interpreted.diff.gz` |
| aarch64 | `ded/sv_bot.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_bot.diff.gz` |
| aarch64 | `ded/sv_ccmds.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_ccmds.diff.gz` |
| aarch64 | `ded/sv_filter.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_filter.diff.gz` |
| aarch64 | `ded/sv_game.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_game.diff.gz` |
| aarch64 | `ded/sv_init.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_init.diff.gz` |
| aarch64 | `ded/sv_main.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_main.diff.gz` |
| aarch64 | `ded/sv_net_chan.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_net_chan.diff.gz` |
| aarch64 | `ded/sv_snapshot.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_snapshot.diff.gz` |
| aarch64 | `ded/sv_world.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-sv_world.diff.gz` |
| aarch64 | `ded/unix_main.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-unix_main.diff.gz` |
| aarch64 | `ded/unix_shared.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-unix_shared.diff.gz` |
| aarch64 | `ded/unzip.o` | `ARCH=aarch64 CC=aarch64-linux-gnu-gcc` | `current-cross-aarch64-ded-unzip.diff.gz` |
| arm | `ded/be_aas_bspq3.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_bspq3.diff.gz` |
| arm | `ded/be_aas_cluster.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_cluster.diff.gz` |
| arm | `ded/be_aas_debug.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_debug.diff.gz` |
| arm | `ded/be_aas_file.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_file.diff.gz` |
| arm | `ded/be_aas_reach.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_reach.diff.gz` |
| arm | `ded/be_aas_route.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_route.diff.gz` |
| arm | `ded/be_aas_routealt.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_routealt.diff.gz` |
| arm | `ded/be_aas_sample.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_aas_sample.diff.gz` |
| arm | `ded/be_ai_char.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_ai_char.diff.gz` |
| arm | `ded/be_ai_chat.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_ai_chat.diff.gz` |
| arm | `ded/be_ai_move.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-be_ai_move.diff.gz` |
| arm | `ded/cm_load.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-cm_load.diff.gz` |
| arm | `ded/cm_patch.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-cm_patch.diff.gz` |
| arm | `ded/cm_trace.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-cm_trace.diff.gz` |
| arm | `ded/cmd.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-cmd.diff.gz` |
| arm | `ded/common.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-common.diff.gz` |
| arm | `ded/cvar.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-cvar.diff.gz` |
| arm | `ded/files.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-files.diff.gz` |
| arm | `ded/history.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-history.diff.gz` |
| arm | `ded/huffman.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-huffman.diff.gz` |
| arm | `ded/keys.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-keys.diff.gz` |
| arm | `ded/l_memory.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-l_memory.diff.gz` |
| arm | `ded/l_precomp.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-l_precomp.diff.gz` |
| arm | `ded/l_script.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-l_script.diff.gz` |
| arm | `ded/l_struct.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-l_struct.diff.gz` |
| arm | `ded/md5.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-md5.diff.gz` |
| arm | `ded/msg.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-msg.diff.gz` |
| arm | `ded/net_chan.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-net_chan.diff.gz` |
| arm | `ded/net_ip.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-net_ip.diff.gz` |
| arm | `ded/q_math.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-q_math.diff.gz` |
| arm | `ded/q_shared.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-q_shared.diff.gz` |
| arm | `ded/qvm/vm.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-qvm-vm.diff.gz` |
| arm | `ded/qvm/vm_armv7l.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-qvm-vm_armv7l.diff.gz` |
| arm | `ded/qvm/vm_interpreted.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-qvm-vm_interpreted.diff.gz` |
| arm | `ded/sv_bot.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_bot.diff.gz` |
| arm | `ded/sv_ccmds.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_ccmds.diff.gz` |
| arm | `ded/sv_filter.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_filter.diff.gz` |
| arm | `ded/sv_game.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_game.diff.gz` |
| arm | `ded/sv_init.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_init.diff.gz` |
| arm | `ded/sv_main.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_main.diff.gz` |
| arm | `ded/sv_net_chan.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_net_chan.diff.gz` |
| arm | `ded/sv_snapshot.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_snapshot.diff.gz` |
| arm | `ded/sv_world.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-sv_world.diff.gz` |
| arm | `ded/unix_main.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-unix_main.diff.gz` |
| arm | `ded/unix_shared.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-unix_shared.diff.gz` |
| arm | `ded/unzip.o` | `ARCH=arm CC=arm-linux-gnueabihf-gcc LONG_BIT=32` | `current-cross-arm-ded-unzip.diff.gz` |
| ppc64le | `ded/be_aas_bspq3.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_bspq3.diff.gz` |
| ppc64le | `ded/be_aas_cluster.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_cluster.diff.gz` |
| ppc64le | `ded/be_aas_debug.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_debug.diff.gz` |
| ppc64le | `ded/be_aas_entity.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_entity.diff.gz` |
| ppc64le | `ded/be_aas_file.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_file.diff.gz` |
| ppc64le | `ded/be_aas_main.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_main.diff.gz` |
| ppc64le | `ded/be_aas_move.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_move.diff.gz` |
| ppc64le | `ded/be_aas_optimize.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_optimize.diff.gz` |
| ppc64le | `ded/be_aas_reach.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_reach.diff.gz` |
| ppc64le | `ded/be_aas_route.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_route.diff.gz` |
| ppc64le | `ded/be_aas_routealt.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_routealt.diff.gz` |
| ppc64le | `ded/be_aas_sample.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_aas_sample.diff.gz` |
| ppc64le | `ded/be_ai_char.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ai_char.diff.gz` |
| ppc64le | `ded/be_ai_chat.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ai_chat.diff.gz` |
| ppc64le | `ded/be_ai_gen.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ai_gen.diff.gz` |
| ppc64le | `ded/be_ai_goal.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ai_goal.diff.gz` |
| ppc64le | `ded/be_ai_move.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ai_move.diff.gz` |
| ppc64le | `ded/be_ai_weap.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ai_weap.diff.gz` |
| ppc64le | `ded/be_ai_weight.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ai_weight.diff.gz` |
| ppc64le | `ded/be_ea.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_ea.diff.gz` |
| ppc64le | `ded/be_interface.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-be_interface.diff.gz` |
| ppc64le | `ded/cm_load.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-cm_load.diff.gz` |
| ppc64le | `ded/cm_patch.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-cm_patch.diff.gz` |
| ppc64le | `ded/cm_polylib.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-cm_polylib.diff.gz` |
| ppc64le | `ded/cm_test.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-cm_test.diff.gz` |
| ppc64le | `ded/cm_trace.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-cm_trace.diff.gz` |
| ppc64le | `ded/cmd.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-cmd.diff.gz` |
| ppc64le | `ded/common.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-common.diff.gz` |
| ppc64le | `ded/cvar.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-cvar.diff.gz` |
| ppc64le | `ded/files.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-files.diff.gz` |
| ppc64le | `ded/history.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-history.diff.gz` |
| ppc64le | `ded/huffman.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-huffman.diff.gz` |
| ppc64le | `ded/huffman_static.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-huffman_static.diff.gz` |
| ppc64le | `ded/keys.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-keys.diff.gz` |
| ppc64le | `ded/l_crc.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-l_crc.diff.gz` |
| ppc64le | `ded/l_libvar.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-l_libvar.diff.gz` |
| ppc64le | `ded/l_log.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-l_log.diff.gz` |
| ppc64le | `ded/l_memory.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-l_memory.diff.gz` |
| ppc64le | `ded/l_precomp.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-l_precomp.diff.gz` |
| ppc64le | `ded/l_script.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-l_script.diff.gz` |
| ppc64le | `ded/l_struct.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-l_struct.diff.gz` |
| ppc64le | `ded/linux_signals.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-linux_signals.diff.gz` |
| ppc64le | `ded/md4.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-md4.diff.gz` |
| ppc64le | `ded/md5.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-md5.diff.gz` |
| ppc64le | `ded/msg.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-msg.diff.gz` |
| ppc64le | `ded/net_chan.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-net_chan.diff.gz` |
| ppc64le | `ded/net_ip.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-net_ip.diff.gz` |
| ppc64le | `ded/q_math.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-q_math.diff.gz` |
| ppc64le | `ded/q_shared.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-q_shared.diff.gz` |
| ppc64le | `ded/qvm/vm.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-qvm-vm.diff.gz` |
| ppc64le | `ded/qvm/vm_interpreted.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-qvm-vm_interpreted.diff.gz` |
| ppc64le | `ded/qvm/vm_powerpc.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-qvm-vm_powerpc.diff.gz` |
| ppc64le | `ded/sv_bot.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_bot.diff.gz` |
| ppc64le | `ded/sv_ccmds.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_ccmds.diff.gz` |
| ppc64le | `ded/sv_filter.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_filter.diff.gz` |
| ppc64le | `ded/sv_game.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_game.diff.gz` |
| ppc64le | `ded/sv_init.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_init.diff.gz` |
| ppc64le | `ded/sv_main.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_main.diff.gz` |
| ppc64le | `ded/sv_net_chan.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_net_chan.diff.gz` |
| ppc64le | `ded/sv_snapshot.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_snapshot.diff.gz` |
| ppc64le | `ded/sv_world.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-sv_world.diff.gz` |
| ppc64le | `ded/unix_main.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-unix_main.diff.gz` |
| ppc64le | `ded/unix_shared.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-unix_shared.diff.gz` |
| ppc64le | `ded/unzip.o` | `ARCH=ppc64le CC=powerpc64le-linux-gnu-gcc` | `current-cross-ppc64le-ded-unzip.diff.gz` |
| native | `client/be_aas_bspq3.o` | `defaults` | `current-native-context-native-client-be_aas_bspq3.diff.gz` |
| native | `client/be_aas_cluster.o` | `defaults` | `current-native-context-native-client-be_aas_cluster.diff.gz` |
| native | `client/be_aas_debug.o` | `defaults` | `current-native-context-native-client-be_aas_debug.diff.gz` |
| native | `client/be_aas_file.o` | `defaults` | `current-native-context-native-client-be_aas_file.diff.gz` |
| native | `client/be_aas_reach.o` | `defaults` | `current-native-context-native-client-be_aas_reach.diff.gz` |
| native | `client/be_aas_route.o` | `defaults` | `current-native-context-native-client-be_aas_route.diff.gz` |
| native | `client/be_aas_routealt.o` | `defaults` | `current-native-context-native-client-be_aas_routealt.diff.gz` |
| native | `client/be_aas_sample.o` | `defaults` | `current-native-context-native-client-be_aas_sample.diff.gz` |
| native | `client/be_ai_char.o` | `defaults` | `current-native-context-native-client-be_ai_char.diff.gz` |
| native | `client/be_ai_chat.o` | `defaults` | `current-native-context-native-client-be_ai_chat.diff.gz` |
| native | `client/be_ai_move.o` | `defaults` | `current-native-context-native-client-be_ai_move.diff.gz` |
| native | `client/cl_avi.o` | `defaults` | `current-native-context-native-client-cl_avi.diff.gz` |
| native | `client/cl_cgame.o` | `defaults` | `current-native-context-native-client-cl_cgame.diff.gz` |
| native | `client/cl_cin.o` | `defaults` | `current-native-context-native-client-cl_cin.diff.gz` |
| native | `client/cl_console.o` | `defaults` | `current-native-context-native-client-cl_console.diff.gz` |
| native | `client/cl_curl.o` | `defaults` | `current-native-context-native-client-cl_curl.diff.gz` |
| native | `client/cl_input.o` | `defaults` | `current-native-context-native-client-cl_input.diff.gz` |
| native | `client/cl_jpeg.o` | `defaults` | `current-native-context-native-client-cl_jpeg.diff.gz` |
| native | `client/cl_keys.o` | `defaults` | `current-native-context-native-client-cl_keys.diff.gz` |
| native | `client/cl_main.o` | `defaults` | `current-native-context-native-client-cl_main.diff.gz` |
| native | `client/cl_net_chan.o` | `defaults` | `current-native-context-native-client-cl_net_chan.diff.gz` |
| native | `client/cl_parse.o` | `defaults` | `current-native-context-native-client-cl_parse.diff.gz` |
| native | `client/cl_scrn.o` | `defaults` | `current-native-context-native-client-cl_scrn.diff.gz` |
| native | `client/cl_ui.o` | `defaults` | `current-native-context-native-client-cl_ui.diff.gz` |
| native | `client/cm_load.o` | `defaults` | `current-native-context-native-client-cm_load.diff.gz` |
| native | `client/cm_patch.o` | `defaults` | `current-native-context-native-client-cm_patch.diff.gz` |
| native | `client/cm_trace.o` | `defaults` | `current-native-context-native-client-cm_trace.diff.gz` |
| native | `client/cmd.o` | `defaults` | `current-native-context-native-client-cmd.diff.gz` |
| native | `client/common.o` | `defaults` | `current-native-context-native-client-common.diff.gz` |
| native | `client/cvar.o` | `defaults` | `current-native-context-native-client-cvar.diff.gz` |
| native | `client/files.o` | `defaults` | `current-native-context-native-client-files.diff.gz` |
| native | `client/history.o` | `defaults` | `current-native-context-native-client-history.diff.gz` |
| native | `client/huffman.o` | `defaults` | `current-native-context-native-client-huffman.diff.gz` |
| native | `client/keys.o` | `defaults` | `current-native-context-native-client-keys.diff.gz` |
| native | `client/l_memory.o` | `defaults` | `current-native-context-native-client-l_memory.diff.gz` |
| native | `client/l_precomp.o` | `defaults` | `current-native-context-native-client-l_precomp.diff.gz` |
| native | `client/l_script.o` | `defaults` | `current-native-context-native-client-l_script.diff.gz` |
| native | `client/l_struct.o` | `defaults` | `current-native-context-native-client-l_struct.diff.gz` |
| native | `client/md5.o` | `defaults` | `current-native-context-native-client-md5.diff.gz` |
| native | `client/msg.o` | `defaults` | `current-native-context-native-client-msg.diff.gz` |
| native | `client/net_chan.o` | `defaults` | `current-native-context-native-client-net_chan.diff.gz` |
| native | `client/net_ip.o` | `defaults` | `current-native-context-native-client-net_ip.diff.gz` |
| native | `client/puff.o` | `defaults` | `current-native-context-native-client-puff.diff.gz` |
| native | `client/q_shared.o` | `defaults` | `current-native-context-native-client-q_shared.diff.gz` |
| native | `client/qvm/vm.o` | `defaults` | `current-native-context-native-client-qvm-vm.diff.gz` |
| native | `client/qvm/vm_interpreted.o` | `defaults` | `current-native-context-native-client-qvm-vm_interpreted.diff.gz` |
| native | `client/qvm/vm_x86.o` | `defaults` | `current-native-context-native-client-qvm-vm_x86.diff.gz` |
| native | `client/sdl_input.o` | `defaults` | `current-native-context-native-client-sdl_input.diff.gz` |
| native | `client/snd_dma.o` | `defaults` | `current-native-context-native-client-snd_dma.diff.gz` |
| native | `client/snd_mem.o` | `defaults` | `current-native-context-native-client-snd_mem.diff.gz` |
| native | `client/snd_mix.o` | `defaults` | `current-native-context-native-client-snd_mix.diff.gz` |
| native | `client/sv_bot.o` | `defaults` | `current-native-context-native-client-sv_bot.diff.gz` |
| native | `client/sv_ccmds.o` | `defaults` | `current-native-context-native-client-sv_ccmds.diff.gz` |
| native | `client/sv_filter.o` | `defaults` | `current-native-context-native-client-sv_filter.diff.gz` |
| native | `client/sv_game.o` | `defaults` | `current-native-context-native-client-sv_game.diff.gz` |
| native | `client/sv_init.o` | `defaults` | `current-native-context-native-client-sv_init.diff.gz` |
| native | `client/sv_main.o` | `defaults` | `current-native-context-native-client-sv_main.diff.gz` |
| native | `client/sv_net_chan.o` | `defaults` | `current-native-context-native-client-sv_net_chan.diff.gz` |
| native | `client/sv_snapshot.o` | `defaults` | `current-native-context-native-client-sv_snapshot.diff.gz` |
| native | `client/sv_world.o` | `defaults` | `current-native-context-native-client-sv_world.diff.gz` |
| native | `client/unix_main.o` | `defaults` | `current-native-context-native-client-unix_main.diff.gz` |
| native | `client/unix_shared.o` | `defaults` | `current-native-context-native-client-unix_shared.diff.gz` |
| native | `client/unzip.o` | `defaults` | `current-native-context-native-client-unzip.diff.gz` |
| native | `ded/be_aas_bspq3.o` | `defaults` | `current-native-context-native-ded-be_aas_bspq3.diff.gz` |
| native | `ded/be_aas_cluster.o` | `defaults` | `current-native-context-native-ded-be_aas_cluster.diff.gz` |
| native | `ded/be_aas_debug.o` | `defaults` | `current-native-context-native-ded-be_aas_debug.diff.gz` |
| native | `ded/be_aas_file.o` | `defaults` | `current-native-context-native-ded-be_aas_file.diff.gz` |
| native | `ded/be_aas_reach.o` | `defaults` | `current-native-context-native-ded-be_aas_reach.diff.gz` |
| native | `ded/be_aas_route.o` | `defaults` | `current-native-context-native-ded-be_aas_route.diff.gz` |
| native | `ded/be_aas_routealt.o` | `defaults` | `current-native-context-native-ded-be_aas_routealt.diff.gz` |
| native | `ded/be_aas_sample.o` | `defaults` | `current-native-context-native-ded-be_aas_sample.diff.gz` |
| native | `ded/be_ai_char.o` | `defaults` | `current-native-context-native-ded-be_ai_char.diff.gz` |
| native | `ded/be_ai_chat.o` | `defaults` | `current-native-context-native-ded-be_ai_chat.diff.gz` |
| native | `ded/be_ai_move.o` | `defaults` | `current-native-context-native-ded-be_ai_move.diff.gz` |
| native | `ded/cm_load.o` | `defaults` | `current-native-context-native-ded-cm_load.diff.gz` |
| native | `ded/cm_patch.o` | `defaults` | `current-native-context-native-ded-cm_patch.diff.gz` |
| native | `ded/cm_trace.o` | `defaults` | `current-native-context-native-ded-cm_trace.diff.gz` |
| native | `ded/cmd.o` | `defaults` | `current-native-context-native-ded-cmd.diff.gz` |
| native | `ded/common.o` | `defaults` | `current-native-context-native-ded-common.diff.gz` |
| native | `ded/cvar.o` | `defaults` | `current-native-context-native-ded-cvar.diff.gz` |
| native | `ded/files.o` | `defaults` | `current-native-context-native-ded-files.diff.gz` |
| native | `ded/history.o` | `defaults` | `current-native-context-native-ded-history.diff.gz` |
| native | `ded/huffman.o` | `defaults` | `current-native-context-native-ded-huffman.diff.gz` |
| native | `ded/keys.o` | `defaults` | `current-native-context-native-ded-keys.diff.gz` |
| native | `ded/l_memory.o` | `defaults` | `current-native-context-native-ded-l_memory.diff.gz` |
| native | `ded/l_precomp.o` | `defaults` | `current-native-context-native-ded-l_precomp.diff.gz` |
| native | `ded/l_script.o` | `defaults` | `current-native-context-native-ded-l_script.diff.gz` |
| native | `ded/l_struct.o` | `defaults` | `current-native-context-native-ded-l_struct.diff.gz` |
| native | `ded/md5.o` | `defaults` | `current-native-context-native-ded-md5.diff.gz` |
| native | `ded/msg.o` | `defaults` | `current-native-context-native-ded-msg.diff.gz` |
| native | `ded/net_chan.o` | `defaults` | `current-native-context-native-ded-net_chan.diff.gz` |
| native | `ded/net_ip.o` | `defaults` | `current-native-context-native-ded-net_ip.diff.gz` |
| native | `ded/q_shared.o` | `defaults` | `current-native-context-native-ded-q_shared.diff.gz` |
| native | `ded/qvm/vm.o` | `defaults` | `current-native-context-native-ded-qvm-vm.diff.gz` |
| native | `ded/qvm/vm_interpreted.o` | `defaults` | `current-native-context-native-ded-qvm-vm_interpreted.diff.gz` |
| native | `ded/qvm/vm_x86.o` | `defaults` | `current-native-context-native-ded-qvm-vm_x86.diff.gz` |
| native | `ded/sv_bot.o` | `defaults` | `current-native-context-native-ded-sv_bot.diff.gz` |
| native | `ded/sv_ccmds.o` | `defaults` | `current-native-context-native-ded-sv_ccmds.diff.gz` |
| native | `ded/sv_filter.o` | `defaults` | `current-native-context-native-ded-sv_filter.diff.gz` |
| native | `ded/sv_game.o` | `defaults` | `current-native-context-native-ded-sv_game.diff.gz` |
| native | `ded/sv_init.o` | `defaults` | `current-native-context-native-ded-sv_init.diff.gz` |
| native | `ded/sv_main.o` | `defaults` | `current-native-context-native-ded-sv_main.diff.gz` |
| native | `ded/sv_net_chan.o` | `defaults` | `current-native-context-native-ded-sv_net_chan.diff.gz` |
| native | `ded/sv_snapshot.o` | `defaults` | `current-native-context-native-ded-sv_snapshot.diff.gz` |
| native | `ded/sv_world.o` | `defaults` | `current-native-context-native-ded-sv_world.diff.gz` |
| native | `ded/unix_main.o` | `defaults` | `current-native-context-native-ded-unix_main.diff.gz` |
| native | `ded/unix_shared.o` | `defaults` | `current-native-context-native-ded-unix_shared.diff.gz` |
| native | `ded/unzip.o` | `defaults` | `current-native-context-native-ded-unzip.diff.gz` |
| native | `rend1/puff.o` | `defaults` | `current-native-context-native-rend1-puff.diff.gz` |
| native | `rend1/q_shared.o` | `defaults` | `current-native-context-native-rend1-q_shared.diff.gz` |
| native | `rend1/tr_animation.o` | `defaults` | `current-native-context-native-rend1-tr_animation.diff.gz` |
| native | `rend1/tr_arb.o` | `defaults` | `current-native-context-native-rend1-tr_arb.diff.gz` |
| native | `rend1/tr_backend.o` | `defaults` | `current-native-context-native-rend1-tr_backend.diff.gz` |
| native | `rend1/tr_bsp.o` | `defaults` | `current-native-context-native-rend1-tr_bsp.diff.gz` |
| native | `rend1/tr_cmds.o` | `defaults` | `current-native-context-native-rend1-tr_cmds.diff.gz` |
| native | `rend1/tr_curve.o` | `defaults` | `current-native-context-native-rend1-tr_curve.diff.gz` |
| native | `rend1/tr_flares.o` | `defaults` | `current-native-context-native-rend1-tr_flares.diff.gz` |
| native | `rend1/tr_image.o` | `defaults` | `current-native-context-native-rend1-tr_image.diff.gz` |
| native | `rend1/tr_image_tga.o` | `defaults` | `current-native-context-native-rend1-tr_image_tga.diff.gz` |
| native | `rend1/tr_init.o` | `defaults` | `current-native-context-native-rend1-tr_init.diff.gz` |
| native | `rend1/tr_light.o` | `defaults` | `current-native-context-native-rend1-tr_light.diff.gz` |
| native | `rend1/tr_main.o` | `defaults` | `current-native-context-native-rend1-tr_main.diff.gz` |
| native | `rend1/tr_mesh.o` | `defaults` | `current-native-context-native-rend1-tr_mesh.diff.gz` |
| native | `rend1/tr_model.o` | `defaults` | `current-native-context-native-rend1-tr_model.diff.gz` |
| native | `rend1/tr_model_iqm.o` | `defaults` | `current-native-context-native-rend1-tr_model_iqm.diff.gz` |
| native | `rend1/tr_noise.o` | `defaults` | `current-native-context-native-rend1-tr_noise.diff.gz` |
| native | `rend1/tr_scene.o` | `defaults` | `current-native-context-native-rend1-tr_scene.diff.gz` |
| native | `rend1/tr_shade.o` | `defaults` | `current-native-context-native-rend1-tr_shade.diff.gz` |
| native | `rend1/tr_shade_calc.o` | `defaults` | `current-native-context-native-rend1-tr_shade_calc.diff.gz` |
| native | `rend1/tr_shader.o` | `defaults` | `current-native-context-native-rend1-tr_shader.diff.gz` |
| native | `rend1/tr_shadows.o` | `defaults` | `current-native-context-native-rend1-tr_shadows.diff.gz` |
| native | `rend1/tr_sky.o` | `defaults` | `current-native-context-native-rend1-tr_sky.diff.gz` |
| native | `rend1/tr_surface.o` | `defaults` | `current-native-context-native-rend1-tr_surface.diff.gz` |
| native | `rend1/tr_vbo.o` | `defaults` | `current-native-context-native-rend1-tr_vbo.diff.gz` |
| native | `rend1/tr_world.o` | `defaults` | `current-native-context-native-rend1-tr_world.diff.gz` |
| native | `rendv/puff.o` | `defaults` | `current-native-context-native-rendv-puff.diff.gz` |
| native | `rendv/q_shared.o` | `defaults` | `current-native-context-native-rendv-q_shared.diff.gz` |
| native | `rendv/tr_animation.o` | `defaults` | `current-native-context-native-rendv-tr_animation.diff.gz` |
| native | `rendv/tr_backend.o` | `defaults` | `current-native-context-native-rendv-tr_backend.diff.gz` |
| native | `rendv/tr_bsp.o` | `defaults` | `current-native-context-native-rendv-tr_bsp.diff.gz` |
| native | `rendv/tr_cmds.o` | `defaults` | `current-native-context-native-rendv-tr_cmds.diff.gz` |
| native | `rendv/tr_curve.o` | `defaults` | `current-native-context-native-rendv-tr_curve.diff.gz` |
| native | `rendv/tr_image.o` | `defaults` | `current-native-context-native-rendv-tr_image.diff.gz` |
| native | `rendv/tr_image_tga.o` | `defaults` | `current-native-context-native-rendv-tr_image_tga.diff.gz` |
| native | `rendv/tr_init.o` | `defaults` | `current-native-context-native-rendv-tr_init.diff.gz` |
| native | `rendv/tr_light.o` | `defaults` | `current-native-context-native-rendv-tr_light.diff.gz` |
| native | `rendv/tr_main.o` | `defaults` | `current-native-context-native-rendv-tr_main.diff.gz` |
| native | `rendv/tr_mesh.o` | `defaults` | `current-native-context-native-rendv-tr_mesh.diff.gz` |
| native | `rendv/tr_model.o` | `defaults` | `current-native-context-native-rendv-tr_model.diff.gz` |
| native | `rendv/tr_model_iqm.o` | `defaults` | `current-native-context-native-rendv-tr_model_iqm.diff.gz` |
| native | `rendv/tr_noise.o` | `defaults` | `current-native-context-native-rendv-tr_noise.diff.gz` |
| native | `rendv/tr_scene.o` | `defaults` | `current-native-context-native-rendv-tr_scene.diff.gz` |
| native | `rendv/tr_shade.o` | `defaults` | `current-native-context-native-rendv-tr_shade.diff.gz` |
| native | `rendv/tr_shade_calc.o` | `defaults` | `current-native-context-native-rendv-tr_shade_calc.diff.gz` |
| native | `rendv/tr_shader.o` | `defaults` | `current-native-context-native-rendv-tr_shader.diff.gz` |
| native | `rendv/tr_shadows.o` | `defaults` | `current-native-context-native-rendv-tr_shadows.diff.gz` |
| native | `rendv/tr_sky.o` | `defaults` | `current-native-context-native-rendv-tr_sky.diff.gz` |
| native | `rendv/tr_surface.o` | `defaults` | `current-native-context-native-rendv-tr_surface.diff.gz` |
| native | `rendv/tr_world.o` | `defaults` | `current-native-context-native-rendv-tr_world.diff.gz` |
| native | `rendv/vk.o` | `defaults` | `current-native-context-native-rendv-vk.diff.gz` |
| native | `rendv/vk_flares.o` | `defaults` | `current-native-context-native-rendv-vk_flares.diff.gz` |
| native | `rendv/vk_vbo.o` | `defaults` | `current-native-context-native-rendv-vk_vbo.diff.gz` |
| native-nosdl | `client/be_aas_bspq3.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_bspq3.diff.gz` |
| native-nosdl | `client/be_aas_cluster.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_cluster.diff.gz` |
| native-nosdl | `client/be_aas_debug.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_debug.diff.gz` |
| native-nosdl | `client/be_aas_file.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_file.diff.gz` |
| native-nosdl | `client/be_aas_reach.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_reach.diff.gz` |
| native-nosdl | `client/be_aas_route.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_route.diff.gz` |
| native-nosdl | `client/be_aas_routealt.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_routealt.diff.gz` |
| native-nosdl | `client/be_aas_sample.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_aas_sample.diff.gz` |
| native-nosdl | `client/be_ai_char.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_ai_char.diff.gz` |
| native-nosdl | `client/be_ai_chat.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_ai_chat.diff.gz` |
| native-nosdl | `client/be_ai_move.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-be_ai_move.diff.gz` |
| native-nosdl | `client/cl_avi.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_avi.diff.gz` |
| native-nosdl | `client/cl_cgame.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_cgame.diff.gz` |
| native-nosdl | `client/cl_cin.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_cin.diff.gz` |
| native-nosdl | `client/cl_console.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_console.diff.gz` |
| native-nosdl | `client/cl_curl.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_curl.diff.gz` |
| native-nosdl | `client/cl_input.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_input.diff.gz` |
| native-nosdl | `client/cl_jpeg.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_jpeg.diff.gz` |
| native-nosdl | `client/cl_keys.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_keys.diff.gz` |
| native-nosdl | `client/cl_main.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_main.diff.gz` |
| native-nosdl | `client/cl_net_chan.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_net_chan.diff.gz` |
| native-nosdl | `client/cl_parse.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_parse.diff.gz` |
| native-nosdl | `client/cl_scrn.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_scrn.diff.gz` |
| native-nosdl | `client/cl_ui.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cl_ui.diff.gz` |
| native-nosdl | `client/cm_load.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cm_load.diff.gz` |
| native-nosdl | `client/cm_patch.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cm_patch.diff.gz` |
| native-nosdl | `client/cm_trace.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cm_trace.diff.gz` |
| native-nosdl | `client/cmd.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cmd.diff.gz` |
| native-nosdl | `client/common.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-common.diff.gz` |
| native-nosdl | `client/cvar.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-cvar.diff.gz` |
| native-nosdl | `client/files.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-files.diff.gz` |
| native-nosdl | `client/history.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-history.diff.gz` |
| native-nosdl | `client/huffman.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-huffman.diff.gz` |
| native-nosdl | `client/keys.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-keys.diff.gz` |
| native-nosdl | `client/l_memory.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-l_memory.diff.gz` |
| native-nosdl | `client/l_precomp.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-l_precomp.diff.gz` |
| native-nosdl | `client/l_script.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-l_script.diff.gz` |
| native-nosdl | `client/l_struct.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-l_struct.diff.gz` |
| native-nosdl | `client/linux_glimp.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-linux_glimp.diff.gz` |
| native-nosdl | `client/linux_qgl.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-linux_qgl.diff.gz` |
| native-nosdl | `client/linux_snd.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-linux_snd.diff.gz` |
| native-nosdl | `client/md5.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-md5.diff.gz` |
| native-nosdl | `client/msg.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-msg.diff.gz` |
| native-nosdl | `client/net_chan.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-net_chan.diff.gz` |
| native-nosdl | `client/net_ip.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-net_ip.diff.gz` |
| native-nosdl | `client/puff.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-puff.diff.gz` |
| native-nosdl | `client/q_shared.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-q_shared.diff.gz` |
| native-nosdl | `client/qvm/vm.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-qvm-vm.diff.gz` |
| native-nosdl | `client/qvm/vm_interpreted.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-qvm-vm_interpreted.diff.gz` |
| native-nosdl | `client/qvm/vm_x86.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-qvm-vm_x86.diff.gz` |
| native-nosdl | `client/snd_dma.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-snd_dma.diff.gz` |
| native-nosdl | `client/snd_mem.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-snd_mem.diff.gz` |
| native-nosdl | `client/snd_mix.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-snd_mix.diff.gz` |
| native-nosdl | `client/sv_bot.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_bot.diff.gz` |
| native-nosdl | `client/sv_ccmds.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_ccmds.diff.gz` |
| native-nosdl | `client/sv_filter.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_filter.diff.gz` |
| native-nosdl | `client/sv_game.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_game.diff.gz` |
| native-nosdl | `client/sv_init.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_init.diff.gz` |
| native-nosdl | `client/sv_main.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_main.diff.gz` |
| native-nosdl | `client/sv_net_chan.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_net_chan.diff.gz` |
| native-nosdl | `client/sv_snapshot.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_snapshot.diff.gz` |
| native-nosdl | `client/sv_world.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-sv_world.diff.gz` |
| native-nosdl | `client/unix_main.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-unix_main.diff.gz` |
| native-nosdl | `client/unix_shared.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-unix_shared.diff.gz` |
| native-nosdl | `client/unzip.o` | `USE_SDL=0` | `current-native-context-native-nosdl-client-unzip.diff.gz` |
| native-nosdl | `ded/be_aas_bspq3.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_bspq3.diff.gz` |
| native-nosdl | `ded/be_aas_cluster.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_cluster.diff.gz` |
| native-nosdl | `ded/be_aas_debug.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_debug.diff.gz` |
| native-nosdl | `ded/be_aas_file.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_file.diff.gz` |
| native-nosdl | `ded/be_aas_reach.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_reach.diff.gz` |
| native-nosdl | `ded/be_aas_route.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_route.diff.gz` |
| native-nosdl | `ded/be_aas_routealt.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_routealt.diff.gz` |
| native-nosdl | `ded/be_aas_sample.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_aas_sample.diff.gz` |
| native-nosdl | `ded/be_ai_char.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_ai_char.diff.gz` |
| native-nosdl | `ded/be_ai_chat.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_ai_chat.diff.gz` |
| native-nosdl | `ded/be_ai_move.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-be_ai_move.diff.gz` |
| native-nosdl | `ded/cm_load.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-cm_load.diff.gz` |
| native-nosdl | `ded/cm_patch.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-cm_patch.diff.gz` |
| native-nosdl | `ded/cm_trace.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-cm_trace.diff.gz` |
| native-nosdl | `ded/cmd.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-cmd.diff.gz` |
| native-nosdl | `ded/common.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-common.diff.gz` |
| native-nosdl | `ded/cvar.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-cvar.diff.gz` |
| native-nosdl | `ded/files.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-files.diff.gz` |
| native-nosdl | `ded/history.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-history.diff.gz` |
| native-nosdl | `ded/huffman.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-huffman.diff.gz` |
| native-nosdl | `ded/keys.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-keys.diff.gz` |
| native-nosdl | `ded/l_memory.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-l_memory.diff.gz` |
| native-nosdl | `ded/l_precomp.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-l_precomp.diff.gz` |
| native-nosdl | `ded/l_script.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-l_script.diff.gz` |
| native-nosdl | `ded/l_struct.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-l_struct.diff.gz` |
| native-nosdl | `ded/md5.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-md5.diff.gz` |
| native-nosdl | `ded/msg.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-msg.diff.gz` |
| native-nosdl | `ded/net_chan.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-net_chan.diff.gz` |
| native-nosdl | `ded/net_ip.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-net_ip.diff.gz` |
| native-nosdl | `ded/q_shared.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-q_shared.diff.gz` |
| native-nosdl | `ded/qvm/vm.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-qvm-vm.diff.gz` |
| native-nosdl | `ded/qvm/vm_interpreted.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-qvm-vm_interpreted.diff.gz` |
| native-nosdl | `ded/qvm/vm_x86.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-qvm-vm_x86.diff.gz` |
| native-nosdl | `ded/sv_bot.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_bot.diff.gz` |
| native-nosdl | `ded/sv_ccmds.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_ccmds.diff.gz` |
| native-nosdl | `ded/sv_filter.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_filter.diff.gz` |
| native-nosdl | `ded/sv_game.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_game.diff.gz` |
| native-nosdl | `ded/sv_init.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_init.diff.gz` |
| native-nosdl | `ded/sv_main.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_main.diff.gz` |
| native-nosdl | `ded/sv_net_chan.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_net_chan.diff.gz` |
| native-nosdl | `ded/sv_snapshot.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_snapshot.diff.gz` |
| native-nosdl | `ded/sv_world.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-sv_world.diff.gz` |
| native-nosdl | `ded/unix_main.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-unix_main.diff.gz` |
| native-nosdl | `ded/unix_shared.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-unix_shared.diff.gz` |
| native-nosdl | `ded/unzip.o` | `USE_SDL=0` | `current-native-context-native-nosdl-ded-unzip.diff.gz` |
| native-nosdl | `rend1/puff.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-puff.diff.gz` |
| native-nosdl | `rend1/q_shared.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-q_shared.diff.gz` |
| native-nosdl | `rend1/tr_animation.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_animation.diff.gz` |
| native-nosdl | `rend1/tr_arb.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_arb.diff.gz` |
| native-nosdl | `rend1/tr_backend.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_backend.diff.gz` |
| native-nosdl | `rend1/tr_bsp.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_bsp.diff.gz` |
| native-nosdl | `rend1/tr_cmds.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_cmds.diff.gz` |
| native-nosdl | `rend1/tr_curve.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_curve.diff.gz` |
| native-nosdl | `rend1/tr_flares.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_flares.diff.gz` |
| native-nosdl | `rend1/tr_image.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_image.diff.gz` |
| native-nosdl | `rend1/tr_image_tga.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_image_tga.diff.gz` |
| native-nosdl | `rend1/tr_init.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_init.diff.gz` |
| native-nosdl | `rend1/tr_light.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_light.diff.gz` |
| native-nosdl | `rend1/tr_main.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_main.diff.gz` |
| native-nosdl | `rend1/tr_mesh.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_mesh.diff.gz` |
| native-nosdl | `rend1/tr_model.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_model.diff.gz` |
| native-nosdl | `rend1/tr_model_iqm.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_model_iqm.diff.gz` |
| native-nosdl | `rend1/tr_noise.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_noise.diff.gz` |
| native-nosdl | `rend1/tr_scene.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_scene.diff.gz` |
| native-nosdl | `rend1/tr_shade.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_shade.diff.gz` |
| native-nosdl | `rend1/tr_shade_calc.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_shade_calc.diff.gz` |
| native-nosdl | `rend1/tr_shader.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_shader.diff.gz` |
| native-nosdl | `rend1/tr_shadows.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_shadows.diff.gz` |
| native-nosdl | `rend1/tr_sky.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_sky.diff.gz` |
| native-nosdl | `rend1/tr_surface.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_surface.diff.gz` |
| native-nosdl | `rend1/tr_vbo.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_vbo.diff.gz` |
| native-nosdl | `rend1/tr_world.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rend1-tr_world.diff.gz` |
| native-nosdl | `rendv/puff.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-puff.diff.gz` |
| native-nosdl | `rendv/q_shared.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-q_shared.diff.gz` |
| native-nosdl | `rendv/tr_animation.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_animation.diff.gz` |
| native-nosdl | `rendv/tr_backend.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_backend.diff.gz` |
| native-nosdl | `rendv/tr_bsp.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_bsp.diff.gz` |
| native-nosdl | `rendv/tr_cmds.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_cmds.diff.gz` |
| native-nosdl | `rendv/tr_curve.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_curve.diff.gz` |
| native-nosdl | `rendv/tr_image.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_image.diff.gz` |
| native-nosdl | `rendv/tr_image_tga.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_image_tga.diff.gz` |
| native-nosdl | `rendv/tr_init.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_init.diff.gz` |
| native-nosdl | `rendv/tr_light.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_light.diff.gz` |
| native-nosdl | `rendv/tr_main.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_main.diff.gz` |
| native-nosdl | `rendv/tr_mesh.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_mesh.diff.gz` |
| native-nosdl | `rendv/tr_model.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_model.diff.gz` |
| native-nosdl | `rendv/tr_model_iqm.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_model_iqm.diff.gz` |
| native-nosdl | `rendv/tr_noise.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_noise.diff.gz` |
| native-nosdl | `rendv/tr_scene.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_scene.diff.gz` |
| native-nosdl | `rendv/tr_shade.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_shade.diff.gz` |
| native-nosdl | `rendv/tr_shade_calc.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_shade_calc.diff.gz` |
| native-nosdl | `rendv/tr_shader.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_shader.diff.gz` |
| native-nosdl | `rendv/tr_shadows.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_shadows.diff.gz` |
| native-nosdl | `rendv/tr_sky.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_sky.diff.gz` |
| native-nosdl | `rendv/tr_surface.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_surface.diff.gz` |
| native-nosdl | `rendv/tr_world.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-tr_world.diff.gz` |
| native-nosdl | `rendv/vk.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-vk.diff.gz` |
| native-nosdl | `rendv/vk_flares.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-vk_flares.diff.gz` |
| native-nosdl | `rendv/vk_vbo.o` | `USE_SDL=0` | `current-native-context-native-nosdl-rendv-vk_vbo.diff.gz` |
| native-nosdl | `client/linux_joystick.o` | `USE_SDL=0 CFLAGS=-DUSE_JOYSTICK` | `current-native-context-native-nosdl-client-linux_joystick.diff.gz` |
| static-opengl | `client/cl_main.o` | `USE_RENDERER_DLOPEN=0 RENDERER_DEFAULT=opengl` | `current-static-opengl-cl_main.diff.gz` |
| static-opengl | `rend1/tr_init.o` | `USE_RENDERER_DLOPEN=0 RENDERER_DEFAULT=opengl` | `current-static-opengl-tr_init.diff.gz` |
| static-vulkan | `client/cl_main.o` | `USE_RENDERER_DLOPEN=0 RENDERER_DEFAULT=vulkan` | `current-static-vulkan-cl_main.diff.gz` |
| static-vulkan | `rendv/tr_init.o` | `USE_RENDERER_DLOPEN=0 RENDERER_DEFAULT=vulkan` | `current-static-vulkan-tr_init.diff.gz` |

