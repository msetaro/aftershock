#include "devtools_public.h"
#include "../qcommon/qcommon_public.h"
#include "../qcommon/cm_public.h"
#include "../botlib/botlib_public.h"
#include <cmath>

static devLine_t lines[2048], worldLines[4096];
static devText_t texts[128];
static uint32_t lineCount, worldCount, textCount, dropped, worldDropped, now;
static bool active;
static uint32_t worldLimit = ARRAY_LEN( worldLines );

void DevTools_BeginDebugFrame( bool enabled, uint32_t milliseconds ) {
	active = enabled;
	now = milliseconds;
	dropped = 0;
	uint32_t kept = 0;
	for ( uint32_t i = 0; enabled && i < lineCount; ++i ) {
		if ( (int32_t)( now - lines[i].expires ) < 0 )
			lines[kept++] = lines[i];
	}
	lineCount = kept;
	kept = 0;
	for ( uint32_t i = 0; enabled && i < textCount; ++i ) {
		if ( (int32_t)( now - texts[i].expires ) < 0 )
			texts[kept++] = texts[i];
	}
	textCount = kept;
}

static bool FinitePoint( const float *point ) {
	return std::isfinite( point[0] ) && std::isfinite( point[1] ) && std::isfinite( point[2] );
}

void Dev_DrawLine( const float *start, const float *end, uint32_t color, int durationMsec ) {
	if ( !active || !FinitePoint( start ) || !FinitePoint( end ) )
		return;
	if ( lineCount == ARRAY_LEN( lines ) ) {
		++dropped;
		return;
	}
	devLine_t *line = &lines[lineCount++];
	VectorCopy( start, line->start );
	VectorCopy( end, line->end );
	line->color = color;
	line->expires = now + (uint32_t)MAX( 0, MIN( durationMsec, 60000 ) );
}

void Dev_DrawBox( const float *mins, const float *maxs, uint32_t color, int durationMsec ) {
	for ( int corner = 0; corner < 8; ++corner ) {
		vec3_t start;
		for ( int axis = 0; axis < 3; ++axis )
			start[axis] = corner & ( 1 << axis ) ? maxs[axis] : mins[axis];
		for ( int axis = 0; axis < 3; ++axis ) {
			if ( corner & ( 1 << axis ) )
				continue;
			vec3_t end;
			VectorCopy( start, end );
			end[axis] = maxs[axis];
			Dev_DrawLine( start, end, color, durationMsec );
		}
	}
}

void Dev_DrawText( const float *origin, const char *text, uint32_t color, int durationMsec ) {
	if ( !active || !FinitePoint( origin ) )
		return;
	if ( textCount == ARRAY_LEN( texts ) ) {
		++dropped;
		return;
	}
	devText_t *label = &texts[textCount++];
	VectorCopy( origin, label->origin );
	Q_strncpyz( label->text, text, sizeof( label->text ) );
	label->color = color;
	label->expires = now + (uint32_t)MAX( 0, MIN( durationMsec, 60000 ) );
}

uint32_t DevTools_Lines( const devLine_t **output, bool world ) {
	*output = world ? worldLines : lines;
	return world ? worldCount : lineCount;
}
uint32_t DevTools_Text( const devText_t **output ) {
	*output = texts;
	return textCount;
}
uint32_t DevTools_DebugDropped( void ) {
	return dropped + worldDropped;
}
void DevTools_ClearWorld( void ) {
	worldCount = worldDropped = 0;
}

static void WorldLine( const float *start, const float *end, uint32_t color ) {
	if ( worldCount == worldLimit ) {
		++worldDropped;
		return;
	}
	devLine_t *line = &worldLines[worldCount++];
	VectorCopy( start, line->start );
	VectorCopy( end, line->end );
	line->color = color;
}

void DevTools_RebuildWorld( bool collision, bool navigation, float radius ) {
	DevTools_ClearWorld();
	const refdef_t *view = DevTools_View();
	if ( !view )
		return;
	worldLimit = navigation ? ARRAY_LEN( worldLines ) / 2 : ARRAY_LEN( worldLines );
	Com_Printf( "Developer world center: %g %g %g radius %g\n", view->vieworg[0], view->vieworg[1], view->vieworg[2], radius );
	if ( collision )
		CM_DeveloperSurfaces( view->vieworg, radius, WorldLine );
	worldLimit = ARRAY_LEN( worldLines );
	if ( navigation )
		Bot_DeveloperNavigation( view->vieworg, radius, WorldLine );
	Com_Printf( "Developer world: %u lines, %u omitted (collision=%d navigation=%d)\n", worldCount, worldDropped, collision, navigation );
}

bool DevTools_Project( const refdef_t *view, const float *point, float *screen ) {
	vec3_t delta;
	VectorSubtract( point, view->vieworg, delta );
	const float depth = DotProduct( delta, view->viewaxis[0] );
	if ( depth < 0.1f || !FinitePoint( point ) || view->width <= 0 || view->height <= 0 )
		return false;
	const float sx = tanf( DEG2RAD( view->fov_x * 0.5f ) );
	const float sy = tanf( DEG2RAD( view->fov_y * 0.5f ) );
	if ( sx <= 0 || sy <= 0 )
		return false;
	screen[0] = (float)view->x + (float)view->width * 0.5f * ( 1.0f - DotProduct( delta, view->viewaxis[1] ) / ( depth * sx ) );
	screen[1] = (float)view->y + (float)view->height * 0.5f * ( 1.0f - DotProduct( delta, view->viewaxis[2] ) / ( depth * sy ) );
	return std::isfinite( screen[0] ) && std::isfinite( screen[1] );
}

int DevTools_PickEntity( float x, float y ) {
	const refdef_t *view = DevTools_View();
	const devGameTools_t *game = DevTools_Game();
	if ( !view || !game || view->width <= 0 || view->height <= 0 )
		return -1;
	const float side = ( 1.0f - ( x - (float)view->x ) * 2.0f / (float)view->width ) * tanf( DEG2RAD( view->fov_x * 0.5f ) );
	const float up = ( 1.0f - ( y - (float)view->y ) * 2.0f / (float)view->height ) * tanf( DEG2RAD( view->fov_y * 0.5f ) );
	vec3_t direction, end;
	for ( int axis = 0; axis < 3; ++axis )
		direction[axis] = view->viewaxis[0][axis] + side * view->viewaxis[1][axis] + up * view->viewaxis[2][axis];
	VectorNormalize( direction );
	VectorMA( view->vieworg, 8192, direction, end );
	trace_t trace;
	CM_BoxTrace( &trace, view->vieworg, end, vec3_origin, vec3_origin, 0, CONTENTS_SOLID, qfalse );
	float nearest = trace.fraction * 8192;
	int result = -1;
	for ( int i = 0; i < MAX_GENTITIES; ++i ) {
		if ( i == DevTools_ViewClient() )
			continue;
		devEntity_t entity;
		if ( !game->ReadEntity( i, &entity ) )
			continue;
		float first = 0, last = nearest;
		for ( int axis = 0; axis < 3; ++axis ) {
			const float low = entity.linked ? entity.mins[axis] : entity.origin[axis] - 8;
			const float high = entity.linked ? entity.maxs[axis] : entity.origin[axis] + 8;
			if ( fabsf( direction[axis] ) < 1e-6f ) {
				if ( view->vieworg[axis] < low || view->vieworg[axis] > high )
					last = -1;
			} else {
				const float a = ( low - view->vieworg[axis] ) / direction[axis];
				const float b = ( high - view->vieworg[axis] ) / direction[axis];
				first = MAX( first, MIN( a, b ) );
				last = MIN( last, MAX( a, b ) );
			}
		}
		if ( first <= last ) {
			nearest = first;
			result = i;
		}
	}
	return result;
}

static void WorldCommand( void ) {
	const char *mode = Cmd_Argv( 1 );
	if ( strcmp( mode, "collision" ) && strcmp( mode, "nav" ) && strcmp( mode, "both" ) ) {
		Com_Printf( "dev_world collision|nav|both [radius 64..2048]\n" );
		return;
	}
	const float radius = Cmd_Argc() > 2 ? Com_Clamp( 64, 2048, (float)atoi( Cmd_Argv( 2 ) ) ) : 512;
	DevTools_RebuildWorld( strcmp( mode, "nav" ) != 0, strcmp( mode, "collision" ) != 0, radius );
}
void DevTools_InitWorld( void ) {
	Cmd_AddCommand( "dev_world", WorldCommand );
}
