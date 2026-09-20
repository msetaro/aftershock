#include "cg_local.h"

static vmCvar_t weaponTrace;
static weaponState_t predictedWeapons[2];
static bool predictedWeaponValid[2];

void CG_InitWeapons( void ) {
	BG_ClearWeapons();
	memset( predictedWeaponValid, 0, sizeof( predictedWeaponValid ) );
	trap_Cvar_Register( &weaponTrace, "cg_weaponTrace", "0", 0 );
	for ( int index = 0; index < int( WEAPON_MAX_DEFINITIONS ); ++index ) {
		char text[MAX_QPATH + 66], path[MAX_QPATH], expected[65], actual[65];
		Q_strncpyz( text, CG_ConfigString( CS_WEAPONS + index ), sizeof( text ) );
		if ( !text[0] )
			continue;
		char *cursor = text;
		Q_strncpyz( path, COM_Parse( &cursor ), sizeof( path ) );
		Q_strncpyz( expected, COM_Parse( &cursor ), sizeof( expected ) );
		if ( !BG_LoadWeapon( index, path, actual ) || strcmp( actual, expected ) )
			CG_Error( "Weapon rejected: server definition differs for %s", path );
		CG_Printf( "Weapon client definition: index=%d name=%s\n", index, BG_WeaponDefinition( index )->name );
	}
}
void CG_WeaponSnapshot( const entityState_t *entity ) {
	weaponState_t state;
	uint32_t spawn;
	if ( !BG_EntityStateToWeapon( entity, &state, &spawn ) || !BG_WeaponDefinition( entity->modelindex ) )
		CG_Error( "Weapon rejected: snapshot record" );
	trap_Cvar_Update( &weaponTrace );
	if ( weaponTrace.integer )
		CG_Printf( "Weapon client state: owner=%d hand=%d tick=%u sequence=%u magazine=%u reserve=%u chamber=%u ads=%u\n", entity->otherEntityNum,
			entity->otherEntityNum2, state.time, state.sequence, state.magazine, state.reserve, state.chamber, state.adsQ16 );
}
void CG_PredictWeapons( void ) {
	memset( predictedWeaponValid, 0, sizeof( predictedWeaponValid ) );
	if ( !BG_WeaponDefinition( 0 ) || !cg.snap )
		return;
	const snapshot_t *snapshot = cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ? cg.nextSnap : cg.snap;
	const int current = trap_GetCurrentCmdNumber();
	usercmd_t latest, oldest;
	trap_GetUserCmd( current, &latest );
	trap_GetUserCmd( current - CMD_BACKUP + 1, &oldest );
	for ( int e = 0; e < snapshot->numEntities; ++e ) {
		const auto &entity = snapshot->entities[e];
		if ( entity.eType != ET_WEAPON_STATE || entity.otherEntityNum != snapshot->ps.clientNum )
			continue;
		weaponState_t state;
		uint32_t spawn;
		if ( !BG_EntityStateToWeapon( &entity, &state, &spawn ) || spawn != uint32_t( snapshot->ps.persistant[PERS_SPAWN_COUNT] ) )
			continue;
		weaponDef_t definition;
		if ( !Weapon_Configure( BG_WeaponDefinition( entity.modelindex ), uint32_t( entity.modelindex2 ), &definition ) )
			CG_Error( "Weapon rejected: snapshot definition" );
		const int hand = entity.otherEntityNum2;
		// When input history is missing, retain the authoritative state until an acknowledgement catches up.
		if ( !cg.demoPlayback && !cg_nopredict.integer && !cg_synchronousClients.integer &&
			 int32_t( uint32_t( oldest.serverTime ) - state.time ) <= 20 ) {
			for ( int number = current - CMD_BACKUP + 1; number <= current; ++number ) {
				usercmd_t cmd;
				trap_GetUserCmd( number, &cmd );
				if ( cmd.serverTime > latest.serverTime )
					continue;
				if ( pmove_fixed.integer ) {
					const int step = pmove_msec.integer < 8 ? 8 : pmove_msec.integer > 33 ? 33
																						  : pmove_msec.integer;
					cmd.serverTime = ( ( cmd.serverTime + step - 1 ) / step ) * step;
				}
				weaponEvents_t events;
				if ( !Weapon_Command( &definition, BG_WeaponButtons( &cmd, hand, &snapshot->ps ), uint32_t( cmd.serverTime ), &state, &events ) )
					break;
			}
		}
		predictedWeapons[hand] = state;
		predictedWeaponValid[hand] = true;
	}
}
