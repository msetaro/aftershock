// Offline adapter: reuse the production CM loader/exporter, then Recast/Detour.
#include "../../engine/qcommon/q_shared.h"
#include "../../engine/qcommon/qcommon_public.h"
#include "../../engine/qcommon/surfaceflags_public.h"
#include "../../third_party/recast/Recast/Include/Recast.h"
#include "../../third_party/recast/Detour/Include/DetourNavMeshBuilder.h"
#include "../../third_party/recast/Detour/Include/DetourAlloc.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <vector>

static std::vector<void *> allocations;
void QDECL Com_Error( errorParm_t, const char *format, ... ) {
	va_list args;
	va_start( args, format );
	vfprintf( stderr, format, args );
	va_end( args );
	exit( 1 );
}
void QDECL Com_Printf( const char *, ... ) {
}
void QDECL Com_DPrintf( const char *, ... ) {
}
void *Z_Malloc( size_t size ) {
	void *memory = calloc( 1, size ? size : 1 );
	if ( !memory )
		Com_Error( ERR_FATAL, "collision allocation failed" );
	allocations.push_back( memory );
	return memory;
}
void Z_Free( void *memory ) {
	auto position = std::find( allocations.begin(), allocations.end(), memory );
	if ( position == allocations.end() )
		Com_Error( ERR_FATAL, "unknown collision allocation" );
	allocations.erase( position );
	free( memory );
}
void *Hunk_Alloc( size_t size, ha_pref ) {
	return Z_Malloc( size );
}
void *Hunk_AllocateTempMemory( size_t size ) {
	return Z_Malloc( size );
}
void Hunk_FreeTempMemory( void *memory ) {
	Z_Free( memory );
}
cvar_t *Cvar_Get( const char *name, const char *value, int ) {
	static cvar_t variables[3];
	auto *variable = &variables[!strcmp( name, "cm_noAreas" ) ? 0 : !strcmp( name, "cm_noCurves" ) ? 1
																								   : 2];
	variable->integer = atoi( value );
	return variable;
}
void Cvar_SetDescription( cvar_t *, const char * ) {
}
void BotDrawDebugPolygons( void ( * )( int, int, float * ), int ) {
	Com_Error( ERR_FATAL, "debug draw is unavailable in the offline cooker" );
}
int FS_ReadFile( const char *path, void **buffer ) {
	FILE *file = fopen( path, "rb" );
	if ( !file || fseek( file, 0, SEEK_END ) )
		Com_Error( ERR_FATAL, "cannot read collision BSP" );
	const auto length = ftell( file );
	if ( length <= 0 || length > 256 * 1024 * 1024 )
		Com_Error( ERR_FATAL, "collision BSP size exceeds limit" );
	rewind( file );
	*buffer = Z_Malloc( size_t( length ) + 1 );
	if ( fread( *buffer, 1, size_t( length ), file ) != size_t( length ) )
		Com_Error( ERR_FATAL, "incomplete collision BSP" );
	fclose( file );
	return int( length );
}

struct Parameters {
	float radius, height, climb, slope, cell, cellHeight;
	uint32_t links;
};
struct Link {
	float start[3], end[3], radius;
	uint32_t bidirectional, kind, id;
};
static_assert( std::endian::native == std::endian::little );
static_assert( sizeof( Parameters ) == 28 && offsetof( Parameters, links ) == 24 );
static_assert( sizeof( Link ) == 40 && offsetof( Link, bidirectional ) == 28 && offsetof( Link, id ) == 36 );
struct Geometry {
	std::vector<float> vertices;
	std::vector<int> triangles;
};
static void ToNavigation( const float *point, float *result ) {
	result[0] = point[0];
	result[1] = point[2];
	result[2] = -point[1];
}
static bool Triangle( void *context, const float *a, const float *b, const float *c ) {
	auto &geometry = *static_cast<Geometry *>( context );
	if ( geometry.triangles.size() >= 3 * 262144 )
		return false;
	for ( const float *point : { a, b, c } ) {
		float vertex[3];
		ToNavigation( point, vertex );
		for ( float value : vertex )
			if ( !std::isfinite( value ) || fabsf( value ) > 131072 )
				return false;
		geometry.triangles.push_back( int( geometry.vertices.size() / 3 ) );
		geometry.vertices.insert( geometry.vertices.end(), vertex, vertex + 3 );
	}
	return true;
}
static void Require( bool valid, const char *stage ) {
	if ( !valid )
		Com_Error( ERR_FATAL, "navigation cook failed: %s", stage );
}
int main( int argc, char **argv ) {
	Require( argc == 4, "arguments" );
	FILE *input = fopen( argv[2], "rb" );
	Parameters parameters{};
	Require( input && fread( &parameters, sizeof( parameters ), 1, input ) == 1 && parameters.links <= 256, "parameters" );
	const float values[] = { parameters.radius, parameters.height, parameters.climb, parameters.slope, parameters.cell, parameters.cellHeight };
	for ( float value : values )
		Require( std::isfinite( value ), "finite agent settings" );
	Require( parameters.radius > 0 && parameters.radius <= 128 && parameters.height >= 8 && parameters.height <= 256 &&
				 parameters.climb >= 0 && parameters.climb < parameters.height && parameters.slope > 0 && parameters.slope < 85 &&
				 parameters.cell >= 1 && parameters.cell <= 32 && parameters.cellHeight >= .5f && parameters.cellHeight <= 16,
		"agent limits" );
	std::vector<Link> links( parameters.links );
	Require( fread( links.data(), sizeof( Link ), links.size(), input ) == links.size() && fgetc( input ) == EOF, "link extents" );
	fclose( input );
	for ( const auto &link : links ) {
		for ( int i = 0; i < 3; ++i )
			Require( std::isfinite( link.start[i] ) && std::isfinite( link.end[i] ) &&
						 fabsf( link.start[i] ) <= 131072 && fabsf( link.end[i] ) <= 131072,
				"link coordinates" );
		Require( std::isfinite( link.radius ) && link.radius > 0 && link.radius <= 256 && link.bidirectional <= 1 &&
					 link.kind >= 1 && link.kind <= 4 && link.id,
			"link settings" );
	}
	int checksum = 0;
	CM_LoadMap( argv[1], qfalse, &checksum );
	Geometry geometry;
	Require( CM_PhysicsTriangles( Triangle, &geometry, CONTENTS_SOLID | CONTENTS_PLAYERCLIP ) && !geometry.triangles.empty(), "collision triangles" );
	rcConfig config{};
	config.cs = parameters.cell;
	config.ch = parameters.cellHeight;
	config.walkableSlopeAngle = parameters.slope;
	config.walkableHeight = int( ceilf( parameters.height / config.ch ) );
	config.walkableClimb = int( floorf( parameters.climb / config.ch ) );
	config.walkableRadius = int( ceilf( parameters.radius / config.cs ) );
	config.maxEdgeLen = int( 48 / config.cs );
	config.maxSimplificationError = 1.3f;
	config.minRegionArea = 8 * 8;
	config.mergeRegionArea = 20 * 20;
	config.maxVertsPerPoly = 6;
	config.detailSampleDist = config.cs * 6;
	config.detailSampleMaxError = config.ch;
	const int vertices = int( geometry.vertices.size() / 3 ), triangles = int( geometry.triangles.size() / 3 );
	rcCalcBounds( geometry.vertices.data(), vertices, config.bmin, config.bmax );
	rcCalcGridSize( config.bmin, config.bmax, config.cs, &config.width, &config.height );
	Require( config.width > 0 && config.height > 0 && uint64_t( config.width ) * uint64_t( config.height ) <= 16 * 1024 * 1024, "voxel budget" );
	rcContext context;
	rcHeightfield heightfield;
	Require( rcCreateHeightfield( &context, heightfield, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch ), "heightfield" );
	std::vector<unsigned char> areas( size_t( triangles ), RC_NULL_AREA );
	rcMarkWalkableTriangles( &context, config.walkableSlopeAngle, geometry.vertices.data(), vertices, geometry.triangles.data(), triangles, areas.data() );
	Require( rcRasterizeTriangles( &context, geometry.vertices.data(), vertices, geometry.triangles.data(), areas.data(), triangles, heightfield, config.walkableClimb ), "rasterization" );
	rcFilterLowHangingWalkableObstacles( &context, config.walkableClimb, heightfield );
	rcFilterLedgeSpans( &context, config.walkableHeight, config.walkableClimb, heightfield );
	rcFilterWalkableLowHeightSpans( &context, config.walkableHeight, heightfield );
	rcCompactHeightfield compact;
	Require( rcBuildCompactHeightfield( &context, config.walkableHeight, config.walkableClimb, heightfield, compact ), "compact heightfield" );
	Require( rcErodeWalkableArea( &context, config.walkableRadius, compact ), "agent radius" );
	Require( rcBuildDistanceField( &context, compact ) && rcBuildRegions( &context, compact, 0, config.minRegionArea, config.mergeRegionArea ), "regions" );
	rcContourSet contours;
	Require( rcBuildContours( &context, compact, config.maxSimplificationError, config.maxEdgeLen, contours ), "contours" );
	rcPolyMesh polygons;
	Require( rcBuildPolyMesh( &context, contours, config.maxVertsPerPoly, polygons ) && polygons.npolys > 0 && polygons.npolys <= 32768 - int( parameters.links ) && polygons.nverts < 65535 - 2 * int( parameters.links ), "polygons and off-mesh vertex capacity" );
	rcPolyMeshDetail detail;
	Require( rcBuildPolyMeshDetail( &context, polygons, compact, config.detailSampleDist, config.detailSampleMaxError, detail ), "detail" );
	for ( int i = 0; i < polygons.npolys; ++i ) {
		polygons.areas[i] = 0;
		polygons.flags[i] = 1;
	}
	std::vector<float> endpoints, radii;
	std::vector<unsigned short> flags;
	std::vector<unsigned char> directions, linkAreas;
	std::vector<unsigned int> ids;
	for ( const auto &link : links ) {
		float point[3];
		ToNavigation( link.start, point );
		endpoints.insert( endpoints.end(), point, point + 3 );
		ToNavigation( link.end, point );
		endpoints.insert( endpoints.end(), point, point + 3 );
		radii.push_back( link.radius );
		flags.push_back( 2 );
		directions.push_back( static_cast<unsigned char>( link.bidirectional ) );
		linkAreas.push_back( static_cast<unsigned char>( link.kind ) );
		ids.push_back( link.id );
	}
	dtNavMeshCreateParams create{};
	create.verts = polygons.verts;
	create.vertCount = polygons.nverts;
	create.polys = polygons.polys;
	create.polyAreas = polygons.areas;
	create.polyFlags = polygons.flags;
	create.polyCount = polygons.npolys;
	create.nvp = polygons.nvp;
	create.detailMeshes = detail.meshes;
	create.detailVerts = detail.verts;
	create.detailVertsCount = detail.nverts;
	create.detailTris = detail.tris;
	create.detailTriCount = detail.ntris;
	create.offMeshConVerts = endpoints.data();
	create.offMeshConRad = radii.data();
	create.offMeshConFlags = flags.data();
	create.offMeshConAreas = linkAreas.data();
	create.offMeshConDir = directions.data();
	create.offMeshConUserID = ids.data();
	create.offMeshConCount = int( links.size() );
	create.walkableHeight = parameters.height;
	create.walkableRadius = parameters.radius;
	create.walkableClimb = parameters.climb;
	memcpy( create.bmin, polygons.bmin, sizeof( create.bmin ) );
	memcpy( create.bmax, polygons.bmax, sizeof( create.bmax ) );
	create.cs = config.cs;
	create.ch = config.ch;
	create.buildBvTree = true;
	unsigned char *data = nullptr;
	int size = 0;
	Require( dtCreateNavMeshData( &create, &data, &size ) && size > 0 && size <= 16 * 1024 * 1024 - 112, "Detour tile plus envelope capacity" );
	FILE *output = fopen( argv[3], "wb" );
	Require( output && fwrite( data, 1, size_t( size ), output ) == size_t( size ) && fclose( output ) == 0, "output" );
	dtFree( data );
	for ( void *memory : allocations )
		free( memory );
	printf( "{\"triangles\":%d,\"polygons\":%d,\"links\":%u,\"checksum\":%u}\n", triangles, polygons.npolys, parameters.links, uint32_t( checksum ) );
}
