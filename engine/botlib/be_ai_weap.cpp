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

/*****************************************************************************
 * name:		be_ai_weap.c
 *
 * desc:		weapon AI
 *
 * $Archive: /MissionPack/code/botlib/be_ai_weap.c $
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "l_libvar.h"
#include "l_log.h"
#include "l_memory.h"
#include "l_utils.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botlib_public.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_ai_weight.h" //fuzzy weights
#include "be_ai_weap.h"
#include "../../third_party/sha256/sha-256.h"

//#define DEBUG_AI_WEAP

//structure field offsets
#define WEAPON_OFS( x ) (int)(size_t)&(((weaponinfo_t *)0)->x)
#define PROJECTILE_OFS( x ) (int)(size_t)&(((projectileinfo_t *)0)->x)

//weapon definition
static const fielddef_t weaponinfo_fields[] = {
	{ "number", WEAPON_OFS( number ), FT_INT, 0, 0, 0, NULL }, //weapon number
	{ "name", WEAPON_OFS( name ), FT_STRING, 0, 0, 0, NULL }, //name of the weapon
	{ "level", WEAPON_OFS( level ), FT_INT, 0, 0, 0, NULL },
	{ "model", WEAPON_OFS( model ), FT_STRING, 0, 0, 0, NULL }, //model of the weapon
	{ "weaponindex", WEAPON_OFS( weaponindex ), FT_INT, 0, 0, 0, NULL }, //index of weapon in inventory
	{ "flags", WEAPON_OFS( flags ), FT_INT, 0, 0, 0, NULL }, //special flags
	{ "projectile", WEAPON_OFS( projectile ), FT_STRING, 0, 0, 0, NULL }, //projectile used by the weapon
	{ "numprojectiles", WEAPON_OFS( numprojectiles ), FT_INT, 0, 0, 0, NULL }, //number of projectiles
	{ "hspread", WEAPON_OFS( hspread ), FT_FLOAT, 0, 0, 0, NULL }, //horizontal spread of projectiles (degrees from middle)
	{ "vspread", WEAPON_OFS( vspread ), FT_FLOAT, 0, 0, 0, NULL }, //vertical spread of projectiles (degrees from middle)
	{ "speed", WEAPON_OFS( speed ), FT_FLOAT, 0, 0, 0, NULL }, //speed of the projectile (0 = instant hit)
	{ "acceleration", WEAPON_OFS( acceleration ), FT_FLOAT, 0, 0, 0, NULL }, //"acceleration" * time (in seconds) + "speed" = projectile speed
	{ "recoil", WEAPON_OFS( recoil ), FT_FLOAT | FT_ARRAY, 3, 0, 0, NULL }, //amount of recoil the player gets from the weapon
	{ "offset", WEAPON_OFS( offset ), FT_FLOAT | FT_ARRAY, 3, 0, 0, NULL }, //projectile start offset relative to eye and view angles
	{ "angleoffset", WEAPON_OFS( angleoffset ), FT_FLOAT | FT_ARRAY, 3, 0, 0, NULL }, //offset of the shoot angles relative to the view angles
	{ "extrazvelocity", WEAPON_OFS( extrazvelocity ), FT_FLOAT, 0, 0, 0, NULL }, //extra z velocity the projectile gets
	{ "ammoamount", WEAPON_OFS( ammoamount ), FT_INT, 0, 0, 0, NULL }, //ammo amount used per shot
	{ "ammoindex", WEAPON_OFS( ammoindex ), FT_INT, 0, 0, 0, NULL }, //index of ammo in inventory
	{ "activate", WEAPON_OFS( activate ), FT_FLOAT, 0, 0, 0, NULL }, //time it takes to select the weapon
	{ "reload", WEAPON_OFS( reload ), FT_FLOAT, 0, 0, 0, NULL }, //time it takes to reload the weapon
	{ "spinup", WEAPON_OFS( spinup ), FT_FLOAT, 0, 0, 0, NULL }, //time it takes before first shot
	{ "spindown", WEAPON_OFS( spindown ), FT_FLOAT, 0, 0, 0, NULL }, //time it takes before weapon stops firing
	{ NULL, 0, 0, 0, 0, 0, NULL }
};

//projectile definition
static const fielddef_t projectileinfo_fields[] = {
	{ "name", PROJECTILE_OFS( name ), FT_STRING, 0, 0, 0, NULL }, //name of the projectile
	{ "model", PROJECTILE_OFS( model ), FT_STRING, 0, 0, 0, NULL }, //model of the projectile
	{ "flags", PROJECTILE_OFS( flags ), FT_INT, 0, 0, 0, NULL }, //special flags
	{ "gravity", PROJECTILE_OFS( gravity ), FT_FLOAT, 0, 0, 0, NULL }, //amount of gravity applied to the projectile [0,1]
	{ "damage", PROJECTILE_OFS( damage ), FT_INT, 0, 0, 0, NULL }, //damage of the projectile
	{ "radius", PROJECTILE_OFS( radius ), FT_FLOAT, 0, 0, 0, NULL }, //radius of damage
	{ "visdamage", PROJECTILE_OFS( visdamage ), FT_INT, 0, 0, 0, NULL }, //damage of the projectile to visible entities
	{ "damagetype", PROJECTILE_OFS( damagetype ), FT_INT, 0, 0, 0, NULL }, //type of damage (combination of the DAMAGETYPE_? flags)
	{ "healthinc", PROJECTILE_OFS( healthinc ), FT_INT, 0, 0, 0, NULL }, //health increase the owner gets
	{ "push", PROJECTILE_OFS( push ), FT_FLOAT, 0, 0, 0, NULL }, //amount a player is pushed away from the projectile impact
	{ "detonation", PROJECTILE_OFS( detonation ), FT_FLOAT, 0, 0, 0, NULL }, //time before projectile explodes after fire pressed
	{ "bounce", PROJECTILE_OFS( bounce ), FT_FLOAT, 0, 0, 0, NULL }, //amount the projectile bounces
	{ "bouncefric", PROJECTILE_OFS( bouncefric ), FT_FLOAT, 0, 0, 0, NULL }, //amount the bounce decreases per bounce
	{ "bouncestop", PROJECTILE_OFS( bouncestop ), FT_FLOAT, 0, 0, 0, NULL }, //minimum bounce value before bouncing stops
	//recursive projectile definition??
	{ NULL, 0, 0, 0, 0, 0, NULL }
};

static structdef_t weaponinfo_struct = {
	sizeof( weaponinfo_t ), weaponinfo_fields
};
static structdef_t projectileinfo_struct = {
	sizeof( projectileinfo_t ), projectileinfo_fields
};

//weapon configuration: set of weapons with projectiles
typedef struct weaponconfig_s {
	int numweapons;
	int numprojectiles;
	projectileinfo_t *projectileinfo;
	weaponinfo_t *weaponinfo;
} weaponconfig_t;

//the bot weapon state
typedef struct bot_weaponstate_s {
	struct weightconfig_s *weaponweightconfig; //weapon weight configuration
	int *weaponweightindex; //weapon weight index
} bot_weaponstate_t;

static bot_weaponstate_t *botweaponstates[MAX_CLIENTS + 1];
static weaponconfig_t *weaponconfig;

//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
static int BotValidWeaponNumber( int weaponnum ) {
	if ( weaponnum <= 0 || weaponnum > weaponconfig->numweapons ) {
		botimport.Print( PRT_ERROR, "weapon number out of range\n" );
		return qfalse;
	} //end if
	return qtrue;
} //end of the function BotValidWeaponNumber
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
static bot_weaponstate_t *BotWeaponStateFromHandle( int handle ) {
	if ( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "weapon state handle %d out of range\n", handle );
		return NULL;
	} //end if
	if ( !botweaponstates[handle] ) {
		botimport.Print( PRT_FATAL, "invalid weapon state %d\n", handle );
		return NULL;
	} //end if
	return botweaponstates[handle];
} //end of the function BotWeaponStateFromHandle
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
#ifdef DEBUG_AI_WEAP
static void DumpWeaponConfig( weaponconfig_t *wc ) {
	FILE *fp;
	int i;

	fp = Log_FileStruct();
	if ( !fp )
		return;
	for ( i = 0; i < wc->numprojectiles; i++ ) {
		WriteStructure( fp, &projectileinfo_struct, (char *)&wc->projectileinfo[i] );
		Log_Flush();
	} //end for
	for ( i = 0; i < wc->numweapons; i++ ) {
		WriteStructure( fp, &weaponinfo_struct, (char *)&wc->weaponinfo[i] );
		Log_Flush();
	} //end for
} //end of the function DumpWeaponConfig
#endif //DEBUG_AI_WEAP
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static weaponconfig_t *LoadWeaponConfig( const char *filename ) {
	int max_weaponinfo, max_projectileinfo;
	token_t token;
	char path[MAX_PATH];
	int i, j;
	source_t *source;
	weaponconfig_t *wc;
	weaponinfo_t weaponinfo;

	max_weaponinfo = LibVarInteger( "max_weaponinfo", "32", 0, 4096 );
	max_projectileinfo = LibVarInteger( "max_projectileinfo", "32", 0, 4096 );

	Q_strncpyz( path, filename, sizeof( path ) );
	PC_SetBaseFolder( BOTFILESBASEFOLDER );
	source = LoadSourceFile( path );
	if ( !source ) {
		botimport.Print( PRT_ERROR, "couldn't load %s\n", path );
		return NULL;
	} //end if
	//initialize weapon config
	wc = (weaponconfig_t *)GetClearedHunkMemory(sizeof(weaponconfig_t) +
										max_weaponinfo * sizeof(weaponinfo_t) +
										max_projectileinfo * sizeof(projectileinfo_t));
	wc->weaponinfo = (weaponinfo_t *)( (char *)wc + sizeof( weaponconfig_t ) );
	wc->projectileinfo = (projectileinfo_t *)( (char *)wc->weaponinfo +
											   max_weaponinfo * sizeof( weaponinfo_t ) );
	wc->numweapons = max_weaponinfo;
	wc->numprojectiles = 0;
	//parse the source file
	while ( PC_ReadToken( source, &token ) ) {
		if ( !strcmp( token.string, "weaponinfo" ) ) {
			Com_Memset( &weaponinfo, 0, sizeof( weaponinfo_t ) );
			if ( !ReadStructure( source, &weaponinfo_struct, (char *)&weaponinfo ) ) {
				FreeMemory( wc );
				FreeSource( source );
				return NULL;
			} //end if
			if ( weaponinfo.number < 0 || weaponinfo.number >= max_weaponinfo ) {
				botimport.Print( PRT_ERROR, "weapon info number %d out of range in %s\n", weaponinfo.number, path );
				FreeMemory( wc );
				FreeSource( source );
				return NULL;
			} //end if
			Com_Memcpy( &wc->weaponinfo[weaponinfo.number], &weaponinfo, sizeof( weaponinfo_t ) );
			wc->weaponinfo[weaponinfo.number].valid = qtrue;
		} //end if
		else if ( !strcmp( token.string, "projectileinfo" ) ) {
			if ( wc->numprojectiles >= max_projectileinfo ) {
				botimport.Print( PRT_ERROR, "more than %d projectiles defined in %s\n", max_projectileinfo, path );
				FreeMemory( wc );
				FreeSource( source );
				return NULL;
			} //end if
			Com_Memset( &wc->projectileinfo[wc->numprojectiles], 0, sizeof( projectileinfo_t ) );
			if ( !ReadStructure( source, &projectileinfo_struct, (char *)&wc->projectileinfo[wc->numprojectiles] ) ) {
				FreeMemory( wc );
				FreeSource( source );
				return NULL;
			} //end if
			wc->numprojectiles++;
		} //end if
		else {
			botimport.Print( PRT_ERROR, "unknown definition %s in %s\n", token.string, path );
			FreeMemory( wc );
			FreeSource( source );
			return NULL;
		} //end else
	} //end while
	FreeSource( source );
	//fix up weapons
	for ( i = 0; i < wc->numweapons; i++ ) {
		if ( !wc->weaponinfo[i].valid )
			continue;
		if ( !wc->weaponinfo[i].name[0] ) {
			botimport.Print( PRT_ERROR, "weapon %d has no name in %s\n", i, path );
			FreeMemory( wc );
			return NULL;
		} //end if
		if ( !wc->weaponinfo[i].projectile[0] ) {
			botimport.Print( PRT_ERROR, "weapon %s has no projectile in %s\n", wc->weaponinfo[i].name, path );
			FreeMemory( wc );
			return NULL;
		} //end if
		//find the projectile info and copy it to the weapon info
		for ( j = 0; j < wc->numprojectiles; j++ ) {
			if ( !strcmp( wc->projectileinfo[j].name, wc->weaponinfo[i].projectile ) ) {
				Com_Memcpy( &wc->weaponinfo[i].proj, &wc->projectileinfo[j], sizeof( projectileinfo_t ) );
				break;
			} //end if
		} //end for
		if ( j == wc->numprojectiles ) {
			botimport.Print( PRT_ERROR, "weapon %s uses undefined projectile in %s\n", wc->weaponinfo[i].name, path );
			FreeMemory( wc );
			return NULL;
		} //end if
	} //end for
	if ( !wc->numweapons )
		botimport.Print( PRT_WARNING, "no weapon info loaded\n" );
	botimport.Print( PRT_MESSAGE, "loaded %s\n", path );
	return wc;
} //end of the function LoadWeaponConfig
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static int *WeaponWeightIndex( const weightconfig_t *wwc, const weaponconfig_t *wc ) {
	int *index, i;

	//initialize item weight index
	index = (int *)GetClearedMemory(sizeof(int) * wc->numweapons);

	for ( i = 0; i < wc->numweapons; i++ ) {
		index[i] = FindFuzzyWeight( wwc, wc->weaponinfo[i].name );
	} //end for
	return index;
} //end of the function WeaponWeightIndex
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotFreeWeaponWeights( int weaponstate ) {
	bot_weaponstate_t *ws;

	ws = BotWeaponStateFromHandle( weaponstate );
	if ( !ws )
		return;
	if ( ws->weaponweightconfig )
		FreeWeightConfig( ws->weaponweightconfig );
	if ( ws->weaponweightindex )
		FreeMemory( ws->weaponweightindex );
} //end of the function BotFreeWeaponWeights
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotLoadWeaponWeights( int weaponstate, const char *filename ) {
	bot_weaponstate_t *ws;

	ws = BotWeaponStateFromHandle( weaponstate );
	if ( !ws )
		return BLERR_CANNOTLOADWEAPONWEIGHTS;
	BotFreeWeaponWeights( weaponstate );
	//
	ws->weaponweightconfig = ReadWeightConfig( filename );
	if ( !ws->weaponweightconfig ) {
		botimport.Print( PRT_FATAL, "couldn't load weapon config %s\n", filename );
		return BLERR_CANNOTLOADWEAPONWEIGHTS;
	} //end if
	if ( !weaponconfig )
		return BLERR_CANNOTLOADWEAPONCONFIG;
	ws->weaponweightindex = WeaponWeightIndex( ws->weaponweightconfig, weaponconfig );
	return BLERR_NOERROR;
} //end of the function BotLoadWeaponWeights
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotGetWeaponInfo( int weaponstate, int weapon, weaponinfo_t *weaponinfo ) {
	bot_weaponstate_t *ws;

	if ( !BotValidWeaponNumber( weapon ) )
		return;
	ws = BotWeaponStateFromHandle( weaponstate );
	if ( !ws )
		return;
	if ( !weaponconfig )
		return;
	Com_Memcpy( weaponinfo, &weaponconfig->weaponinfo[weapon], sizeof( weaponinfo_t ) );
} //end of the function BotGetWeaponInfo
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotChooseBestFightWeapon( int weaponstate, int *inventory ) {
	int i, index, bestweapon;
	float weight, bestweight;
	weaponconfig_t *wc;
	bot_weaponstate_t *ws;

	ws = BotWeaponStateFromHandle( weaponstate );
	if ( !ws )
		return 0;
	wc = weaponconfig;
	if ( !weaponconfig )
		return 0;

	//if the bot has no weapon weight configuration
	if ( !ws->weaponweightconfig )
		return 0;

	bestweight = 0;
	bestweapon = 0;
	for ( i = 0; i < wc->numweapons; i++ ) {
		if ( !wc->weaponinfo[i].valid )
			continue;
		index = ws->weaponweightindex[i];
		if ( index < 0 )
			continue;
		weight = FuzzyWeight( inventory, ws->weaponweightconfig, index );
		if ( weight > bestweight ) {
			bestweight = weight;
			bestweapon = i;
		} //end if
	} //end for
	return bestweapon;
} //end of the function BotChooseBestFightWeapon
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotResetWeaponState( int weaponstate [[maybe_unused]] ) {
} //end of the function BotResetWeaponState
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
int BotAllocWeaponState( void ) {
	int i;

	for ( i = 1; i <= MAX_CLIENTS; i++ ) {
		if ( !botweaponstates[i] ) {
			botweaponstates[i] = (bot_weaponstate_t *)GetClearedMemory(sizeof(bot_weaponstate_t));
			return i;
		} //end if
	} //end for
	return 0;
} //end of the function BotAllocWeaponState
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
void BotFreeWeaponState( int handle ) {
	if ( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "weapon state handle %d out of range\n", handle );
		return;
	} //end if
	if ( !botweaponstates[handle] ) {
		botimport.Print( PRT_FATAL, "invalid weapon state %d\n", handle );
		return;
	} //end if
	BotFreeWeaponWeights( handle );
	FreeMemory( botweaponstates[handle] );
	botweaponstates[handle] = NULL;
} //end of the function BotFreeWeaponState
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotSetupWeaponAI( void ) {
	const char *file;

	file = LibVarString( "weaponconfig", "weapons.c" );
	weaponconfig = LoadWeaponConfig( file );
	if ( !weaponconfig ) {
		botimport.Print( PRT_FATAL, "couldn't load the weapon config\n" );
		return BLERR_CANNOTLOADWEAPONCONFIG;
	} //end if

#ifdef DEBUG_AI_WEAP
	DumpWeaponConfig( weaponconfig );
#endif //DEBUG_AI_WEAP
	//
	return BLERR_NOERROR;
} //end of the function BotSetupWeaponAI
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotShutdownWeaponAI( void ) {
	int i;

	if ( weaponconfig )
		FreeMemory( weaponconfig );
	weaponconfig = NULL;

	for ( i = 1; i <= MAX_CLIENTS; i++ ) {
		if ( botweaponstates[i] ) {
			BotFreeWeaponState( i );
		} //end if
	} //end for
} //end of the function BotShutdownWeaponAI

struct weaponPoolSave_t {
	uint32_t present[MAX_CLIENTS + 1], indices[MAX_CLIENTS + 1], config;
	int32_t cachedWeights[MAX_CLIENTS + 1], weapons, projectiles;
	uint8_t contentHash[32], indexHash[MAX_CLIENTS + 1][32];
};
static_assert( sizeof( weaponPoolSave_t ) == 2904 );
static constexpr stateField_t weaponPoolFields[] = {
	{ "present", offsetof( weaponPoolSave_t, present ), MAX_CLIENTS + 1, stateType_t::UInt32 },
	{ "indices", offsetof( weaponPoolSave_t, indices ), MAX_CLIENTS + 1, stateType_t::UInt32 },
	{ "config", offsetof( weaponPoolSave_t, config ), 1, stateType_t::UInt32 },
	{ "cachedWeights", offsetof( weaponPoolSave_t, cachedWeights ), MAX_CLIENTS + 1, stateType_t::Int32 },
	{ "weapons", offsetof( weaponPoolSave_t, weapons ), 1, stateType_t::Int32 },
	{ "projectiles", offsetof( weaponPoolSave_t, projectiles ), 1, stateType_t::Int32 },
	{ "contentHash", offsetof( weaponPoolSave_t, contentHash ), 32, stateType_t::Bytes },
	{ "indexHash", offsetof( weaponPoolSave_t, indexHash ), ( MAX_CLIENTS + 1 ) * 32, stateType_t::Bytes }
};
static constexpr stateSchema_t weaponPoolSchema = { "botlib.weaponPool", 1, 1, sizeof( weaponPoolSave_t ), weaponPoolFields, 8 };
static bool WeaponPoolIdentity( weaponPoolSave_t *saved, bot_weaponstate_t *const *states = botweaponstates ) {
	*saved = {};
	for ( auto &index : saved->cachedWeights )
		index = -1;
	if ( states[0] )
		return false;
	saved->config = weaponconfig != nullptr;
	if ( weaponconfig ) {
		// These loaded descriptors have no pointers or padding on supported ABIs.
		static_assert( sizeof( projectileinfo_t ) == 2 * MAX_STRINGFIELD + 12 * 4 );
		static_assert( sizeof( weaponinfo_t ) == 3 * MAX_STRINGFIELD + 26 * 4 + sizeof( projectileinfo_t ) );
		saved->weapons = weaponconfig->numweapons;
		saved->projectiles = weaponconfig->numprojectiles;
		if ( saved->weapons < 0 || saved->weapons > 4096 || saved->projectiles < 0 || saved->projectiles > 4096 ||
			 ( saved->weapons && !weaponconfig->weaponinfo ) || ( saved->projectiles && !weaponconfig->projectileinfo ) )
			return false;
		Sha_256 hash;
		sha_256_init( &hash, saved->contentHash );
		const int32_t counts[] = { saved->weapons, saved->projectiles };
		sha_256_write( &hash, counts, sizeof( counts ) );
		if ( saved->weapons )
			sha_256_write( &hash, weaponconfig->weaponinfo, size_t( saved->weapons ) * sizeof( weaponinfo_t ) );
		if ( saved->projectiles )
			sha_256_write( &hash, weaponconfig->projectileinfo, size_t( saved->projectiles ) * sizeof( projectileinfo_t ) );
		sha_256_close( &hash );
	}
	for ( int i = 1; i <= MAX_CLIENTS; ++i ) {
		const auto *weapon = states[i];
		if ( !weapon )
			continue;
		saved->present[i] = 1;
		saved->cachedWeights[i] = Bot_WeightCacheIndex( weapon->weaponweightconfig );
		if ( saved->cachedWeights[i] == -2 )
			for ( int j = 1; j < i; ++j )
				if ( states[j] && states[j]->weaponweightconfig == weapon->weaponweightconfig )
					return false;
		saved->indices[i] = weapon->weaponweightindex != nullptr;
		if ( weapon->weaponweightindex ) {
			if ( !weaponconfig || !weapon->weaponweightconfig )
				return false;
			for ( int j = 0; j < saved->weapons; ++j )
				if ( weapon->weaponweightindex[j] < -1 || weapon->weaponweightindex[j] >= weapon->weaponweightconfig->numweights )
					return false;
			calc_sha_256( saved->indexHash[i], weapon->weaponweightindex, size_t( saved->weapons ) * sizeof( int32_t ) );
		}
	}
	return true;
}
bool Bot_WriteWeaponState( stateWriter_t *writer ) {
	if ( !writer )
		return false;
	weaponPoolSave_t pool;
	if ( !WeaponPoolIdentity( &pool ) ) {
		writer->failed = true;
		return false;
	}
	if ( !State_Append( writer, weaponPoolSchema, 0, &pool ) )
		return false;
	for ( uint32_t i = 1; i <= MAX_CLIENTS; ++i )
		if ( pool.present[i] && pool.cachedWeights[i] == -2 && !Bot_WriteWeightState( writer, 256 + i, botweaponstates[i]->weaponweightconfig ) )
			return false;
	return true;
}
bool Bot_ReadWeaponState( const stateReader_t &reader, bool apply ) {
	weaponPoolSave_t pool, current;
	uint32_t version;
	if ( !State_Find( reader, weaponPoolSchema, 0, &pool, &version ) || !WeaponPoolIdentity( &current ) || memcmp( &pool, &current, sizeof( pool ) ) )
		return false;
	for ( uint32_t i = 1; i <= MAX_CLIENTS; ++i )
		if ( pool.present[i] && pool.cachedWeights[i] == -2 && !Bot_ReadWeightState( reader, 256 + i, botweaponstates[i]->weaponweightconfig, false ) )
			return false;
	if ( apply )
		for ( uint32_t i = 1; i <= MAX_CLIENTS; ++i )
			if ( pool.present[i] && pool.cachedWeights[i] == -2 && !Bot_ReadWeightState( reader, 256 + i, botweaponstates[i]->weaponweightconfig, true ) )
				return false;
	return true;
}

bool Bot_PrepareWeaponState( const stateReader_t &reader ) {
	for ( const auto *state : botweaponstates )
		if ( state )
			return false;
	weaponPoolSave_t pool, loaded;
	uint32_t version;
	if ( !State_Find( reader, weaponPoolSchema, 0, &pool, &version ) || pool.present[0] )
		return false;
	bot_weaponstate_t *draft[MAX_CLIENTS + 1] = {};
	bool valid = true;
	for ( uint32_t i = 1; i <= MAX_CLIENTS && valid; ++i ) {
		if ( pool.present[i] > 1 || pool.indices[i] > 1 || pool.cachedWeights[i] < -2 || pool.cachedWeights[i] >= 128 ) {
			valid = false;
			break;
		}
		if ( !pool.present[i] )
			continue;
		auto *state = (bot_weaponstate_t *)GetClearedMemory( sizeof( bot_weaponstate_t ) );
		draft[i] = state;
		if ( pool.cachedWeights[i] == -2 )
			valid = Bot_CreateWeightState( reader, 256 + i, &state->weaponweightconfig );
		else if ( pool.cachedWeights[i] >= 0 ) {
			state->weaponweightconfig = Bot_WeightCacheAt( pool.cachedWeights[i] );
			valid = state->weaponweightconfig != nullptr;
		}
		if ( valid && pool.indices[i] ) {
			valid = weaponconfig && state->weaponweightconfig;
			if ( valid )
				state->weaponweightindex = WeaponWeightIndex( state->weaponweightconfig, weaponconfig );
		}
	}
	valid = valid && WeaponPoolIdentity( &loaded, draft ) && !memcmp( &pool, &loaded, sizeof( pool ) );
	if ( !valid ) {
		for ( auto *state : draft )
			if ( state ) {
				Bot_FreePrivateWeightState( state->weaponweightconfig );
				if ( state->weaponweightindex )
					FreeMemory( state->weaponweightindex );
				FreeMemory( state );
			}
		return false;
	}
	memcpy( botweaponstates, draft, sizeof( draft ) );
	return true;
}
