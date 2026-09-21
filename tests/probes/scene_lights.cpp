// Native light submission owns a bounded copy; no wire or simulation changes.
#include "../../engine/render/tr_scene.cpp"
#include <assert.h>
#include <limits>

refimport_t ri;
trGlobals_t tr;
static backEndData_t storage;
backEndData_t *backEndData = &storage;

int main() {
	sceneLight_t light = {};
	light.radius = 512;
	light.color[0] = light.color[1] = light.color[2] = 1;
	light.intensity = 2;
	assert( !RE_AddSceneLight( &light ) );
	tr.registered = qtrue;
	R_InitNextFrame();
	assert( RE_AddSceneLight( &light ) );
	light.origin[0] = 64;
	assert( storage.sceneLights[0].light.origin[0] == 0 );
	assert( RE_AddSceneLight( &light ) ); // 12 of 16 local atlas tiles.
	assert( !RE_AddSceneLight( &light ) );
	assert( r_numSceneLights == 2 );
	light.type = sceneLightType_t::Spot;
	light.direction[2] = -3;
	light.innerCone = 25;
	light.outerCone = 40;
	for ( int i = 0; i < 4; ++i )
		assert( RE_AddSceneLight( &light ) );
	assert( !RE_AddSceneLight( &light ) && r_numSceneLights == 6 );
	assert( storage.sceneLights[2].light.direction[2] == -1 );
	RE_ClearScene();
	assert( r_firstSceneLight == 6 );
	assert( RE_AddSceneLight( &light ) );
	light.innerCone = 45;
	assert( !RE_AddSceneLight( &light ) && r_numSceneLights == 7 );
	light.innerCone = 25;
	light.radius = std::numeric_limits<float>::infinity();
	assert( !RE_AddSceneLight( &light ) && r_numSceneLights == 7 );
	light.radius = 512;
	for ( int i = 7; i < MAX_SCENE_LIGHTS; ++i )
		assert( RE_AddSceneLight( &light ) );
	assert( !RE_AddSceneLight( &light ) );
	R_InitNextFrame();
	assert( r_numSceneLights == 0 && r_firstSceneLight == 0 );
	assert( RE_AddSceneLight( &light ) );
	assert( storage.sceneLights[0].light.type == sceneLightType_t::Spot );
	assert( !RE_AddSceneLight( nullptr ) );
	puts( "PASS: native point/spot copies, tile admission, validation and per-frame/per-scene reset" );
}
