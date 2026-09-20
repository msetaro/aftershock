#include "devtools_public.h"
#include "../qcommon/qcommon_public.h"

static const devGameTools_t *gameTools;
static refdef_t gameView;
static bool hasView;
static int viewClient = -1;

void Dev_RegisterGameTools( const devGameTools_t *tools ) {
	gameTools = tools;
}

const devGameTools_t *DevTools_Game( void ) {
	return Cvar_VariableIntegerValue( "sv_running" ) ? gameTools : nullptr;
}

void DevTools_SetView( const refdef_t *view, int clientEntity ) {
	if ( !view ) {
		hasView = false;
		return;
	}
	if ( !( view->rdflags & RDF_NOWORLDMODEL ) ) {
		gameView = *view;
		viewClient = clientEntity;
		hasView = true;
	}
}

int DevTools_ViewClient( void ) {
	return viewClient;
}

const refdef_t *DevTools_View( void ) {
	return hasView ? &gameView : nullptr;
}

bool DevTools_SaveEntities( void ) {
	const devGameTools_t *tools = DevTools_Game();
	if ( !tools || !Cvar_VariableIntegerValue( "sv_cheats" ) || tools->MapCount() <= 0 )
		return false;
	char path[MAX_QPATH];
	const char *map = Cvar_VariableString( "mapname" );
	if ( strlen( map ) + sizeof( "maps/.dev.999.ent" ) > sizeof( path ) )
		return false;
	int revision;
	for ( revision = 0; revision < 1000; ++revision ) {
		Com_sprintf( path, sizeof( path ), "maps/%s.dev.%03d.ent", map, revision );
		if ( !FS_FileExists( path ) )
			break;
	}
	if ( revision == 1000 )
		return false;
	const fileHandle_t file = FS_FOpenFileWrite( path );
	if ( !file )
		return false;
	bool success = true;
	for ( int i = 0; i < tools->MapCount(); ++i ) {
		const char *text = tools->MapText( i );
		const int length = (int)strlen( text );
		if ( length && FS_Write( text, length, file ) != length ) {
			success = false;
			break;
		}
	}
	FS_FCloseFile( file );
	if ( success )
		Cvar_Set( "dev_entityFile", path );
	Com_Printf( "Developer entities: %s %s\n", success ? "saved" : "write failed", path );
	return success;
}

static void EntityCommand( void ) {
	const devGameTools_t *tools = DevTools_Game();
	if ( !tools ) {
		Com_Printf( "Developer entities require a local native game\n" );
		return;
	}
	static int last = -1;
	const char *operation = Cmd_Argv( 1 );
	int entity = last;
	if ( strcmp( Cmd_Argv( 2 ), "last" ) ) {
		char extra;
		if ( sscanf( Cmd_Argv( 2 ), "%d%c", &entity, &extra ) != 1 )
			entity = -1;
	}
	bool success = false;
	if ( !strcmp( operation, "spawn" ) && Cmd_Argc() == 6 ) {
		float origin[3];
		char extra;
		bool valid = true;
		for ( int i = 0; i < 3; ++i )
			valid &= sscanf( Cmd_Argv( i + 3 ), "%f%c", &origin[i], &extra ) == 1;
		if ( valid ) {
			last = tools->Spawn( Cmd_Argv( 2 ), origin );
			success = last >= 0;
		}
	} else if ( !strcmp( operation, "set" ) && Cmd_Argc() == 5 ) {
		success = tools->WriteField( entity, Cmd_Argv( 3 ), Cmd_Argv( 4 ) );
	} else if ( !strcmp( operation, "delete" ) && Cmd_Argc() == 3 ) {
		success = tools->Delete( entity );
	} else if ( !strcmp( operation, "save" ) && Cmd_Argc() == 2 ) {
		success = DevTools_SaveEntities();
	} else if ( !strcmp( operation, "get" ) && Cmd_Argc() == 4 ) {
		char value[1024];
		success = tools->ReadField( entity, Cmd_Argv( 3 ), value, sizeof( value ) );
		if ( success )
			Com_Printf( "Developer entity %d %s = %s\n", entity, Cmd_Argv( 3 ), value );
	} else if ( !strcmp( operation, "sample" ) && Cmd_Argc() == 3 ) {
		devEntity_t info;
		success = tools->ReadEntity( entity, &info );
		if ( success )
			Com_Printf( "Developer sample: id=%d origin=%.9f %.9f %.9f health=%d linked=%d\n", entity,
				info.origin[0], info.origin[1], info.origin[2], info.health, info.linked );
	} else if ( !strcmp( operation, "find" ) && Cmd_Argc() == 3 ) {
		char value[1024];
		for ( int i = 0; i < MAX_GENTITIES; ++i ) {
			if ( tools->ReadField( i, "targetname", value, sizeof( value ) ) && !strcmp( value, Cmd_Argv( 2 ) ) ) {
				last = i;
				success = true;
				break;
			}
		}
	} else {
		Com_Printf( "dev_entity spawn <pickup/point class> <x> <y> <z> | set/get <id/last> <field> [value] | sample <id/last> | delete <id/last> | find <targetname> | save\n" );
		return;
	}
	Com_Printf( "Developer entity %s: %s (last %d)\n", operation, success ? "completed" : "rejected", last );
}

void DevTools_InitEntities( void ) {
	Cmd_AddCommand( "dev_entity", EntityCommand );
	Cvar_Get( "dev_entityFile", "", CVAR_TEMP );
	Cvar_Get( "dev_loadEntities", "0", CVAR_TEMP );
}
