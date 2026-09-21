#include "tr_local.h"
#include "tr_cooked.h"
#include <algorithm>

static constexpr uint32_t MAX_EFFECT_ASSETS = 64;
static fxSystem_t effects;
struct effectBinding_t {
	qhandle_t shader[FX_MAX_EMITTERS], model[FX_MAX_EMITTERS];
};
static struct {
	char path[MAX_QPATH];
	uint8_t hash[32];
	fxAsset_t asset;
	effectBinding_t bindings;
} effectAssets[MAX_EFFECT_ASSETS];
static effectBinding_t effectBindings[FX_MAX_INSTANCES];
static uint32_t effectCount, effectReloads, effectDraws, effectTime, effectLightDraws, effectLightDrops, effectSoftDraws, effectSoftDrops;
static bool effectClock;

void R_InitEffects( void ) {
	FX_Reset( &effects );
	memset( effectAssets, 0, sizeof( effectAssets ) );
	memset( effectBindings, 0, sizeof( effectBindings ) );
	effectCount = effectReloads = effectDraws = effectTime = effectLightDraws = effectLightDrops = effectSoftDraws = effectSoftDrops = 0;
	effectClock = false;
}
static bool ReadEffect( uint32_t index, const char *path, const cookedEntry_t *published = nullptr ) {
	void *data = nullptr;
	const int size = ri.FS_ReadFile( path, &data );
	fxAsset_t asset;
	const bool valid = size > 0 && ( !published || ( (uint32_t)size == published->size && R_CookedHashMatches( data, (size_t)size, published->hash ) ) ) && FX_Open( data, (size_t)size, &asset );
	uint8_t hash[32];
	if ( valid )
		memcpy( hash, (const byte *)data + 16, sizeof( hash ) );
	if ( data )
		ri.FS_FreeFile( data );
	if ( !valid )
		return false;
	effectBinding_t bindings{};
	for ( uint32_t i = 0; i < asset.header.emitterCount; ++i ) {
		bindings.shader[i] = RE_RegisterShader( asset.emitters[i].material );
		if ( asset.emitters[i].kind == FX_MESH )
			bindings.model[i] = RE_RegisterModel( asset.emitters[i].model );
		if ( !bindings.shader[i] || ( asset.emitters[i].kind == FX_MESH && !bindings.model[i] ) )
			return false;
	}
	auto &record = effectAssets[index];
	record.asset = asset;
	record.bindings = bindings;
	memcpy( record.hash, hash, sizeof( hash ) );
	if ( path != record.path )
		Q_strncpyz( record.path, path, sizeof( record.path ) );
	return true;
}
qhandle_t RE_RegisterEffect( const char *path ) {
	if ( !path || !path[0] || strlen( path ) >= MAX_QPATH || path[0] == '/' || strstr( path, ".." ) )
		return 0;
	for ( uint32_t i = 0; i < effectCount; ++i )
		if ( !strcmp( path, effectAssets[i].path ) )
			return (qhandle_t)( i + 1 );
	if ( effectCount == MAX_EFFECT_ASSETS || !ReadEffect( effectCount, path ) )
		return 0;
	return (qhandle_t)++effectCount;
}
uint32_t RE_StartEffect( qhandle_t asset, const vec3_t origin, const vec3_t axis[3], uint32_t seed ) {
	if ( asset <= 0 || (uint32_t)asset > effectCount )
		return 0;
	const auto &record = effectAssets[asset - 1];
	const uint32_t handle = FX_Start( &effects, &record.asset, origin, axis, seed );
	if ( handle )
		for ( uint32_t i = 0; i < FX_MAX_INSTANCES; ++i )
			if ( effects.instances[i].handle == handle ) {
				effectBindings[i] = record.bindings;
				break;
			}
	return handle;
}
bool RE_StopEffect( uint32_t handle ) {
	return FX_Stop( &effects, handle );
}
void RE_EffectStats( fxRenderStats_t *stats ) {
	*stats = { effects.stats, effectCount, effectReloads, effectDraws, effectLightDraws, effectLightDrops, effectSoftDraws, effectSoftDrops };
}
void R_EffectSoftDraw( bool drawn ) {
	++( drawn ? effectSoftDraws : effectSoftDrops );
}
static bool EffectTrace( const float start[3], const float end[3], fxTrace_t *result, void * ) {
	if ( !ri.CM_BoxTrace )
		return false;
	trace_t hit;
	ri.CM_BoxTrace( &hit, start, end, vec3_origin, vec3_origin, 0, CONTENTS_SOLID, qfalse );
	result->fraction = hit.fraction;
	VectorCopy( hit.plane.normal, result->normal );
	return hit.fraction < 1;
}
void R_AddEffects( const refdef_t *view ) {
	if ( view->rdflags & RDF_NOWORLDMODEL )
		return;
	const uint32_t now = (uint32_t)view->time;
	if ( effectClock && int32_t( now - effectTime ) < 0 )
		FX_Reset( &effects );
	else if ( effectClock && now != effectTime && effects.stats.instances )
		FX_Update( &effects, now - effectTime, EffectTrace, nullptr );
	effectClock = true;
	effectTime = now;
	if ( !effects.stats.particles )
		return;
	float lights[FX_MAX_INSTANCES][FX_MAX_EMITTERS]{};
	for ( const auto &particle : effects.particles ) {
		if ( !particle.active )
			continue;
		const auto &instance = effects.instances[particle.instance];
		const auto &emitter = instance.asset.emitters[particle.emitter];
		const auto &binding = effectBindings[particle.instance];
		const float life = float( particle.ageMs ) / float( emitter.lifetimeMs );
		const float size = emitter.size + ( emitter.endSize - emitter.size ) * life;
		lights[particle.instance][particle.emitter] = std::max( lights[particle.instance][particle.emitter], 1 - life );
		color4ub_t color;
		for ( uint32_t c = 0; c < 4; ++c )
			color.rgba[c] = (byte)( emitter.color[c] * 255 );
		color.rgba[3] = (byte)( float( color.rgba[3] ) * ( 1 - life ) );
		if ( emitter.kind == FX_MESH ) {
			refEntity_t entity{};
			entity.reType = RT_MODEL;
			entity.hModel = binding.model[particle.emitter];
			entity.customShader = binding.shader[particle.emitter];
			VectorCopy( particle.origin, entity.origin );
			VectorCopy( particle.previous, entity.oldorigin );
			VectorCopy( particle.origin, entity.lightingOrigin );
			for ( uint32_t i = 0; i < 3; ++i )
				VectorScale( instance.axis[i], size, entity.axis[i] );
			if ( particle.rotation ) {
				for ( uint32_t i = 1; i < 3; ++i ) {
					vec3_t rotated;
					RotatePointAroundVector( rotated, instance.axis[0], instance.axis[i], particle.rotation );
					VectorScale( rotated, size, entity.axis[i] );
				}
			}
			entity.nonNormalizedAxes = qtrue;
			entity.shader = color;
			RE_AddRefEntityToScene( &entity, qfalse );
		} else {
			if ( emitter.flags & FX_LIT ) {
				vec3_t at, ambient, directed, direction;
				VectorCopy( particle.origin, at );
				if ( R_LightForPoint( at, ambient, directed, direction ) )
					for ( uint32_t c = 0; c < 3; ++c )
						color.rgba[c] = (byte)( float( color.rgba[c] ) * std::clamp( ( ambient[c] + directed[c] ) / 255.f, 0.f, 1.f ) );
			}
			vec3_t right, up;
			VectorScale( view->viewaxis[1], size, right );
			VectorScale( view->viewaxis[2], size, up );
			if ( emitter.kind == FX_TRAIL ) {
				VectorSubtract( particle.origin, particle.previous, up );
				CrossProduct( up, view->viewaxis[0], right );
				if ( VectorNormalize( right ) == 0 )
					continue;
				VectorScale( right, size, right );
				VectorScale( up, .5f, up );
			}
			if ( emitter.kind == FX_SPRITE && particle.rotation ) {
				vec3_t rotated;
				RotatePointAroundVector( rotated, view->viewaxis[0], right, particle.rotation );
				VectorCopy( rotated, right );
				RotatePointAroundVector( rotated, view->viewaxis[0], up, particle.rotation );
				VectorCopy( rotated, up );
			}
			const float u = float( particle.frame % emitter.columns ) / float( emitter.columns );
			const float v = float( particle.frame / emitter.columns ) / float( emitter.rows );
			polyVert_t vertices[4];
			static constexpr float corners[4][2] = { { 1, 1 }, { -1, 1 }, { -1, -1 }, { 1, -1 } };
			for ( uint32_t vertex = 0; vertex < 4; ++vertex ) {
				for ( uint32_t c = 0; c < 3; ++c ) {
					const float center = emitter.kind == FX_TRAIL ? ( particle.origin[c] + particle.previous[c] ) * .5f : particle.origin[c];
					vertices[vertex].xyz[c] = center + corners[vertex][0] * right[c] + corners[vertex][1] * up[c];
				}
				vertices[vertex].st[0] = u + ( 1 - corners[vertex][0] ) * .5f / float( emitter.columns );
				vertices[vertex].st[1] = v + ( 1 - corners[vertex][1] ) * .5f / float( emitter.rows );
				vertices[vertex].modulate = color;
			}
			R_AddEffectPoly( binding.shader[particle.emitter], vertices, emitter.flags & FX_SOFT ? std::max( size, .01f ) : 0 );
		}
		++effectDraws;
	}
	// One light per active emitter, bounded by the scene's existing light pool.
	for ( uint32_t i = 0; i < FX_MAX_INSTANCES; i++ ) {
		const auto &instance = effects.instances[i];
		if ( !instance.handle )
			continue;
		for ( uint32_t e = 0; e < instance.asset.header.emitterCount; e++ ) {
			const auto &emitter = instance.asset.emitters[e];
			const float intensity = lights[i][e] * emitter.lightIntensity;
			if ( intensity <= 0 || emitter.lightRadius <= 0 )
				continue;
			const int before = r_numdlights;
			RE_AddLightToScene( instance.origin, emitter.lightRadius, emitter.lightColor[0] * intensity, emitter.lightColor[1] * intensity, emitter.lightColor[2] * intensity );
			if ( r_numdlights > before )
				++effectLightDraws;
			else
				++effectLightDrops;
		}
	}
}
#ifdef AFTERSHOCK_DEVTOOLS
void R_ReloadEffects( const cookedIndex_t *index ) {
	for ( uint32_t row = 0; row < index->count; ++row ) {
		cookedEntry_t entry;
		memcpy( &entry, index->entries + row * sizeof( entry ), sizeof( entry ) );
		if ( entry.kind != 8 )
			continue;
		for ( uint32_t i = 0; i < effectCount; ++i ) {
			if ( strcmp( entry.path, effectAssets[i].path ) )
				continue;
			// The publication index hashes whole files; definitions retain payload hashes.
			uint8_t old[32];
			memcpy( old, effectAssets[i].hash, sizeof( old ) );
			if ( ReadEffect( i, effectAssets[i].path, &entry ) && memcmp( old, effectAssets[i].hash, sizeof( old ) ) )
				++effectReloads;
		}
	}
}
#endif
