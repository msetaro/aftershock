/* G5 C driver. Allocation/log/file stubs isolate the tested engine functions. */
#include "../../engine/qcommon/q_shared.h"
#include "../../engine/qcommon/qcommon_public.h"
#include <assert.h>

qboolean com_errorEntered;
static const char *map_path;
static unsigned hash = 2166136261U;

void QDECL Com_Error( errorParm_t level, const char *fmt, ... ) {
	va_list ap;
	(void)level;
	va_start( ap, fmt );
	vfprintf( stderr, fmt, ap );
	va_end( ap );
	exit( 1 );
}
void QDECL Com_Printf( const char *fmt, ... ) {
	(void)fmt;
}
void QDECL Com_DPrintf( const char *fmt, ... ) {
	(void)fmt;
}
void *Z_Malloc( size_t size ) {
	void *p = calloc( 1, size ? size : 1 );
	assert( p );
	return p;
}
void *S_Malloc( size_t size ) {
	return Z_Malloc( size );
}
void *Z_TagMalloc( size_t size, memtag_t tag ) {
	(void)tag;
	return Z_Malloc( size );
}
void Z_Free( void *p ) {
	free( p );
}
char *CopyString( const char *s ) {
	char *p = Z_Malloc( strlen( s ) + 1 );
	strcpy( p, s );
	return p;
}
void *Hunk_Alloc( size_t size, ha_pref pref ) {
	(void)pref;
	return Z_Malloc( size );
}
void *Hunk_AllocateTempMemory( size_t size ) {
	return Z_Malloc( size );
}
void Hunk_FreeTempMemory( void *p ) {
	free( p );
}

/* A local BSP extracted by the runner; no archive or game data enters git. */
int __wrap_FS_ReadFile( const char *name, void **buffer ) {
	FILE *f = fopen( map_path, "rb" );
	long size;
	(void)name;
	assert( f );
	assert( !fseek( f, 0, SEEK_END ) );
	size = ftell( f );
	assert( size > 0 && size < 100000000 );
	rewind( f );
	*buffer = Z_Malloc( (size_t)size + 1 );
	assert( fread( *buffer, 1, size, f ) == (size_t)size );
	fclose( f );
	return (int)size;
}

static void bytes( const void *data, size_t size ) {
	const byte *p = data;
	while ( size-- )
		hash = ( hash ^ *p++ ) * 16777619U;
}
static void number( int value ) {
	bytes( &value, sizeof( value ) );
}
static void string( const char *s ) {
	bytes( s, strlen( s ) + 1 );
}
static void result( const char *name ) {
	printf( "%s %08x\n", name, hash );
	hash = 2166136261U;
}

int main( int argc, char **argv ) {
	const char *input = "// comment\nfirst \"two words\" { -1.25 /* block */ last }";
	char info[MAX_INFO_STRING] = "", text[128];
	byte data[8192] = { 0 }, original[256];
	msg_t msg;
	netadr_t address;
	trace_t trace;
	vec3_t mins = { -15, -15, -24 }, maxs = { 15, 15, 32 }, start, end;
	int i, j, checksum, hits = 0;
	usercmd_t u0 = { 0 }, u1 = { 0 }, ur = { 0 };
	entityState_t e0 = { 0 }, e1 = { 0 }, er = { 0 };
	playerState_t p0 = { 0 }, p1 = { 0 }, pr = { 0 };
	float value, root;
	vec3_t axis = { 0, 0, 1 }, point = { 1, 2, 3 }, out, forward, right, up;
	assert( argc == 2 );
	map_path = argv[1];
	COM_BeginParseSession( "port" );
	while ( input )
		string( COM_ParseExt( &input, qtrue ) );
	result( "COM_ParseExt" );
	Info_SetValueForKey( info, "name", "sarge" );
	Info_SetValueForKey( info, "score", "17" );
	string( info );
	string( Info_ValueForKey( info, "name" ) );
	Info_RemoveKey( info, "name" );
	string( info );
	number( Info_Validate( info ) );
	result( "Info" );
	Q_strncpyz( text, "AbCd", sizeof( text ) );
	Q_strcat( text, sizeof( text ), " xyz" );
	string( text );
	number( Q_stricmp( text, "abcd XYZ" ) );
	string( Q_strlwr( text ) );
	Com_sprintf( text, sizeof( text ), "%d %.3f %s", -172, 1.25, "value" );
	string( text );
	string( va( "%s/%u", text, 123U ) );
	result( "Q_str_va_sprintf" );
	Cmd_TokenizeString( "set \"two words\" 17 // trailing" );
	number( Cmd_Argc() );
	for ( i = 0; i < Cmd_Argc(); i++ )
		string( Cmd_Argv( i ) );
	string( Cmd_ArgsFrom( 1 ) );
	result( "Cmd_Tokenize" );
	Cvar_Get( "port_value", "12", 0 );
	Cvar_Set( "port_value", "-37" );
	number( Cvar_VariableIntegerValue( "port_value" ) );
	string( Cvar_VariableString( "port_value" ) );
	Cvar_Reset( "port_value" );
	string( Cvar_VariableString( "port_value" ) );
	result( "Cvar" );
	string( FS_BuildOSPath( "/base", "baseq3", "maps\\q3dm17.bsp" ) );
	number( FS_AllowedExtension( "module.so.1", qfalse, NULL ) );
	number( FS_AllowedExtension( "data.pk3", qtrue, NULL ) );
	COM_StripExtension( "maps/test.bsp", text, sizeof( text ) );
	string( text );
	result( "FS_paths" );
	for ( i = 0; i < 256; i++ )
		original[i] = (byte)( i * 17 + i / 7 );
	MSG_Init( &msg, data, sizeof( data ) );
	MSG_WriteData( &msg, original, sizeof( original ) );
	MSG_WriteShort( &msg, -1234 );
	MSG_WriteLong( &msg, 0x12345678 );
	MSG_WriteFloat( &msg, 1.25f );
	MSG_WriteString( &msg, "message" );
	bytes( data, msg.cursize );
	MSG_BeginReading( &msg );
	for ( i = 0; i < 256; i++ )
		assert( MSG_ReadByte( &msg ) == original[i] );
	assert( MSG_ReadShort( &msg ) == -1234 );
	assert( MSG_ReadLong( &msg ) == 0x12345678 );
	assert( MSG_ReadFloat( &msg ) == 1.25f );
	assert( !strcmp( MSG_ReadString( &msg ), "message" ) );
	result( "MSG_roundtrip" );
	for ( i = 0; i < 256; i++ ) {
		memset( data, 0, sizeof( data ) );
		MSG_Init( &msg, data, sizeof( data ) );
		u1.serverTime = i * 257;
		u1.angles[1] = i * 251;
		u1.forwardmove = (signed char)( i % 127 );
		u1.buttons = i;
		u1.weapon = i % 10;
		e1.number = 17;
		e1.pos.trBase[0] = (float)i / 7.0f;
		e1.eType = i % 10;
		p1.commandTime = i * 17;
		p1.origin[1] = (float)i / 11.0f;
		p1.stats[0] = i;
		MSG_WriteDeltaUsercmdKey( &msg, 0x1234, &u0, &u1 );
		MSG_WriteDeltaEntity( &msg, &e0, &e1, qtrue );
		MSG_WriteDeltaPlayerstate( &msg, &p0, &p1 );
		bytes( data, msg.cursize );
		MSG_BeginReading( &msg );
		MSG_ReadDeltaUsercmdKey( &msg, 0x1234, &u0, &ur );
		j = MSG_ReadEntitynum( &msg );
		MSG_ReadDeltaEntity( &msg, &e0, &er, j );
		MSG_ReadDeltaPlayerstate( &msg, &p0, &pr );
		assert( !memcmp( &u1, &ur, sizeof( u1 ) ) );
		assert( !memcmp( &e1, &er, sizeof( e1 ) ) );
		assert( !memcmp( &p1, &pr, sizeof( p1 ) ) );
		u0 = u1;
		e0 = e1;
		p0 = p1;
	}
	result( "MSG_deltas" );
	memset( data, 0, sizeof( data ) );
	MSG_Init( &msg, data, sizeof( data ) );
	memcpy( data, original, sizeof( original ) );
	msg.cursize = sizeof( original );
	Huff_Compress( &msg, 0 );
	bytes( data, msg.cursize );
	Huff_Decompress( &msg, 0 );
	assert( msg.cursize == sizeof( original ) && !memcmp( data, original, sizeof( original ) ) );
	result( "Huffman_roundtrip" );
	memset( &address, 0, sizeof( address ) );
	number( NET_StringToAdr( "127.0.0.1:27960", &address, NA_IP ) );
	bytes( &address, sizeof( address ) );
	number( NET_StringToAdr( "localhost", &address, NA_IP ) );
	bytes( &address, sizeof( address ) );
	result( "NET_StringToAdr" );
	CM_LoadMap( "maps/q3dm17.bsp", qfalse, &checksum );
	number( checksum );
	for ( i = 0; i < 1000; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			start[j] = (float)( ( i * ( 37 + j * 13 ) ) % 2000 - 1000 );
			end[j] = start[j] - 512;
		}
		CM_BoxTrace( &trace, start, end, mins, maxs, 0, CONTENTS_SOLID, qfalse );
		hits += trace.fraction < 1.0f;
		number( trace.allsolid );
		number( trace.startsolid );
		bytes( &trace.fraction, sizeof( trace.fraction ) );
		bytes( trace.endpos, sizeof( trace.endpos ) );
		bytes( trace.plane.normal, sizeof( trace.plane.normal ) );
		bytes( &trace.plane.dist, sizeof( trace.plane.dist ) );
		number( trace.contents );
		number( trace.surfaceFlags );
	}
	assert( hits > 0 && hits < 1000 );
	number( hits );
	result( "CM_map_boxtraces" );
	for ( i = 1; i <= 10000; i++ ) {
		value = (float)i / 37.0f;
		root = Q_rsqrt( value );
		bytes( &root, sizeof( root ) );
		root = Q_fabs( -value );
		bytes( &root, sizeof( root ) );
	}
	result( "Q_rsqrt_Q_fabs" );
	for ( i = 1; i <= 10000; i++ ) {
		value = (float)i / 37.0f;
		RotatePointAroundVector( out, axis, point, value );
		bytes( out, sizeof( out ) );
		point[0] = value;
		vectoangles( point, out );
		bytes( out, sizeof( out ) );
		AngleVectors( out, forward, right, up );
		bytes( forward, sizeof( forward ) );
		bytes( right, sizeof( right ) );
		bytes( up, sizeof( up ) );
	}
	result( "vector_math" );
	return 0;
}
