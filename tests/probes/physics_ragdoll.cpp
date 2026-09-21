#include "engine/physics/physics_public.h"
#include "engine/animation/animation_public.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include "physics_allocations.h"

alignas( 64 ) static unsigned char arena[128 * 1024 * 1024];
static unsigned char assetBytes[1024 * 1024];
static void Fatal() {
	std::abort();
}
int main( int argc, char **argv ) {
	assert(argc==2);
	FILE *file = std::fopen( argv[1], "rb" );
	assert(file);
	size_t bytes = std::fread( assetBytes, 1, sizeof( assetBytes ), file );
	assert(bytes && !std::ferror(file) && std::feof(file));
	std::fclose( file );
	animAsset_t asset;
	assert(Anim_Open(assetBytes,bytes,&asset));
	animState_t state;
	float parameters[ANIM_MAX_PARAMETERS];
	animPose_t pose;
	Anim_Reset( &asset, 0, &state );
	Anim_DefaultParameters( &asset, parameters );
	assert(Anim_Evaluate(&asset,&state,parameters,0,&pose));
	for ( unsigned restart = 0; restart < 2; ++restart ) {
		assert(Phys_Init(arena,sizeof(arena),Fatal));
		physBodyDesc_t floor{};
		floor.transform.rotation[3] = 1;
		floor.transform.position[2] = -1;
		floor.halfExtent[0] = floor.halfExtent[1] = 50;
		floor.halfExtent[2] = 1;
		assert(Phys_Prepare(&floor)==0);
		uint32_t rigs[4];
		for ( auto &rig : rigs ) {
			rig = Phys_PrepareRagdoll( &asset );
			assert(rig!=PHYS_INVALID_BODY);
		}
		assert(Phys_PrepareRagdoll(&asset)==PHYS_INVALID_BODY);
		assert(Phys_Start());
		const auto baseline = Phys_Stats();
		assert(cppAllocations==0);
		for ( unsigned cycle = 0; cycle < 3; ++cycle ) {
			for ( unsigned i = 0; i < 4; ++i ) {
				const float origin[3] = { float( i ) * 128, 0, 48 }, axis[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } }, velocity[3] = { 30, 0, 0 };
				assert(Phys_SpawnRagdoll(rigs[i],&pose,origin,axis,velocity));
			}
			for ( unsigned step = 0; step < 180; ++step )
				assert(Phys_Step());
			for ( auto rig : rigs ) {
				animPose_t result;
				float origin[3];
				assert(Phys_RagdollPose(rig,&result,origin));
				assert(result.jointCount==pose.jointCount && origin[2]<48);
				for ( unsigned i = 0; i < result.jointCount; ++i )
					for ( float value : result.world[i] )
						assert(std::isfinite(value));
				assert(Phys_DespawnRagdoll(rig));
				assert(!Phys_RagdollPose(rig,&result,origin));
			}
			const auto current = Phys_Stats();
			assert(current.allocations==baseline.allocations && current.used==baseline.used && cppAllocations==0);
		}
		Phys_Shutdown();
		assert(Phys_Stats().liveBlocks==0);
	}
	std::puts( "PASS: owned skeleton ragdolls, pose sampling, allocation-free activation/recycle and restart" );
}
