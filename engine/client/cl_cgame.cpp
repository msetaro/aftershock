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
// cl_cgame.c  -- client system interaction with client game

#include "client.h"
#include "../public/cg_native_public.h"
#include "../public/ui_native_public.h"

#include "../botlib/botlib_public.h"

extern	botlib_export_t	*botlib_export;

//extern qboolean loadCamera(const char *name);
//extern void startCamera(int time);
//extern qboolean getCameraInfo(int time, vec3_t *origin, vec3_t *angles);

/*
====================
CL_GetGameState
====================
*/
static void CL_GetGameState( gameState_t *gs ) {
	*gs = cl.gameState;
}


/*
====================
CL_GetGlconfig
====================
*/
static void CL_GetGlconfig( glconfig_t *glconfig ) {
	*glconfig = cls.glconfig;
}


/*
====================
CL_GetUserCmd
====================
*/
static qboolean CL_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if ( cl.cmdNumber - cmdNumber < 0 ) {
		Com_Error( ERR_DROP, "CL_GetUserCmd: cmdNumber (%i) > cl.cmdNumber (%i)", cmdNumber, cl.cmdNumber );
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if ( cl.cmdNumber - cmdNumber >= CMD_BACKUP ) {
		return qfalse;
	}

	*ucmd = cl.cmds[ cmdNumber & CMD_MASK ];

	return qtrue;
}


/*
====================
CL_GetCurrentCmdNumber
====================
*/
static int CL_GetCurrentCmdNumber( void ) {
	return cl.cmdNumber;
}


/*
====================
CL_GetCurrentSnapshotNumber
====================
*/
static void CL_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	*snapshotNumber = cl.snap.messageNum;
	*serverTime = cl.snap.serverTime;
}


/*
====================
CL_GetSnapshot
====================
*/
static qboolean CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	clSnapshot_t	*clSnap;
	int				i, count;

	if ( cl.snap.messageNum - snapshotNumber < 0 ) {
		Com_Error( ERR_DROP, "CL_GetSnapshot: snapshotNumber (%i) > cl.snapshot.messageNum (%i)", snapshotNumber, cl.snap.messageNum );
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if ( cl.snap.messageNum - snapshotNumber >= PACKET_BACKUP ) {
		return qfalse;
	}

	// if the frame is not valid, we can't return it
	clSnap = &cl.snapshots[snapshotNumber & PACKET_MASK];
	if ( !clSnap->valid ) {
		return qfalse;
	}

	// if the entities in the frame have fallen out of their
	// circular buffer, we can't return it
	if ( cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES ) {
		return qfalse;
	}

	// write the snapshot
	snapshot->snapFlags = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->ping = clSnap->ping;
	snapshot->serverTime = clSnap->serverTime;
	Com_Memcpy( snapshot->areamask, clSnap->areamask, sizeof( snapshot->areamask ) );
	snapshot->ps = clSnap->ps;
	count = clSnap->numEntities;
	if ( count > MAX_ENTITIES_IN_SNAPSHOT ) {
		Com_DPrintf( "CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT );
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}
	snapshot->numEntities = count;
	for ( i = 0 ; i < count ; i++ ) {
		snapshot->entities[i] =
			cl.parseEntities[ ( clSnap->parseEntitiesNum + i ) & (MAX_PARSE_ENTITIES-1) ];
	}

	// FIXME: configstring changes and server commands!!!

	return qtrue;
}


/*
=====================
CL_SetUserCmdValue
=====================
*/
static void CL_SetUserCmdValue( int userCmdValue, float sensitivityScale ) {
	cl.cgameUserCmdValue = userCmdValue;
	cl.cgameSensitivity = sensitivityScale;
}


/*
=====================
CL_AddCgameCommand
=====================
*/
static void CL_AddCgameCommand( const char *cmdName ) {
	Cmd_AddCommand( cmdName, NULL );
}


/*
=====================
CL_ConfigstringModified
=====================
*/
static void CL_ConfigstringModified( void ) {
	const char	*old, *s;
	int			i, index;
	const char	*dup;
	gameState_t	oldGs;
	int			len;

	index = atoi( Cmd_Argv(1) );
	if ( (unsigned) index >= MAX_CONFIGSTRINGS ) {
		Com_Error( ERR_DROP, "%s: bad configstring index %i", __func__, index );
	}
	// get everything after "cs <num>"
	s = Cmd_ArgsFrom(2);

	old = cl.gameState.stringData + cl.gameState.stringOffsets[ index ];
	if ( !strcmp( old, s ) ) {
		return;		// unchanged
	}

	// build the new gameState_t
	oldGs = cl.gameState;

	Com_Memset( &cl.gameState, 0, sizeof( cl.gameState ) );

	// leave the first 0 for uninitialized strings
	cl.gameState.dataCount = 1;

	for ( i = 0; i < MAX_CONFIGSTRINGS; i++ ) {
		if ( i == index ) {
			dup = s;
		} else {
			dup = oldGs.stringData + oldGs.stringOffsets[ i ];
		}
		if ( !dup[0] ) {
			continue;		// leave with the default empty string
		}

		len = strlen( dup );

		if ( len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS ) {
			Com_Error( ERR_DROP, "%s: MAX_GAMESTATE_CHARS exceeded", __func__ );
		}

		// append it to the gameState string buffer
		cl.gameState.stringOffsets[ i ] = cl.gameState.dataCount;
		Com_Memcpy( cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1 );
		cl.gameState.dataCount += len + 1;
	}

	if ( index == CS_SYSTEMINFO ) {
		// parse serverId and other cvars
		CL_SystemInfoChanged( qfalse );
	}
}


/*
===================
CL_GetServerCommand

Set up argc/argv for the given command
===================
*/
static qboolean CL_GetServerCommand( int serverCommandNumber ) {
	const char *s;
	const char *cmd;
	static char bigConfigString[BIG_INFO_STRING];
	int argc, index;

	// if we have irretrievably lost a reliable command, drop the connection
	if ( clc.serverCommandSequence - serverCommandNumber >= MAX_RELIABLE_COMMANDS ) {
		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if ( clc.demoplaying ) {
			Cmd_Clear();
			return qfalse;
		}
		Com_Error( ERR_DROP, "CL_GetServerCommand: a reliable command was cycled out" );
		return qfalse;
	}

	if ( clc.serverCommandSequence - serverCommandNumber < 0 ) {
		Com_Error( ERR_DROP, "CL_GetServerCommand: requested a command not received" );
		return qfalse;
	}

	index = serverCommandNumber & ( MAX_RELIABLE_COMMANDS - 1 );
	s = clc.serverCommands[ index ];
	clc.lastExecutedServerCommand = serverCommandNumber;

	Com_DPrintf( "serverCommand: %i : %s\n", serverCommandNumber, s );

	if ( clc.serverCommandsIgnore[ index ] ) {
		Cmd_Clear();
		return qfalse;
	}

rescan:
	Cmd_TokenizeString( s );
	cmd = Cmd_Argv(0);
	argc = Cmd_Argc();

	if ( !strcmp( cmd, "disconnect" ) ) {
		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=552
		// allow server to indicate why they were disconnected
		if ( argc >= 2 )
			Com_Error( ERR_SERVERDISCONNECT, "Server disconnected - %s", Cmd_Argv( 1 ) );
		else
			Com_Error( ERR_SERVERDISCONNECT, "Server disconnected" );
	}

	if ( !strcmp( cmd, "bcs0" ) ) {
		Com_sprintf( bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv(1), Cmd_Argv(2) );
		return qfalse;
	}

	if ( !strcmp( cmd, "bcs1" ) ) {
		s = Cmd_Argv(2);
		if( strlen(bigConfigString) + strlen(s) >= BIG_INFO_STRING ) {
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( bigConfigString, s );
		return qfalse;
	}

	if ( !strcmp( cmd, "bcs2" ) ) {
		s = Cmd_Argv(2);
		if( strlen(bigConfigString) + strlen(s) + 1 >= BIG_INFO_STRING ) {
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( bigConfigString, s );
		strcat( bigConfigString, "\"" );
		s = bigConfigString;
		goto rescan;
	}

	if ( !strcmp( cmd, "cs" ) ) {
		CL_ConfigstringModified();
		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString( s );
		return qtrue;
	}

	if ( !strcmp( cmd, "map_restart" ) ) {
		// clear notify lines and outgoing commands before passing
		// the restart to the cgame
		Con_ClearNotify();
		// reparse the string, because Con_ClearNotify() may have done another Cmd_TokenizeString()
		Cmd_TokenizeString( s );
		Com_Memset( cl.cmds, 0, sizeof( cl.cmds ) );
		cls.lastVidRestart = Sys_Milliseconds(); // hack for OSP mod
		return qtrue;
	}

	// the clientLevelShot command is used during development
	// to generate 128*128 screenshots from the intermission
	// point of levels for the menu system to use
	// we pass it along to the cgame to make appropriate adjustments,
	// but we also clear the console and notify lines here
	if ( !strcmp( cmd, "clientLevelShot" ) ) {
		// don't do it if we aren't running the server locally,
		// otherwise malicious remote servers could overwrite
		// the existing thumbnails
		if ( !com_sv_running->integer ) {
			return qfalse;
		}
		// close the console
		Con_Close();
		// take a special screenshot next frame
		Cbuf_AddText( "wait ; wait ; wait ; wait ; screenshot levelshot\n" );
		return qtrue;
	}

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return qtrue;
}


/*
====================
CL_CM_LoadMap

Just adds default parameters that cgame doesn't need to know about
====================
*/
static void CL_CM_LoadMap( const char *mapname ) {
	int		checksum;
	void	*buf;

	Hunk_AllocPreference( h_low );

	buf = CM_LoadMap( mapname, (qboolean)1, &checksum );
	if ( buf ) {
		// we need this memory for a renderer module later
		// Hunk_FreeTempMemory( buf );
	}
}


/*
====================
CL_ShutdonwCGame

====================
*/
void CL_ShutdownCGame( void ) {

	Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_CGAME );
	cls.cgameStarted = qfalse;

	if ( !NativeCGame_Running ) {
		return;
	}

	re.VertexLighting( qfalse );

	NativeCGame_Shutdown(  );
	NativeCGame_Running = false;
	NativeCGame_CallDepth = 0;
	FS_VM_CloseFiles( H_CGAME );
}


void CGameImport_Print( const char * fmt ) {

	Com_Printf( "%s", (const char*)fmt );
	return;
}
void CGameImport_Error( const char * fmt ) {

	Com_Error( ERR_DROP, "%s", (const char*)fmt );
	return;
}
int CGameImport_Milliseconds( void ) {

	return Sys_Milliseconds();
}
void CGameImport_Cvar_Register( void * vmCvar, const char * varName, const char * defaultValue, int flags ) {

	Cvar_Register( (vmCvar_t *)vmCvar, (const char *)varName, (const char *)defaultValue, flags, 0 );
	return;
}
void CGameImport_Cvar_Update( void * vmCvar ) {

	Cvar_Update( (vmCvar_t *)vmCvar, 0 );
	return;
}
void CGameImport_Cvar_Set( const char * var_name, const char * value ) {

	Cvar_SetSafe( (const char *)var_name, (const char *)value );
	return;
}
void CGameImport_Cvar_VariableStringBuffer( const char * var_name, char * buffer, int bufsize ) {

	Cvar_VariableStringBufferSafe( (const char *)var_name, (char *)buffer, bufsize, CVAR_PRIVATE );
	return;
}
int CGameImport_Argc( void ) {

	return Cmd_Argc();
}
void CGameImport_Argv( int n, char * buffer, int bufferLength ) {

	Cmd_ArgvBuffer( n, (char *)buffer, bufferLength );
	return;
}
void CGameImport_Args( char * buffer, int bufferLength ) {

	Cmd_ArgsBuffer( (char *)buffer, bufferLength );
	return;

}
int CGameImport_FS_FOpenFile( const char * qpath, void * f, int mode ) {

	return FS_VM_OpenFile( (const char *)qpath, (fileHandle_t *)f, (fsMode_t)mode, H_CGAME );
}
void CGameImport_FS_Read( void * buffer, int len, int f ) {

	FS_VM_ReadFile( buffer, len, f, H_CGAME );
	return;
}
void CGameImport_FS_Write( const void * buffer, int len, int f ) {

	FS_VM_WriteFile( (void *)buffer, len, f, H_CGAME );
	return;
}
void CGameImport_FS_FCloseFile( int f ) {

	FS_VM_CloseFile( f, H_CGAME );
	return;
}
int CGameImport_FS_Seek( int f, int64_t offset, int origin ) {

	return FS_VM_SeekFile( f, offset, (fsOrigin_t)origin, H_CGAME );

}
void CGameImport_SendConsoleCommand( const char * text ) {
 {
	const char *cmd = (const char *)text;
	Cbuf_NestedAdd( cmd );
	return;
}
}
void CGameImport_AddCommand( const char * cmdName ) {

	CL_AddCgameCommand( (const char *)cmdName );
	return;
}
void CGameImport_RemoveCommand( const char * cmdName ) {

	Cmd_RemoveCommandSafe( (const char *)cmdName );
	return;
}
void CGameImport_SendClientCommand( const char * s ) {

	CL_AddReliableCommand( (const char *)s, qfalse );
	return;
}
void CGameImport_UpdateScreen( void ) {

	// this is used during lengthy level loading, so pump message loop
	// Com_EventLoop();	// FIXME: if a server restarts here, BAD THINGS HAPPEN!
	// We can't call Com_EventLoop here, a restart will crash and this _does_ happen
	// if there is a map change while we are downloading at pk3.
	// ZOID
	SCR_UpdateScreen();
	return;
}
void CGameImport_CM_LoadMap( const char * mapname ) {

	CL_CM_LoadMap( (const char *)mapname );
	return;
}
int CGameImport_CM_NumInlineModels( void ) {

	return CM_NumInlineModels();
}
int CGameImport_CM_InlineModel( int index ) {

	return CM_InlineModel( index );
}
int CGameImport_CM_TempBoxModel( const float * mins, const float * maxs ) {

	return CM_TempBoxModel( (const vec_t *)mins, (const vec_t *)maxs,  qfalse );
}
int CGameImport_CM_TempCapsuleModel( const float * mins, const float * maxs ) {

	return CM_TempBoxModel( (const vec_t *)mins, (const vec_t *)maxs,  qtrue );
}
int CGameImport_CM_PointContents( const float * p, int model ) {

	return CM_PointContents( (const vec_t *)p, model );
}
int CGameImport_CM_TransformedPointContents( const float * p, int model, const float * origin, const float * angles ) {

	return CM_TransformedPointContents( (const vec_t *)p, model, (const vec_t *)origin, (const vec_t *)angles );
}
void CGameImport_CM_BoxTrace( void * results, const float * start, const float * end, const float * mins, const float * maxs, int model, int brushmask ) {

	CM_BoxTrace( (trace_t *)results, (const vec_t *)start, (const vec_t *)end, (const vec_t *)mins, (const vec_t *)maxs, model, brushmask,  qfalse );
	return;
}
void CGameImport_CM_CapsuleTrace( void * results, const float * start, const float * end, const float * mins, const float * maxs, int model, int brushmask ) {

	CM_BoxTrace( (trace_t *)results, (const vec_t *)start, (const vec_t *)end, (const vec_t *)mins, (const vec_t *)maxs, model, brushmask,  qtrue );
	return;
}
void CGameImport_CM_TransformedBoxTrace( void * results, const float * start, const float * end, const float * mins, const float * maxs, int model, int brushmask, const float * origin, const float * angles ) {

	CM_TransformedBoxTrace( (trace_t *)results, (const vec_t *)start, (const vec_t *)end, (const vec_t *)mins, (const vec_t *)maxs, model, brushmask, (const vec_t *)origin, (const vec_t *)angles,  qfalse );
	return;
}
void CGameImport_CM_TransformedCapsuleTrace( void * results, const float * start, const float * end, const float * mins, const float * maxs, int model, int brushmask, const float * origin, const float * angles ) {

	CM_TransformedBoxTrace( (trace_t *)results, (const vec_t *)start, (const vec_t *)end, (const vec_t *)mins, (const vec_t *)maxs, model, brushmask, (const vec_t *)origin, (const vec_t *)angles,  qtrue );
	return;
}
int CGameImport_CM_MarkFragments( int numPoints, const void * points, const float * projection, int maxPoints, float * pointBuffer, int maxFragments, void * fragmentBuffer ) {



	return re.MarkFragments( numPoints, (const vec_t ( *)[3])points, (const vec_t *)projection, maxPoints, (vec_t *)pointBuffer, maxFragments, (markFragment_t *)fragmentBuffer );
}
void CGameImport_S_StartSound( float * origin, int entityNum, int entchannel, int sfx ) {

	S_StartSound( (vec_t *)origin, entityNum, entchannel, sfx );
	return;
}
void CGameImport_S_StartLocalSound( int sfx, int channelNum ) {

	S_StartLocalSound( sfx, channelNum );
	return;
}
void CGameImport_S_ClearLoopingSounds( int killall ) {

	S_ClearLoopingSounds((qboolean)killall);
	return;
}
void CGameImport_S_AddLoopingSound( int entityNum, const float * origin, const float * velocity, int sfx ) {

	S_AddLoopingSound( entityNum, (const vec_t *)origin, (const vec_t *)velocity, sfx );
	return;
}
void CGameImport_S_AddRealLoopingSound( int entityNum, const float * origin, const float * velocity, int sfx ) {

	S_AddRealLoopingSound( entityNum, (const vec_t *)origin, (const vec_t *)velocity, sfx );
	return;
}
void CGameImport_S_StopLoopingSound( int entityNum ) {

	S_StopLoopingSound( entityNum );
	return;
}
void CGameImport_S_UpdateEntityPosition( int entityNum, const float * origin ) {

	S_UpdateEntityPosition( entityNum, (const vec_t *)origin );
	return;
}
void CGameImport_S_Respatialize( int entityNum, const float * origin, void * axis, int inwater ) {

	S_Respatialize( entityNum, (const vec_t *)origin, (vec_t ( *)[3])axis, inwater );
	return;
}
int CGameImport_S_RegisterSound( const char * sample, int compressed ) {

	return S_RegisterSound( (const char *)sample, (qboolean)compressed );
}
void CGameImport_S_StartBackgroundTrack( const char * intro, const char * loop ) {

	S_StartBackgroundTrack( (const char *)intro, (const char *)loop );
	return;
}
void CGameImport_R_LoadWorldMap( const char * mapname ) {

	re.LoadWorld( (const char *)mapname );
	return;
}
int CGameImport_R_RegisterModel( const char * name ) {

	return re.RegisterModel( (const char *)name );
}
int CGameImport_R_RegisterSkin( const char * name ) {

	return re.RegisterSkin( (const char *)name );
}
int CGameImport_R_RegisterShader( const char * name ) {

	return re.RegisterShader( (const char *)name );
}
int CGameImport_R_RegisterShaderNoMip( const char * name ) {

	return re.RegisterShaderNoMip( (const char *)name );
}
void CGameImport_R_RegisterFont( const char * fontName, int pointSize, void * font ) {

	re.RegisterFont( (const char *)fontName, pointSize, (fontInfo_t *)font);
	return;
}
void CGameImport_R_ClearScene( void ) {

	re.ClearScene();
	return;
}
void CGameImport_R_AddRefEntityToScene( const void * entity ) {

	re.AddRefEntityToScene( (const refEntity_t *)entity, qfalse );
	return;
}
void CGameImport_R_AddPolyToScene( int hShader, int numVerts, const void * verts ) {

	re.AddPolyToScene( hShader, numVerts, (const polyVert_t *)verts, 1 );
	return;
}
void CGameImport_R_AddPolysToScene( int hShader, int numVerts, const void * verts, int num ) {

	re.AddPolyToScene( hShader, numVerts, (const polyVert_t *)verts, num );
	return;
}
int CGameImport_R_LightForPoint( float * point, float * ambientLight, float * directedLight, float * lightDir ) {

	return re.LightForPoint( (vec_t *)point, (vec_t *)ambientLight, (vec_t *)directedLight, (vec_t *)lightDir );
}
void CGameImport_R_AddLightToScene( const float * org, float intensity, float r, float g, float b ) {

	re.AddLightToScene( (const vec_t *)org, intensity, r, g, b );
	return;
}
void CGameImport_R_AddAdditiveLightToScene( const float * org, float intensity, float r, float g, float b ) {

	re.AddAdditiveLightToScene( (const vec_t *)org, intensity, r, g, b );
	return;
}
void CGameImport_R_RenderScene( const void * fd ) {

	re.RenderScene( (const refdef_t *)fd );
	return;
}
void CGameImport_R_SetColor( const float * rgba ) {

	re.SetColor( (const float *)rgba );
	return;
}
void CGameImport_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, int hShader ) {

	re.DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
	return;
}
void CGameImport_R_ModelBounds( int model, float * mins, float * maxs ) {

	re.ModelBounds( model, (vec_t *)mins, (vec_t *)maxs );
	return;
}
int CGameImport_R_LerpTag( void * tag, int mod, int startFrame, int endFrame, float frac, const char * tagName ) {

	return re.LerpTag( (orientation_t *)tag, mod, startFrame, endFrame, frac, (const char *)tagName );
}
void CGameImport_R_RemapShader( const char * oldShader, const char * newShader, const char * timeOffset ) {

	re.RemapShader( (const char *)oldShader, (const char *)newShader, (const char *)timeOffset );
	return;


}
void CGameImport_GetGlconfig( void * glconfig ) {

	CL_GetGlconfig( (glconfig_t *)glconfig );
	return;
}
void CGameImport_GetGameState( void * gamestate ) {

	CL_GetGameState( (gameState_t *)gamestate );
	return;
}
void CGameImport_GetCurrentSnapshotNumber( int * snapshotNumber, int * serverTime ) {

	CL_GetCurrentSnapshotNumber( (int *)snapshotNumber, (int *)serverTime );
	return;
}
int CGameImport_GetSnapshot( int snapshotNumber, void * snapshot ) {

	return CL_GetSnapshot( snapshotNumber, (snapshot_t *)snapshot );
}
int CGameImport_GetServerCommand( int serverCommandNumber ) {

	return CL_GetServerCommand( serverCommandNumber );
}
int CGameImport_GetCurrentCmdNumber( void ) {

	return CL_GetCurrentCmdNumber();
}
int CGameImport_GetUserCmd( int cmdNumber, void * ucmd ) {

	return CL_GetUserCmd( cmdNumber, (usercmd_t *)ucmd );
}
void CGameImport_SetUserCmdValue( int stateValue, float sensitivityScale ) {

	CL_SetUserCmdValue( stateValue, sensitivityScale );
	return;
}
void CGameImport_testPrintInt( char * string, int i ) {

	sprintf( (char *)string, "%i", (int)i );
	return;
}
void CGameImport_testPrintFloat( char * string, float f ) {

	sprintf( (char *)string, "%f", f );
	return;
}
int CGameImport_MemoryRemaining( void ) {

	return Hunk_MemoryRemaining();
}
int CGameImport_Key_IsDown( int keynum ) {

	return Key_IsDown( keynum );
}
int CGameImport_Key_GetCatcher( void ) {

	return Key_GetCatcher();
}
void CGameImport_Key_SetCatcher( int catcher ) {

	// Don't allow the cgame module to close the console
	Key_SetCatcher( catcher | ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) );
	return;
}
int CGameImport_Key_GetKey( const char * binding ) {

	return Key_GetKey( (const char *)binding );

// shared syscalls
}
int CGameImport_PC_AddGlobalDefine( char * define ) {

	return botlib_export->PC_AddGlobalDefine( (const char *)define );
}
int CGameImport_PC_LoadSource( const char * filename ) {

	return botlib_export->PC_LoadSourceHandle( (const char *)filename );
}
int CGameImport_PC_FreeSource( int handle ) {

	return botlib_export->PC_FreeSourceHandle( handle );
}
int CGameImport_PC_ReadToken( int handle, void * pc_token ) {

	return botlib_export->PC_ReadTokenHandle( handle, (pc_token_t *)pc_token );
}
int CGameImport_PC_SourceFileAndLine( int handle, char * filename, int * line ) {

	return botlib_export->PC_SourceFileAndLine( handle, (char *)filename, (int *)line );

}
void CGameImport_S_StopBackgroundTrack( void ) {

	S_StopBackgroundTrack();
	return;

}
int CGameImport_RealTime( void * qtime ) {

	return Com_RealTime( (qtime_t *)qtime );
}
void CGameImport_SnapVector( float * v ) {

	Sys_SnapVector( (float *)v );
	return;

}
int CGameImport_CIN_PlayCinematic( const char * arg0, int xpos, int ypos, int width, int height, int bits ) {

	return CIN_PlayCinematic((const char *)arg0, xpos, ypos, width, height, bits);

}
int CGameImport_CIN_StopCinematic( int handle ) {

	return CIN_StopCinematic(handle);

}
int CGameImport_CIN_RunCinematic( int handle ) {

	return CIN_RunCinematic(handle);

}
void CGameImport_CIN_DrawCinematic( int handle ) {

	CIN_DrawCinematic(handle);
	return;

}
void CGameImport_CIN_SetExtents( int handle, int x, int y, int w, int h ) {

	CIN_SetExtents(handle, x, y, w, h);
	return;

}
int CGameImport_loadCamera( const char * name [[maybe_unused]] ) {
	Com_Error( ERR_DROP, "Unsupported native service: CG_LOADCAMERA" );
	return 0;
}
void CGameImport_startCamera( int time [[maybe_unused]] ) {
	Com_Error( ERR_DROP, "Unsupported native service: CG_STARTCAMERA" );
	return;
}
int CGameImport_getCameraInfo( int time [[maybe_unused]], void * origin [[maybe_unused]], void * angles [[maybe_unused]] ) {
	Com_Error( ERR_DROP, "Unsupported native service: CG_GETCAMERAINFO" );
	return 0;
}
int CGameImport_GetEntityToken( char * buffer, int bufferSize ) {

	return re.GetEntityToken( (char *)buffer, bufferSize );

}
int CGameImport_R_inPVS( const float * p1, const float * p2 ) {

	return re.inPVS( (const vec_t *)p1, (const vec_t *)p2 );

// engine extensions
}


/*
====================
CL_InitCGame

Should only be called by CL_StartHunkUsers
====================
*/
void CL_InitCGame( void ) {
	const char			*info;
	const char			*mapname;
	int					t1, t2;

	Cbuf_NestedReset();

	t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SERVERINFO ];
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cl.mapname, sizeof( cl.mapname ), "maps/%s.bsp", mapname );

	// allow vertex lighting for in-game elements
	re.VertexLighting( qtrue );

	NativeCGame_Running = true;
	Com_Printf( "Static cgame loaded.\n" );
	cls.state = CA_LOADING;

	// init for this gamestate
	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	NativeCGame_Init( clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum );

	// reset any CVAR_CHEAT cvars registered by cgame
	if ( !clc.demoplaying && !cl_connectedToCheatServer )
		Cvar_SetCheatState();

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = CA_PRIMED;

	t2 = Sys_Milliseconds();

	Com_Printf( "CL_InitCGame: %5.2f seconds\n", (t2-t1)/1000.0 );

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	re.EndRegistration();

	// make sure everything is paged in
	if (!Sys_LowPhysicalMemory()) {
		Com_TouchMemory();
	}

	// clear anything that got printed
	Con_ClearNotify ();

	// do not allow vid_restart for first time
	cls.lastVidRestart = Sys_Milliseconds();
}


/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/

qboolean CL_GameCommand( void ) {
	qboolean bRes;

	if ( !NativeCGame_Running ) {
		return qfalse;
	}

	bRes = (qboolean)NativeCGame_ConsoleCommand(  );

	Cbuf_NestedReset();

	return bRes;
}


/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering( stereoFrame_t stereo ) {
	NativeCGame_DrawActiveFrame( cl.serverTime, stereo, clc.demoplaying );
}


/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time approach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives with a rational
latency, which keeps the adjustment process framerate independent and
prevents massive overadjustment during times of significant packet loss
or bursted delayed packets.
=================
*/

#define	RESET_TIME	500

static void CL_AdjustTimeDelta( void ) {
	int		newDelta;
	int		deltaDelta;

	cl.newSnapshots = qfalse;

	// the delta never drifts when replaying a demo
	if ( clc.demoplaying ) {
		return;
	}

	newDelta = cl.snap.serverTime - cls.realtime;
	deltaDelta = abs( newDelta - cl.serverTimeDelta );

	if ( deltaDelta > RESET_TIME ) {
		cl.serverTimeDelta = newDelta;
		cl.oldServerTime = cl.snap.serverTime;	// FIXME: is this a problem for cgame?
		cl.serverTime = cl.snap.serverTime;
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<RESET> " );
		}
	} else if ( deltaDelta > 100 ) {
		// fast adjust, cut the difference in half
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<FAST> " );
		}
		cl.serverTimeDelta = ( cl.serverTimeDelta + newDelta ) >> 1;
	} else {
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if ( com_timescale->value == 0 || com_timescale->value == 1 ) {
			if ( cl.extrapolatedSnapshot ) {
				cl.extrapolatedSnapshot = qfalse;
				cl.serverTimeDelta -= 2;
			} else {
				// otherwise, move our sense of time forward to minimize total latency
				cl.serverTimeDelta++;
			}
		}
	}

	if ( cl_showTimeDelta->integer ) {
		Com_Printf( "%i ", cl.serverTimeDelta );
	}
}


/*
==================
CL_FirstSnapshot
==================
*/
static void CL_FirstSnapshot( void ) {
	// ignore snapshots that don't have entities
	if ( cl.snap.snapFlags & SNAPFLAG_NOT_ACTIVE ) {
		return;
	}
	cls.state = CA_ACTIVE;

	// clear old game so we will not switch back to old mod on disconnect
	CL_ResetOldGame();

	// set the timedelta so we are exactly on this first frame
	cl.serverTimeDelta = cl.snap.serverTime - cls.realtime;
	cl.oldServerTime = cl.snap.serverTime;

	clc.timeDemoBaseTime = cl.snap.serverTime;

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if ( cl_activeAction->string[0] ) {
		Cbuf_AddText( cl_activeAction->string );
		Cbuf_AddText( "\n" );
		Cvar_Set( "activeAction", "" );
	}

	Sys_BeginProfiling();
}


/*
==================
CL_AvgPing

Calculates Average Ping from snapshots in buffer. Used by AutoNudge.
==================
*/
static float CL_AvgPing( void ) {
	int ping[PACKET_BACKUP];
	int count = 0;
	int i, j, iTemp;
	float result;

	for ( i = 0; i < PACKET_BACKUP; i++ ) {
		if ( cl.snapshots[i].ping > 0 && cl.snapshots[i].ping < 999 ) {
			ping[count] = cl.snapshots[i].ping;
			count++;
		}
	}

	if ( count == 0 )
		return 0;

	// sort ping array
	for ( i = count - 1; i > 0; --i ) {
		for ( j = 0; j < i; ++j ) {
			if (ping[j] > ping[j + 1]) {
				iTemp = ping[j];
				ping[j] = ping[j + 1];
				ping[j + 1] = iTemp;
			}
		}
	}

	// use median average ping
	if ( (count % 2) == 0 )
		result = (ping[count / 2] + ping[(count / 2) - 1]) / 2.0f;
	else
		result = ping[count / 2];

	return result;
}


/*
==================
CL_TimeNudge

Returns either auto-nudge or cl_timeNudge value.
==================
*/
static int CL_TimeNudge( void ) {
	float autoNudge = cl_autoNudge->value;

	if ( autoNudge != 0.0f )
		return (int)((CL_AvgPing() * autoNudge) + 0.5f) * -1;
	else
		return cl_timeNudge->integer;
}


/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime( void ) {
	qboolean demoFreezed;

	// getting a valid frame message ends the connection process
	if ( cls.state != CA_ACTIVE ) {
		if ( cls.state != CA_PRIMED ) {
			return;
		}
		if ( clc.demoplaying ) {
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if ( !clc.firstDemoFrameSkipped ) {
				clc.firstDemoFrameSkipped = qtrue;
				return;
			}
			CL_ReadDemoMessage();
		}
		if ( cl.newSnapshots ) {
			cl.newSnapshots = qfalse;
			CL_FirstSnapshot();
		}
		if ( cls.state != CA_ACTIVE ) {
			return;
		}
	}

	// if we have gotten to this point, cl.snap is guaranteed to be valid
	if ( !cl.snap.valid ) {
		Com_Error( ERR_DROP, "CL_SetCGameTime: !cl.snap.valid" );
	}

	// allow pause in single player
	if ( sv_paused->integer && CL_CheckPaused() && com_sv_running->integer ) {
		// paused
		return;
	}

	if ( cl.snap.serverTime - cl.oldFrameServerTime < 0 ) {
		Com_Error( ERR_DROP, "cl.snap.serverTime < cl.oldFrameServerTime" );
	}
	cl.oldFrameServerTime = cl.snap.serverTime;

	// get our current view of time
	demoFreezed = (qboolean)( clc.demoplaying && com_timescale->value == 0.0f );
	if ( demoFreezed ) {
		// \timescale 0 is used to lock a demo in place for single frame advances
		cl.serverTimeDelta -= cls.frametime;
	} else {
		// cl_timeNudge is a user adjustable cvar that allows more
		// or less latency to be added in the interest of better
		// smoothness or better responsiveness.
		cl.serverTime = cls.realtime + cl.serverTimeDelta - CL_TimeNudge();

		// guarantee that time will never flow backwards, even if
		// serverTimeDelta made an adjustment or cl_timeNudge was changed
		if ( cl.serverTime - cl.oldServerTime < 0 ) {
			cl.serverTime = cl.oldServerTime;
		}
		cl.oldServerTime = cl.serverTime;

		// note if we are almost past the latest frame (without timeNudge),
		// so we will try and adjust back a bit when the next snapshot arrives
		//if ( cls.realtime + cl.serverTimeDelta >= cl.snap.serverTime - 5 ) {
		if ( cls.realtime + cl.serverTimeDelta - cl.snap.serverTime >= -5 ) {
			cl.extrapolatedSnapshot = qtrue;
		}
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if ( cl.newSnapshots ) {
		CL_AdjustTimeDelta();
	}

	if ( !clc.demoplaying ) {
		return;
	}

	// if we are playing a demo back, we can just keep reading
	// messages from the demo file until the cgame definitely
	// has valid snapshots to interpolate between

	// a timedemo will always use a deterministic set of time samples
	// no matter what speed machine it is run on,
	// while a normal demo may have different time samples
	// each time it is played back
	if ( com_timedemo->integer ) {
		if ( !clc.timeDemoStart ) {
			clc.timeDemoStart = Sys_Milliseconds();
		}
		clc.timeDemoFrames++;
		cl.serverTime = clc.timeDemoBaseTime + clc.timeDemoFrames * 50;
	}

	//while ( cl.serverTime >= cl.snap.serverTime ) {
	while ( cl.serverTime - cl.snap.serverTime >= 0 ) {
		// feed another message, which should change
		// the contents of cl.snap
		CL_ReadDemoMessage();
		if ( cls.state != CA_ACTIVE ) {
			return; // end of demo
		}
	}
}
