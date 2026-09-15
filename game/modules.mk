# Native game objects; each source keeps its own translation unit.
NATIVE_GAME_SOURCES := \
  game/game/ai_chat.cpp \
  game/game/ai_cmd.cpp \
  game/game/ai_dmnet.cpp \
  game/game/ai_dmq3.cpp \
  game/game/ai_main.cpp \
  game/game/ai_team.cpp \
  game/game/ai_vcmd.cpp \
  game/bg/bg_misc.cpp \
  game/bg/bg_pmove.cpp \
  game/bg/bg_slidemove.cpp \
  game/game/g_active.cpp \
  game/game/g_arenas.cpp \
  game/game/g_bot.cpp \
  game/game/g_client.cpp \
  game/game/g_cmds.cpp \
  game/game/g_combat.cpp \
  game/game/g_items.cpp \
  game/game/g_main.cpp \
  game/game/g_mem.cpp \
  game/game/g_misc.cpp \
  game/game/g_missile.cpp \
  game/game/g_mover.cpp \
  game/game/g_session.cpp \
  game/game/g_spawn.cpp \
  game/game/g_svcmds.cpp \
  game/game/g_native.cpp \
  game/game/g_target.cpp \
  game/game/g_team.cpp \
  game/game/g_trigger.cpp \
  game/game/g_utils.cpp \
  game/game/g_weapon.cpp \
  game/bg/q_math.cpp \
  game/bg/q_shared.cpp \
  game/bg/bg_lib.cpp
NATIVE_CGAME_SOURCES := \
  game/bg/bg_misc.cpp \
  game/bg/bg_pmove.cpp \
  game/bg/bg_slidemove.cpp \
  game/cgame/cg_consolecmds.cpp \
  game/cgame/cg_draw.cpp \
  game/cgame/cg_drawtools.cpp \
  game/cgame/cg_effects.cpp \
  game/cgame/cg_ents.cpp \
  game/cgame/cg_event.cpp \
  game/cgame/cg_info.cpp \
  game/cgame/cg_localents.cpp \
  game/cgame/cg_main.cpp \
  game/cgame/cg_marks.cpp \
  game/cgame/cg_players.cpp \
  game/cgame/cg_playerstate.cpp \
  game/cgame/cg_predict.cpp \
  game/cgame/cg_scoreboard.cpp \
  game/cgame/cg_servercmds.cpp \
  game/cgame/cg_snapshot.cpp \
  game/cgame/cg_native.cpp \
  game/cgame/cg_view.cpp \
  game/cgame/cg_weapons.cpp \
  game/bg/q_math.cpp \
  game/bg/q_shared.cpp \
  game/bg/bg_lib.cpp
NATIVE_UI_SOURCES := \
  game/bg/bg_misc.cpp \
  game/ui/ui_addbots.cpp \
  game/ui/ui_atoms.cpp \
  game/ui/ui_cdkey.cpp \
  game/ui/ui_cinematics.cpp \
  game/ui/ui_confirm.cpp \
  game/ui/ui_connect.cpp \
  game/ui/ui_controls2.cpp \
  game/ui/ui_credits.cpp \
  game/ui/ui_demo2.cpp \
  game/ui/ui_display.cpp \
  game/ui/ui_gameinfo.cpp \
  game/ui/ui_ingame.cpp \
  game/ui/ui_loadconfig.cpp \
  game/ui/ui_main.cpp \
  game/ui/ui_menu.cpp \
  game/ui/ui_mfield.cpp \
  game/ui/ui_mods.cpp \
  game/ui/ui_network.cpp \
  game/ui/ui_options.cpp \
  game/ui/ui_playermodel.cpp \
  game/ui/ui_players.cpp \
  game/ui/ui_playersettings.cpp \
  game/ui/ui_preferences.cpp \
  game/ui/ui_qmenu.cpp \
  game/ui/ui_removebots.cpp \
  game/ui/ui_saveconfig.cpp \
  game/ui/ui_serverinfo.cpp \
  game/ui/ui_servers2.cpp \
  game/ui/ui_setup.cpp \
  game/ui/ui_sound.cpp \
  game/ui/ui_sparena.cpp \
  game/ui/ui_specifyserver.cpp \
  game/ui/ui_splevel.cpp \
  game/ui/ui_sppostgame.cpp \
  game/ui/ui_spskill.cpp \
  game/ui/ui_startserver.cpp \
  game/ui/ui_native.cpp \
  game/ui/ui_team.cpp \
  game/ui/ui_teamorders.cpp \
  game/ui/ui_video.cpp \
  game/bg/q_math.cpp \
  game/bg/q_shared.cpp \
  game/bg/bg_lib.cpp

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
$(B)/native/$(1)-$(notdir $(2:.cpp=.o)): game/module.cpp $(2)
	$(Q)$(MKDIR) $$(dir $$@)
	$(echo_cmd) "NATIVE_CC $(2)"
	$(Q)$(ENGINE_CC) $(NATIVE_CFLAGS) -D$(3) -DNATIVE_NAMESPACE=$(1) \
	  '-DNATIVE_SOURCE="$(patsubst game/%,%,$(2))"' \
	  $(if $(filter %/$(4)_main.cpp,$(2)),'-DNATIVE_EXPORTS="$(1)/$(4)_native_exports.inc"') \
	  $(if $(and $(findstring clang,$(CXX)),$(filter %/bg_lib.cpp,$(2))),-D__NO_INLINE__) \
	  -o $$@ -c game/module.cpp
endef
$(foreach source,$(NATIVE_GAME_SOURCES),$(eval $(call NATIVE_OBJECT,game,$(source),QAGAME,g)))
$(foreach source,$(NATIVE_CGAME_SOURCES),$(eval $(call NATIVE_OBJECT,cgame,$(source),CGAME,cg)))
$(foreach source,$(NATIVE_UI_SOURCES),$(eval $(call NATIVE_OBJECT,ui,$(source),UI,ui)))

NATIVE_GAME_OBJECTS = $(addprefix $(B)/native/game-,$(notdir $(NATIVE_GAME_SOURCES:.cpp=.o)))
NATIVE_CGAME_OBJECTS = $(addprefix $(B)/native/cgame-,$(notdir $(NATIVE_CGAME_SOURCES:.cpp=.o)))
NATIVE_UI_OBJECTS = $(addprefix $(B)/native/ui-,$(notdir $(NATIVE_UI_SOURCES:.cpp=.o)))

-include $(wildcard $(B)/native/*.d)
