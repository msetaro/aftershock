#include "tr_local.h"
#include "tr_cooked.h"
#include <cmath>

static cookedPost_t post;
static image_t *postLut;
static uint8_t postHash[32];
static postRenderStats_t postStats;

void RE_PostStats( postRenderStats_t *stats ) {
	*stats = postStats;
	stats->effectsCpuUsec = tr.effectsCpuUsec;
	stats->decalsCpuUsec = tr.decalsCpuUsec;
	stats->effectsDrawCpuUsec = tr.effectsDrawCpuUsec;
	stats->lodCpuUsec = tr.lodCpuUsec;
	const auto history = R_TemporalStats();
	stats->historyStored = history.stored;
	stats->historyMatched = history.matched;
	stats->historyRejected = history.rejected;
	stats->historyOverflow = history.overflow;
}
void R_PostTemporalResult( bool drawn ) {
	++( drawn ? postStats.temporalFrames : postStats.temporalDropped );
}
void R_PostMotionDraw( bool reactive ) {
	++postStats.motionDraws;
	if ( reactive )
		++postStats.reactiveDraws;
}

void R_PostDrawResult( bool drawn ) {
	++( drawn ? postStats.draws : postStats.dropped );
}

void R_InitPost() {
	R_TemporalReset();
	postStats = {};
	post = { "default", "", 0, 0, 0, 0, 1, 256, 128, 0, 0 };
	postLut = tr.whiteImage;
	memset( postHash, 0, sizeof( postHash ) );
	r_postProfile->modified = qtrue;
}
static bool ReadPost( const cookedEntry_t *published = nullptr ) {
	const char *path = r_postProfile->string;
	if ( !path[0] || strlen( path ) >= MAX_QPATH || path[0] == '/' || strstr( path, ".." ) )
		return false;
	void *data = nullptr;
	const int size = ri.FS_ReadFile( path, &data );
	cookedPost_t settings;
	const bool valid = size > 0 && ( !published || ( (uint32_t)size == published->size && R_CookedHashMatches( data, (size_t)size, published->hash ) ) ) && R_ReadCookedPost( data, (size_t)size, &settings );
	uint8_t hash[32];
	if ( valid )
		memcpy( hash, (const byte *)data + 16, sizeof( hash ) );
	if ( data )
		ri.FS_FreeFile( data );
	if ( !valid )
		return false;
	if ( !memcmp( postHash, hash, sizeof( hash ) ) )
		return true;
	image_t *lut = tr.whiteImage;
	if ( settings.lut[0] ) {
		lut = R_FindImageFile( settings.lut, (imgFlags_t)( IMGFLAG_CLAMPTOEDGE | IMGFLAG_NOSCALE ) );
		// Horizontal 16-slice display-space LUT; never silently decode sRGB values.
		if ( !lut || lut->uploadWidth != 256 || lut->uploadHeight != 16 || lut->internalFormat != rhiFormat_t::BC7 )
			return false;
	}
	postStats.loads++;
	post = settings;
	postLut = lut;
	memcpy( postHash, hash, sizeof( hash ) );
	ri.Printf( PRINT_ALL, "Post profile loaded: %s\n", path );
	return true;
}
void R_UpdatePostProfile() {
	if ( !r_postProfile->modified )
		return;
	r_postProfile->modified = qfalse;
	if ( !r_postProfile->string[0] ) {
		R_InitPost();
		r_postProfile->modified = qfalse;
	} else if ( !ReadPost() )
		ri.Printf( PRINT_WARNING, "Post profile load failed: %s; retaining previous settings\n", r_postProfile->string );
}
#ifdef AFTERSHOCK_DEVTOOLS
void R_ReloadPost( const cookedIndex_t *index ) {
	for ( uint32_t row = 0; row < index->count; ++row ) {
		cookedEntry_t entry;
		memcpy( &entry, index->entries + row * sizeof( entry ), sizeof( entry ) );
		if ( entry.kind == 11 && !strcmp( entry.path, r_postProfile->string ) && !ReadPost( &entry ) )
			ri.Printf( PRINT_WARNING, "Post profile reload failed: %s; retaining previous settings\n", entry.path );
	}
}
#endif
void R_AddPost() {
	tr.refdef.post = {};
	tr.refdef.post.curve[0] = exp2f( post.exposureEV );
	tr.refdef.post.curve[1] = post.sharpen;
	tr.refdef.post.curve[2] = post.vignette;
	tr.refdef.post.curve[3] = post.grain;
	tr.refdef.post.lens[0] = post.lut[0] ? post.lutStrength : 0;
	tr.refdef.post.lens[1] = post.focusDistance;
	tr.refdef.post.lens[2] = post.focusRange;
	tr.refdef.post.lens[3] = post.dofRadius;
	tr.refdef.post.projection[2] = (float)( (uint32_t)tr.refdef.time & 65535u );
	tr.refdef.post.projection[3] = (float)( 1 << tr.overbrightBits );
	tr.refdef.postLut = postLut;
	tr.refdef.temporalBlur = post.motionBlur;
}
