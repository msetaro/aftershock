/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "client.h"
#include "../platform/services_public.h"

#include "../public/cg_native_public.h"
#include "../public/ui_native_public.h"

#include "../botlib/botlib_public.h"

extern botlib_export_t *botlib_export;
static uint64_t uiServiceSearch;


/*
====================
GetClientState
====================
*/
static void GetClientState( uiClientState_t *state ) {
	state->connectPacketCount = clc.connectPacketCount;
	state->connState = cls.state;
	Q_strncpyz( state->servername, cls.servername, sizeof( state->servername ) );
	Q_strncpyz( state->updateInfoString, cls.updateInfoString, sizeof( state->updateInfoString ) );
	Q_strncpyz( state->messageString, clc.serverMessage, sizeof( state->messageString ) );
	state->clientNum = cl.snap.ps.clientNum;
}


/*
====================
LAN_LoadCachedServers
====================
*/
static void LAN_LoadCachedServers( void ) {
	fileHandle_t fileIn;
	int32_t size, file_size;

	cls.numglobalservers = cls.numfavoriteservers = 0;
	cls.numGlobalServerAddresses = 0;

	file_size = FS_Home_FOpenFileRead( "servercache.dat", &fileIn );
	if ( (size_t)file_size < ( 3 * sizeof( int32_t ) ) ) {
		if ( fileIn != FS_INVALID_HANDLE ) {
			FS_FCloseFile( fileIn );
		}
		return;
	}

	size = 0;
	FS_Read( &cls.numglobalservers, sizeof( int32_t ), fileIn );
	FS_Read( &cls.numfavoriteservers, sizeof( int32_t ), fileIn );
	FS_Read( &size, sizeof( int32_t ), fileIn );

	if ( size == sizeof( cls.globalServers ) + sizeof( cls.favoriteServers ) ) {
		FS_Read( &cls.globalServers, sizeof( cls.globalServers ), fileIn );
		FS_Read( &cls.favoriteServers, sizeof( cls.favoriteServers ), fileIn );
	} else {
		cls.numglobalservers = cls.numfavoriteservers = 0;
		cls.numGlobalServerAddresses = 0;
	}

	FS_FCloseFile( fileIn );
}


/*
====================
LAN_SaveServersToCache
====================
*/
static void LAN_SaveServersToCache( void ) {
	fileHandle_t fileOut;
	int32_t size;

	fileOut = FS_FOpenFileWrite( "servercache.dat" );
	if ( fileOut == FS_INVALID_HANDLE )
		return;

	FS_Write( &cls.numglobalservers, sizeof( int32_t ), fileOut );
	FS_Write( &cls.numfavoriteservers, sizeof( int32_t ), fileOut );
	size = sizeof( cls.globalServers ) + sizeof( cls.favoriteServers );
	FS_Write( &size, sizeof( int32_t ), fileOut );
	FS_Write( &cls.globalServers, sizeof( cls.globalServers ), fileOut );
	FS_Write( &cls.favoriteServers, sizeof( cls.favoriteServers ), fileOut );

	FS_FCloseFile( fileOut );
}


/*
====================
LAN_ResetPings
====================
*/
static void LAN_ResetPings( int source ) {
	int count, i;
	serverInfo_t *servers = NULL;
	count = 0;

	switch ( source ) {
	case AS_LOCAL:
		servers = &cls.localServers[0];
		count = MAX_OTHER_SERVERS;
		break;
	case AS_MPLAYER:
	case AS_GLOBAL:
		servers = &cls.globalServers[0];
		count = MAX_GLOBAL_SERVERS;
		break;
	case AS_FAVORITES:
		servers = &cls.favoriteServers[0];
		count = MAX_OTHER_SERVERS;
		break;
	}
	if ( servers ) {
		for ( i = 0; i < count; i++ ) {
			servers[i].ping = -1;
		}
	}
}


/*
====================
LAN_AddServer
====================
*/
static int LAN_AddServer( int source, const char *name, const char *address ) {
	int max, *count, i;
	netadr_t adr;
	serverInfo_t *servers = NULL;
	max = MAX_OTHER_SERVERS;
	count = NULL;

	switch ( source ) {
	case AS_LOCAL:
		count = &cls.numlocalservers;
		servers = &cls.localServers[0];
		break;
	case AS_MPLAYER:
	case AS_GLOBAL:
		max = MAX_GLOBAL_SERVERS;
		count = &cls.numglobalservers;
		servers = &cls.globalServers[0];
		break;
	case AS_FAVORITES:
		count = &cls.numfavoriteservers;
		servers = &cls.favoriteServers[0];
		break;
	}
	if ( servers && *count < max ) {
		NET_StringToAdr( address, &adr, NA_UNSPEC );
		for ( i = 0; i < *count; i++ ) {
			if ( NET_CompareAdr( &servers[i].adr, &adr ) ) {
				break;
			}
		}
		if ( i >= *count ) {
			servers[*count].adr = adr;
			Q_strncpyz( servers[*count].hostName, name, sizeof( servers[*count].hostName ) );
			servers[*count].visible = qtrue;
			( *count )++;
			return 1;
		}
		return 0;
	}
	return -1;
}


/*
====================
LAN_RemoveServer
====================
*/
static void LAN_RemoveServer( int source, const char *addr ) {
	int *count, i;
	serverInfo_t *servers = NULL;
	count = NULL;
	switch ( source ) {
	case AS_LOCAL:
		count = &cls.numlocalservers;
		servers = &cls.localServers[0];
		break;
	case AS_MPLAYER:
	case AS_GLOBAL:
		count = &cls.numglobalservers;
		servers = &cls.globalServers[0];
		break;
	case AS_FAVORITES:
		count = &cls.numfavoriteservers;
		servers = &cls.favoriteServers[0];
		break;
	}
	if ( servers ) {
		netadr_t comp;
		NET_StringToAdr( addr, &comp, NA_UNSPEC );
		for ( i = 0; i < *count; i++ ) {
			if ( NET_CompareAdr( &comp, &servers[i].adr ) ) {
				int j = i;
				while ( j < *count - 1 ) {
					Com_Memcpy( &servers[j], &servers[j + 1], sizeof( servers[j] ) );
					j++;
				}
				( *count )--;
				break;
			}
		}
	}
}


/*
====================
LAN_GetServerCount
====================
*/
static int LAN_GetServerCount( int source ) {
	switch ( source ) {
	case AS_LOCAL:
		return cls.numlocalservers;
		break;
	case AS_MPLAYER:
	case AS_GLOBAL:
		return cls.numglobalservers;
		break;
	case AS_FAVORITES:
		return cls.numfavoriteservers;
		break;
	}
	return 0;
}


/*
====================
LAN_GetLocalServerAddressString
====================
*/
static void LAN_GetServerAddressString( int source, int n, char *buf, int buflen ) {
	switch ( source ) {
	case AS_LOCAL:
		if ( n >= 0 && n < MAX_OTHER_SERVERS ) {
			Q_strncpyz( buf, NET_AdrToStringwPort( &cls.localServers[n].adr ), buflen );
			return;
		}
		break;
	case AS_MPLAYER:
	case AS_GLOBAL:
		if ( n >= 0 && n < MAX_GLOBAL_SERVERS ) {
			Q_strncpyz( buf, NET_AdrToStringwPort( &cls.globalServers[n].adr ), buflen );
			return;
		}
		break;
	case AS_FAVORITES:
		if ( n >= 0 && n < MAX_OTHER_SERVERS ) {
			Q_strncpyz( buf, NET_AdrToStringwPort( &cls.favoriteServers[n].adr ), buflen );
			return;
		}
		break;
	}
	buf[0] = '\0';
}


/*
====================
LAN_GetServerInfo
====================
*/
static void LAN_GetServerInfo( int source, int n, char *buf, int buflen ) {
	char info[MAX_STRING_CHARS];
	serverInfo_t *server = NULL;
	info[0] = '\0';
	switch ( source ) {
	case AS_LOCAL:
		if ( n >= 0 && n < MAX_OTHER_SERVERS ) {
			server = &cls.localServers[n];
		}
		break;
	case AS_MPLAYER:
	case AS_GLOBAL:
		if ( n >= 0 && n < MAX_GLOBAL_SERVERS ) {
			server = &cls.globalServers[n];
		}
		break;
	case AS_FAVORITES:
		if ( n >= 0 && n < MAX_OTHER_SERVERS ) {
			server = &cls.favoriteServers[n];
		}
		break;
	}
	if ( server && buf ) {
		buf[0] = '\0';
		Info_SetValueForKey( info, "hostname", server->hostName );
		Info_SetValueForKey( info, "mapname", server->mapName );
		Info_SetValueForKey( info, "clients", va( "%i", server->clients ) );
		Info_SetValueForKey( info, "sv_maxclients", va( "%i", server->maxClients ) );
		Info_SetValueForKey( info, "ping", va( "%i", server->ping ) );
		Info_SetValueForKey( info, "minping", va( "%i", server->minPing ) );
		Info_SetValueForKey( info, "maxping", va( "%i", server->maxPing ) );
		Info_SetValueForKey( info, "game", server->game );
		Info_SetValueForKey( info, "gametype", va( "%i", server->gameType ) );
		Info_SetValueForKey( info, "nettype", va( "%i", server->netType ) );
		Info_SetValueForKey( info, "addr", NET_AdrToStringwPort( &server->adr ) );
		Info_SetValueForKey( info, "punkbuster", va( "%i", server->punkbuster ) );
		Info_SetValueForKey( info, "g_needpass", va( "%i", server->g_needpass ) );
		Info_SetValueForKey( info, "g_humanplayers", va( "%i", server->g_humanplayers ) );
		Q_strncpyz( buf, info, buflen );
	} else {
		if ( buf ) {
			buf[0] = '\0';
		}
	}
}


/*
====================
LAN_GetServerPing
====================
*/
static int LAN_GetServerPing( int source, int n ) {
	serverInfo_t *server = NULL;
	switch ( source ) {
	case AS_LOCAL:
		if ( n >= 0 && n < MAX_OTHER_SERVERS ) {
			server = &cls.localServers[n];
		}
		break;
	case AS_MPLAYER:
	case AS_GLOBAL:
		if ( n >= 0 && n < MAX_GLOBAL_SERVERS ) {
			server = &cls.globalServers[n];
		}
		break;
	case AS_FAVORITES:
		if ( n >= 0 && n < MAX_OTHER_SERVERS ) {
			server = &cls.favoriteServers[n];
		}
		break;
	}
	if ( server ) {
		return server->ping;
	}
	return -1;
}

/*
====================
LAN_GetServerPtr
====================
*/
static serverInfo_t *LAN_GetServerPtr( int source, int n ) {
	switch ( source ) {
	case AS_LOCAL:
		if ( n >= 0 && n < MAX_OTHER_SERVERS ) {
			return &cls.localServers[n];
		}
		break;
	case AS_MPLAYER:
	case AS_GLOBAL:
		if ( n >= 0 && n < MAX_GLOBAL_SERVERS ) {
			return &cls.globalServers[n];
		}
		break;
	case AS_FAVORITES:
		if ( n >= 0 && n < MAX_OTHER_SERVERS ) {
			return &cls.favoriteServers[n];
		}
		break;
	}
	return NULL;
}


/*
====================
LAN_CompareServers
====================
*/
static int LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 ) {
	int res;
	serverInfo_t *server1, *server2;

	server1 = LAN_GetServerPtr( source, s1 );
	server2 = LAN_GetServerPtr( source, s2 );
	if ( !server1 || !server2 ) {
		return 0;
	}

	res = 0;
	switch ( sortKey ) {
	case SORT_HOST:
		res = Q_stricmp( server1->hostName, server2->hostName );
		break;

	case SORT_MAP:
		res = Q_stricmp( server1->mapName, server2->mapName );
		break;
	case SORT_CLIENTS:
		if ( server1->clients < server2->clients ) {
			res = -1;
		} else if ( server1->clients > server2->clients ) {
			res = 1;
		} else {
			res = 0;
		}
		break;
	case SORT_GAME:
		if ( server1->gameType < server2->gameType ) {
			res = -1;
		} else if ( server1->gameType > server2->gameType ) {
			res = 1;
		} else {
			res = 0;
		}
		break;
	case SORT_PING:
		if ( server1->ping < server2->ping ) {
			res = -1;
		} else if ( server1->ping > server2->ping ) {
			res = 1;
		} else {
			res = 0;
		}
		break;
	}

	if ( sortDir ) {
		if ( res < 0 )
			return 1;
		if ( res > 0 )
			return -1;
		return 0;
	}
	return res;
}


/*
====================
LAN_GetPingQueueCount
====================
*/
static int LAN_GetPingQueueCount( void ) {
	return ( CL_GetPingQueueCount() );
}


/*
====================
LAN_ClearPing
====================
*/
static void LAN_ClearPing( int n ) {
	CL_ClearPing( n );
}


/*
====================
LAN_GetPing
====================
*/
static void LAN_GetPing( int n, char *buf, int buflen, int *pingtime ) {
	CL_GetPing( n, buf, buflen, pingtime );
}


/*
====================
LAN_GetPingInfo
====================
*/
static void LAN_GetPingInfo( int n, char *buf, int buflen ) {
	CL_GetPingInfo( n, buf, buflen );
}


/*
====================
LAN_MarkServerVisible
====================
*/
static void LAN_MarkServerVisible( int source, int n, qboolean visible ) {
	if ( n == -1 ) {
		int count = MAX_OTHER_SERVERS;
		serverInfo_t *server = NULL;
		switch ( source ) {
		case AS_LOCAL:
			server = &cls.localServers[0];
			break;
		case AS_MPLAYER:
		case AS_GLOBAL:
			server = &cls.globalServers[0];
			count = MAX_GLOBAL_SERVERS;
			break;
		case AS_FAVORITES:
			server = &cls.favoriteServers[0];
			break;
		}
		if ( server ) {
			for ( n = 0; n < count; n++ ) {
				server[n].visible = visible;
			}
		}

	} else {
		switch ( source ) {
		case AS_LOCAL:
			if ( n >= 0 && n < MAX_OTHER_SERVERS ) {
				cls.localServers[n].visible = visible;
			}
			break;
		case AS_MPLAYER:
		case AS_GLOBAL:
			if ( n >= 0 && n < MAX_GLOBAL_SERVERS ) {
				cls.globalServers[n].visible = visible;
			}
			break;
		case AS_FAVORITES:
			if ( n >= 0 && n < MAX_OTHER_SERVERS ) {
				cls.favoriteServers[n].visible = visible;
			}
			break;
		}
	}
}


/*
=======================
LAN_ServerIsVisible
=======================
*/
static int LAN_ServerIsVisible( int source, int n ) {
	switch ( source ) {
	case AS_LOCAL:
		if ( n >= 0 && n < MAX_OTHER_SERVERS ) {
			return cls.localServers[n].visible;
		}
		break;
	case AS_MPLAYER:
	case AS_GLOBAL:
		if ( n >= 0 && n < MAX_GLOBAL_SERVERS ) {
			return cls.globalServers[n].visible;
		}
		break;
	case AS_FAVORITES:
		if ( n >= 0 && n < MAX_OTHER_SERVERS ) {
			return cls.favoriteServers[n].visible;
		}
		break;
	}
	return qfalse;
}


/*
=======================
LAN_UpdateVisiblePings
=======================
*/
static qboolean LAN_UpdateVisiblePings( int source ) {
	return CL_UpdateVisiblePings_f( source );
}


/*
====================
LAN_GetServerStatus
====================
*/
static int LAN_GetServerStatus( const char *serverAddress, char *serverStatus, int maxLen ) {
	return CL_ServerStatus( serverAddress, serverStatus, maxLen );
}


/*
====================
CL_GetGlConfig
====================
*/
static void CL_GetGlconfig( glconfig_t *config ) {
	*config = *re.GetConfig();
}


/*
====================
CL_GetClipboardData
====================
*/
static void CL_GetClipboardData( char *buf, int buflen ) {
	char *cbd;

	cbd = Sys_GetClipboardData();

	if ( !cbd ) {
		*buf = '\0';
		return;
	}

	Q_strncpyz( buf, cbd, buflen );

	Z_Free( cbd );
}


/*
====================
Key_KeynumToStringBuf
====================
*/
static void Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) {
	Q_strncpyz( buf, Key_KeynumToString( keynum ), buflen );
}


/*
====================
Key_GetBindingBuf
====================
*/
static void Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	const char *value;

	value = Key_GetBinding( keynum );
	if ( value ) {
		Q_strncpyz( buf, value, buflen );
	} else {
		*buf = '\0';
	}
}


/*
====================
CLUI_GetCDKey
====================
*/
static void CLUI_GetCDKey( char *buf, int buflen [[maybe_unused]] ) {
#ifndef STANDALONE
	const char *gamedir;
	gamedir = Cvar_VariableString( "fs_game" );
	if ( UI_usesUniqueCDKey() && gamedir[0] != '\0' ) {
		Com_Memcpy( buf, &cl_cdkey[16], 16 );
		buf[16] = '\0';
	} else {
		Com_Memcpy( buf, cl_cdkey, 16 );
		buf[16] = '\0';
	}
#else
	*buf = '\0';
#endif
}


/*
====================
CLUI_SetCDKey
====================
*/
#ifndef STANDALONE
static void CLUI_SetCDKey( char *buf ) {
	const char *gamedir;
	gamedir = Cvar_VariableString( "fs_game" );
	if ( UI_usesUniqueCDKey() && gamedir[0] != '\0' ) {
		Com_Memcpy( &cl_cdkey[16], buf, 16 );
		cl_cdkey[32] = '\0';
		// set the flag so the flag will be written at the next opportunity
		cvar_modifiedFlags |= CVAR_ARCHIVE;
	} else {
		Com_Memcpy( cl_cdkey, buf, 16 );
		// set the flag so the flag will be written at the next opportunity
		cvar_modifiedFlags |= CVAR_ARCHIVE;
	}
}
#endif


/*
====================
GetConfigString
====================
*/
static int GetConfigString( int index, char *buf, int size ) {
	int offset;

	if ( index < 0 || index >= MAX_CONFIGSTRINGS )
		return qfalse;

	offset = cl.gameState.stringOffsets[index];
	if ( !offset ) {
		if ( size ) {
			buf[0] = 0;
		}
		return qfalse;
	}

	Q_strncpyz( buf, cl.gameState.stringData + offset, size );

	return qtrue;
}


/*
====================
FloatAsInt
====================
*/
void UIImport_Print( const char *string ) {

	Com_Printf( "%s", (const char *)string );
	return;
}
void UIImport_Error( const char *string ) {

	Com_Error( ERR_DROP, "%s", (const char *)string );
	return;
}
int UIImport_Milliseconds( void ) {

	return Sys_Milliseconds();
}
void UIImport_Cvar_Register( void *cvar, const char *var_name, const char *value, int flags ) {

	Cvar_Register( (vmCvar_t *)cvar, (const char *)var_name, (const char *)value, flags, 0 );
	return;
}
void UIImport_Cvar_Update( void *cvar ) {

	Cvar_Update( (vmCvar_t *)cvar, 0 );
	return;
}
void UIImport_Cvar_Set( const char *var_name, const char *value ) {

	Cvar_SetSafe( (const char *)var_name, (const char *)value );
	return;
}
float UIImport_Cvar_VariableValue( const char *var_name ) {

	return Cvar_VariableValue( (const char *)var_name );
}
void UIImport_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {

	Cvar_VariableStringBufferSafe( (const char *)var_name, (char *)buffer, bufsize, CVAR_PRIVATE );
	return;
}
void UIImport_Cvar_SetValue( const char *var_name, float value ) {

	Cvar_SetValueSafe( (const char *)var_name, value );
	return;
}
void UIImport_Cvar_Reset( const char *name ) {

	Cvar_Reset( (const char *)name );
	return;
}
void UIImport_Cvar_Create( const char *var_name, const char *var_value, int flags ) {

	Cvar_Register( NULL, (const char *)var_name, (const char *)var_value, flags, 0 );
	return;
}
void UIImport_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize ) {

	Cvar_InfoStringBuffer( bit, (char *)buffer, bufsize );
	return;
}
int UIImport_Argc( void ) {

	return Cmd_Argc();
}
void UIImport_Argv( int n, char *buffer, int bufferLength ) {

	Cmd_ArgvBuffer( n, (char *)buffer, bufferLength );
	return;
}
void UIImport_Cmd_ExecuteText( int exec_when, const char *text ) {

	if ( exec_when == EXEC_NOW && ( !strncmp( (const char *)text, "snd_restart", 11 ) || !strncmp( (const char *)text, "vid_restart", 11 ) || !strncmp( (const char *)text, "disconnect", 10 ) || !strncmp( (const char *)text, "quit", 5 ) ) ) {
		Com_Printf( S_COLOR_YELLOW "turning EXEC_NOW '%.11s' into EXEC_INSERT\n", (const char *)text );
		exec_when = EXEC_INSERT;
	}
	Cbuf_ExecuteText( (cbufExec_t)exec_when, (const char *)text );
	return;
}
int UIImport_FS_FOpenFile( const char *qpath, void *f, int mode ) {

	return FS_VM_OpenFile( (const char *)qpath, (fileHandle_t *)f, (fsMode_t)mode, H_Q3UI );
}
void UIImport_FS_Read( void *buffer, int len, int f ) {

	FS_VM_ReadFile( buffer, len, f, H_Q3UI );
	return;
}
void UIImport_FS_Write( const void *buffer, int len, int f ) {

	FS_VM_WriteFile( (void *)buffer, len, f, H_Q3UI );
	return;
}
void UIImport_FS_FCloseFile( int f ) {

	FS_VM_CloseFile( f, H_Q3UI );
	return;
}
int UIImport_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize ) {

	return FS_GetFileList( (const char *)path, (const char *)extension, (char *)listbuf, bufsize );
}
int UIImport_FS_Seek( int f, int64_t offset, int origin ) {

	return FS_VM_SeekFile( f, (fsOffset_t)offset, (fsOrigin_t)origin, H_Q3UI );
}
int UIImport_R_RegisterModel( const char *name ) {

	return re.RegisterModel( (const char *)name );
}
int UIImport_R_RegisterSkin( const char *name ) {

	return re.RegisterSkin( (const char *)name );
}
void UIImport_R_RegisterFont( const char *fontName, int pointSize, void *font ) {

	re.RegisterFont( (const char *)fontName, pointSize, (fontInfo_t *)font );
	return;

	// shared syscalls
}
int UIImport_R_RegisterShaderNoMip( const char *name ) {

	return re.RegisterShaderNoMip( (const char *)name );
}
void UIImport_R_ClearScene( void ) {

	re.ClearScene();
	return;
}
void UIImport_R_AddRefEntityToScene( const void *entity ) {

	re.AddRefEntityToScene( (const refEntity_t *)entity, qfalse );
	return;
}
void UIImport_R_AddPolyToScene( int hShader, int numVerts, const void *verts ) {

	re.AddPolyToScene( hShader, numVerts, (const polyVert_t *)verts, 1 );
	return;
}
void UIImport_R_AddLightToScene( const float *org, float intensity, float r, float g, float b ) {

	re.AddLightToScene( (const vec_t *)org, intensity, r, g, b );
	return;
}
void UIImport_R_RenderScene( const void *fd ) {

	re.RenderScene( (const refdef_t *)fd );
	return;
}
void UIImport_R_SetColor( const float *rgba ) {

	re.SetColor( (const float *)rgba );
	return;
}
void UIImport_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, int hShader ) {

	re.DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
	return;
}
void UIImport_R_ModelBounds( int model, float *mins, float *maxs ) {

	re.ModelBounds( model, (vec_t *)mins, (vec_t *)maxs );
	return;
}
void UIImport_UpdateScreen( void ) {

	SCR_UpdateScreen();
	return;
}
int UIImport_CM_LerpTag( void *tag, int mod, int startFrame, int endFrame, float frac, const char *tagName ) {

	re.LerpTag( (orientation_t *)tag, mod, startFrame, endFrame, frac, (const char *)tagName );
	return 0;
}
void UIImport_S_StartLocalSound( int sfx, int channelNum ) {

	S_StartLocalSound( sfx, channelNum );
	return;
}
int UIImport_S_RegisterSound( const char *sample, int compressed ) {

	return S_RegisterSound( (const char *)sample, (qboolean)compressed );
}
void UIImport_Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) {

	Key_KeynumToStringBuf( keynum, (char *)buf, buflen );
	return;
}
void UIImport_Key_GetBindingBuf( int keynum, char *buf, int buflen ) {

	Key_GetBindingBuf( keynum, (char *)buf, buflen );
	return;
}
void UIImport_Key_SetBinding( int keynum, const char *binding ) {

	Key_SetBinding( keynum, (const char *)binding );
	return;
}
int UIImport_Key_IsDown( int keynum ) {

	return Key_IsDown( keynum );
}
int UIImport_Key_GetOverstrikeMode( void ) {

	return Key_GetOverstrikeMode();
}
void UIImport_Key_SetOverstrikeMode( int state ) {

	Key_SetOverstrikeMode( (qboolean)state );
	return;
}
void UIImport_Key_ClearStates( void ) {

	Key_ClearStates();
	return;
}
int UIImport_Key_GetCatcher( void ) {

	return Key_GetCatcher();
}
void UIImport_Key_SetCatcher( int catcher ) {

	// Don't allow the ui module to close the console
	Key_SetCatcher( catcher | ( Key_GetCatcher() & KEYCATCH_CONSOLE ) );
	return;
}
void UIImport_GetClipboardData( char *buf, int bufsize ) {

	CL_GetClipboardData( (char *)buf, bufsize );
	return;
}
void UIImport_GetClientState( void *state ) {

	GetClientState( (uiClientState_t *)state );
	return;
}
void UIImport_GetGlconfig( void *glconfig ) {

	CL_GetGlconfig( (glconfig_t *)glconfig );
	return;
}
int UIImport_GetConfigString( int index, char *buff, int buffsize ) {

	return GetConfigString( index, (char *)buff, buffsize );
}
int UIImport_LAN_GetServerCount( int source ) {

	return LAN_GetServerCount( source );
}
void UIImport_LAN_GetServerAddressString( int source, int n, char *buf, int buflen ) {

	LAN_GetServerAddressString( source, n, (char *)buf, buflen );
	return;
}
void UIImport_LAN_GetServerInfo( int source, int n, char *buf, int buflen ) {

	LAN_GetServerInfo( source, n, (char *)buf, buflen );
	return;
}
int UIImport_LAN_GetServerPing( int source, int n ) {

	return LAN_GetServerPing( source, n );
}
int UIImport_LAN_GetPingQueueCount( void ) {

	return LAN_GetPingQueueCount();
}
int UIImport_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen ) {

	return LAN_GetServerStatus( (const char *)serverAddress, (char *)serverStatus, maxLen );
}
void UIImport_LAN_SaveCachedServers( void ) {

	LAN_SaveServersToCache();
	return;
}
void UIImport_LAN_LoadCachedServers( void ) {

	LAN_LoadCachedServers();
	return;
}
void UIImport_LAN_ResetPings( int n ) {

	LAN_ResetPings( n );
	return;
}
void UIImport_LAN_ClearPing( int n ) {

	LAN_ClearPing( n );
	return;
}
void UIImport_LAN_GetPing( int n, char *buf, int buflen, int *pingtime ) {

	LAN_GetPing( n, (char *)buf, buflen, (int *)pingtime );
	return;
}
void UIImport_LAN_GetPingInfo( int n, char *buf, int buflen ) {

	LAN_GetPingInfo( n, (char *)buf, buflen );
	return;
}
void UIImport_LAN_MarkServerVisible( int source, int n, int visible ) {

	LAN_MarkServerVisible( source, n, (qboolean)visible );
	return;
}
int UIImport_LAN_ServerIsVisible( int source, int n ) {

	return LAN_ServerIsVisible( source, n );
}
int UIImport_LAN_UpdateVisiblePings( int source ) {

	return LAN_UpdateVisiblePings( source );
}
int UIImport_LAN_AddServer( int source, const char *name, const char *addr ) {

	return LAN_AddServer( source, (const char *)name, (const char *)addr );
}
void UIImport_LAN_RemoveServer( int source, const char *addr ) {

	LAN_RemoveServer( source, (const char *)addr );
	return;
}
int UIImport_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 ) {

	return LAN_CompareServers( source, sortKey, sortDir, s1, s2 );
}
int UIImport_MemoryRemaining( void ) {

	return Hunk_MemoryRemaining();
}
void UIImport_GetCDKey( char *buf, int buflen ) {

	CLUI_GetCDKey( (char *)buf, buflen );
	return;
}
void UIImport_SetCDKey( char *buf ) {

#ifndef STANDALONE
	CLUI_SetCDKey( (char *)buf );
#endif
	return;
}
int UIImport_PC_AddGlobalDefine( char *define ) {

	return botlib_export->PC_AddGlobalDefine( (const char *)define );
}
int UIImport_PC_LoadSource( const char *filename ) {

	return botlib_export->PC_LoadSourceHandle( (const char *)filename );
}
int UIImport_PC_FreeSource( int handle ) {

	return botlib_export->PC_FreeSourceHandle( handle );
}
int UIImport_PC_ReadToken( int handle, void *pc_token ) {

	return botlib_export->PC_ReadTokenHandle( handle, (pc_token_t *)pc_token );
}
int UIImport_PC_SourceFileAndLine( int handle, char *filename, int *line ) {

	return botlib_export->PC_SourceFileAndLine( handle, (char *)filename, (int *)line );
}
void UIImport_S_StopBackgroundTrack( void ) {

	S_StopBackgroundTrack();
	return;
}
void UIImport_S_StartBackgroundTrack( const char *intro, const char *loop ) {

	S_StartBackgroundTrack( (const char *)intro, (const char *)loop );
	return;
}
int UIImport_RealTime( void *qtime ) {

	return Com_RealTime( (qtime_t *)qtime );
}
int UIImport_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits ) {

	Com_DPrintf( "UI_CIN_PlayCinematic\n" );
	return CIN_PlayCinematic( (const char *)arg0, xpos, ypos, width, height, bits );
}
int UIImport_CIN_StopCinematic( int handle ) {

	return CIN_StopCinematic( handle );
}
int UIImport_CIN_RunCinematic( int handle ) {

	return CIN_RunCinematic( handle );
}
void UIImport_CIN_DrawCinematic( int handle ) {

	CIN_DrawCinematic( handle );
	return;
}
void UIImport_CIN_SetExtents( int handle, int x, int y, int w, int h ) {

	CIN_SetExtents( handle, x, y, w, h );
	return;
}
void UIImport_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset ) {

	re.RemapShader( (const char *)oldShader, (const char *)newShader, (const char *)timeOffset );
	return;
}
int UIImport_VerifyCDKey( const char *key, const char *chksum ) {

	return Com_CDKeyValidate( (const char *)key, (const char *)chksum );

	// engine extensions
}
void UIImport_SetPbClStatus( int status [[maybe_unused]] ) {

	return;
}


/*
====================
CL_ShutdownUI
====================
*/
void CL_ShutdownUI( void ) {
	CL_DataUIShutdown();
	Sys_StopServerSearch( uiServiceSearch );
	uiServiceSearch = 0;
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_UI );
	cls.uiStarted = qfalse;
	if ( !NativeUI_Running ) {
		return;
	}
	NativeUI_Shutdown();
	NativeUI_Running = false;
	NativeUI_CallDepth = 0;
	FS_VM_CloseFiles( H_Q3UI );
}


/*
====================
CL_InitUI
====================
*/
void CL_InitUI( void ) {
	re.VertexLighting( qfalse );
	NativeUI_Running = true;
	Com_Printf( "Static ui loaded.\n" );
	NativeUI_Init( cls.state >= CA_AUTHORIZING && cls.state < CA_ACTIVE );
	CL_DataUIInit();
}


#ifndef STANDALONE
qboolean UI_usesUniqueCDKey( void ) {
	if ( NativeUI_Running ) {
		return (qboolean)qtrue;
	} else {
		return qfalse;
	}
}
#endif


/*
====================
UI_GameCommand

See if the current console command is claimed by the ui
====================
*/
qboolean UI_GameCommand( void ) {
	if ( !NativeUI_Running ) {
		return qfalse;
	}

	return (qboolean)NativeUI_ConsoleCommand( cls.realtime );
}

uint64_t UIImport_StartServerSearch( int matchmaking, const char *filter ) {
	Sys_StopServerSearch( uiServiceSearch );
	uiServiceSearch = Sys_StartServerSearch( matchmaking ? SERVICE_MATCH : SERVICE_BROWSE, filter );
	return uiServiceSearch;
}
int UIImport_NextServerSearch( uint64_t request, char *address, int addressSize, char *name, int nameSize ) {
	if ( !address || addressSize < 1 || !name || nameSize < 1 )
		return 0;
	address[0] = name[0] = 0;
	serviceServer_t result;
	if ( !Sys_NextServer( request, &result ) )
		return 0;
	if ( strlen( result.address ) >= (size_t)addressSize || strlen( result.name ) >= (size_t)nameSize ) {
		UIImport_StopServerSearch( request );
		return SERVICE_SEARCH_FAILED;
	}
	Q_strncpyz( address, result.address, addressSize );
	Q_strncpyz( name, result.name, nameSize );
	return result.state;
}
void UIImport_StopServerSearch( uint64_t request ) {
	Sys_StopServerSearch( request );
	if ( request == uiServiceSearch )
		uiServiceSearch = 0;
}
