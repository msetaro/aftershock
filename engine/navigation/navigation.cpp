#include "navigation_public.h"
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon_public.h"
#include "../../third_party/sha256/sha-256.h"
#include <DetourAlloc.h>
#include <DetourCrowd.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <bit>
#include <cmath>
#include <cstring>
#include <type_traits>

struct navFileHeader_t {
	uint8_t collisionHash[32];
	float radius, height, climb, slope, cellSize, cellHeight;
	uint32_t tileSize, collisionChecksum;
};
static_assert( sizeof( navFileHeader_t ) == 64 && offsetof( navFileHeader_t, tileSize ) == 56 );
static_assert( std::endian::native == std::endian::little );
static_assert( sizeof( dtMeshHeader ) == 100 && offsetof( dtMeshHeader, bmin ) == 72 );
static_assert( sizeof( dtPoly ) == 32 && sizeof( dtLink ) == 12 && sizeof( dtPolyDetail ) == 12 );
static_assert( sizeof( dtBVNode ) == 16 && sizeof( dtOffMeshConnection ) == 36 );
struct navWorld_t {
	dtNavMesh *mesh;
	dtNavMeshQuery *query;
	dtCrowd *crowd;
	uint8_t *tile;
	float radius, height;
	uint32_t maxAgents;
};
static_assert( std::is_trivially_destructible_v<navWorld_t> );
static void *Allocate( size_t size, dtAllocHint ) {
	return Z_Malloc( size );
}
static bool Finite( const float *values, size_t count ) {
	for ( size_t i = 0; i < count; ++i )
		if ( !std::isfinite( values[i] ) || std::fabs( values[i] ) > 1048576 )
			return false;
	return true;
}
static void ToDetour( const float *input, float *output ) {
	output[0] = input[0];
	output[1] = input[2];
	output[2] = -input[1];
}
static void FromDetour( const float *input, float *output ) {
	output[0] = input[0];
	output[1] = -input[2];
	output[2] = input[1];
}
template <typename T>
static T Read( const uint8_t *bytes, size_t index = 0 ) {
	T result;
	std::memcpy( &result, bytes + index * sizeof( T ), sizeof( T ) );
	return result;
}
// Detour's tile initializer trusts counts and references. Validate the pinned
// single-tile representation before letting it derive or mutate any pointers.
static bool TileValid( const uint8_t *data, size_t size, const navFileHeader_t &file ) {
	if ( size < sizeof( dtMeshHeader ) )
		return false;
	const auto h = Read<dtMeshHeader>( data );
	if ( h.magic != DT_NAVMESH_MAGIC || h.version != DT_NAVMESH_VERSION ||
		 h.polyCount < 1 || h.polyCount > 32768 || h.vertCount < 3 || h.vertCount > 65535 ||
		 h.maxLinkCount < 1 || h.maxLinkCount > 1048576 ||
		 h.detailMeshCount < 1 || h.detailMeshCount > h.polyCount ||
		 h.detailVertCount < 0 || h.detailVertCount > 1048576 ||
		 h.detailTriCount < 0 || h.detailTriCount > 1048576 ||
		 h.bvNodeCount < 0 || h.bvNodeCount > 2 * h.detailMeshCount ||
		 h.offMeshConCount < 0 || h.offMeshConCount > 256 ||
		 h.offMeshBase != h.detailMeshCount || h.polyCount != h.offMeshBase + h.offMeshConCount ||
		 h.walkableRadius != file.radius || h.walkableHeight != file.height || h.walkableClimb != file.climb ||
		 !Finite( h.bmin, 3 ) || !Finite( h.bmax, 3 ) || !std::isfinite( h.bvQuantFactor ) || h.bvQuantFactor <= 0 )
		return false;
	for ( int axis = 0; axis < 3; ++axis )
		if ( h.bmin[axis] >= h.bmax[axis] )
			return false;
	const size_t sizes[] = { size_t( h.vertCount ) * 12, size_t( h.polyCount ) * sizeof( dtPoly ),
		size_t( h.maxLinkCount ) * sizeof( dtLink ), size_t( h.detailMeshCount ) * sizeof( dtPolyDetail ),
		size_t( h.detailVertCount ) * 12, size_t( h.detailTriCount ) * 4,
		size_t( h.bvNodeCount ) * sizeof( dtBVNode ), size_t( h.offMeshConCount ) * sizeof( dtOffMeshConnection ) };
	const uint8_t *sections[8];
	size_t offset = sizeof( h );
	for ( size_t i = 0; i < 8; ++i ) {
		const size_t bytes = ( sizes[i] + 3 ) & ~size_t( 3 );
		if ( bytes > size - offset )
			return false;
		sections[i] = data + offset;
		offset += bytes;
	}
	if ( offset != size )
		return false;
	constexpr int floatSections[] = { 0, 4 };
	for ( int section : floatSections ) {
		const size_t count = sizes[section] / sizeof( float );
		for ( size_t i = 0; i < count; ++i ) {
			const float value = Read<float>( sections[section], i );
			if ( !Finite( &value, 1 ) )
				return false;
		}
	}
	for ( int i = 0; i < h.polyCount; ++i ) {
		const auto poly = Read<dtPoly>( sections[1], size_t( i ) );
		const bool link = i >= h.offMeshBase;
		if ( poly.vertCount < ( link ? 2 : 3 ) || poly.vertCount > ( link ? 2 : DT_VERTS_PER_POLYGON ) ||
			 poly.getType() != ( link ? DT_POLYTYPE_OFFMESH_CONNECTION : DT_POLYTYPE_GROUND ) ||
			 poly.flags != ( link ? 2 : 1 ) || poly.getArea() > 3 )
			return false;
		for ( int j = 0; j < poly.vertCount; ++j )
			if ( poly.verts[j] >= h.vertCount || poly.neis[j] > h.detailMeshCount )
				return false;
		if ( link )
			continue;
		const auto detail = Read<dtPolyDetail>( sections[3], size_t( i ) );
		if ( detail.vertBase > uint32_t( h.detailVertCount ) || detail.vertCount > uint32_t( h.detailVertCount ) - detail.vertBase ||
			 detail.triBase > uint32_t( h.detailTriCount ) || detail.triCount > uint32_t( h.detailTriCount ) - detail.triBase )
			return false;
		for ( uint32_t j = 0; j < detail.triCount; ++j )
			for ( uint32_t k = 0; k < 3; ++k )
				if ( sections[5][( detail.triBase + j ) * 4 + k] >= poly.vertCount + detail.vertCount )
					return false;
	}
	for ( int i = 0; i < h.bvNodeCount; ++i ) {
		const auto node = Read<dtBVNode>( sections[6], size_t( i ) );
		if ( node.i >= h.detailMeshCount || node.i < -( h.bvNodeCount - i ) )
			return false;
		for ( int axis = 0; axis < 3; ++axis )
			if ( node.bmin[axis] > node.bmax[axis] )
				return false;
	}
	for ( int i = 0; i < h.offMeshConCount; ++i ) {
		const auto link = Read<dtOffMeshConnection>( sections[7], size_t( i ) );
		if ( !Finite( link.pos, 6 ) || !std::isfinite( link.rad ) || link.rad < 0.125f || link.rad > 256 ||
			 link.poly != h.offMeshBase + i || !link.userId || link.flags > DT_OFFMESH_CON_BIDIR ||
			 ( link.side != 0xff && link.side > 7 ) )
			return false;
		for ( int j = 0; j < i; ++j )
			if ( Read<dtOffMeshConnection>( sections[7], size_t( j ) ).userId == link.userId )
				return false;
	}
	return true;
}
navWorld_t *Nav_Open( const void *bytes, size_t size, uint32_t collisionChecksum, uint32_t maxAgents ) {
	if ( !bytes || size < 112 || size > ( 16u << 20 ) || !maxAgents || maxAgents > 64 )
		return nullptr;
	const auto *data = static_cast<const uint8_t *>( bytes );
	if ( std::memcmp( data, "ASNAV\0\0\0", 8 ) || Read<uint32_t>( data + 8 ) != 1 || Read<uint32_t>( data + 12 ) != size - 48 )
		return nullptr;
	uint8_t hash[32];
	calc_sha_256( hash, data + 48, size - 48 );
	if ( std::memcmp( hash, data + 16, 32 ) )
		return nullptr;
	const auto file = Read<navFileHeader_t>( data + 48 );
	if ( file.collisionChecksum != collisionChecksum || file.tileSize != size - 112 ||
		 !std::isfinite( file.radius ) || file.radius < 0.125f || file.radius > 128 ||
		 !std::isfinite( file.height ) || file.height < 8 || file.height > 256 ||
		 !std::isfinite( file.climb ) || file.climb < 0 || file.climb >= file.height ||
		 !std::isfinite( file.slope ) || file.slope < 0.1f || file.slope > 84.9f ||
		 !std::isfinite( file.cellSize ) || file.cellSize < 1 || file.cellSize > 32 ||
		 !std::isfinite( file.cellHeight ) || file.cellHeight < 0.5f || file.cellHeight > 16 ||
		 !TileValid( data + 112, file.tileSize, file ) )
		return nullptr;
	dtAllocSetCustom( Allocate, Z_Free );
	auto *world = static_cast<navWorld_t *>( Z_Malloc(sizeof(navWorld_t)) );
	world->radius = file.radius;
	world->height = file.height;
	world->maxAgents = maxAgents;
	world->tile = static_cast<uint8_t *>( Z_Malloc(file.tileSize) );
	std::memcpy( world->tile, data + 112, file.tileSize );
	world->mesh = dtAllocNavMesh();
	world->query = dtAllocNavMeshQuery();
	world->crowd = dtAllocCrowd();
	if ( !world->mesh || !world->query || !world->crowd ||
		 dtStatusFailed( world->mesh->init( world->tile, int( file.tileSize ), 0 ) ) ||
		 dtStatusFailed( world->query->init( world->mesh, 8192 ) ) ||
		 !world->crowd->init( int( maxAgents ), file.radius, world->mesh ) ) {
		Nav_Close( world );
		return nullptr;
	}
	// Pmove handles off-mesh actions explicitly using Nav_Path link IDs. Crowd
	// steering stays on the ground and never teleports gameplay over a link.
	world->crowd->getEditableFilter( 0 )->setIncludeFlags( 1 );
	return world;
}
void Nav_Close( navWorld_t *world ) {
	if ( !world )
		return;
	dtFreeCrowd( world->crowd );
	dtFreeNavMeshQuery( world->query );
	dtFreeNavMesh( world->mesh );
	Z_Free( world->tile );
	Z_Free( world );
}
static bool Nearest( navWorld_t *world, const float *position, const dtQueryFilter &filter, dtPolyRef *ref, float *point ) {
	if ( !Finite( position, 3 ) )
		return false;
	float converted[3];
	ToDetour( position, converted );
	const float extents[3] = { world->radius * 2, world->height, world->radius * 2 };
	return dtStatusSucceed( world->query->findNearestPoly( converted, extents, &filter, ref, point ) ) && *ref;
}
bool Nav_Path( navWorld_t *world, const float start[3], const float end[3], bool links, navPath_t *out ) {
	if ( !world || !start || !end || !out )
		return false;
	*out = {};
	dtQueryFilter filter;
	filter.setIncludeFlags( links ? 3 : 1 );
	dtPolyRef from, to;
	float a[3], b[3];
	if ( !Nearest( world, start, filter, &from, a ) || !Nearest( world, end, filter, &to, b ) )
		return false;
	dtPolyRef corridor[NAV_MAX_POINTS];
	int count = 0;
	const auto status = world->query->findPath( from, to, a, b, &filter, corridor, &count, NAV_MAX_POINTS );
	if ( dtStatusFailed( status ) || !count )
		return false;
	out->complete = corridor[count - 1] == to && !dtStatusDetail( status, DT_PARTIAL_RESULT ) && !dtStatusDetail( status, DT_BUFFER_TOO_SMALL );
	if ( !out->complete && dtStatusFailed( world->query->closestPointOnPoly( corridor[count - 1], b, b, nullptr ) ) )
		return false;
	// Funnel each ground segment separately: a coincident start/link endpoint can
	// otherwise disappear from Detour's straight-path flags. Keep the authored
	// action explicit even when the ground segment has zero length.
	for ( int first = 0; first < count; ) {
		int next = first;
		const dtOffMeshConnection *link = nullptr;
		while ( next < count && !( link = world->mesh->getOffMeshConnectionByRef( corridor[next] ) ) )
			++next;
		float segmentEnd[3], landing[3];
		std::memcpy( segmentEnd, b, sizeof( b ) );
		if ( link && ( next == first || dtStatusFailed( world->mesh->getOffMeshConnectionPolyEndPoints( corridor[next - 1], corridor[next], segmentEnd, landing ) ) ) )
			return false;
		float points[NAV_MAX_POINTS * 3];
		int pointCount = 0;
		const auto straight = world->query->findStraightPath( a, segmentEnd, corridor + first, next - first, points, nullptr, nullptr, &pointCount, NAV_MAX_POINTS );
		if ( dtStatusFailed( straight ) || !pointCount )
			return false;
		for ( int i = 0; i < pointCount; ++i ) {
			float point[3];
			FromDetour( points + i * 3, point );
			if ( out->count && !std::memcmp( point, out->points[out->count - 1].position, sizeof( point ) ) )
				continue;
			if ( out->count == NAV_MAX_POINTS ) {
				out->complete = false;
				return true;
			}
			std::memcpy( out->points[out->count++].position, point, sizeof( point ) );
		}
		if ( dtStatusDetail( straight, DT_BUFFER_TOO_SMALL ) ) {
			out->complete = false;
			return true;
		}
		if ( !link )
			break;
		out->points[out->count - 1].link = link->userId;
		std::memcpy( a, landing, sizeof( a ) );
		first = next + 1;
	}
	return out->count > 0;
}
bool Nav_CoverPoints( navWorld_t *world, const float position[3], float range, navCoverQuery_t *out ) {
	if ( !world || !position || !out || !Finite( position, 3 ) || !std::isfinite( range ) || range <= 0 || range > 4096 )
		return false;
	*out = {};
	float center[3];
	ToDetour( position, center );
	const float extents[3] = { range, range, range };
	dtQueryFilter filter;
	filter.setIncludeFlags( 1 );
	dtPolyRef refs[NAV_MAX_POINTS];
	int count = 0;
	const auto status = world->query->queryPolygons( center, extents, &filter, refs, &count, NAV_MAX_POINTS );
	if ( dtStatusFailed( status ) )
		return false;
	out->complete = !dtStatusDetail( status, DT_BUFFER_TOO_SMALL );
	float distances[NAV_MAX_POINTS];
	for ( int i = 0; i < count; ++i ) {
		const dtMeshTile *tile;
		const dtPoly *poly;
		if ( dtStatusFailed( world->mesh->getTileAndPolyByRef( refs[i], &tile, &poly ) ) )
			return false;
		float centroid[3] = {};
		for ( int vertex = 0; vertex < poly->vertCount; ++vertex )
			for ( int axis = 0; axis < 3; ++axis )
				centroid[axis] += tile->verts[poly->verts[vertex] * 3 + axis] / poly->vertCount;
		for ( int edge = 0; edge < poly->vertCount; ++edge ) {
			if ( poly->neis[edge] )
				continue;
			const float *a = tile->verts + poly->verts[edge] * 3, *b = tile->verts + poly->verts[( edge + 1 ) % poly->vertCount] * 3;
			float point[3];
			for ( int axis = 0; axis < 3; ++axis )
				point[axis] = ( a[axis] + b[axis] ) * 0.5f;
			const float dx = centroid[0] - point[0], dz = centroid[2] - point[2];
			const float length = std::sqrt( dx * dx + dz * dz );
			if ( length > 0 ) {
				const float step = std::fmin( world->radius * 0.5f, length ) / length;
				point[0] += dx * step;
				point[2] += dz * step;
			}
			if ( dtStatusFailed( world->query->getPolyHeight( refs[i], point, &point[1] ) ) )
				continue;
			float squared = 0;
			for ( int axis = 0; axis < 3; ++axis ) {
				const float delta = point[axis] - center[axis];
				squared += delta * delta;
			}
			if ( squared > range * range )
				continue;
			const uint32_t identity = world->mesh->decodePolyIdPoly( refs[i] ) * DT_VERTS_PER_POLYGON + uint32_t( edge );
			uint32_t slot = 0;
			while ( slot < out->count && ( distances[slot] < squared || ( distances[slot] == squared && out->points[slot].identity < identity ) ) )
				++slot;
			if ( out->count == NAV_MAX_POINTS )
				out->complete = false;
			if ( slot == NAV_MAX_POINTS )
				continue;
			const uint32_t end = out->count < NAV_MAX_POINTS ? out->count++ : NAV_MAX_POINTS - 1;
			for ( uint32_t j = end; j > slot; --j ) {
				out->points[j] = out->points[j - 1];
				distances[j] = distances[j - 1];
			}
			FromDetour( point, out->points[slot].position );
			out->points[slot].identity = identity;
			distances[slot] = squared;
		}
	}
	return true;
}
static bool AgentValid( const navWorld_t *world, int index ) {
	return world && index >= 0 && uint32_t( index ) < world->maxAgents && world->crowd->getAgent( index )->active;
}
int Nav_AddAgent( navWorld_t *world, const float position[3], float speed ) {
	if ( !world || !position || !Finite( position, 3 ) || !std::isfinite( speed ) || speed <= 0 || speed > 2000 )
		return -1;
	dtCrowdAgentParams params{};
	params.radius = world->radius;
	params.height = world->height;
	params.maxAcceleration = 800;
	params.maxSpeed = speed;
	params.collisionQueryRange = world->radius * 12;
	params.pathOptimizationRange = world->radius * 30;
	params.separationWeight = 2;
	params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OBSTACLE_AVOIDANCE | DT_CROWD_SEPARATION | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO;
	float converted[3];
	ToDetour( position, converted );
	return world->crowd->addAgent( converted, &params );
}
void Nav_RemoveAgent( navWorld_t *world, int index ) {
	if ( AgentValid( world, index ) )
		world->crowd->removeAgent( index );
}
bool Nav_AgentTarget( navWorld_t *world, int index, const float position[3] ) {
	if ( !AgentValid( world, index ) || !position )
		return false;
	dtPolyRef ref;
	float target[3];
	return Nearest( world, position, *world->crowd->getFilter( 0 ), &ref, target ) && world->crowd->requestMoveTarget( index, ref, target );
}
bool Nav_Agent( const navWorld_t *world, int index, navAgent_t *out ) {
	if ( !out || !AgentValid( world, index ) )
		return false;
	const auto *agent = world->crowd->getAgent( index );
	*out = {};
	FromDetour( agent->npos, out->position );
	FromDetour( agent->vel, out->velocity );
	out->offMesh = agent->state == DT_CROWDAGENT_STATE_OFFMESH;
	out->partial = agent->partial;
	return true;
}
void Nav_Update( navWorld_t *world, float seconds ) {
	if ( world && std::isfinite( seconds ) && seconds > 0 && seconds <= 0.1f )
		world->crowd->update( seconds, nullptr );
}
