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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//

#include "g_local.h"
#include "../../engine/entities/entities_public.h"
#include "../../engine/public/g_native_public.h"

static entityDefinitions_t entityDefinitions, authoredDefinitions;

static const entityDefinition_t *SpawnDefinition() {
	for ( int i = 0; i < level.numSpawnVars; ++i )
		if ( !Q_stricmp( level.spawnVars[i][0], "classname" ) )
			return Entity_FindDefinition( entityDefinitions, level.spawnVars[i][1] );
	return nullptr;
}
#ifdef AFTERSHOCK_DEVTOOLS
static void G_DevCapture( void );
static void G_DevMapEntity( gentity_t *entity );
static void G_DevReset( void );
#endif

qboolean G_SpawnString( const char *key, const char *defaultString, char **out ) {
	int i;

	if ( !level.spawning ) {
		*out = (char *)defaultString;
		//		G_Error( "G_SpawnString() called while not spawning" );
	}

	for ( i = 0; i < level.numSpawnVars; i++ ) {
		if ( !Q_stricmp( key, level.spawnVars[i][0] ) ) {
			*out = level.spawnVars[i][1];
			return qtrue;
		}
	}

	if ( level.spawning ) {
		const auto *definition = SpawnDefinition();
		const auto *field = definition ? Entity_Field( entityDefinitions, *definition, key ) : nullptr;
		if ( field ) {
			*out = (char *)field->value;
			return qtrue;
		}
	}
	*out = (char *)defaultString;
	return qfalse;
}

qboolean G_SpawnFloat( const char *key, const char *defaultString, float *out ) {
	char *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	*out = (float)( atof( s ) );
	return present;
}

qboolean G_SpawnInt( const char *key, const char *defaultString, int *out ) {
	char *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	return present;
}

qboolean G_SpawnVector( const char *key, const char *defaultString, float *out ) {
	char *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out[0], &out[1], &out[2] );
	return present;
}


//
// fields are needed for spawning from the entity string
//
typedef enum {
	F_INT,
	F_FLOAT,
	F_LSTRING, // string on disk, pointer in memory, TAG_LEVEL
	F_GSTRING, // string on disk, pointer in memory, TAG_GAME
	F_VECTOR,
	F_ANGLEHACK,
	F_ENTITY, // index on disk, pointer in memory
	F_ITEM, // index on disk, pointer in memory
	F_CLIENT, // index on disk, pointer in memory
	F_IGNORE
} fieldtype_t;

typedef struct
{
	const char *name;
	int ofs;
	fieldtype_t type;
	int flags;
} field_t;

field_t fields[] = {
	{ "classname", FOFS( classname ), F_LSTRING, 0 },
	{ "origin", FOFS( s.origin ), F_VECTOR, 0 },
	{ "model", FOFS( model ), F_LSTRING, 0 },
	{ "model2", FOFS( model2 ), F_LSTRING, 0 },
	{ "spawnflags", FOFS( spawnflags ), F_INT, 0 },
	{ "speed", FOFS( speed ), F_FLOAT, 0 },
	{ "target", FOFS( target ), F_LSTRING, 0 },
	{ "targetname", FOFS( targetname ), F_LSTRING, 0 },
	{ "message", FOFS( message ), F_LSTRING, 0 },
	{ "team", FOFS( team ), F_LSTRING, 0 },
	{ "wait", FOFS( wait ), F_FLOAT, 0 },
	{ "random", FOFS( random ), F_FLOAT, 0 },
	{ "count", FOFS( count ), F_INT, 0 },
	{ "health", FOFS( health ), F_INT, 0 },
	{ "light", 0, F_IGNORE, 0 },
	{ "dmg", FOFS( damage ), F_INT, 0 },
	{ "angles", FOFS( s.angles ), F_VECTOR, 0 },
	{ "angle", FOFS( s.angles ), F_ANGLEHACK, 0 },
	{ "targetShaderName", FOFS( targetShaderName ), F_LSTRING, 0 },
	{ "targetShaderNewName", FOFS( targetShaderNewName ), F_LSTRING, 0 },

	{ NULL, 0, F_INT, 0 }
};


typedef struct {
	const char *name;
	void ( *spawn )( gentity_t *ent );
} spawn_t;

void SP_info_player_start( gentity_t *ent );
void SP_info_player_deathmatch( gentity_t *ent );
void SP_info_player_intermission( gentity_t *ent );
void SP_info_firstplace( gentity_t *ent );
void SP_info_secondplace( gentity_t *ent );
void SP_info_thirdplace( gentity_t *ent );
void SP_info_podium( gentity_t *ent );

void SP_func_plat( gentity_t *ent );
void SP_func_static( gentity_t *ent );
void SP_func_rotating( gentity_t *ent );
void SP_func_bobbing( gentity_t *ent );
void SP_func_pendulum( gentity_t *ent );
void SP_func_button( gentity_t *ent );
void SP_func_door( gentity_t *ent );
void SP_func_train( gentity_t *ent );
void SP_func_timer( gentity_t *self );

void SP_trigger_always( gentity_t *ent );
void SP_trigger_multiple( gentity_t *ent );
void SP_trigger_push( gentity_t *ent );
void SP_trigger_teleport( gentity_t *ent );
void SP_trigger_hurt( gentity_t *ent );

void SP_target_remove_powerups( gentity_t *ent );
void SP_target_give( gentity_t *ent );
void SP_target_delay( gentity_t *ent );
void SP_target_speaker( gentity_t *ent );
void SP_target_print( gentity_t *ent );
void SP_target_laser( gentity_t *self );
void SP_target_character( gentity_t *ent );
void SP_target_score( gentity_t *ent );
void SP_target_teleporter( gentity_t *ent );
void SP_target_relay( gentity_t *ent );
void SP_target_kill( gentity_t *ent );
void SP_target_position( gentity_t *ent );
void SP_target_location( gentity_t *ent );
void SP_target_push( gentity_t *ent );

void SP_light( gentity_t *self );
void SP_info_null( gentity_t *self );
void SP_info_notnull( gentity_t *self );
void SP_info_camp( gentity_t *self );
void SP_path_corner( gentity_t *self );

void SP_misc_teleporter_dest( gentity_t *self );
void SP_misc_model( gentity_t *ent );
void SP_misc_portal_camera( gentity_t *ent );
void SP_misc_portal_surface( gentity_t *ent );

void SP_shooter_rocket( gentity_t *ent );
void SP_shooter_plasma( gentity_t *ent );
void SP_shooter_grenade( gentity_t *ent );

void SP_team_CTF_redplayer( gentity_t *ent );
void SP_team_CTF_blueplayer( gentity_t *ent );

void SP_team_CTF_redspawn( gentity_t *ent );
void SP_team_CTF_bluespawn( gentity_t *ent );

#ifdef MISSIONPACK
void SP_team_blueobelisk( gentity_t *ent );
void SP_team_redobelisk( gentity_t *ent );
void SP_team_neutralobelisk( gentity_t *ent );
#endif
void SP_item_botroam( gentity_t *ent [[maybe_unused]] ) {};

spawn_t spawns[] = {
	{ "composed", SP_composed },
	// info entities don't do anything at all, but provide positional
	// information for things controlled by other processes
	{ "info_player_start", SP_info_player_start },
	{ "info_player_deathmatch", SP_info_player_deathmatch },
	{ "info_player_intermission", SP_info_player_intermission },
	{ "info_null", SP_info_null },
	{ "info_notnull", SP_info_notnull }, // use target_position instead
	{ "info_camp", SP_info_camp },

	{ "func_plat", SP_func_plat },
	{ "func_button", SP_func_button },
	{ "func_door", SP_func_door },
	{ "func_static", SP_func_static },
	{ "func_rotating", SP_func_rotating },
	{ "func_bobbing", SP_func_bobbing },
	{ "func_pendulum", SP_func_pendulum },
	{ "func_train", SP_func_train },
	{ "func_group", SP_info_null },
	{ "func_timer", SP_func_timer }, // rename trigger_timer?

	// Triggers are brush objects that cause an effect when contacted
	// by a living player, usually involving firing targets.
	// While almost everything could be done with
	// a single trigger class and different targets, triggered effects
	// could not be client side predicted (push and teleport).
	{ "trigger_always", SP_trigger_always },
	{ "trigger_multiple", SP_trigger_multiple },
	{ "trigger_push", SP_trigger_push },
	{ "trigger_teleport", SP_trigger_teleport },
	{ "trigger_hurt", SP_trigger_hurt },

	// targets perform no action by themselves, but must be triggered
	// by another entity
	{ "target_give", SP_target_give },
	{ "target_remove_powerups", SP_target_remove_powerups },
	{ "target_delay", SP_target_delay },
	{ "target_speaker", SP_target_speaker },
	{ "target_print", SP_target_print },
	{ "target_laser", SP_target_laser },
	{ "target_score", SP_target_score },
	{ "target_teleporter", SP_target_teleporter },
	{ "target_relay", SP_target_relay },
	{ "target_kill", SP_target_kill },
	{ "target_position", SP_target_position },
	{ "target_location", SP_target_location },
	{ "target_push", SP_target_push },

	{ "light", SP_light },
	{ "path_corner", SP_path_corner },

	{ "misc_teleporter_dest", SP_misc_teleporter_dest },
	{ "misc_model", SP_misc_model },
	{ "misc_portal_surface", SP_misc_portal_surface },
	{ "misc_portal_camera", SP_misc_portal_camera },

	{ "shooter_rocket", SP_shooter_rocket },
	{ "shooter_grenade", SP_shooter_grenade },
	{ "shooter_plasma", SP_shooter_plasma },

	{ "team_CTF_redplayer", SP_team_CTF_redplayer },
	{ "team_CTF_blueplayer", SP_team_CTF_blueplayer },

	{ "team_CTF_redspawn", SP_team_CTF_redspawn },
	{ "team_CTF_bluespawn", SP_team_CTF_bluespawn },

#ifdef MISSIONPACK
	{ "team_redobelisk", SP_team_redobelisk },
	{ "team_blueobelisk", SP_team_blueobelisk },
	{ "team_neutralobelisk", SP_team_neutralobelisk },
#endif
	{ "item_botroam", SP_item_botroam },

	{ 0, 0 }
};

static void RegisterLegacyDefinition( const char *name ) {
	if ( entityDefinitions.header.count == 256 )
		G_Error( "Entity definitions exceed 256 records" );
	auto &definition = entityDefinitions.definitions[entityDefinitions.header.count++];
	Q_strncpyz( definition.name, name, sizeof( definition.name ) );
	Q_strncpyz( definition.native, name, sizeof( definition.native ) );
}
static void InitEntityDefinitions() {
	memset( &entityDefinitions, 0, sizeof( entityDefinitions ) );
	memset( &authoredDefinitions, 0, sizeof( authoredDefinitions ) );
	for ( const gitem_t *item = bg_itemlist + 1; item->classname; ++item )
		RegisterLegacyDefinition( item->classname );
	for ( const spawn_t *spawn = spawns; spawn->name; ++spawn )
		RegisterLegacyDefinition( spawn->name );
	vmCvar_t path;
	trap_Cvar_Register( &path, "g_entityDefinitions", "", CVAR_LATCH );
	if ( !path.string[0] )
		return;
	fileHandle_t file;
	const int length = trap_FS_FOpenFile( path.string, &file, FS_READ );
	static uint8_t bytes[sizeof( entityDefinitions_t ) + 48];
	auto &authored = authoredDefinitions;
	if ( !file || length <= 0 || length > int( sizeof( bytes ) ) ) {
		if ( file )
			trap_FS_FCloseFile( file );
		G_Error( "Entity definitions rejected: cannot read %s", path.string );
	}
	trap_FS_Read( bytes, length, file );
	trap_FS_FCloseFile( file );
	if ( !Entity_ReadDefinitions( bytes, size_t( length ), &authored ) )
		G_Error( "Entity definitions rejected: %s", path.string );
	for ( uint32_t i = 0; i < authored.header.count; ++i ) {
		const auto &definition = authored.definitions[i];
		bool native = false;
		for ( const gitem_t *item = bg_itemlist + 1; item->classname; ++item )
			native |= !strcmp( item->classname, definition.native );
		for ( const spawn_t *spawn = spawns; spawn->name; ++spawn )
			native |= !strcmp( spawn->name, definition.native );
		if ( !native )
			G_Error( "Entity definition %s has unknown native behavior %s", definition.name, definition.native );
		const auto *previous = Entity_FindDefinition( entityDefinitions, definition.name );
		uint32_t index;
		if ( previous )
			index = uint32_t( previous - entityDefinitions.definitions );
		else {
			if ( entityDefinitions.header.count == 256 )
				G_Error( "Entity definitions exceed 256 records" );
			index = entityDefinitions.header.count++;
		}
		entityDefinitions.definitions[index] = definition;
	}
	entityDefinitions.header.fieldCount = authored.header.fieldCount;
	memcpy( entityDefinitions.fields, authored.fields, authored.header.fieldCount * sizeof( entityDefinitionField_t ) );
	G_Printf( "Entity definitions: %s authored=%u total=%u\n", path.string, authored.header.count, entityDefinitions.header.count );
}

/*
===============
G_CallSpawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
qboolean G_CallSpawn( gentity_t *ent ) {
	spawn_t *s;
	gitem_t *item;

	if ( !ent->classname ) {
		G_Printf( "G_CallSpawn: NULL classname\n" );
		return qfalse;
	}

	const auto *definition = Entity_FindDefinition( entityDefinitions, ent->classname );
	const char *native = definition ? definition->native : ent->classname;

	// check item spawn functions
	for ( item = bg_itemlist + 1; item->classname; item++ ) {
		if ( !strcmp( item->classname, native ) ) {
			G_SpawnItem( ent, item );
			return qtrue;
		}
	}

	// check normal spawn functions
	for ( s = spawns; s->name; s++ ) {
		if ( !strcmp( s->name, native ) ) {
			// found it
			s->spawn( ent );
			return qtrue;
		}
	}
	G_Printf( "%s doesn't have a spawn function\n", ent->classname );
	return qfalse;
}

/*
=============
G_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *G_NewString( const char *string ) {
	char *newb, *new_p;
	int i, l;

	l = (int)( strlen( string ) + 1 );

	newb = (char *)G_Alloc( l );

	new_p = newb;

	// turn \n into a real linefeed
	for ( i = 0; i < l; i++ ) {
		if ( string[i] == '\\' && i < l - 1 ) {
			i++;
			if ( string[i] == 'n' ) {
				*new_p++ = '\n';
			} else {
				*new_p++ = '\\';
			}
		} else {
			*new_p++ = string[i];
		}
	}

	return newb;
}


/*
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
void G_ParseField( const char *key, const char *value, gentity_t *ent ) {
	field_t *f;
	byte *b;
	float v;
	vec3_t vec;

	for ( f = fields; f->name; f++ ) {
		if ( !Q_stricmp( f->name, key ) ) {
			// found it
			b = (byte *)ent;

			switch ( f->type ) {
			case F_LSTRING:
				*(char **)( b + f->ofs ) = G_NewString( value );
				break;
			case F_VECTOR:
				sscanf( value, "%f %f %f", &vec[0], &vec[1], &vec[2] );
				( (float *)( b + f->ofs ) )[0] = vec[0];
				( (float *)( b + f->ofs ) )[1] = vec[1];
				( (float *)( b + f->ofs ) )[2] = vec[2];
				break;
			case F_INT:
				*(int *)( b + f->ofs ) = atoi( value );
				break;
			case F_FLOAT:
				*(float *)( b + f->ofs ) = (float)( atof( value ) );
				break;
			case F_ANGLEHACK:
				v = (float)( atof( value ) );
				( (float *)( b + f->ofs ) )[0] = 0;
				( (float *)( b + f->ofs ) )[1] = v;
				( (float *)( b + f->ofs ) )[2] = 0;
				break;
			default:
			case F_IGNORE:
				break;
			}
			return;
		}
	}
}


/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
void G_SpawnGEntityFromSpawnVars( void ) {
	int i;
	gentity_t *ent;
	char *s, *value, *gametypeName;
	static char *gametypeNames[] = { (char *)"ffa", (char *)"tournament", (char *)"single", (char *)"team", (char *)"ctf", (char *)"oneflag", (char *)"obelisk", (char *)"harvester", (char *)"teamtournament" };

	// get the next free entity
	ent = G_Spawn();
#ifdef AFTERSHOCK_DEVTOOLS
	G_DevMapEntity( ent );
#endif

	const auto *definition = SpawnDefinition();
	if ( definition )
		for ( uint32_t field = definition->firstField; field < definition->firstField + definition->fieldCount; ++field )
			G_ParseField( entityDefinitions.fields[field].key, entityDefinitions.fields[field].value, ent );

	for ( i = 0; i < level.numSpawnVars; i++ ) {
		G_ParseField( level.spawnVars[i][0], level.spawnVars[i][1], ent );
	}

	// check for "notsingle" flag
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		G_SpawnInt( "notsingle", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	}
	// check for "notteam" flag (GT_FFA, GT_TOURNAMENT, GT_SINGLE_PLAYER)
	if ( g_gametype.integer >= GT_TEAM ) {
		G_SpawnInt( "notteam", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	} else {
		G_SpawnInt( "notfree", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	}

#ifdef MISSIONPACK
	G_SpawnInt( "notta", "0", &i );
	if ( i ) {
		G_FreeEntity( ent );
		return;
	}
#else
	G_SpawnInt( "notq3a", "0", &i );
	if ( i ) {
		G_FreeEntity( ent );
		return;
	}
#endif

	if ( G_SpawnString( "gametype", NULL, &value ) ) {
		if ( g_gametype.integer >= GT_FFA && g_gametype.integer < GT_MAX_GAME_TYPE ) {
			gametypeName = gametypeNames[g_gametype.integer];

			s = strstr( value, gametypeName );
			if ( !s ) {
				G_FreeEntity( ent );
				return;
			}
		}
	}

	// move editor origin to pos
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	// if we didn't get a classname, don't bother spawning anything
	if ( !G_CallSpawn( ent ) ) {
		G_FreeEntity( ent );
	} else if ( ent->inuse && definition && ( definition->priority || definition->radius ) ) {
		GameImport_SetEntityReplication( ent->s.number, int( definition->priority ), definition->radius );
	}
}


/*
====================
G_AddSpawnVarToken
====================
*/
char *G_AddSpawnVarToken( const char *string ) {
	int l;
	char *dest;

	l = (int)( strlen( string ) );
	if ( level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		G_Error( "G_AddSpawnVarToken: MAX_SPAWN_CHARS" );
	}

	dest = level.spawnVarChars + level.numSpawnVarChars;
	memcpy( dest, string, l + 1 );

	level.numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean G_ParseSpawnVars( void ) {
	char keyname[MAX_TOKEN_CHARS];
	char com_token[MAX_TOKEN_CHARS];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		G_Error( "G_ParseSpawnVars: found %s when expecting {", com_token );
	}

	// go through all the key / value pairs
	while ( 1 ) {
		// parse key
		if ( !trap_GetEntityToken( keyname, sizeof( keyname ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( keyname[0] == '}' ) {
			break;
		}

		// parse value
		if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( com_token[0] == '}' ) {
			G_Error( "G_ParseSpawnVars: closing brace without data" );
		}
		if ( level.numSpawnVars == MAX_SPAWN_VARS ) {
			G_Error( "G_ParseSpawnVars: MAX_SPAWN_VARS" );
		}
		level.spawnVars[level.numSpawnVars][0] = G_AddSpawnVarToken( keyname );
		level.spawnVars[level.numSpawnVars][1] = G_AddSpawnVarToken( com_token );
		level.numSpawnVars++;
	}

#ifdef AFTERSHOCK_DEVTOOLS
	G_DevCapture();
#endif

	return qtrue;
}


/*QUAKED worldspawn (0 0 0) ?

Every map should have exactly one worldspawn.
"music"		music wav file
"gravity"	800 is default gravity
"message"	Text to print during connection process
*/
void SP_worldspawn( void ) {
	char *s;

	G_SpawnString( "classname", "", &s );
	if ( Q_stricmp( s, "worldspawn" ) ) {
		G_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	// make some data visible to connecting client
	trap_SetConfigstring( CS_GAME_VERSION, GAME_VERSION );

	trap_SetConfigstring( CS_LEVEL_START_TIME, va( (char *)"%i", level.startTime ) );

	G_SpawnString( "music", "", &s );
	trap_SetConfigstring( CS_MUSIC, s );

	G_SpawnString( "message", "", &s );
	trap_SetConfigstring( CS_MESSAGE, s ); // map specific message

	trap_SetConfigstring( CS_MOTD, g_motd.string ); // message of the day

	G_SpawnString( "gravity", "800", &s );
	trap_Cvar_Set( "g_gravity", s );

	G_SpawnString( "enableDust", "0", &s );
	trap_Cvar_Set( "g_enableDust", s );

	G_SpawnString( "enableBreath", "0", &s );
	trap_Cvar_Set( "g_enableBreath", s );

	g_entities[ENTITYNUM_WORLD].s.number = ENTITYNUM_WORLD;
	g_entities[ENTITYNUM_WORLD].classname = "worldspawn";

	// see if we want a warmup time
	trap_SetConfigstring( CS_WARMUP, "" );
	if ( g_restarted.integer ) {
		trap_Cvar_Set( "g_restarted", "0" );
		level.warmupTime = 0;
	} else if ( g_doWarmup.integer ) { // Turn it on
		level.warmupTime = -1;
		trap_SetConfigstring( CS_WARMUP, va( (char *)"%i", level.warmupTime ) );
		G_LogPrintf( "Warmup:\n" );
	}
}


/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void G_SpawnEntitiesFromString( void ) {
	G_ResetComposed();
	InitEntityDefinitions();
#ifdef AFTERSHOCK_DEVTOOLS
	G_DevReset();
#endif
	// allow calls to G_Spawn*()
	level.spawning = qtrue;
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars() ) {
		G_Error( "SpawnEntities: no entities" );
	}
	SP_worldspawn();

	// parse ents
	while ( G_ParseSpawnVars() ) {
		G_SpawnGEntityFromSpawnVars();
	}

	level.spawning = qfalse; // any future calls to G_Spawn*() will be errors
}

#ifdef AFTERSHOCK_DEVTOOLS
// Developer documents retain every map key, including keys unknown to fields[].
// ponytail: fixed 8 MiB document capacity; disable saving on overflow, never truncate.
static char devDocuments[MAX_GENTITIES][8192];
static int devSource[MAX_GENTITIES], devDocumentCount, devCurrentSource, devSpawned;
static bool devComplete;

static bool Dev_Append( char *text, const char *part ) {
	const size_t used = strlen( text ), size = strlen( part );
	if ( size >= sizeof( devDocuments[0] ) - used )
		return false;
	memcpy( text + used, part, size + 1 );
	return true;
}

static bool Dev_Pair( char *text, const char *key, const char *value ) {
	if ( strchr( key, '"' ) || strchr( value, '"' ) )
		return false;
	return Dev_Append( text, "\"" ) && Dev_Append( text, key ) && Dev_Append( text, "\" \"" ) &&
		   Dev_Append( text, value ) && Dev_Append( text, "\"\n" );
}

static void G_DevCapture( void ) {
	devCurrentSource = -1;
	if ( !devComplete || devDocumentCount == MAX_GENTITIES ) {
		devComplete = false;
		return;
	}
	char *text = devDocuments[devDocumentCount];
	strcpy( text, "{\n" );
	for ( int i = 0; i < level.numSpawnVars; ++i ) {
		if ( !Dev_Pair( text, level.spawnVars[i][0], level.spawnVars[i][1] ) ) {
			devComplete = false;
			return;
		}
	}
	if ( !Dev_Append( text, "}\n" ) ) {
		devComplete = false;
		return;
	}
	devCurrentSource = devDocumentCount++;
}

void G_DevForgetEntity( int entity ) {
	devSource[entity] = -1;
}

static void G_DevMapEntity( gentity_t *entity ) {
	devSpawned = (int)( entity - g_entities );
	devSource[devSpawned] = devCurrentSource;
}

static bool Dev_ReadEntity( int index, devEntity_t *out ) {
	*out = {};
	if ( index < 0 || index >= level.num_entities || !g_entities[index].inuse )
		return false;
	const gentity_t *entity = &g_entities[index];
	Q_strncpyz( out->classname, entity->classname ? entity->classname : "", sizeof( out->classname ) );
	VectorCopy( entity->r.currentOrigin, out->origin );
	VectorCopy( entity->r.absmin, out->mins );
	VectorCopy( entity->r.absmax, out->maxs );
	out->source = devSource[index];
	out->health = entity->health;
	out->model = entity->s.modelindex;
	out->frame = entity->s.frame;
	out->sound = entity->s.loopSound;
	out->contents = entity->r.contents;
	out->linked = entity->r.linked != 0;
	return true;
}

static const field_t *Dev_Field( const char *key ) {
	for ( const field_t *field = fields; field->name; ++field ) {
		if ( !Q_stricmp( field->name, key ) )
			return field;
	}
	return nullptr;
}

static const char *Dev_FieldName( int index ) {
	return index >= 0 && (size_t)index < sizeof( fields ) / sizeof( fields[0] ) - 1 ? fields[index].name : nullptr;
}

static bool Dev_ReadField( int index, const char *key, char *value, int capacity ) {
	devEntity_t info;
	const field_t *field = Dev_Field( key );
	if ( capacity <= 0 || !Dev_ReadEntity( index, &info ) || !field )
		return false;
	const byte *data = (const byte *)&g_entities[index] + field->ofs;
	switch ( field->type ) {
	case F_LSTRING: {
		const char *text = *(const char *const *)data;
		Q_strncpyz( value, text ? text : "", capacity );
		break;
	}
	case F_VECTOR:
		Com_sprintf( value, capacity, "%.9f %.9f %.9f", ( (const float *)data )[0], ( (const float *)data )[1], ( (const float *)data )[2] );
		break;
	case F_ANGLEHACK:
		Com_sprintf( value, capacity, "%.9f", ( (const float *)data )[1] );
		break;
	case F_FLOAT:
		Com_sprintf( value, capacity, "%.9f", *(const float *)data );
		break;
	case F_INT:
		Com_sprintf( value, capacity, "%d", *(const int *)data );
		break;
	default:
		value[0] = '\0';
		return false;
	}
	return true;
}

static bool Dev_Value( const field_t *field, const char *value ) {
	if ( !*value && field->type != F_LSTRING )
		return false;
	if ( field->type == F_LSTRING )
		return !strchr( value, '\n' ) && !strchr( value, '\r' ) && !strchr( value, '"' );
	const char *end = value + strlen( value );
	const int count = field->type == F_VECTOR ? 3 : 1;
	for ( int i = 0; i < count; ++i ) {
		while ( *value == ' ' )
			++value;
		if ( field->type == F_INT ) {
			int number;
			const std::from_chars_result result = std::from_chars( value, end, number );
			if ( result.ec != std::errc() )
				return false;
			value = result.ptr;
		} else {
			char *next;
			const float number = ::strtof( value, &next );
			if ( next == value || !std::isfinite( number ) )
				return false;
			// The native game's legacy numeric reader accepts decimal notation only.
			for ( const char *p = value; p < next; ++p ) {
				if ( !( ( *p >= '0' && *p <= '9' ) || *p == '.' || *p == '+' || *p == '-' ) )
					return false;
			}
			value = next;
		}
	}
	while ( *value == ' ' )
		++value;
	return value == end;
}

static bool Dev_ReplaceField( int source, const char *key, const char *value, char *output ) {
	char *cursor = devDocuments[source];
	COM_Parse( &cursor ); // opening brace from our retained document
	strcpy( output, "{\n" );
	bool replaced = false;
	while ( cursor ) {
		char name[MAX_TOKEN_CHARS];
		Q_strncpyz( name, COM_Parse( &cursor ), sizeof( name ) );
		if ( !strcmp( name, "}" ) )
			break;
		const char *oldValue = COM_Parse( &cursor );
		const bool matches = !Q_stricmp( name, key );
		// Both spellings write the same field; retain only the intentionally edited one.
		if ( ( !Q_stricmp( key, "angle" ) && !Q_stricmp( name, "angles" ) ) ||
			 ( !Q_stricmp( key, "angles" ) && !Q_stricmp( name, "angle" ) ) )
			continue;
		if ( !Dev_Pair( output, name, matches ? value : oldValue ) )
			return false;
		replaced |= matches;
	}
	return ( replaced || Dev_Pair( output, key, value ) ) && Dev_Append( output, "}\n" );
}

static bool Dev_WriteField( int index, const char *key, const char *value ) {
	devEntity_t info;
	const field_t *field = Dev_Field( key );
	if ( !trap_Cvar_VariableIntegerValue( "sv_cheats" ) || !Dev_ReadEntity( index, &info ) || info.source < 0 || !field || field->type == F_IGNORE ||
		 !Q_stricmp( key, "classname" ) || !Q_stricmp( key, "model" ) || !Q_stricmp( key, "model2" ) || !Q_stricmp( key, "team" ) || !Dev_Value( field, value ) )
		return false;
	char text[sizeof( devDocuments[0] )];
	if ( !Dev_ReplaceField( info.source, key, value, text ) )
		return false;
	gentity_t *entity = &g_entities[index];
	G_ParseField( key, value, entity );
	if ( !Q_stricmp( key, "origin" ) )
		G_SetOrigin( entity, entity->s.origin );
	if ( !Q_stricmp( key, "angles" ) || !Q_stricmp( key, "angle" ) ) {
		VectorCopy( entity->s.angles, entity->s.apos.trBase );
		VectorCopy( entity->s.angles, entity->r.currentAngles );
	}
	if ( entity->r.linked )
		trap_LinkEntity( entity );
	Q_strncpyz( devDocuments[info.source], text, sizeof( devDocuments[0] ) );
	return true;
}

static int Dev_Spawn( const char *classname, const float *origin ) {
	if ( !trap_Cvar_VariableIntegerValue( "sv_cheats" ) || !devComplete || devDocumentCount == MAX_GENTITIES || ( level.num_entities >= ENTITYNUM_MAX_NORMAL && !G_EntitiesFree() ) )
		return -1;
	const auto *definition = Entity_FindDefinition( entityDefinitions, classname );
	const char *native = definition ? definition->native : classname;
	bool allowed = !strcmp( native, "composed" ) || !strcmp( native, "target_position" ) || !strcmp( native, "info_notnull" ) ||
				   !strcmp( native, "info_player_deathmatch" ) || !strcmp( native, "misc_teleporter_dest" );
	for ( const gitem_t *item = bg_itemlist + 1; item->classname; ++item )
		allowed |= !strcmp( native, item->classname );
	if ( !allowed || !std::isfinite( origin[0] ) || !std::isfinite( origin[1] ) || !std::isfinite( origin[2] ) )
		return -1;
	level.numSpawnVars = level.numSpawnVarChars = 0;
	char position[128];
	Com_sprintf( position, sizeof( position ), "%.9f %.9f %.9f", origin[0], origin[1], origin[2] );
	level.spawnVars[0][0] = G_AddSpawnVarToken( "classname" );
	level.spawnVars[0][1] = G_AddSpawnVarToken( classname );
	level.spawnVars[1][0] = G_AddSpawnVarToken( "origin" );
	level.spawnVars[1][1] = G_AddSpawnVarToken( position );
	level.numSpawnVars = 2;
	G_DevCapture();
	level.spawning = qtrue;
	G_SpawnGEntityFromSpawnVars();
	level.spawning = qfalse;
	return g_entities[devSpawned].inuse ? devSpawned : -1;
}

static bool Dev_Delete( int index ) {
	devEntity_t info;
	if ( !trap_Cvar_VariableIntegerValue( "sv_cheats" ) || !Dev_ReadEntity( index, &info ) || info.source < 0 || index < MAX_CLIENTS || g_entities[index].neverFree )
		return false;
	G_FreeEntity( &g_entities[index] );
	devDocuments[info.source][0] = '\0';
	return true;
}

static int Dev_MapCount( void ) {
	return devComplete ? devDocumentCount : -1;
}
static const char *Dev_MapText( int index ) {
	return index >= 0 && index < devDocumentCount ? devDocuments[index] : nullptr;
}

static const entityDefinitions_t *Dev_Definitions() {
	return authoredDefinitions.header.count ? &authoredDefinitions : nullptr;
}
static bool Dev_WriteDefinition( const char *name, const char *key, const char *value ) {
	if ( !trap_Cvar_VariableIntegerValue( "sv_cheats" ) || !Entity_SetField( &authoredDefinitions, name, key, value ) )
		return false;
	const auto *source = Entity_FindDefinition( authoredDefinitions, name );
	const auto *target = Entity_FindDefinition( entityDefinitions, name );
	entityDefinitions.definitions[target - entityDefinitions.definitions] = *source;
	for ( uint32_t i = source->firstField; i < source->firstField + source->fieldCount; ++i )
		entityDefinitions.fields[i] = authoredDefinitions.fields[i];
	return true;
}

static void G_DevReset( void ) {
	memset( devSource, 0xff, sizeof( devSource ) );
	devDocumentCount = 0;
	devCurrentSource = -1;
	devComplete = true;
	static const devGameTools_t tools = { Dev_ReadEntity, Dev_FieldName, Dev_ReadField, Dev_WriteField,
		Dev_Spawn, Dev_Delete, Dev_MapCount, Dev_MapText, G_DevWeapon, G_DevAnimation, Dev_Definitions, Dev_WriteDefinition };
	Dev_RegisterGameTools( &tools );
}
#endif
