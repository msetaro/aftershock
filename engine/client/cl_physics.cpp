#include "client.h"
#include "../physics/physics_public.h"

static void *physicsStorage;
static bool physicsTerminating;
static constexpr size_t PHYSICS_ARENA_BYTES = 128 * 1024 * 1024;
static void PhysicsFatal() {
	// Sys_Error shuts down the client before exiting. Do not reenter Jolt or
	// retire its arena while an exhausted foreign allocation is on the stack.
	physicsTerminating = true;
	Sys_Error( "Cosmetic physics arena exhausted (%u MiB)", unsigned( PHYSICS_ARENA_BYTES / ( 1024 * 1024 ) ) );
}
void CL_ShutdownPhysics() {
	if ( physicsTerminating )
		return;
	Phys_Shutdown();
	if ( physicsStorage )
		Com_Printf( "Physics shutdown: live=%u\n", Phys_Stats().liveBlocks );
	if ( physicsStorage )
		Z_Free( physicsStorage );
	physicsStorage = nullptr;
}
struct physicsMap_t {
	physTriangle_t *triangles;
	uint32_t count;
};
static bool PhysicsTriangle( void *context, const float *a, const float *b, const float *c ) {
	auto *map = static_cast<physicsMap_t *>( context );
	if ( map->count == PHYS_MAX_TRIANGLES )
		return false;
	auto &triangle = map->triangles[map->count++];
	const float *points[] = { a, b, c };
	for ( unsigned i = 0; i < 3; ++i )
		for ( unsigned axis = 0; axis < 3; ++axis )
			triangle.vertex[i][axis] = points[i][axis] * .0254f;
	return true;
}
void CL_InitPhysicsMap() {
	CL_ShutdownPhysics();
	physicsStorage = Z_TagMalloc(PHYSICS_ARENA_BYTES, TAG_GENERAL);
	if ( !Phys_Init( physicsStorage, PHYSICS_ARENA_BYTES, PhysicsFatal ) ) {
		CL_ShutdownPhysics();
		Com_Error( ERR_DROP, "Cannot initialize cosmetic physics" );
	}
	physicsMap_t map = { static_cast<physTriangle_t *>( Hunk_AllocateTempMemory( sizeof( physTriangle_t ) * PHYS_MAX_TRIANGLES ) ), 0 };
	const bool exported = CM_PhysicsTriangles( PhysicsTriangle, &map );
	const uint32_t body = exported ? Phys_PrepareMesh( map.triangles, map.count ) : PHYS_INVALID_BODY;
	Hunk_FreeTempMemory( map.triangles );
	if ( body == PHYS_INVALID_BODY ) {
		CL_ShutdownPhysics();
		Com_Error( ERR_DROP, "Cosmetic physics world mesh rejected (limit %u triangles)", PHYS_MAX_TRIANGLES );
	}
	Com_Printf( "Physics world: %u solid triangles, %zu/%zu arena bytes\n", map.count, Phys_Stats().used, PHYSICS_ARENA_BYTES );
}
