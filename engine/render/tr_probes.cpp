// Offline reflection atlases share the existing map/image lifetime.
#include "tr_local.h"
#include <cmath>

struct probeHeader_t {
	char magic[8];
	uint32_t version, count, size, levels;
};
struct probeRecord_t {
	vec3_t origin;
	float radius;
};
static_assert( sizeof( probeHeader_t ) == 24 && offsetof( probeHeader_t, version ) == 8 );
static_assert( sizeof( probeRecord_t ) == 16 && offsetof( probeRecord_t, radius ) == 12 );

void R_LoadReflectionProbes( const char *worldName ) {
	tr.numReflectionProbes = 0;
	char name[MAX_QPATH];
	COM_StripExtension( worldName, name, sizeof( name ) );
	Q_strcat( name, sizeof( name ), ".asprobe" );
	const int expected = ri.FS_ReadFile( name, nullptr );
	if ( expected < 0 )
		return;
	constexpr int maximum = 24 + MAX_REFLECTION_PROBES * ( 16 + 128 * 128 * 6 * 5 * 4 );
	if ( expected < 24 || expected > maximum ) {
		ri.Printf( PRINT_WARNING, "Invalid reflection probe file size: %s\n", name );
		return;
	}
	void *file = nullptr;
	const int length = ri.FS_ReadFile( name, &file );
	if ( !file )
		return;
	probeHeader_t header = {};
	if ( length >= (int)sizeof( header ) )
		memcpy( &header, file, sizeof( header ) );
	bool valid = length == expected && !memcmp( header.magic, "ASPROBE\0", 8 ) && header.version == 1 &&
				 header.count > 0 && header.count <= MAX_REFLECTION_PROBES && header.levels == 5 &&
				 ( header.size == 16 || header.size == 32 || header.size == 64 || header.size == 128 );
	const uint32_t pixels = valid ? header.size * header.size * 6 * 5 * 4 : 0;
	const uint32_t stride = sizeof( probeRecord_t ) + pixels;
	valid = valid && sizeof( header ) + header.count * stride == (uint32_t)length;
	const byte *records = (const byte *)file;
	for ( uint32_t i = 0; valid && i < header.count; ++i ) {
		probeRecord_t record;
		memcpy( &record, records + sizeof( header ) + i * stride, sizeof( record ) );
		valid = std::isfinite( record.radius ) && record.radius >= 1 && record.radius <= 32752;
		for ( float coordinate : record.origin )
			valid = valid && std::isfinite( coordinate ) && fabsf( coordinate ) <= 32752;
	}
	if ( !valid || RHI_GetCapabilities().maxBoundDescriptorSets < RHI_BINDING_COUNT ) {
		ri.FS_FreeFile( file );
		ri.Printf( PRINT_WARNING, "Reflection probes unavailable: invalid file or fewer than five bindings (%s)\n", name );
		return;
	}
	for ( uint32_t i = 0; i < header.count; ++i ) {
		probeRecord_t record;
		memcpy( &record, records + sizeof( header ) + i * stride, sizeof( record ) );
		auto &probe = tr.reflectionProbes[i];
		VectorCopy( record.origin, probe.origin );
		probe.radius = record.radius;
		char imageName[MAX_QPATH];
		Com_sprintf( imageName, sizeof( imageName ), "*reflection%u", i );
		probe.image = R_CreateImage( imageName, nullptr, (byte *)records + sizeof( header ) + i * stride + sizeof( record ),
			(int)header.size * 6, (int)header.size * 5,
			(imgFlags_t)( IMGFLAG_CLAMPTOEDGE | IMGFLAG_NOSCALE | IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION ) );
	}
	tr.numReflectionProbes = header.count;
	ri.FS_FreeFile( file );
	rhiPipelineDesc_t def = {};
	def.shader_type = TYPE_REFLECTION;
	def.state_bits = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL;
	for ( uint32_t cull = 0; cull < 3; ++cull )
		for ( uint32_t mirror = 0; mirror < 2; ++mirror )
			for ( uint32_t offset = 0; offset < 2; ++offset ) {
				def.face_culling = (cullType_t)cull;
				def.mirror = (qboolean)mirror;
				def.polygon_offset = (qboolean)offset;
				r_pipelines.reflection[cull][mirror][offset] = R_FindPipeline( 0, &def, true );
			}
	ri.Printf( PRINT_ALL, "Reflection probes: %u\n", tr.numReflectionProbes );
}
