#include "cg_local.h"

static vmCvar_t weaponTrace;
static qhandle_t weaponProjectileModels[WEAPON_MAX_DEFINITIONS];
static qhandle_t weaponImpacts[WEAPON_MAX_DEFINITIONS][WEAPON_MAX_ROWS];
static weaponState_t predictedWeapons[2];
static bool predictedWeaponValid[2];
static struct {
	bool valid;
	uint32_t spawn;
	int definition, attachments;
	weaponState_t state;
} weaponPredictionHistory[2][CMD_BACKUP];

void CG_InitWeapons( void ) {
	BG_ClearWeapons();
	memset( weaponPredictionHistory, 0, sizeof( weaponPredictionHistory ) );
	memset( weaponProjectileModels, 0, sizeof( weaponProjectileModels ) );
	memset( weaponImpacts, 0, sizeof( weaponImpacts ) );
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
		const auto *definition = BG_WeaponDefinition( index );
		weaponProjectileModels[index] = trap_R_RegisterModel( definition->projectile.model );
		if ( !weaponProjectileModels[index] )
			CG_Error( "Weapon rejected: projectile model %s", definition->projectile.model );
		for ( uint32_t material = 0; material < definition->materialCount; ++material ) {
			weaponImpacts[index][material] = trap_R_RegisterShader( definition->materials[material].effect );
			if ( !weaponImpacts[index][material] )
				CG_Error( "Weapon rejected: impact effect %s", definition->materials[material].effect );
		}
		CG_Printf( "Weapon client definition: index=%d name=%s\n", index, BG_WeaponDefinition( index )->name );
	}
	if ( BG_WeaponDefinition( 0 ) )
		cg.weaponSelect = 1;
}
void CG_WeaponSnapshot( const entityState_t *entity ) {
	weaponState_t state;
	uint32_t spawn;
	if ( !BG_EntityStateToWeapon( entity, &state, &spawn ) || !BG_WeaponDefinition( entity->modelindex ) )
		CG_Error( "Weapon rejected: snapshot record" );
	const auto &previous = weaponPredictionHistory[entity->otherEntityNum2][( state.time / 20 ) % CMD_BACKUP];
	if ( weaponTrace.integer && entity->otherEntityNum == cg.clientNum && previous.valid && previous.spawn == spawn &&
		 previous.definition == entity->modelindex && previous.attachments == entity->modelindex2 && previous.state.time == state.time )
		CG_Printf( "Weapon prediction: hand=%d tick=%u equal=%d\n", entity->otherEntityNum2, state.time, int( !memcmp( &state, &previous.state, sizeof( state ) ) ) );
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
		const uint32_t acknowledgedTime = state.time;
		// When input history is missing, retain the authoritative state until an acknowledgement catches up.
		if ( !cg.demoPlayback && !cg_nopredict.integer && !cg_synchronousClients.integer &&
			 int32_t( uint32_t( oldest.serverTime ) - state.time ) <= 20 ) {
			for ( int number = current - CMD_BACKUP + 1; number <= current; ++number ) {
				usercmd_t cmd;
				trap_GetUserCmd( number, &cmd );
				if ( int32_t( uint32_t( cmd.serverTime ) - state.time ) > 0 && int( cmd.weapon ) != entity.modelindex + 1 )
					break; // Selection waits for acknowledgement; inventory stays server-owned.
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
				if ( int32_t( state.time - acknowledgedTime ) > 0 ) {
					auto &prediction = weaponPredictionHistory[hand][( state.time / 20 ) % CMD_BACKUP];
					prediction.valid = true;
					prediction.spawn = spawn;
					prediction.definition = entity.modelindex;
					prediction.attachments = entity.modelindex2;
					prediction.state = state;
				}
			}
		}
		predictedWeapons[hand] = state;
		predictedWeaponValid[hand] = true;
	}
}

void CG_WeaponImpact( const entityState_t *entity, const vec3_t position ) {
	const auto *definition = BG_WeaponDefinition( entity->modelindex );
	if ( !definition || entity->modelindex2 < 0 || uint32_t( entity->modelindex2 ) >= definition->materialCount )
		CG_Error( "Weapon rejected: impact material" );
	const auto &material = definition->materials[entity->modelindex2];
	const qhandle_t shader = weaponImpacts[entity->modelindex][entity->modelindex2];
	if ( !shader )
		CG_Error( "Weapon rejected: impact effect %s", material.effect );
	vec3_t normal;
	ByteToDir( entity->eventParm, normal );
	if ( entity->generic1 ) {
		vec3_t origin;
		VectorCopy( position, origin );
		CG_MakeExplosion( origin, normal, 0, shader, 250, qtrue );
	} else {
		CG_ImpactMark( shader, position, normal, 0, 1, 1, 1, 1, qfalse, 4, qfalse );
	}
	if ( weaponTrace.integer )
		CG_Printf( "Weapon impact: material=%s target=%d\n", material.effect, entity->otherEntityNum2 );
}

bool CG_WeaponProjectile( centity_t *cent ) {
	const auto &state = cent->currentState;
	if ( state.generic1 != WEAPON_PROJECTILE_TAG )
		return false;
	if ( !BG_WeaponDefinition( state.modelindex ) || state.otherEntityNum < 0 || state.otherEntityNum >= MAX_CLIENTS ||
		 state.otherEntityNum2 < 0 || state.otherEntityNum2 > 1 || state.pos.trDuration < 0 || state.pos.trDuration > 60020 )
		CG_Error( "Weapon rejected: projectile snapshot" );
	if ( state.pos.trType != TR_LINEAR || state.pos.trDuration % 20 )
		CG_Error( "Weapon rejected: projectile clock" );
	for ( int axis = 0; axis < 3; ++axis )
		if ( !isfinite( state.pos.trBase[axis] ) || !isfinite( state.pos.trDelta[axis] ) )
			CG_Error( "Weapon rejected: projectile coordinates" );
	refEntity_t entity = {};
	entity.reType = RT_MODEL;
	entity.hModel = weaponProjectileModels[state.modelindex];
	AxisClear( entity.axis );
	VectorCopy( cent->lerpOrigin, entity.origin );
	VectorCopy( entity.origin, entity.oldorigin );
	VectorCopy( entity.origin, entity.lightingOrigin );
	trap_R_AddRefEntityToScene( &entity );
	if ( weaponTrace.integer && cent->miscTime != state.pos.trTime ) {
		CG_Printf( "Weapon projectile client: owner=%d hand=%d sequence=%u age=%d\n", state.otherEntityNum, state.otherEntityNum2,
			uint32_t( state.time2 ), state.pos.trDuration );
		cent->miscTime = state.pos.trTime;
	}
	return true;
}
