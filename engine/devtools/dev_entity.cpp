#include "devtools_public.h"
#include "../qcommon/qcommon_public.h"
#include <cmath>

static const devGameTools_t *gameTools;
static refdef_t gameView;
static bool hasView;
static int viewClient = -1;
static sceneLight_t sceneLight;
static bool hasLight;

void Dev_RegisterGameTools( const devGameTools_t *tools ) {
	gameTools = tools;
}

const devGameTools_t *DevTools_Game( void ) {
	return Cvar_VariableIntegerValue( "sv_running" ) ? gameTools : nullptr;
}

void DevTools_SetView( const refdef_t *view, int clientEntity ) {
	if ( !view ) {
		hasView = false;
		hasLight = false;
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

const sceneLight_t *DevTools_SceneLight( void ) {
	return hasLight && Cvar_VariableIntegerValue( "sv_running" ) && Cvar_VariableIntegerValue( "sv_cheats" ) ? &sceneLight : nullptr;
}

static void LightCommand( void ) {
	if ( !strcmp( Cmd_Argv( 1 ), "off" ) ) {
		hasLight = false;
		return;
	}
	const bool spot = !strcmp( Cmd_Argv( 1 ), "spot" );
	if ( ( spot || !strcmp( Cmd_Argv( 1 ), "point" ) ) && Cmd_Argc() == ( spot ? 15 : 10 ) &&
		 Cvar_VariableIntegerValue( "sv_running" ) && Cvar_VariableIntegerValue( "sv_cheats" ) ) {
		float values[13] = {};
		for ( int i = 2; i < Cmd_Argc(); ++i ) {
			char extra;
			if ( sscanf( Cmd_Argv( i ), "%f%c", &values[i - 2], &extra ) != 1 || !std::isfinite( values[i - 2] ) )
				return;
		}
		sceneLight = {};
		VectorCopy( values, sceneLight.origin );
		sceneLight.radius = values[3];
		VectorCopy( values + 4, sceneLight.color );
		sceneLight.intensity = values[7];
		VectorCopy( values + 8, sceneLight.direction );
		sceneLight.innerCone = values[11];
		sceneLight.outerCone = values[12];
		sceneLight.type = spot ? sceneLightType_t::Spot : sceneLightType_t::Point;
		hasLight = true;
		Com_Printf( "Developer light: %s\n", spot ? "spot" : "point" );
		return;
	}
	Com_Printf( "dev_light off | point x y z radius r g b intensity | spot x y z radius r g b intensity dx dy dz inner outer (local cheats required)\n" );
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

bool DevTools_SaveDefinitions() {
	const auto *tools = DevTools_Game();
	const auto *definitions = tools && tools->Definitions ? tools->Definitions() : nullptr;
	if ( !definitions || !Cvar_VariableIntegerValue( "sv_cheats" ) )
		return false;
	char path[MAX_QPATH];
	int revision;
	for ( revision = 0; revision < 1000; ++revision ) {
		Com_sprintf( path, sizeof( path ), "entities/definitions.%03d.asent", revision );
		if ( !FS_FileExists( path ) )
			break;
	}
	if ( revision == 1000 )
		return false;
	static uint8_t bytes[sizeof( entityDefinitions_t ) + 48];
	const size_t length = Entity_WriteDefinitions( *definitions, bytes, sizeof( bytes ) );
	if ( !length )
		return false;
	const fileHandle_t file = FS_FOpenFileWrite( path );
	if ( !file )
		return false;
	const bool success = FS_Write( bytes, int( length ), file ) == int( length );
	FS_FCloseFile( file );
	if ( success )
		Cvar_Set( "dev_definitionFile", path );
	Com_Printf( "Developer definitions: %s %s\n", success ? "saved" : "write failed", path );
	return success;
}
static void DefinitionCommand() {
	const auto *tools = DevTools_Game();
	const auto *definitions = tools && tools->Definitions ? tools->Definitions() : nullptr;
	const char *operation = Cmd_Argv( 1 );
	bool success = false;
	if ( !strcmp( operation, "save" ) && Cmd_Argc() == 2 )
		success = DevTools_SaveDefinitions();
	else if ( definitions && ( !strcmp( operation, "select" ) || !strcmp( operation, "set" ) ) ) {
		const char *name = Cmd_Argv( 2 ), *key = Cmd_Argv( 3 );
		const auto *definition = Entity_FindDefinition( *definitions, name );
		if ( definition && Entity_Field( *definitions, *definition, key ) ) {
			if ( !strcmp( operation, "select" ) && Cmd_Argc() == 4 )
				success = true;
			else if ( !strcmp( operation, "set" ) && Cmd_Argc() == 5 && tools->WriteDefinition )
				success = tools->WriteDefinition( name, key, Cmd_Argv( 4 ) );
			if ( success ) {
				Cvar_Set( "dev_definitionName", name );
				Cvar_Set( "dev_definitionKey", key );
			}
		}
	}
	Com_Printf( "Developer definition %s: %s\n", operation, success ? "completed" : "rejected" );
}

void DevTools_InitEntities( void ) {
	Cmd_AddCommand( "dev_entity", EntityCommand );
	Cmd_AddCommand( "dev_definition", DefinitionCommand );
	Cvar_Get( "dev_definitionName", "", CVAR_TEMP );
	Cvar_Get( "dev_definitionKey", "", CVAR_TEMP );
	Cvar_Get( "dev_definitionFile", "", CVAR_TEMP );
	Cmd_AddCommand( "dev_light", LightCommand );
	Cvar_Get( "dev_entityFile", "", CVAR_TEMP );
	Cvar_Get( "dev_loadEntities", "0", CVAR_TEMP );
}
