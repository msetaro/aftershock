#include "../../engine/botlib/be_aas_route.cpp"
#include <assert.h>
#include <cstddef>
aas_t aasworld;
botlib_import_t botimport;
int botDeveloper;
static int frees;
void *GetClearedMemory(size_t size) {
	alignas( std::max_align_t ) static unsigned char memory[1024 * 1024];
	static size_t offset;
	const size_t start = offset;
	offset += ( size + alignof( std::max_align_t ) - 1 ) / alignof( std::max_align_t ) * alignof( std::max_align_t );
	assert(offset<=sizeof(memory));
	memset( memory + start, 0, size );
	return memory + start;
}
void FreeMemory( void *memory ) {
	assert(memory);
	++frees;
}
float AAS_Time() {
	return aasworld.time;
}
static aas_routingcache_t *Cache( int type, int area, int flags, float time ) {
	auto *cache = AAS_AllocRoutingCache( type == CACHETYPE_AREA ? 2 : 1 );
	cache->type = uint8_t( type );
	cache->cluster = 1;
	cache->areanum = area;
	cache->time = time;
	cache->travelflags = flags;
	cache->starttraveltime = 1;
	cache->traveltimes[0] = 65535;
	cache->reachabilities[0] = 0;
	if ( type == CACHETYPE_AREA ) {
		cache->traveltimes[1] = 42;
		cache->reachabilities[1] = 0;
	}
	auto **head = type == CACHETYPE_AREA ? &aasworld.clusterareacache[1][area - 1] : &aasworld.portalcache[area];
	cache->next = *head;
	if ( *head )
		( *head )->prev = cache;
	*head = cache;
	AAS_LinkCache( cache );
	return cache;
}
int main() {
	aasworld.initialized = aasworld.loaded = 1;
	aasworld.time = 50;
	aasworld.numareas = aasworld.numareasettings = 3;
	aasworld.numclusters = 2;
	aasworld.numportals = 1;
	aasworld.reachabilitysize = 2;
	aas_cluster_t clusters[2]{ {}, { 2, 2, 0, 0 } };
	aasworld.clusters = clusters;
	aas_areasettings_t areas[3]{};
	aasworld.areasettings = areas;
	for ( int i = 1; i < 3; ++i ) {
		areas[i].cluster = 1;
		areas[i].clusterareanum = i - 1;
		areas[i].numreachableareas = 1;
		areas[i].firstreachablearea = i - 1;
	}
	aas_routingcache_t *empty[1]{}, *bucket[2]{}, **buckets[2]{ empty, bucket }, *portals[3]{};
	aasworld.clusterareacache = buckets;
	aasworld.portalcache = portals;
	int contents[3]{}, portalTimes[1]{};
	aasworld.areacontentstravelflags = contents;
	aasworld.portalmaxtraveltimes = portalTimes;
	aas_routingupdate_t areaUpdates[2]{}, portalUpdates[2]{};
	aasworld.areaupdate = areaUpdates;
	aasworld.portalupdate = portalUpdates;
	aas_reversedlink_t links[2]{ { 1, 2, nullptr }, { 0, 1, nullptr } };
	aas_reversedreachability_t reverse[3]{ {}, { 1, &links[0] }, { 1, &links[1] } };
	aasworld.reversedreachability = reverse;
	uint16_t travel[2]{ 21, 37 }, *rows[2]{ &travel[0], &travel[1] }, **tables[3]{ nullptr, &rows[0], &rows[1] };
	aasworld.areatraveltimes = tables;
	aas_reachabilityareas_t reachAreas[2]{};
	int passAreas[1]{};
	aasworld.reachabilityareas = reachAreas;
	aasworld.reachabilityareaindex = passAreas;
	max_routingcachesize = 1024 * 1024;
	numareacacheupdates = 7;
	numportalcacheupdates = 3;
	auto *oldest = Cache( CACHETYPE_AREA, 1, 1, 10 );
	Cache( CACHETYPE_PORTAL, 2, 1, 20 );
	Cache( CACHETYPE_AREA, 1, 2, 30 );
	Cache( CACHETYPE_AREA, 2, 1, 40 );
	const int originalBytes = routingcachesize;
	static unsigned char bytes[32768];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(AAS_WriteRoutingState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	routeHeaderSave_t header;
	uint32_t version;
	assert(State_Find(reader,routeHeaderSchema,0,&header,&version));
	assert(AAS_FreeOldestCache() && frees==1 && aasworld.oldestcache->type==CACHETYPE_PORTAL);
	auto *before = aasworld.oldestcache;
	assert(AAS_ReadRoutingState(reader,false) && aasworld.oldestcache==before);
	assert(AAS_ReadRoutingState(reader,true) && routingcachesize==originalBytes);
	assert(aasworld.oldestcache!=oldest && aasworld.oldestcache->type==CACHETYPE_AREA && aasworld.oldestcache->time==10);
	assert(bucket[0]->travelflags==2 && bucket[0]->next==aasworld.oldestcache && bucket[0]->next->prev==bucket[0]);
	auto *hit = AAS_GetAreaRoutingCache( 1, 1, 1 );
	assert(hit->traveltimes[0]==65535 && hit->traveltimes[1]==42 && hit->time==50 && numareacacheupdates==7 && aasworld.newestcache==hit);
	assert(AAS_ReadRoutingState(reader,true));
	assert(AAS_FreeOldestCache() && aasworld.oldestcache->type==CACHETYPE_PORTAL && aasworld.oldestcache->time==20);
	before = aasworld.oldestcache;
	travel[0] = 22;
	assert(!AAS_ReadRoutingState(reader,true) && aasworld.oldestcache==before);
	travel[0] = 21;
	// Missing last cache cannot free or replace any current cache.
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,routeHeaderSchema,0,&header));
	for ( uint32_t i = 0; i < 3; ++i )
		assert(State_Append(&writer,routeMetaSchema,i,&savedRouteMeta[i]));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	const int freed = frees;
	assert(!AAS_ReadRoutingState(reader,true) && frees==freed && aasworld.oldestcache==before);
	// A fully framed cache with a widened travel time outside uint16_t rejects.
	header.count = 1;
	header.bytes = int( sizeof( aas_routingcache_t ) + 6 );
	savedRouteCache = {};
	savedRouteCache.meta = savedRouteMeta[0];
	savedRouteCache.meta.prev = savedRouteCache.meta.next = -1;
	savedRouteCache.traveltimes[0] = UINT16_MAX + 1u;
	stateField_t fields[12];
	memcpy( fields, routeMetaFields, sizeof( routeMetaFields ) );
	fields[10] = { "traveltimes", offsetof( routeCacheSave_t, traveltimes ), 2, stateType_t::UInt32 };
	fields[11] = { "reachabilities", offsetof( routeCacheSave_t, reachabilities ), 2, stateType_t::Bytes };
	const stateSchema_t schema = { "botlib.aasRouteCache", 1, 1, sizeof( savedRouteCache ), fields, 12 };
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,routeHeaderSchema,0,&header));
	assert(State_Append(&writer,schema,0,&savedRouteCache));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!AAS_ReadRoutingState(reader,true) && frees==freed && aasworld.oldestcache==before);
	// A routing update in progress is outside the command-boundary contract.
	areaUpdates[0].inlist = qtrue;
	writer = { bytes, sizeof( bytes ) };
	assert(!AAS_WriteRoutingState(&writer) && !State_Finish(&writer));
	puts( "PASS: AAS routing caches retain hot-query results, bucket/LRU order and eviction across fresh allocations" );
}
