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
#ifdef __cplusplus
}
#endif
#else
#define Dev_BeginScope( name ) UINT64_MAX
#define Dev_EndScope( token ) ((void)0)
#define Dev_PredictionError( distance ) ((void)0)
#endif
