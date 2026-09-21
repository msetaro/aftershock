#pragma once

#include <assert.h>

#if defined( AFTERSHOCK_DEVTOOLS ) && !defined( NDEBUG ) && defined( __cplusplus )
// One trivial callback per linked image. Renderer modules receive the engine's
// reporter through their existing import table; standalone probes need none.
using qAssertReporter_t = void ( * )( const char *expression, const char *file, int line );
inline qAssertReporter_t q_assertReporter = nullptr;
inline void Q_ReportAssertion( const char *expression, const char *file, int line ) {
	if ( q_assertReporter )
		q_assertReporter( expression, file, line );
}
#define Q_ASSERT( condition ) ((condition) ? (void)0 : (Q_ReportAssertion(#condition, __FILE__, __LINE__), assert(0 && #condition)))
#else
#define Q_ASSERT assert
#endif
