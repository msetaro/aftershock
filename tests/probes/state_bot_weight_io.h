// Fixed owned source and filesystem/allocator doubles for checkpoint construction.
#pragma once
#include <assert.h>
#include <stdlib.h>

botlib_import_t botimport{};
static char reloadName[] = "bot_reloadcharacters", reloadText[] = "0";
static libvar_t reload{ reloadName, reloadText, 17, qtrue, 0, nullptr };
libvar_t *LibVarGet( const char *name ) {
	assert(!strcmp(name,reloadName));
	return &reload;
}
float LibVarGetValue( const char *name ) {
	return LibVarGet( name )->value;
}
static int allocations;
void *GetMemory( size_t size ) {
	void *p = malloc( size );
	assert(p);
	++allocations;
	return p;
}
void *GetClearedMemory( size_t size ) {
	void *p = GetMemory( size );
	memset( p, 0, size );
	return p;
}
void FreeMemory( void *p ) {
	if ( p ) {
		--allocations;
		free( p );
	}
}
void Q_strncpyz( char *out, const char *in, int size ) {
	assert(size>0 && strlen(in)<size_t(size));
	strcpy( out, in );
}
int QDECL Com_sprintf( char *out, int size, const char *format, ... ) {
	va_list args;
	va_start( args, format );
	const int result = vsnprintf( out, size_t( size ), format, args );
	va_end( args );
	assert(result>=0 && result<size);
	return result;
}
// The fixed weight source has no includes or clock macros.
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
int Q_stricmp( const char *, const char * ) {
	abort();
}
void Q_strcat( char *, int, const char * ) {
	abort();
}
time_t Sys_Time( time_t * ) {
	abort();
}
const char *Sys_CTime( const time_t * ) {
	abort();
}
static const char *sourceText = "weight \"choice\" return balance(7.5, 1, 12);";
static int Open( const char *path, fileHandle_t *file, fsMode_t mode ) {
	assert(!strcmp(path,"botfiles/checkpoint_w.c") && mode==FS_READ);
	*file = 1;
	return int( strlen( sourceText ) );
}
static int Read( void *out, int size, fileHandle_t file ) {
	assert(file==1 && size==int(strlen(sourceText)));
	memcpy( out, sourceText, size_t( size ) );
	return size;
}
static void Close( fileHandle_t file ) {
	assert(file==1);
}
static void QDECL Print( int, const char *, ... ) {
}
void QDECL Log_Write( const char *, ... ) {
}
