# Native game objects; each source keeps its own translation unit.
NATIVE_GAME_SOURCES := \
  code/game/ai_chat.cpp \
  code/game/ai_cmd.cpp \
  code/game/ai_dmnet.cpp \
  code/game/ai_dmq3.cpp \
  code/game/ai_main.cpp \
  code/game/ai_team.cpp \
  code/game/ai_vcmd.cpp \
  code/game/bg_misc.cpp \
  code/game/bg_pmove.cpp \
  code/game/bg_slidemove.cpp \
  code/game/g_active.cpp \
  code/game/g_arenas.cpp \
  code/game/g_bot.cpp \
  code/game/g_client.cpp \
  code/game/g_cmds.cpp \
  code/game/g_combat.cpp \
  code/game/g_items.cpp \
  code/game/g_main.cpp \
  code/game/g_mem.cpp \
  code/game/g_misc.cpp \
  code/game/g_missile.cpp \
  code/game/g_mover.cpp \
  code/game/g_session.cpp \
  code/game/g_spawn.cpp \
  code/game/g_svcmds.cpp \
  code/game/g_native.cpp \
  code/game/g_target.cpp \
  code/game/g_team.cpp \
  code/game/g_trigger.cpp \
  code/game/g_utils.cpp \
  code/game/g_weapon.cpp \
  code/game/q_math.cpp \
  code/game/q_shared.cpp \
  code/game/bg_lib.cpp
NATIVE_CGAME_SOURCES := \
  code/game/bg_misc.cpp \
  code/game/bg_pmove.cpp \
  code/game/bg_slidemove.cpp \
  code/cgame/cg_consolecmds.cpp \
  code/cgame/cg_draw.cpp \
  code/cgame/cg_drawtools.cpp \
  code/cgame/cg_effects.cpp \
  code/cgame/cg_ents.cpp \
  code/cgame/cg_event.cpp \
  code/cgame/cg_info.cpp \
  code/cgame/cg_localents.cpp \
  code/cgame/cg_main.cpp \
  code/cgame/cg_marks.cpp \
  code/cgame/cg_players.cpp \
  code/cgame/cg_playerstate.cpp \
  code/cgame/cg_predict.cpp \
  code/cgame/cg_scoreboard.cpp \
  code/cgame/cg_servercmds.cpp \
  code/cgame/cg_snapshot.cpp \
  code/cgame/cg_native.cpp \
  code/cgame/cg_view.cpp \
  code/cgame/cg_weapons.cpp \
  code/game/q_math.cpp \
  code/game/q_shared.cpp \
  code/game/bg_lib.cpp
NATIVE_UI_SOURCES := \
  code/game/bg_misc.cpp \
  code/ui/ui_addbots.cpp \
  code/ui/ui_atoms.cpp \
  code/ui/ui_cdkey.cpp \
  code/ui/ui_cinematics.cpp \
  code/ui/ui_confirm.cpp \
  code/ui/ui_connect.cpp \
  code/ui/ui_controls2.cpp \
  code/ui/ui_credits.cpp \
  code/ui/ui_demo2.cpp \
  code/ui/ui_display.cpp \
  code/ui/ui_gameinfo.cpp \
  code/ui/ui_ingame.cpp \
  code/ui/ui_loadconfig.cpp \
  code/ui/ui_main.cpp \
  code/ui/ui_menu.cpp \
  code/ui/ui_mfield.cpp \
  code/ui/ui_mods.cpp \
  code/ui/ui_network.cpp \
  code/ui/ui_options.cpp \
  code/ui/ui_playermodel.cpp \
  code/ui/ui_players.cpp \
  code/ui/ui_playersettings.cpp \
  code/ui/ui_preferences.cpp \
  code/ui/ui_qmenu.cpp \
  code/ui/ui_removebots.cpp \
  code/ui/ui_saveconfig.cpp \
  code/ui/ui_serverinfo.cpp \
  code/ui/ui_servers2.cpp \
  code/ui/ui_setup.cpp \
  code/ui/ui_sound.cpp \
  code/ui/ui_sparena.cpp \
  code/ui/ui_specifyserver.cpp \
  code/ui/ui_splevel.cpp \
  code/ui/ui_sppostgame.cpp \
  code/ui/ui_spskill.cpp \
  code/ui/ui_startserver.cpp \
  code/ui/ui_native.cpp \
  code/ui/ui_team.cpp \
  code/ui/ui_teamorders.cpp \
  code/ui/ui_video.cpp \
  code/game/q_math.cpp \
  code/game/q_shared.cpp \
  code/game/bg_lib.cpp

NATIVE_WARNINGS = -Wall -Wextra -Werror
ifneq ($(findstring clang,$(CXX)),)
  NATIVE_WARNINGS += -Wno-missing-field-initializers -Wno-null-pointer-subtraction -Wno-parentheses-equality -Wno-pointer-bool-conversion -Wno-self-assign -Wno-sign-compare -Wno-unneeded-internal-declaration -Wno-unused-but-set-variable -Wno-unused-const-variable -Wno-unused-parameter
else
  NATIVE_WARNINGS += -Wno-address -Wno-array-bounds -Wno-implicit-fallthrough -Wno-missing-field-initializers -Wno-sign-compare -Wno-unused-but-set-variable -Wno-unused-const-variable -Wno-unused-parameter
endif
# Same Apple SDK C deprecations already frozen for the engine.
ifeq ($(PLATFORM),darwin)
  NATIVE_WARNINGS += -Wno-deprecated-declarations
endif
NATIVE_CFLAGS = $(filter-out -Wstrict-prototypes -Wimplicit -ffast-math,$(CFLAGS)) \
  -std=c++20 -fno-exceptions -fno-rtti -fno-fast-math -ffp-contract=off \
  -fno-strict-aliasing -fwrapv -fno-builtin -fPIC -U_GNU_SOURCE -D_DEFAULT_SOURCE \
  $(NATIVE_WARNINGS)

# A shared wrapper gives each module a namespace without amalgamating its sources.
define NATIVE_OBJECT
$(B)/native/$(1)-$(notdir $(2:.cpp=.o)): code/native/module.cpp $(2)
	$(Q)$(MKDIR) $$(dir $$@)
	$(echo_cmd) "NATIVE_CC $(2)"
	$(Q)$(ENGINE_CC) $(NATIVE_CFLAGS) -D$(3) -DNATIVE_NAMESPACE=$(1) \
	  '-DNATIVE_SOURCE="../$(patsubst code/%,%,$(2))"' \
	  $(if $(filter %/$(4)_main.cpp,$(2)),'-DNATIVE_EXPORTS="../$(1)/$(4)_native_exports.inc"') \
	  $(if $(and $(findstring clang,$(CXX)),$(filter %/bg_lib.cpp,$(2))),-D__NO_INLINE__) \
	  -o $$@ -c code/native/module.cpp
endef
$(foreach source,$(NATIVE_GAME_SOURCES),$(eval $(call NATIVE_OBJECT,game,$(source),QAGAME,g)))
$(foreach source,$(NATIVE_CGAME_SOURCES),$(eval $(call NATIVE_OBJECT,cgame,$(source),CGAME,cg)))
$(foreach source,$(NATIVE_UI_SOURCES),$(eval $(call NATIVE_OBJECT,ui,$(source),UI,ui)))

NATIVE_GAME_OBJECTS = $(addprefix $(B)/native/game-,$(notdir $(NATIVE_GAME_SOURCES:.cpp=.o)))
NATIVE_CGAME_OBJECTS = $(addprefix $(B)/native/cgame-,$(notdir $(NATIVE_CGAME_SOURCES:.cpp=.o)))
NATIVE_UI_OBJECTS = $(addprefix $(B)/native/ui-,$(notdir $(NATIVE_UI_SOURCES:.cpp=.o)))

-include $(wildcard $(B)/native/*.d)
