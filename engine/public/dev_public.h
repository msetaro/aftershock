#pragma once

#include <stdint.h>

// Engine-thread only. Explicit begin/end keeps the longjmp error model intact.
// Tokens are invalidated every frame; abandoned scopes are not reported.
#ifdef AFTERSHOCK_DEVTOOLS
#ifdef __cplusplus
extern "C" {
#endif
uint64_t Dev_BeginScope( const char *name );
void Dev_EndScope( uint64_t token );
void Dev_PredictionError( float distance );
void Dev_RewindReport( uint32_t age, uint32_t limit, int clamped, int hit );
// Colors are 0xAABBGGRR. Zero duration lasts one frame; positive durations cap at 60s.
void Dev_DrawLine( const float *start, const float *end, uint32_t color, int durationMsec );
void Dev_DrawBox( const float *mins, const float *maxs, uint32_t color, int durationMsec );
void Dev_DrawText( const float *origin, const char *text, uint32_t color, int durationMsec );
#ifdef __cplusplus
}
#endif
#else
#define Dev_BeginScope( name ) UINT64_MAX
#define Dev_EndScope( token ) ((void)(token))
#define Dev_PredictionError( distance ) ((void)0)
#define Dev_RewindReport( ... ) ((void)0)
#define Dev_DrawLine( ... ) ((void)0)
#define Dev_DrawBox( ... ) ((void)0)
#define Dev_DrawText( ... ) ((void)0)
#endif
