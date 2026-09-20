#include "../../engine/devtools/dev_debug.cpp"
#include <assert.h>

static refdef_t view;
static devGameTools_t game;
static devEntity_t entity;
static float worldFraction = 1;
int DevTools_ViewClient( void ) {
	return 0;
}
const refdef_t *DevTools_View( void ) {
	return &view;
}
const devGameTools_t *DevTools_Game( void ) {
	return &game;
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
void QDECL Com_Printf( const char *, ... ) {
}
void CM_BoxTrace( trace_t *trace, const vec3_t, const vec3_t, const vec3_t, const vec3_t,
	clipHandle_t, int, qboolean ) {
	trace->fraction = worldFraction;
}
void CM_DeveloperSurfaces( const float *, float, void ( *line )( const float *, const float *, uint32_t ) ) {
	const float a[3] = { 10, 0, 0 }, b[3] = { 20, 0, 0 };
	for ( int i = 0; i < 5000; ++i )
		line( a, b, 1 );
}
void Bot_DeveloperNavigation( const float *, float, void ( *line )( const float *, const float *, uint32_t ) ) {
	const float a[3] = { 20, 0, 0 }, b[3] = { 30, 0, 0 };
	for ( int i = 0; i < 5000; ++i )
		line( a, b, 2 );
}
static bool ReadEntity( int index, devEntity_t *out ) {
	*out = entity;
	return index == MAX_CLIENTS;
}
int main() {
	const float a[3] = { 10, 0, 0 }, b[3] = { 20, 10, 10 };
	const devLine_t *output;
	const devText_t *labels;
	Dev_DrawLine( a, b, 1, 100 );
	assert( DevTools_Lines( &output, false ) == 0 );
	DevTools_BeginDebugFrame( true, UINT32_MAX - 50 );
	Dev_DrawBox( a, b, 2, 100 );
	assert( DevTools_Lines( &output, false ) == 12 );
	Dev_DrawText( a, "persistent", 3, 100 );
	assert( DevTools_Text( &labels ) == 1 && !strcmp( labels[0].text, "persistent" ) );
	DevTools_BeginDebugFrame( true, 20 );
	assert( DevTools_Lines( &output, false ) == 12 && DevTools_Text( &labels ) == 1 );
	DevTools_BeginDebugFrame( true, 50 );
	assert( DevTools_Lines( &output, false ) == 0 && DevTools_Text( &labels ) == 0 );
	for ( int i = 0; i < 2050; ++i )
		Dev_DrawLine( a, b, 1, 0 );
	assert( DevTools_Lines( &output, false ) == 2048 && DevTools_DebugDropped() == 2 );
	DevTools_BeginDebugFrame( true, 50 );
	assert( DevTools_Lines( &output, false ) == 0 );
	DevTools_RebuildWorld( true, true, 256 );
	assert( DevTools_Lines( &output, true ) == 4096 && output[0].color == 1 && output[2048].color == 2 );
	assert( DevTools_DebugDropped() == 5904 );
	DevTools_ClearWorld();
	assert( DevTools_Lines( &output, true ) == 0 );
	view.width = 640;
	view.height = 480;
	view.fov_x = view.fov_y = 90;
	AxisClear( view.viewaxis );
	float screen[2];
	assert( DevTools_Project( &view, a, screen ) && fabsf( screen[0] - 320 ) < 0.01f && fabsf( screen[1] - 240 ) < 0.01f );
	const float behind[3] = { -1, 0, 0 };
	assert( !DevTools_Project( &view, behind, screen ) );
	game.ReadEntity = ReadEntity;
	entity.origin[0] = 100;
	assert( DevTools_PickEntity( 320, 240 ) == MAX_CLIENTS );
	assert( DevTools_PickEntity( 0, 0 ) == -1 );
	worldFraction = 0.001f;
	assert( DevTools_PickEntity( 320, 240 ) == -1 );
}
