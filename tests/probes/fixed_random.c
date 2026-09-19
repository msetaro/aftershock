/* Test-only OS input fixture, loaded only by the offline demo runner.
 * Each /dev/urandom stream contains identical bytes, making checksumFeed repeat.
 * Never link or preload this into a production or network-facing invocation.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

static unsigned char input[4096];

FILE *fopen( const char *name, const char *mode ) {
	FILE *( *next )( const char *, const char * );
	if ( !strcmp( name, "/dev/urandom" ) ) {
		return fmemopen( input, sizeof( input ), "rb" );
	}
	next = ( FILE * (*)(const char *, const char *)) dlsym( RTLD_NEXT, "fopen" );
	return next( name, mode );
}

FILE *fopen64( const char *name, const char *mode ) {
	FILE *( *next )( const char *, const char * );
	if ( !strcmp( name, "/dev/urandom" ) ) {
		return fmemopen( input, sizeof( input ), "rb" );
	}
	next = ( FILE * (*)(const char *, const char *)) dlsym( RTLD_NEXT, "fopen64" );
	return next( name, mode );
}
