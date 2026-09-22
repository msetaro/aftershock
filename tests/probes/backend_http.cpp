#include "../../engine/platform/http_public.h"
#include <assert.h>
#include <chrono>
#include <cstring>
#include <thread>
#include <cstdio>
#ifdef USE_CURL
#include "../../engine/platform/curl_public.h"
#endif
#ifdef USE_CURL_DLOPEN
#include <dlfcn.h>
#include <cstdarg>
void *Sys_LoadLibrary( const char *name ) {
	return dlopen( name, RTLD_NOW | RTLD_LOCAL );
}
void *Sys_LoadFunction( void *lib, const char *name ) {
	return dlsym( lib, name );
}
void Sys_UnloadLibrary( void *lib ) {
	dlclose( lib );
}
const char *Sys_Pwd() {
	return ".";
}
void QDECL Com_Printf( const char *, ... ) {
}
void QDECL Com_DPrintf( const char *, ... ) {
}
int QDECL Com_sprintf( char *out, int capacity, const char *format, ... ) {
	va_list args;
	va_start( args, format );
	const int size = vsnprintf( out, size_t( capacity ), format, args );
	va_end( args );
	return size;
}
#endif

static httpResult_t Complete() {
	httpResult_t result{};
	for ( int i = 0; i < 1500; ++i ) {
		if ( Sys_HTTPPoll( &result ) )
			return result;
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	}
	assert( false && "HTTPS request did not finish" );
	return result;
}
int main( int argc, char **argv ) {
	assert( argc == 3 );
	const char *base = argv[1], *ca = argv[2];
#ifndef USE_CURL
	assert( !Sys_HTTPStart( base, "GET", "", "", ca ) );
	httpResult_t result{};
	assert( !Sys_HTTPPoll( &result ) );
	Sys_HTTPCancel();
	puts( "PASS: HTTPS explicitly unavailable without curl" );
	return 0;
#else
	char url[1024];
	assert( !Sys_HTTPStart( "http://127.0.0.1", "GET", "", "", ca ) );
	assert( !Sys_HTTPStart( "https://user:password@example.test", "GET", "", "", ca ) );
	assert( !Sys_HTTPStart( base, "GET", "bad\r\nHeader: x", "", ca ) );
	assert( !Sys_HTTPStart( base, "CONNECT", "", "", ca ) );
	assert( Sys_HTTPStart( base, "POST", "aabb", "{\"test\":1}", ca ) );
	assert( !Sys_HTTPStart( base, "GET", "", "", ca ) );
	// A download releasing its own reference cannot unload an active HTTPS request.
	assert( Sys_CurlAcquire( DEFAULT_CURL_LIB ) );
	Sys_CurlRelease();
	auto result = Complete();

	assert( result.status == 200 && !result.failed && !strcmp( result.body, "{\"ok\":true}" ) );
	assert( !Sys_HTTPPoll( &result ) );
	snprintf( url, sizeof( url ), "%s/redirect", base );
	assert( Sys_HTTPStart( url, "GET", "secret", "", ca ) );
	result = Complete();
	assert( result.status == 302 && !result.failed );
	snprintf( url, sizeof( url ), "%s/large", base );
	assert( Sys_HTTPStart( url, "GET", "", "", ca ) );
	result = Complete();
	assert( result.failed && result.size <= HTTP_MAX_RESPONSE );
	assert( Sys_HTTPStart( base, "GET", "", "", "" ) );
	result = Complete();
	assert( result.failed ); // A private CA is never trusted implicitly.
	assert( Sys_HTTPStart( base, "GET", "", "", ca ) );
	Sys_HTTPCancel();
	assert( !Sys_HTTPPoll( &result ) );
	assert( Sys_HTTPStart( base, "POST", "aabb", "{\"test\":1}", ca ) );
	result = Complete();
	assert( result.status == 200 && !result.failed );
	puts( "PASS: native HTTPS trust, bounds, redirects, cancellation and reuse" );
#endif
}
