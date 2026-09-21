#include "cg_local.h"
#include "../../engine/physics/physics_public.h"

static struct {
	uint32_t slot;
	bool grenade;
	int expires;
} physicsProps[32];
static uint32_t physicsNextProp, physicsSteps;
static int64_t physicsTick;
static qhandle_t physicsGrenade;
#ifdef AFTERSHOCK_DEVTOOLS
static vmCvar_t physicsDebug;
#endif

void CG_InitPhysics() {
#ifdef AFTERSHOCK_DEVTOOLS
	trap_Cvar_Register( &physicsDebug, "cg_physicsDebug", "0", 0 );
#endif
	memset( physicsProps, 0, sizeof( physicsProps ) );
	physicsNextProp = physicsSteps = 0;
	physicsTick = -1;
	physicsGrenade = trap_R_RegisterModel( "models/ammo/grenade1.md3" );
	for ( uint32_t i = 0; i < ( sizeof( physicsProps ) / sizeof( *physicsProps ) ); ++i ) {
		auto &prop = physicsProps[i];
		prop.grenade = ( i & 1 ) != 0;
		physBodyDesc_t desc{};
		desc.dynamic = true;
		desc.transform.rotation[3] = 1;
		desc.radius = prop.grenade ? .1f : 0;
		desc.halfExtent[0] = desc.halfExtent[1] = desc.halfExtent[2] = .15f;
		desc.restitution = prop.grenade ? .6f : .1f;
		prop.slot = Phys_Prepare( &desc );
		if ( prop.slot == PHYS_INVALID_BODY )
			CG_Error( "Cosmetic prop pool exhausted during setup" );
	}
	CG_PrepareRagdolls();
	if ( !Phys_Start() )
		CG_Error( "Cosmetic physics start failed" );
}
void CG_PhysicsProp() {
	const bool grenade = !Q_stricmp( CG_Argv( 1 ), "grenade" );
	const uint32_t index = ( ( physicsNextProp++ % 16 ) * 2 ) + ( grenade ? 1 : 0 );
	auto &prop = physicsProps[index];
	physTransform_t pose{};
	pose.rotation[3] = 1;
	float velocity[3];
	for ( int i = 0; i < 3; ++i ) {
		pose.position[i] = ( cg.refdef.vieworg[i] + cg.refdef.viewaxis[0][i] * 32 ) * .0254f;
		velocity[i] = cg.refdef.viewaxis[0][i] * 8;
	}
	velocity[2] += 2;
	if ( !Q_stricmp( CG_Argv( 2 ), "drop" ) ) {
		VectorClear( velocity );
		for ( int i = 0; i < 3; ++i )
			pose.position[i] = cg.refdef.vieworg[i] * .0254f;
	}
	if ( Phys_Spawn( prop.slot, &pose, velocity ) ) {
		prop.expires = cg.time + 15000;
		CG_Printf( "Physics prop: slot=%u kind=%s cosmetic=1\n", prop.slot, grenade ? "grenade" : "box" );
	}
}
void CG_PhysicsStatus() {
	CG_RagdollStatus();
	const auto stats = Phys_Stats();
	CG_Printf( "Physics status: steps=%u arena=%u allocations=%u live=%u\n", physicsSteps, unsigned( stats.used ), stats.allocations, stats.liveBlocks );
	for ( const auto &prop : physicsProps ) {
		physTransform_t pose;
		if ( Phys_Transform( prop.slot, &pose ) ) {
			float velocity[3];
			Phys_Velocity( prop.slot, velocity );
			CG_Printf( "Physics body: slot=%u kind=%s position=%.6f,%.6f,%.6f velocity=%.6f,%.6f,%.6f\n", prop.slot, prop.grenade ? "grenade" : "box", pose.position[0], pose.position[1], pose.position[2], velocity[0], velocity[1], velocity[2] );
		}
	}
}
static void PhysicsAxis( const float q[4], vec3_t axis[3] ) {
	const float x = q[0], y = q[1], z = q[2], w = q[3];
	VectorSet( axis[0], 1 - 2 * ( y * y + z * z ), 2 * ( x * y + w * z ), 2 * ( x * z - w * y ) );
	VectorSet( axis[1], 2 * ( x * y - w * z ), 1 - 2 * ( x * x + z * z ), 2 * ( y * z + w * x ) );
	VectorSet( axis[2], 2 * ( x * z + w * y ), 2 * ( y * z - w * x ), 1 - 2 * ( x * x + y * y ) );
}
void CG_ClearPhysics() {
	for ( auto &prop : physicsProps ) {
		Phys_Despawn( prop.slot );
		prop.expires = 0;
	}
	CG_ClearRagdolls();
	physicsTick = -1;
}
void CG_AddPhysics() {
#ifdef AFTERSHOCK_DEVTOOLS
	trap_Cvar_Update( &physicsDebug );
#endif
	const int64_t tick = int64_t( cg.time ) * 60 / 1000;
	if ( physicsTick < 0 )
		physicsTick = tick;
	if ( tick < physicsTick || tick - physicsTick > 120 ) {
		CG_ClearPhysics();
		physicsTick = tick; // A seek/stall retires cosmetic state; never catch up without a bound.
	}
	while ( physicsTick < tick ) {
		if ( !Phys_Step() )
			CG_Error( "Cosmetic physics contact capacity exceeded" );
		++physicsTick;
		++physicsSteps;
	}
	static const int faces[6][4] = { { 0, 2, 3, 1 }, { 4, 5, 7, 6 }, { 0, 1, 5, 4 }, { 2, 6, 7, 3 }, { 0, 4, 6, 2 }, { 1, 3, 7, 5 } };
	for ( auto &prop : physicsProps ) {
		if ( prop.expires && cg.time >= prop.expires ) {
			Phys_Despawn( prop.slot );
			prop.expires = 0;
		}
		physTransform_t pose;
		if ( !Phys_Transform( prop.slot, &pose ) )
			continue;
		refEntity_t entity{};
		entity.reType = RT_MODEL;
		entity.hModel = physicsGrenade;
		PhysicsAxis( pose.rotation, entity.axis );
		for ( int i = 0; i < 3; ++i )
			entity.origin[i] = pose.position[i] / .0254f;
		VectorCopy( entity.origin, entity.oldorigin );
#ifdef AFTERSHOCK_DEVTOOLS
		if ( physicsDebug.integer ) {
			vec3_t mins, maxs;
			for ( int axis = 0; axis < 3; ++axis ) {
				const float extent = prop.grenade ? .1f : .15f * ( fabsf( entity.axis[0][axis] ) + fabsf( entity.axis[1][axis] ) + fabsf( entity.axis[2][axis] ) );
				mins[axis] = entity.origin[axis] - extent / .0254f;
				maxs[axis] = entity.origin[axis] + extent / .0254f;
			}
			Dev_DrawBox( mins, maxs, 0xff00ffffU, 0 );
		}
#endif
		memset( entity.shaderRGBA, 255, sizeof( entity.shaderRGBA ) );
		if ( prop.grenade && physicsGrenade ) {
			trap_R_AddRefEntityToScene( &entity );
			continue;
		}
		for ( const auto &face : faces ) {
			polyVert_t vertices[4]{};
			for ( int i = 0; i < 4; ++i ) {
				VectorCopy( entity.origin, vertices[i].xyz );
				for ( int axis = 0; axis < 3; ++axis )
					VectorMA( vertices[i].xyz, ( face[i] & ( 1 << axis ) ? 1.f : -1.f ) * .15f / .0254f, entity.axis[axis], vertices[i].xyz );
				vertices[i].st[0] = i == 1 || i == 2;
				vertices[i].st[1] = i >= 2;
				vertices[i].modulate[0] = 180;
				vertices[i].modulate[1] = 110;
				vertices[i].modulate[2] = 45;
				vertices[i].modulate[3] = 255;
			}
			trap_R_AddPolyToScene( cgs.media.whiteShader, 4, vertices );
		}
	}
}

bool CG_PhysicsDebugEnabled() {
#ifdef AFTERSHOCK_DEVTOOLS
	return physicsDebug.integer != 0;
#else
	return false;
#endif
}
