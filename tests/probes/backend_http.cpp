#include "../../engine/platform/http_public.h"
#include <assert.h>
#include <chrono>
#include <cstring>
#include <thread>
#include <cstdio>

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
	char url[1024];
	assert( !Sys_HTTPStart( "http://127.0.0.1", "GET", "", "", ca ) );
	assert( !Sys_HTTPStart( "https://user:password@example.test", "GET", "", "", ca ) );
	assert( !Sys_HTTPStart( base, "GET", "bad\r\nHeader: x", "", ca ) );
	assert( !Sys_HTTPStart( base, "CONNECT", "", "", ca ) );
	assert( Sys_HTTPStart( base, "POST", "aabb", "{\"test\":1}", ca ) );
	assert( !Sys_HTTPStart( base, "GET", "", "", ca ) );
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
}
