#include <stdio.h>
#include <stddef.h>
#ifdef ENGINE
#include "../../engine/qcommon/q_shared.h"
#include "../../engine/qcommon/qcommon_public.h"
#include "../../engine/public/g_public.h"
#include "../../engine/renderercommon/tr_types_public.h"
#include "../../engine/public/cg_public.h"
#include "../../engine/public/ui_public.h"
#include "../../engine/botlib/botlib_public.h"
#include "../../engine/botlib/be_aas.h"
#include "../../engine/botlib/be_ai_goal.h"
#include "../../engine/botlib/be_ai_move.h"
#include "../../engine/botlib/be_ai_chat.h"
#include "../../engine/botlib/be_ai_weap.h"
#else
#include "../../game/bg/q_shared.h"
#include "../../engine/public/g_public.h"
#include "../../game/bg/tr_types_public.h"
#include "../../engine/public/cg_public.h"
#include "../../engine/public/ui_public.h"
#include "../../game/game/botlib.h"
#include "../../game/game/be_aas.h"
#include "../../game/game/be_ai_goal.h"
#include "../../game/game/be_ai_move.h"
#include "../../game/game/be_ai_chat.h"
#include "../../game/game/be_ai_weap.h"
#endif

#ifdef __cplusplus
#define ALIGN(T) alignof(T)
#else
#define ALIGN(T) _Alignof(T)
#endif
#define SHOW(T) printf(#T " %zu %zu\n", sizeof(T), ALIGN(T))
int main(void) {
    SHOW(entityState_t); SHOW(playerState_t); SHOW(entityShared_t);
    SHOW(vmCvar_t); SHOW(bot_entitystate_t); SHOW(bot_input_t);
    SHOW(bot_goal_t); SHOW(bot_initmove_t); SHOW(bot_moveresult_t); SHOW(bot_match_t);
    SHOW(weaponinfo_t); SHOW(aas_clientmove_t); SHOW(aas_entityinfo_t);
    SHOW(aas_areainfo_t); SHOW(aas_predictroute_t); SHOW(aas_altroutegoal_t);
    SHOW(pc_token_t); SHOW(qtime_t); SHOW(snapshot_t); SHOW(uiClientState_t);
    SHOW(glconfig_t); SHOW(fontInfo_t); SHOW(gameState_t); SHOW(orientation_t);
    SHOW(markFragment_t);
    SHOW(usercmd_t); SHOW(trace_t); SHOW(refEntity_t); SHOW(refdef_t);
    printf("entityShared.ownerNum %zu\n", offsetof(entityShared_t, ownerNum));
    printf("playerState.stats %zu\n", offsetof(playerState_t, stats));
    printf("refEntity.origin %zu\n", offsetof(refEntity_t, origin));
    printf("extension trap %d\n", G_TRAP_GETVALUE);
    return 0;
}
