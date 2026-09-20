#include "cg_local.h"

static vmCvar_t weaponTrace;
static qhandle_t weaponProjectileModels[WEAPON_MAX_DEFINITIONS];
static qhandle_t weaponImpacts[WEAPON_MAX_DEFINITIONS][WEAPON_MAX_ROWS];
static weaponState_t predictedWeapons[2];
static bool predictedWeaponValid[2];
static animState_t predictedWeaponAnimations[2];
static float predictedWeaponParameters[2][ANIM_MAX_PARAMETERS];
static int predictedWeaponDefinitions[2];
static uint32_t predictedWeaponAttachments[2];
static struct {
	bool valid;
	uint32_t spawn;
	int definition, attachments;
	weaponState_t state;
	animState_t animation;
	float parameters[ANIM_MAX_PARAMETERS];
} weaponPredictionHistory[2][CMD_BACKUP];

static struct {
	bool valid;
	int number;
	playerState_t state;
} weaponCommandPoses[CMD_BACKUP];
struct predictedProjectile_t {
	bool used, acknowledged, expired;
	int command, definition, hand;
	uint32_t spawn, sequence, clock;
	weaponProjectile_t state, history[CMD_BACKUP];
};
// ponytail: 64 local predictions; excess shots still render from authoritative snapshots.
static predictedProjectile_t predictedProjectiles[64];

void CG_WeaponPredictionPose( int number, const playerState_t *state ) {
	if ( !BG_WeaponDefinition( 0 ) )
		return;
	auto &pose = weaponCommandPoses[uint32_t( number ) % CMD_BACKUP];
	pose.valid = true;
	pose.number = number;
	pose.state = *state;
}
static void PredictProjectile( int index, int hand, uint32_t spawn, int number, const weaponDef_t *definition, const weaponEvent_t &event, const playerState_t *pose ) {
	predictedProjectile_t *available = nullptr;
	for ( auto &record : predictedProjectiles ) {
		if ( record.used && record.spawn == spawn && record.definition == index && record.hand == hand && record.sequence == event.sequence )
			return;
		if ( !record.used && !available )
			available = &record;
	}
	if ( !available )
		return;
	*available = {};
	available->used = true;
	available->definition = index;
	available->hand = hand;
	available->spawn = spawn;
	available->sequence = event.sequence;
	available->command = number;
	available->clock = event.time;
	BG_LaunchWeaponProjectile( definition, &event, pose, &available->state );
	available->history[0] = available->state;
	if ( weaponTrace.integer )
		CG_Printf( "Weapon projectile predicted: hand=%d sequence=%u tick=%u\n", hand, event.sequence, event.time );
}
static void AdvanceProjectile( predictedProjectile_t *record, uint32_t time ) {
	const auto *definition = BG_WeaponDefinition( record->definition );
	for ( int step = 0; !record->expired && step < 50 && int32_t( time - record->clock ) >= 20; ++step ) {
		trace_t impact;
		record->expired = BG_WeaponProjectileStep( definition, &record->state, cg.clientNum, CG_Trace, &impact ) == WEAPON_EXPLODED;
		record->clock += 20;
		record->history[( record->state.ageMs / 20 ) % CMD_BACKUP] = record->state;
	}
}
static void DrawWeaponProjectile( int index, const vec3_t origin ) {
	refEntity_t entity = {};
	entity.reType = RT_MODEL;
	entity.hModel = weaponProjectileModels[index];
	AxisClear( entity.axis );
	VectorCopy( origin, entity.origin );
	VectorCopy( origin, entity.oldorigin );
	VectorCopy( origin, entity.lightingOrigin );
	trap_R_AddRefEntityToScene( &entity );
}
void CG_AddWeaponProjectiles( void ) {
	const int current = trap_GetCurrentCmdNumber();
	for ( auto &record : predictedProjectiles ) {
		if ( !record.used )
			continue;
		if ( record.acknowledged || record.expired ) {
			if ( uint32_t( current ) - uint32_t( record.command ) >= CMD_BACKUP )
				record.used = false;
			continue;
		}
		AdvanceProjectile( &record, uint32_t( cg.time ) );
		if ( !record.expired )
			DrawWeaponProjectile( record.definition, record.state.position );
	}
}

void CG_InitWeapons( void ) {
	BG_ClearWeapons();
	memset( weaponCommandPoses, 0, sizeof( weaponCommandPoses ) );
	memset( predictedProjectiles, 0, sizeof( predictedProjectiles ) );
	memset( weaponPredictionHistory, 0, sizeof( weaponPredictionHistory ) );
	memset( weaponProjectileModels, 0, sizeof( weaponProjectileModels ) );
	memset( weaponImpacts, 0, sizeof( weaponImpacts ) );
	memset( predictedWeaponValid, 0, sizeof( predictedWeaponValid ) );
	trap_Cvar_Register( &weaponTrace, "cg_weaponTrace", "0", 0 );
	for ( int index = 0; index < int( WEAPON_MAX_DEFINITIONS ); ++index ) {
		char text[MAX_QPATH + 132], path[MAX_QPATH], expected[65], actual[65], expectedGraph[65], actualGraph[65];
		Q_strncpyz( text, CG_ConfigString( CS_WEAPONS + index ), sizeof( text ) );
		if ( !text[0] )
			continue;
		char *cursor = text;
		Q_strncpyz( path, COM_Parse( &cursor ), sizeof( path ) );
		Q_strncpyz( expected, COM_Parse( &cursor ), sizeof( expected ) );
		Q_strncpyz( expectedGraph, COM_Parse( &cursor ), sizeof( expectedGraph ) );
		if ( !BG_LoadWeapon( index, path, actual, actualGraph ) || strcmp( actual, expected ) || strcmp( actualGraph, expectedGraph ) )
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
	if ( weaponTrace.integer && entity->modelindex2 )
		CG_Printf( "Weapon attachment client: owner=%d hand=%d definition=%d mask=%d\n", entity->otherEntityNum, entity->otherEntityNum2,
			entity->modelindex, entity->modelindex2 );
	trap_Cvar_Update( &weaponTrace );
	if ( weaponTrace.integer )
		CG_Printf( "Weapon client state: owner=%d hand=%d tick=%u sequence=%u magazine=%u reserve=%u chamber=%u ads=%u\n", entity->otherEntityNum,
			entity->otherEntityNum2, state.time, state.sequence, state.magazine, state.reserve, state.chamber, state.adsQ16 );
}
void CG_WeaponAnimationSnapshot( const entityState_t *entity ) {
	animState_t state;
	float parameters[ANIM_MAX_PARAMETERS];
	uint32_t spawn;
	if ( !BG_EntityStateToWeaponAnimation( entity, &state, parameters, &spawn ) || !BG_WeaponAnimation( entity->weapon ) )
		CG_Error( "Weapon rejected: animation record" );
	const auto &previous = weaponPredictionHistory[entity->otherEntityNum2][( state.lastTime / 20 ) % CMD_BACKUP];
	if ( weaponTrace.integer && entity->otherEntityNum == cg.clientNum && previous.valid && previous.spawn == spawn &&
		 previous.definition == entity->weapon && previous.attachments == entity->modelindex && previous.state.time == state.lastTime )
		CG_Printf( "Weapon animation prediction: hand=%d tick=%u equal=%d\n", entity->otherEntityNum2, state.lastTime,
			int( !memcmp( &state, &previous.animation, sizeof( state ) ) && !memcmp( parameters, previous.parameters, sizeof( parameters ) ) ) );
	if ( weaponTrace.integer )
		CG_Printf( "Weapon animation client: owner=%d hand=%d state=%s tick=%u sequence=%u\n", entity->otherEntityNum,
			entity->otherEntityNum2, Anim_StateName( BG_WeaponAnimation( entity->weapon ), state.current ), state.lastTime, state.eventSequence );
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
		animState_t animation = {};
		float parameters[ANIM_MAX_PARAMETERS] = {};
		bool haveAnimation = false;
		for ( int a = 0; a < snapshot->numEntities; ++a ) {
			const auto &record = snapshot->entities[a];
			uint32_t animationSpawn;
			if ( record.eType == ET_WEAPON_ANIMATION && record.otherEntityNum == entity.otherEntityNum && record.otherEntityNum2 == hand &&
				 record.weapon == entity.modelindex && record.modelindex == entity.modelindex2 &&
				 BG_EntityStateToWeaponAnimation( &record, &animation, parameters, &animationSpawn ) && animationSpawn == spawn && animation.lastTime == state.time ) {
				haveAnimation = true;
				break;
			}
		}
		if ( !haveAnimation )
			continue; // Wait for a matching pair when the snapshot budget splits auxiliary records.

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
				const auto &pose = weaponCommandPoses[uint32_t( number ) % CMD_BACKUP];
				const bool havePose = pose.valid && pose.number == number && uint32_t( pose.state.persistant[PERS_SPAWN_COUNT] ) == spawn;
				if ( int32_t( uint32_t( cmd.serverTime ) - state.time ) > 1000 )
					break;
				for ( int step = 0; step < 50 && int32_t( uint32_t( cmd.serverTime ) - state.time ) >= 20; ++step ) {
					weaponEvents_t events;
					animEvents_t notifies;
					if ( !Weapon_Tick( &definition, BG_WeaponButtons( &cmd, hand, havePose ? &pose.state : &snapshot->ps ), state.time + 20, &state, &events ) ||
						 !BG_WeaponAnimationStep( BG_WeaponAnimation( entity.modelindex ), &state, &events, &animation, parameters, &notifies ) )
						CG_Error( "Weapon rejected: predicted animation" );
					if ( havePose && definition.ballistics == WEAPON_PROJECTILE )
						for ( uint32_t i = 0; i < events.count; ++i )
							if ( events.items[i].kind == WEAPON_SHOT )
								PredictProjectile( entity.modelindex, hand, spawn, number, &definition, events.items[i], &pose.state );
				}

				if ( int32_t( state.time - acknowledgedTime ) > 0 ) {
					auto &prediction = weaponPredictionHistory[hand][( state.time / 20 ) % CMD_BACKUP];
					prediction.valid = true;
					prediction.spawn = spawn;
					prediction.definition = entity.modelindex;
					prediction.attachments = entity.modelindex2;
					prediction.state = state;
					prediction.animation = animation;
					memcpy( prediction.parameters, parameters, sizeof( parameters ) );
				}
			}
		}
		predictedWeaponAnimations[hand] = animation;
		memcpy( predictedWeaponParameters[hand], parameters, sizeof( parameters ) );
		predictedWeaponDefinitions[hand] = entity.modelindex;
		predictedWeaponAttachments[hand] = uint32_t( entity.modelindex2 );
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
	if ( state.otherEntityNum == cg.clientNum )
		for ( auto &record : predictedProjectiles ) {
			if ( !record.used || record.acknowledged || record.definition != state.modelindex || record.hand != state.otherEntityNum2 ||
				 record.spawn != uint32_t( state.time ) || record.sequence != uint32_t( state.time2 ) )
				continue;
			AdvanceProjectile( &record, uint32_t( state.pos.trTime ) );
			const auto &sample = record.history[( uint32_t( state.pos.trDuration ) / 20 ) % CMD_BACKUP];
			if ( weaponTrace.integer && sample.ageMs == uint32_t( state.pos.trDuration ) ) {
				vec3_t delta;
				VectorSubtract( sample.position, state.pos.trBase, delta );
				CG_Printf( "Weapon projectile correction: hand=%d sequence=%u error=%.6f\n", record.hand, record.sequence, double( VectorLength( delta ) ) );
			}
			record.acknowledged = true;
		}
	DrawWeaponProjectile( state.modelindex, cent->lerpOrigin );
	if ( weaponTrace.integer && cent->miscTime != state.pos.trTime ) {
		CG_Printf( "Weapon projectile client: owner=%d hand=%d sequence=%u age=%d\n", state.otherEntityNum, state.otherEntityNum2,
			uint32_t( state.time2 ), state.pos.trDuration );
		cent->miscTime = state.pos.trTime;
	}
	return true;
}
