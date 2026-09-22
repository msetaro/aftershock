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
 * name:		be_ai_goal.c
 *
 * desc:		goal AI
 *
 * $Archive: /MissionPack/code/botlib/be_ai_goal.c $
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "l_utils.h"
#include "l_libvar.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botlib_public.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_ai_weight.h"
#include "be_ai_goal.h"
#include "be_ai_move.h"
#include "../../third_party/sha256/sha-256.h"
#include <cmath>

//#define DEBUG_AI_GOAL
#ifdef RANDOMIZE
#define UNDECIDEDFUZZY
#endif //RANDOMIZE
#define DROPPEDWEIGHT
//minimum avoid goal time
#define AVOID_MINIMUM_TIME		10
//default avoid goal time
#define AVOID_DEFAULT_TIME		30
//avoid dropped goal time
#define AVOID_DROPPED_TIME		10
//
#define TRAVELTIME_SCALE		0.01
//item flags
#define IFL_NOTFREE				1		//not in free for all
#define IFL_NOTTEAM				2		//not in team play
#define IFL_NOTSINGLE			4		//not in single player
#define IFL_NOTBOT				8		//bot should never go for this
#define IFL_ROAM				16		//bot roam goal

//location in the map "target_location"
typedef struct maplocation_s {
	vec3_t origin;
	int areanum;
	char name[MAX_EPAIRKEY];
	struct maplocation_s *next;
} maplocation_t;

//camp spots "info_camp"
typedef struct campspot_s {
	vec3_t origin;
	int areanum;
	char name[MAX_EPAIRKEY];
	float range;
	float weight;
	float wait;
	float random;
	struct campspot_s *next;
} campspot_t;

//FIXME: these are game specific
typedef enum {
	GT_FFA, // free for all
	GT_TOURNAMENT, // one on one tournament
	GT_SINGLE_PLAYER, // single player tournament

	//-- team games go after this --

	GT_TEAM, // team deathmatch
	GT_CTF, // capture the flag
#ifdef MISSIONPACK
	GT_1FCTF,
	GT_OBELISK,
	GT_HARVESTER,
#endif
	GT_MAX_GAME_TYPE
} gametype_t;

typedef struct levelitem_s {
	int number; //number of the level item
	int iteminfo; //index into the item info
	int flags; //item flags
	float weight; //fixed roam weight
	vec3_t origin; //origin of the item
	int goalareanum; //area the item is in
	vec3_t goalorigin; //goal origin within the area
	int entitynum; //entity number
	float timeout; //item is removed after this time
	struct levelitem_s *prev, *next;
} levelitem_t;

typedef struct iteminfo_s {
	char classname[32]; //classname of the item
	char name[MAX_STRINGFIELD]; //name of the item
	char model[MAX_STRINGFIELD]; //model of the item
	int modelindex; //model index
	int type; //item type
	int index; //index in the inventory
	float respawntime; //respawn time
	vec3_t mins; //mins of the item
	vec3_t maxs; //maxs of the item
	int number; //number of the item info
} iteminfo_t;

#define ITEMINFO_OFS( x )	(int)(size_t)&(((iteminfo_t *)0)->x)

static const fielddef_t iteminfo_fields[] = {
	{ "name", ITEMINFO_OFS( name ), FT_STRING, 0, 0, 0, NULL },
	{ "model", ITEMINFO_OFS( model ), FT_STRING, 0, 0, 0, NULL },
	{ "modelindex", ITEMINFO_OFS( modelindex ), FT_INT, 0, 0, 0, NULL },
	{ "type", ITEMINFO_OFS( type ), FT_INT, 0, 0, 0, NULL },
	{ "index", ITEMINFO_OFS( index ), FT_INT, 0, 0, 0, NULL },
	{ "respawntime", ITEMINFO_OFS( respawntime ), FT_FLOAT, 0, 0, 0, NULL },
	{ "mins", ITEMINFO_OFS( mins ), FT_FLOAT | FT_ARRAY, 3, 0, 0, NULL },
	{ "maxs", ITEMINFO_OFS( maxs ), FT_FLOAT | FT_ARRAY, 3, 0, 0, NULL },
	{ NULL, 0, 0, 0, 0, 0, NULL }
};

static const structdef_t iteminfo_struct = {
	sizeof( iteminfo_t ), iteminfo_fields
};

typedef struct itemconfig_s {
	int numiteminfo;
	iteminfo_t *iteminfo;
} itemconfig_t;

//goal state
typedef struct bot_goalstate_s {
	struct weightconfig_s *itemweightconfig; //weight config
	int *itemweightindex; //index from item to weight
	//
	int client; //client using this goal state
	int lastreachabilityarea; //last area with reachabilities the bot was in
	//
	bot_goal_t goalstack[MAX_GOALSTACK]; //goal stack
	int goalstacktop; //the top of the goal stack
	//
	int avoidgoals[MAX_AVOIDGOALS]; //goals to avoid
	float avoidgoaltimes[MAX_AVOIDGOALS]; //times to avoid the goals
} bot_goalstate_t;

static bot_goalstate_t *botgoalstates[MAX_CLIENTS + 1]; // FIXME: init?
//item configuration
static itemconfig_t *itemconfig = NULL;
//level items
static levelitem_t *levelitemheap = NULL;
static levelitem_t *freelevelitems = NULL;
static levelitem_t *levelitems = NULL;
static int numlevelitems = 0;
static int allocatedLevelItems;
//map locations
static maplocation_t *maplocations = NULL;
//camp spots
static campspot_t *campspots = NULL;
//the game type
static int g_gametype = 0;
//additional dropped item weight
static libvar_t *droppedweight = NULL;

//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
bot_goalstate_t *BotGoalStateFromHandle( int handle ) {
	if ( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "goal state handle %d out of range\n", handle );
		return NULL;
	} //end if
	if ( !botgoalstates[handle] ) {
		botimport.Print( PRT_FATAL, "invalid goal state %d\n", handle );
		return NULL;
	} //end if
	return botgoalstates[handle];
} //end of the function BotGoalStateFromHandle
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotInterbreedGoalFuzzyLogic( int parent1, int parent2, int child ) {
	const bot_goalstate_t *p1, *p2, *c;

	p1 = BotGoalStateFromHandle( parent1 );
	p2 = BotGoalStateFromHandle( parent2 );
	c = BotGoalStateFromHandle( child );

	if ( !p1 || !p2 || !c )
		return;

	InterbreedWeightConfigs( p1->itemweightconfig, p2->itemweightconfig,
		c->itemweightconfig );
} //end of the function BotInterbreedingGoalFuzzyLogic
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotSaveGoalFuzzyLogic( int goalstate [[maybe_unused]], const char *filename [[maybe_unused]] ) {
	//bot_goalstate_t *gs;

	//gs = BotGoalStateFromHandle(goalstate);

	// if ( !gs ) return;
	//WriteWeightConfig(filename, gs->itemweightconfig);
} //end of the function BotSaveGoalFuzzyLogic
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotMutateGoalFuzzyLogic( int goalstate, float range [[maybe_unused]] ) {
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle( goalstate );

	if ( !gs )
		return;
	EvolveWeightConfig( gs->itemweightconfig );
} //end of the function BotMutateGoalFuzzyLogic
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static itemconfig_t *LoadItemConfig( const char *filename ) {
	int max_iteminfo;
	token_t token;
	char path[MAX_PATH];
	source_t *source;
	itemconfig_t *ic;
	iteminfo_t *ii;

	max_iteminfo = LibVarInteger( "max_iteminfo", "256", 0, 4096 );

	Q_strncpyz( path, filename, sizeof( path ) );
	PC_SetBaseFolder( BOTFILESBASEFOLDER );
	source = LoadSourceFile( path );
	if ( !source ) {
		botimport.Print( PRT_ERROR, "couldn't load %s\n", path );
		return NULL;
	} //end if
	//initialize item config
	ic = (itemconfig_t *)GetClearedHunkMemory(sizeof(itemconfig_t) +
														max_iteminfo * sizeof(iteminfo_t));
	ic->iteminfo = (iteminfo_t *)( (char *)ic + sizeof( itemconfig_t ) );
	ic->numiteminfo = 0;
	//parse the item config file
	while ( PC_ReadToken( source, &token ) ) {
		if ( !strcmp( token.string, "iteminfo" ) ) {
			if ( ic->numiteminfo >= max_iteminfo ) {
				SourceError( source, "more than %d item info defined", max_iteminfo );
				FreeMemory( ic );
				FreeSource( source );
				return NULL;
			} //end if
			ii = &ic->iteminfo[ic->numiteminfo];
			Com_Memset( ii, 0, sizeof( iteminfo_t ) );
			if ( !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ) {
				FreeMemory( ic );
				FreeSource( source );
				return NULL;
			} //end if
			StripDoubleQuotes( token.string );
			Q_strncpyz( ii->classname, token.string, sizeof( ii->classname ) );
			if ( !ReadStructure( source, &iteminfo_struct, (char *)ii ) ) {
				FreeMemory( ic );
				FreeSource( source );
				return NULL;
			} //end if
			ii->number = ic->numiteminfo;
			ic->numiteminfo++;
		} //end if
		else {
			SourceError( source, "unknown definition %s", token.string );
			FreeMemory( ic );
			FreeSource( source );
			return NULL;
		} //end else
	} //end while
	FreeSource( source );
	//
	if ( !ic->numiteminfo )
		botimport.Print( PRT_WARNING, "no item info loaded\n" );
	botimport.Print( PRT_MESSAGE, "loaded %s\n", path );
	return ic;
} //end of the function LoadItemConfig
//===========================================================================
// index to find the weight function of an iteminfo
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static int *ItemWeightIndex( const weightconfig_t *iwc, const itemconfig_t *ic ) {
	int *index, i;

	//initialize item weight index
	index = (int *)GetClearedMemory(sizeof(int) * ic->numiteminfo);

	for ( i = 0; i < ic->numiteminfo; i++ ) {
		index[i] = FindFuzzyWeight( iwc, ic->iteminfo[i].classname );
		if ( index[i] < 0 ) {
			Log_Write( "item info %d \"%s\" has no fuzzy weight\r\n", i, ic->iteminfo[i].classname );
		} //end if
	} //end for
	return index;
} //end of the function ItemWeightIndex
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void InitLevelItemHeap( void ) {
	int i, max_levelitems;

	if ( levelitemheap )
		FreeMemory( levelitemheap );

	max_levelitems = LibVarInteger( "max_levelitems", "256", 1, 4096 );
	levelitemheap = (levelitem_t *)GetClearedMemory(max_levelitems * sizeof(levelitem_t));
	allocatedLevelItems = max_levelitems;

	for ( i = 0; i < max_levelitems - 1; i++ ) {
		levelitemheap[i].next = &levelitemheap[i + 1];
	} //end for
	levelitemheap[max_levelitems - 1].next = NULL;
	//
	freelevelitems = levelitemheap;
} //end of the function InitLevelItemHeap
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static levelitem_t *AllocLevelItem( void ) {
	levelitem_t *li;

	li = freelevelitems;
	if ( !li ) {
		botimport.Print( PRT_FATAL, "out of level items\n" );
		return NULL;
	} //end if
	//
	freelevelitems = freelevelitems->next;
	Com_Memset( li, 0, sizeof( levelitem_t ) );
	return li;
} //end of the function AllocLevelItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void FreeLevelItem( levelitem_t *li ) {
	li->next = freelevelitems;
	freelevelitems = li;
} //end of the function FreeLevelItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void AddLevelItemToList( levelitem_t *li ) {
	if ( levelitems )
		levelitems->prev = li;
	li->prev = NULL;
	li->next = levelitems;
	levelitems = li;
} //end of the function AddLevelItemToList
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void RemoveLevelItemFromList( levelitem_t *li ) {
	if ( li->prev )
		li->prev->next = li->next;
	else
		levelitems = li->next;
	if ( li->next )
		li->next->prev = li->prev;
} //end of the function RemoveLevelItemFromList
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
static void BotFreeInfoEntities( void ) {
	maplocation_t *ml, *nextml;
	campspot_t *cs, *nextcs;

	for ( ml = maplocations; ml; ml = nextml ) {
		nextml = ml->next;
		FreeMemory( ml );
	} //end for
	maplocations = NULL;
	for ( cs = campspots; cs; cs = nextcs ) {
		nextcs = cs->next;
		FreeMemory( cs );
	} //end for
	campspots = NULL;
} //end of the function BotFreeInfoEntities
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
static void BotInitInfoEntities( void ) {
	char classname[MAX_EPAIRKEY];
	maplocation_t *ml;
	campspot_t *cs;
	int ent, numlocations, numcampspots;

	BotFreeInfoEntities();
	//
	numlocations = 0;
	numcampspots = 0;
	for ( ent = AAS_NextBSPEntity( 0 ); ent; ent = AAS_NextBSPEntity( ent ) ) {
		if ( !AAS_ValueForBSPEpairKey( ent, "classname", classname, MAX_EPAIRKEY ) )
			continue;

		//map locations
		if ( !strcmp( classname, "target_location" ) ) {
			ml = (maplocation_t *)GetClearedMemory(sizeof(maplocation_t));
			AAS_VectorForBSPEpairKey( ent, "origin", ml->origin );
			AAS_ValueForBSPEpairKey( ent, "message", ml->name, sizeof( ml->name ) );
			ml->areanum = AAS_PointAreaNum( ml->origin );
			ml->next = maplocations;
			maplocations = ml;
			numlocations++;
		} //end if
		//camp spots
		else if ( !strcmp( classname, "info_camp" ) ) {
			cs = (campspot_t *)GetClearedMemory(sizeof(campspot_t));
			AAS_VectorForBSPEpairKey( ent, "origin", cs->origin );
			//cs->origin[2] += 16;
			AAS_ValueForBSPEpairKey( ent, "message", cs->name, sizeof( cs->name ) );
			AAS_FloatForBSPEpairKey( ent, "range", &cs->range );
			AAS_FloatForBSPEpairKey( ent, "weight", &cs->weight );
			AAS_FloatForBSPEpairKey( ent, "wait", &cs->wait );
			AAS_FloatForBSPEpairKey( ent, "random", &cs->random );
			cs->areanum = AAS_PointAreaNum( cs->origin );
			if ( !cs->areanum ) {
				botimport.Print( PRT_MESSAGE, "camp spot at %1.1f %1.1f %1.1f in solid\n", cs->origin[0], cs->origin[1], cs->origin[2] );
				FreeMemory( cs );
				continue;
			} //end if
			cs->next = campspots;
			campspots = cs;
			//AAS_DrawPermanentCross(cs->origin, 4, LINECOLOR_YELLOW);
			numcampspots++;
		} //end else if
	} //end for
	if ( botDeveloper ) {
		botimport.Print( PRT_MESSAGE, "%d map locations\n", numlocations );
		botimport.Print( PRT_MESSAGE, "%d camp spots\n", numcampspots );
	} //end if
} //end of the function BotInitInfoEntities
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotInitLevelItems( void ) {
	int i, spawnflags, value;
	char classname[MAX_EPAIRKEY];
	vec3_t origin, end;
	int ent, goalareanum;
	itemconfig_t *ic;
	levelitem_t *li;
	bsp_trace_t trace;

	//initialize the map locations and camp spots
	BotInitInfoEntities();

	//initialize the level item heap
	InitLevelItemHeap();
	levelitems = NULL;
	numlevelitems = 0;
	//
	ic = itemconfig;
	if ( !ic )
		return;

	//if there's no AAS file loaded
	if ( !AAS_Loaded() )
		return;

	//validate the modelindexes of the item info
	for ( i = 0; i < ic->numiteminfo; i++ ) {
		if ( !ic->iteminfo[i].modelindex ) {
			Log_Write( "item %s has modelindex 0\n", ic->iteminfo[i].classname );
		} //end if
	} //end for

	for ( ent = AAS_NextBSPEntity( 0 ); ent; ent = AAS_NextBSPEntity( ent ) ) {
		if ( !AAS_ValueForBSPEpairKey( ent, "classname", classname, MAX_EPAIRKEY ) )
			continue;
		//
		spawnflags = 0;
		AAS_IntForBSPEpairKey( ent, "spawnflags", &spawnflags );
		//
		for ( i = 0; i < ic->numiteminfo; i++ ) {
			if ( !strcmp( classname, ic->iteminfo[i].classname ) )
				break;
		} //end for
		if ( i >= ic->numiteminfo ) {
			Log_Write( "entity %s unknown item\r\n", classname );
			continue;
		} //end if
		//get the origin of the item
		if ( !AAS_VectorForBSPEpairKey( ent, "origin", origin ) ) {
			botimport.Print( PRT_ERROR, "item %s without origin\n", classname );
			continue;
		} //end else
		//
		goalareanum = 0;
		//if it is a floating item
		if ( spawnflags & 1 ) {
			//if the item is not floating in water
			if ( !( AAS_PointContents( origin ) & CONTENTS_WATER ) ) {
				VectorCopy( origin, end );
				end[2] -= 32;
				trace = AAS_Trace( origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs, end, ENTITYNUM_NONE, CONTENTS_SOLID | CONTENTS_PLAYERCLIP );
				//if the item is not near the ground
				if ( trace.fraction >= 1 ) {
					//if the item is not reachable from a jumppad
					goalareanum = AAS_BestReachableFromJumpPadArea( origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs );
					Log_Write( "item %s reachable from jumppad area %d\r\n", ic->iteminfo[i].classname, goalareanum );
					//botimport.Print(PRT_MESSAGE, "item %s reachable from jumppad area %d\r\n", ic->iteminfo[i].classname, goalareanum);
					if ( !goalareanum )
						continue;
				} //end if
			} //end if
		} //end if

		li = AllocLevelItem();
		if ( !li )
			return;
		//
		li->number = ++numlevelitems;
		li->timeout = 0;
		li->entitynum = 0;
		//
		li->flags = 0;
		AAS_IntForBSPEpairKey( ent, "notfree", &value );
		if ( value )
			li->flags |= IFL_NOTFREE;
		AAS_IntForBSPEpairKey( ent, "notteam", &value );
		if ( value )
			li->flags |= IFL_NOTTEAM;
		AAS_IntForBSPEpairKey( ent, "notsingle", &value );
		if ( value )
			li->flags |= IFL_NOTSINGLE;
		AAS_IntForBSPEpairKey( ent, "notbot", &value );
		if ( value )
			li->flags |= IFL_NOTBOT;
		if ( !strcmp( classname, "item_botroam" ) ) {
			li->flags |= IFL_ROAM;
			AAS_FloatForBSPEpairKey( ent, "weight", &li->weight );
		} //end if
		//if not a stationary item
		if ( !( spawnflags & 1 ) ) {
			if ( !AAS_DropToFloor( origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs ) ) {
				botimport.Print( PRT_MESSAGE, "%s in solid at (%1.1f %1.1f %1.1f)\n",
					classname, origin[0], origin[1], origin[2] );
			} //end if
		} //end if
		//item info of the level item
		li->iteminfo = i;
		//origin of the item
		VectorCopy( origin, li->origin );
		//
		if ( goalareanum ) {
			li->goalareanum = goalareanum;
			VectorCopy( origin, li->goalorigin );
		} //end if
		else {
			//get the item goal area and goal origin
			li->goalareanum = AAS_BestReachableArea( origin,
				ic->iteminfo[i].mins, ic->iteminfo[i].maxs,
				li->goalorigin );
			if ( !li->goalareanum ) {
				botimport.Print( PRT_MESSAGE, "%s not reachable for bots at (%1.1f %1.1f %1.1f)\n",
					classname, origin[0], origin[1], origin[2] );
			} //end if
		} //end else
		//
		AddLevelItemToList( li );
	} //end for
	botimport.Print( PRT_MESSAGE, "found %d level items\n", numlevelitems );
} //end of the function BotInitLevelItems
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotGoalName( int number, char *name, int size ) {
	levelitem_t *li;

	if ( !itemconfig )
		return;
	//
	for ( li = levelitems; li; li = li->next ) {
		if ( li->number == number ) {
			Q_strncpyz( name, itemconfig->iteminfo[li->iteminfo].name, size );
			return;
		} //end for
	} //end for
	strcpy( name, "" );
} //end of the function BotGoalName
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotResetAvoidGoals( int goalstate ) {
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return;
	Com_Memset( gs->avoidgoals, 0, MAX_AVOIDGOALS * sizeof( int ) );
	Com_Memset( gs->avoidgoaltimes, 0, MAX_AVOIDGOALS * sizeof( float ) );
} //end of the function BotResetAvoidGoals
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotDumpAvoidGoals( int goalstate ) {
	int i;
	bot_goalstate_t *gs;
	char name[32];

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return;
	for ( i = 0; i < MAX_AVOIDGOALS; i++ ) {
		if ( gs->avoidgoaltimes[i] >= AAS_Time() ) {
			BotGoalName( gs->avoidgoals[i], name, 32 );
			Log_Write( "avoid goal %s, number %d for %f seconds", name,
				gs->avoidgoals[i], gs->avoidgoaltimes[i] - AAS_Time() );
		} //end if
	} //end for
} //end of the function BotDumpAvoidGoals
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotAddToAvoidGoals( bot_goalstate_t *gs, int number, float avoidtime ) {
	int i;

	for ( i = 0; i < MAX_AVOIDGOALS; i++ ) {
		//if the avoid goal is already stored
		if ( gs->avoidgoals[i] == number ) {
			gs->avoidgoals[i] = number;
			gs->avoidgoaltimes[i] = AAS_Time() + avoidtime;
			return;
		} //end if
	} //end for

	for ( i = 0; i < MAX_AVOIDGOALS; i++ ) {
		//if this avoid goal has expired
		if ( gs->avoidgoaltimes[i] < AAS_Time() ) {
			gs->avoidgoals[i] = number;
			gs->avoidgoaltimes[i] = AAS_Time() + avoidtime;
			return;
		} //end if
	} //end for
} //end of the function BotAddToAvoidGoals
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotRemoveFromAvoidGoals( int goalstate, int number ) {
	int i;
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return;
	//don't use the goals the bot wants to avoid
	for ( i = 0; i < MAX_AVOIDGOALS; i++ ) {
		if ( gs->avoidgoals[i] == number && gs->avoidgoaltimes[i] >= AAS_Time() ) {
			gs->avoidgoaltimes[i] = 0;
			return;
		} //end if
	} //end for
} //end of the function BotRemoveFromAvoidGoals
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float BotAvoidGoalTime( int goalstate, int number ) {
	int i;
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return 0;
	//don't use the goals the bot wants to avoid
	for ( i = 0; i < MAX_AVOIDGOALS; i++ ) {
		if ( gs->avoidgoals[i] == number && gs->avoidgoaltimes[i] >= AAS_Time() ) {
			return gs->avoidgoaltimes[i] - AAS_Time();
		} //end if
	} //end for
	return 0;
} //end of the function BotAvoidGoalTime
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotSetAvoidGoalTime( int goalstate, int number, float avoidtime ) {
	bot_goalstate_t *gs;
	levelitem_t *li;

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return;
	if ( avoidtime < 0 ) {
		if ( !itemconfig )
			return;
		//
		for ( li = levelitems; li; li = li->next ) {
			if ( li->number == number ) {
				avoidtime = itemconfig->iteminfo[li->iteminfo].respawntime;
				if ( !avoidtime )
					avoidtime = AVOID_DEFAULT_TIME;
				if ( avoidtime < AVOID_MINIMUM_TIME )
					avoidtime = AVOID_MINIMUM_TIME;
				BotAddToAvoidGoals( gs, number, avoidtime );
				return;
			} //end for
		} //end for
		return;
	} //end if
	else {
		BotAddToAvoidGoals( gs, number, avoidtime );
	} //end else
} //end of the function BotSetAvoidGoalTime
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotGetLevelItemGoal( int index, const char *name, bot_goal_t *goal ) {
	levelitem_t *li;

	if ( !itemconfig )
		return -1;
	li = levelitems;
	if ( index >= 0 ) {
		for ( ; li; li = li->next ) {
			if ( li->number == index ) {
				li = li->next;
				break;
			} //end if
		} //end for
	} //end for
	for ( ; li; li = li->next ) {
		//
		if ( g_gametype == GT_SINGLE_PLAYER ) {
			if ( li->flags & IFL_NOTSINGLE )
				continue;
		} else if ( g_gametype >= GT_TEAM ) {
			if ( li->flags & IFL_NOTTEAM )
				continue;
		} else {
			if ( li->flags & IFL_NOTFREE )
				continue;
		}
		if ( li->flags & IFL_NOTBOT )
			continue;
		//
		if ( !Q_stricmp( name, itemconfig->iteminfo[li->iteminfo].name ) ) {
			goal->areanum = li->goalareanum;
			VectorCopy( li->goalorigin, goal->origin );
			goal->entitynum = li->entitynum;
			VectorCopy( itemconfig->iteminfo[li->iteminfo].mins, goal->mins );
			VectorCopy( itemconfig->iteminfo[li->iteminfo].maxs, goal->maxs );
			goal->number = li->number;
			goal->flags = GFL_ITEM;
			if ( li->timeout )
				goal->flags |= GFL_DROPPED;
			goal->iteminfo = li->iteminfo;
			//botimport.Print(PRT_MESSAGE, "found li %s\n", itemconfig->iteminfo[li->iteminfo].name);
			return li->number;
		} //end if
	} //end for
	return -1;
} //end of the function BotGetLevelItemGoal
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotGetMapLocationGoal( const char *name, bot_goal_t *goal ) {
	maplocation_t *ml;
	vec3_t mins = { -8, -8, -8 }, maxs = { 8, 8, 8 };

	for ( ml = maplocations; ml; ml = ml->next ) {
		if ( !Q_stricmp( ml->name, name ) ) {
			goal->areanum = ml->areanum;
			VectorCopy( ml->origin, goal->origin );
			goal->entitynum = 0;
			VectorCopy( mins, goal->mins );
			VectorCopy( maxs, goal->maxs );
			goal->number = 0;
			goal->flags = 0;
			goal->iteminfo = 0;
			return qtrue;
		} //end if
	} //end for
	return qfalse;
} //end of the function BotGetMapLocationGoal
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotGetNextCampSpotGoal( int num, bot_goal_t *goal ) {
	int i;
	campspot_t *cs;
	vec3_t mins = { -8, -8, -8 }, maxs = { 8, 8, 8 };

	if ( num < 0 )
		num = 0;
	i = num;
	for ( cs = campspots; cs; cs = cs->next ) {
		if ( --i < 0 ) {
			goal->areanum = cs->areanum;
			VectorCopy( cs->origin, goal->origin );
			goal->entitynum = 0;
			VectorCopy( mins, goal->mins );
			VectorCopy( maxs, goal->maxs );
			goal->number = 0;
			goal->flags = 0;
			goal->iteminfo = 0;
			return num + 1;
		} //end if
	} //end for
	return 0;
} //end of the function BotGetNextCampSpotGoal
#if 0
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
static void BotFindEntityForLevelItem(levelitem_t *li)
{
	int ent, modelindex;
	itemconfig_t *ic;
	aas_entityinfo_t entinfo;
	vec3_t dir;

	ic = itemconfig;
	if (!itemconfig) return;
	for (ent = AAS_NextEntity(0); ent; ent = AAS_NextEntity(ent))
	{
		//get the model index of the entity
		modelindex = AAS_EntityModelindex(ent);
		//
		if (!modelindex) continue;
		//get info about the entity
		AAS_EntityInfo(ent, &entinfo);
		//if the entity is still moving
		if (entinfo.origin[0] != entinfo.lastvisorigin[0] ||
				entinfo.origin[1] != entinfo.lastvisorigin[1] ||
				entinfo.origin[2] != entinfo.lastvisorigin[2]) continue;
		//
		if (ic->iteminfo[li->iteminfo].modelindex == modelindex)
		{
			//check if the entity is very close
			VectorSubtract(li->origin, entinfo.origin, dir);
			if (VectorLength(dir) < 30)
			{
				//found an entity for this level item
				li->entitynum = ent;
			} //end if
		} //end if
	} //end for
} //end of the function BotFindEntityForLevelItem
#endif
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================

//NOTE: enum entityType_t in bg_public.h
#define ET_ITEM			2

void BotUpdateEntityItems( void ) {
	int ent, i, modelindex;
	vec3_t dir;
	levelitem_t *li, *nextli;
	aas_entityinfo_t entinfo;
	itemconfig_t *ic;

	//timeout current entity items if necessary
	for ( li = levelitems; li; li = nextli ) {
		nextli = li->next;
		//if it is an item that will time out
		if ( li->timeout ) {
			//timeout the item
			if ( li->timeout < AAS_Time() ) {
				RemoveLevelItemFromList( li );
				FreeLevelItem( li );
			} //end if
		} //end if
	} //end for
	//find new entity items
	ic = itemconfig;
	if ( !itemconfig )
		return;
	//
	for ( ent = AAS_NextEntity( 0 ); ent; ent = AAS_NextEntity( ent ) ) {
		if ( AAS_EntityType( ent ) != ET_ITEM )
			continue;
		//get the model index of the entity
		modelindex = AAS_EntityModelindex( ent );
		//
		if ( !modelindex )
			continue;
		//get info about the entity
		AAS_EntityInfo( ent, &entinfo );
		//FIXME: don't do this
		//skip all floating items for now
		//if (entinfo.groundent != ENTITYNUM_WORLD) continue;
		//if the entity is still moving
		if ( entinfo.origin[0] != entinfo.lastvisorigin[0] ||
			 entinfo.origin[1] != entinfo.lastvisorigin[1] ||
			 entinfo.origin[2] != entinfo.lastvisorigin[2] )
			continue;
		//check if the entity is already stored as a level item
		for ( li = levelitems; li; li = li->next ) {
			//if the level item is linked to an entity
			if ( li->entitynum && li->entitynum == ent ) {
				//the entity is re-used if the models are different
				if ( ic->iteminfo[li->iteminfo].modelindex != modelindex ) {
					//remove this level item
					RemoveLevelItemFromList( li );
					FreeLevelItem( li );
					li = NULL;
					break;
				} //end if
				else {
					if ( entinfo.origin[0] != li->origin[0] ||
						 entinfo.origin[1] != li->origin[1] ||
						 entinfo.origin[2] != li->origin[2] ) {
						VectorCopy( entinfo.origin, li->origin );
						//also update the goal area number
						li->goalareanum = AAS_BestReachableArea( li->origin,
							ic->iteminfo[li->iteminfo].mins, ic->iteminfo[li->iteminfo].maxs,
							li->goalorigin );
					} //end if
					break;
				} //end else
			} //end if
		} //end for
		if ( li )
			continue;
		//try to link the entity to a level item
		for ( li = levelitems; li; li = li->next ) {
			//if this level item is already linked
			if ( li->entitynum )
				continue;
			//
			if ( g_gametype == GT_SINGLE_PLAYER ) {
				if ( li->flags & IFL_NOTSINGLE )
					continue;
			} else if ( g_gametype >= GT_TEAM ) {
				if ( li->flags & IFL_NOTTEAM )
					continue;
			} else {
				if ( li->flags & IFL_NOTFREE )
					continue;
			}
			//if the model of the level item and the entity are the same
			if ( ic->iteminfo[li->iteminfo].modelindex == modelindex ) {
				//check if the entity is very close
				VectorSubtract( li->origin, entinfo.origin, dir );
				if ( VectorLength( dir ) < 30 ) {
					//found an entity for this level item
					li->entitynum = ent;
					//if the origin is different
					if ( entinfo.origin[0] != li->origin[0] ||
						 entinfo.origin[1] != li->origin[1] ||
						 entinfo.origin[2] != li->origin[2] ) {
						//update the level item origin
						VectorCopy( entinfo.origin, li->origin );
						//also update the goal area number
						li->goalareanum = AAS_BestReachableArea( li->origin,
							ic->iteminfo[li->iteminfo].mins, ic->iteminfo[li->iteminfo].maxs,
							li->goalorigin );
					} //end if
#ifdef DEBUG
					Log_Write( "linked item %s to an entity", ic->iteminfo[li->iteminfo].classname );
#endif //DEBUG
					break;
				} //end if
			} //end else
		} //end for
		if ( li )
			continue;
		//check if the model is from a known item
		for ( i = 0; i < ic->numiteminfo; i++ ) {
			if ( ic->iteminfo[i].modelindex == modelindex ) {
				break;
			} //end if
		} //end for
		//if the model is not from a known item
		if ( i >= ic->numiteminfo )
			continue;
		//allocate a new level item
		li = AllocLevelItem();
		//
		if ( !li )
			continue;
		//entity number of the level item
		li->entitynum = ent;
		//number for the level item
		li->number = numlevelitems + ent;
		//set the item info index for the level item
		li->iteminfo = i;
		//origin of the item
		VectorCopy( entinfo.origin, li->origin );
		//get the item goal area and goal origin
		li->goalareanum = AAS_BestReachableArea( li->origin,
			ic->iteminfo[i].mins, ic->iteminfo[i].maxs,
			li->goalorigin );
		//never go for items dropped into jumppads
		if ( AAS_AreaJumpPad( li->goalareanum ) ) {
			FreeLevelItem( li );
			continue;
		} //end if
		//time this item out after 30 seconds
		//dropped items disappear after 30 seconds
		li->timeout = AAS_Time() + 30;
		//add the level item to the list
		AddLevelItemToList( li );
		//botimport.Print(PRT_MESSAGE, "found new level item %s\n", ic->iteminfo[i].classname);
	} //end for
	/*
	for (li = levelitems; li; li = li->next)
	{
		if (!li->entitynum)
		{
			BotFindEntityForLevelItem(li);
		} //end if
	} //end for*/
} //end of the function BotUpdateEntityItems
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotDumpGoalStack( int goalstate ) {
	int i;
	bot_goalstate_t *gs;
	char name[32];

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return;
	for ( i = 1; i <= gs->goalstacktop; i++ ) {
		BotGoalName( gs->goalstack[i].number, name, 32 );
		Log_Write( "%d: %s", i, name );
	} //end for
} //end of the function BotDumpGoalStack
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotPushGoal( int goalstate, bot_goal_t *goal ) {
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return;
	if ( gs->goalstacktop >= MAX_GOALSTACK - 1 ) {
		botimport.Print( PRT_ERROR, "goal heap overflow\n" );
		BotDumpGoalStack( goalstate );
		return;
	} //end if
	gs->goalstacktop++;
	Com_Memcpy( &gs->goalstack[gs->goalstacktop], goal, sizeof( bot_goal_t ) );
} //end of the function BotPushGoal
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotPopGoal( int goalstate ) {
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return;
	if ( gs->goalstacktop > 0 )
		gs->goalstacktop--;
} //end of the function BotPopGoal
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotEmptyGoalStack( int goalstate ) {
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return;
	gs->goalstacktop = 0;
} //end of the function BotEmptyGoalStack
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotGetTopGoal( int goalstate, bot_goal_t *goal ) {
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return qfalse;
	if ( !gs->goalstacktop )
		return qfalse;
	Com_Memcpy( goal, &gs->goalstack[gs->goalstacktop], sizeof( bot_goal_t ) );
	return qtrue;
} //end of the function BotGetTopGoal
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotGetSecondGoal( int goalstate, bot_goal_t *goal ) {
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return qfalse;
	if ( gs->goalstacktop <= 1 )
		return qfalse;
	Com_Memcpy( goal, &gs->goalstack[gs->goalstacktop - 1], sizeof( bot_goal_t ) );
	return qtrue;
} //end of the function BotGetSecondGoal
//===========================================================================
// pops a new long term goal on the goal stack in the goalstate
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotChooseLTGItem( int goalstate, vec3_t origin, int *inventory, int travelflags ) {
	int areanum, t, weightnum;
	float weight, bestweight, avoidtime;
	iteminfo_t *iteminfo;
	itemconfig_t *ic;
	levelitem_t *li, *bestitem;
	bot_goal_t goal;
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return qfalse;
	if ( !gs->itemweightconfig )
		return qfalse;
	//get the area the bot is in
	areanum = BotReachabilityArea( origin, gs->client );
	//if the bot is in solid or if the area the bot is in has no reachability links
	if ( !areanum || !AAS_AreaReachability( areanum ) ) {
		//use the last valid area the bot was in
		areanum = gs->lastreachabilityarea;
	} //end if
	//remember the last area with reachabilities the bot was in
	gs->lastreachabilityarea = areanum;
	//if still in solid
	if ( !areanum )
		return qfalse;
	//the item configuration
	ic = itemconfig;
	if ( !itemconfig )
		return qfalse;
	//best weight and item so far
	bestweight = 0;
	bestitem = NULL;
	Com_Memset( &goal, 0, sizeof( bot_goal_t ) );
	//go through the items in the level
	for ( li = levelitems; li; li = li->next ) {
		if ( g_gametype == GT_SINGLE_PLAYER ) {
			if ( li->flags & IFL_NOTSINGLE )
				continue;
		} else if ( g_gametype >= GT_TEAM ) {
			if ( li->flags & IFL_NOTTEAM )
				continue;
		} else {
			if ( li->flags & IFL_NOTFREE )
				continue;
		}
		if ( li->flags & IFL_NOTBOT )
			continue;
		//if the item is not in a possible goal area
		if ( !li->goalareanum )
			continue;
		//FIXME: is this a good thing? added this for items that never spawned into the game (f.i. CTF flags in obelisk)
		if ( !li->entitynum && !( li->flags & IFL_ROAM ) )
			continue;
		//get the fuzzy weight function for this item
		iteminfo = &ic->iteminfo[li->iteminfo];
		weightnum = gs->itemweightindex[iteminfo->number];
		if ( weightnum < 0 )
			continue;

#ifdef UNDECIDEDFUZZY
		weight = FuzzyWeightUndecided( inventory, gs->itemweightconfig, weightnum );
#else
		weight = FuzzyWeight( inventory, gs->itemweightconfig, weightnum );
#endif //UNDECIDEDFUZZY
#ifdef DROPPEDWEIGHT
		//HACK: to make dropped items more attractive
		if ( li->timeout )
			weight += droppedweight->value;
#endif //DROPPEDWEIGHT
		//use weight scale for item_botroam
		if ( li->flags & IFL_ROAM )
			weight *= li->weight;
		//
		if ( weight > 0 ) {
			//get the travel time towards the goal area
			t = AAS_AreaTravelTimeToGoalArea( areanum, origin, li->goalareanum, travelflags );
			//if the goal is reachable
			if ( t > 0 ) {
				//if this item won't respawn before we get there
				avoidtime = BotAvoidGoalTime( goalstate, li->number );
				if ( avoidtime - t * 0.009 > 0 )
					continue;
				//
				{
					double expressionValue = (float)t * TRAVELTIME_SCALE;
					weight = (float)( weight / expressionValue );
				}
				//
				if ( weight > bestweight ) {
					bestweight = weight;
					bestitem = li;
				} //end if
			} //end if
		} //end if
	} //end for
	//if no goal item found
	if ( !bestitem ) {
		/*
		//if not in lava or slime
		if (!AAS_AreaLava(areanum) && !AAS_AreaSlime(areanum))
		{
			if (AAS_RandomGoalArea(areanum, travelflags, &goal.areanum, goal.origin))
			{
				VectorSet(goal.mins, -15, -15, -15);
				VectorSet(goal.maxs, 15, 15, 15);
				goal.entitynum = 0;
				goal.number = 0;
				goal.flags = GFL_ROAM;
				goal.iteminfo = 0;
				//push the goal on the stack
				BotPushGoal(goalstate, &goal);
				//
#ifdef DEBUG
				botimport.Print(PRT_MESSAGE, "chosen roam goal area %d\n", goal.areanum);
#endif //DEBUG
				return qtrue;
			} //end if
		} //end if
		*/
		return qfalse;
	} //end if
	//create a bot goal for this item
	iteminfo = &ic->iteminfo[bestitem->iteminfo];
	VectorCopy( bestitem->goalorigin, goal.origin );
	VectorCopy( iteminfo->mins, goal.mins );
	VectorCopy( iteminfo->maxs, goal.maxs );
	goal.areanum = bestitem->goalareanum;
	goal.entitynum = bestitem->entitynum;
	goal.number = bestitem->number;
	goal.flags = GFL_ITEM;
	if ( bestitem->timeout )
		goal.flags |= GFL_DROPPED;
	if ( bestitem->flags & IFL_ROAM )
		goal.flags |= GFL_ROAM;
	goal.iteminfo = bestitem->iteminfo;
	//if it's a dropped item
	if ( bestitem->timeout ) {
		avoidtime = AVOID_DROPPED_TIME;
	} //end if
	else {
		avoidtime = iteminfo->respawntime;
		if ( !avoidtime )
			avoidtime = AVOID_DEFAULT_TIME;
		if ( avoidtime < AVOID_MINIMUM_TIME )
			avoidtime = AVOID_MINIMUM_TIME;
	} //end else
	//add the chosen goal to the goals to avoid for a while
	BotAddToAvoidGoals( gs, bestitem->number, avoidtime );
	//push the goal on the stack
	BotPushGoal( goalstate, &goal );
	//
	return qtrue;
} //end of the function BotChooseLTGItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotChooseNBGItem( int goalstate, vec3_t origin, int *inventory, int travelflags,
	bot_goal_t *ltg, float maxtime ) {
	int areanum, t, weightnum, ltg_time;
	float weight, bestweight, avoidtime;
	iteminfo_t *iteminfo;
	itemconfig_t *ic;
	levelitem_t *li, *bestitem;
	bot_goal_t goal;
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return qfalse;
	if ( !gs->itemweightconfig )
		return qfalse;
	//get the area the bot is in
	areanum = BotReachabilityArea( origin, gs->client );
	//if the bot is in solid or if the area the bot is in has no reachability links
	if ( !areanum || !AAS_AreaReachability( areanum ) ) {
		//use the last valid area the bot was in
		areanum = gs->lastreachabilityarea;
	} //end if
	//remember the last area with reachabilities the bot was in
	gs->lastreachabilityarea = areanum;
	//if still in solid
	if ( !areanum )
		return qfalse;
	//
	if ( ltg )
		ltg_time = AAS_AreaTravelTimeToGoalArea( areanum, origin, ltg->areanum, travelflags );
	else
		ltg_time = 99999;
	//the item configuration
	ic = itemconfig;
	if ( !itemconfig )
		return qfalse;
	//best weight and item so far
	bestweight = 0;
	bestitem = NULL;
	Com_Memset( &goal, 0, sizeof( bot_goal_t ) );
	//go through the items in the level
	for ( li = levelitems; li; li = li->next ) {
		if ( g_gametype == GT_SINGLE_PLAYER ) {
			if ( li->flags & IFL_NOTSINGLE )
				continue;
		} else if ( g_gametype >= GT_TEAM ) {
			if ( li->flags & IFL_NOTTEAM )
				continue;
		} else {
			if ( li->flags & IFL_NOTFREE )
				continue;
		}
		if ( li->flags & IFL_NOTBOT )
			continue;
		//if the item is in a possible goal area
		if ( !li->goalareanum )
			continue;
		//FIXME: is this a good thing? added this for items that never spawned into the game (f.i. CTF flags in obelisk)
		if ( !li->entitynum && !( li->flags & IFL_ROAM ) )
			continue;
		//get the fuzzy weight function for this item
		iteminfo = &ic->iteminfo[li->iteminfo];
		weightnum = gs->itemweightindex[iteminfo->number];
		if ( weightnum < 0 )
			continue;
		//
#ifdef UNDECIDEDFUZZY
		weight = FuzzyWeightUndecided( inventory, gs->itemweightconfig, weightnum );
#else
		weight = FuzzyWeight( inventory, gs->itemweightconfig, weightnum );
#endif //UNDECIDEDFUZZY
#ifdef DROPPEDWEIGHT
		//HACK: to make dropped items more attractive
		if ( li->timeout )
			weight += droppedweight->value;
#endif //DROPPEDWEIGHT
		//use weight scale for item_botroam
		if ( li->flags & IFL_ROAM )
			weight *= li->weight;
		//
		if ( weight > 0 ) {
			//get the travel time towards the goal area
			t = AAS_AreaTravelTimeToGoalArea( areanum, origin, li->goalareanum, travelflags );
			//if the goal is reachable
			if ( t > 0 && t < maxtime ) {
				//if this item won't respawn before we get there
				avoidtime = BotAvoidGoalTime( goalstate, li->number );
				if ( avoidtime - t * 0.009 > 0 )
					continue;
				//
				{
					double expressionValue = (float)t * TRAVELTIME_SCALE;
					weight = (float)( weight / expressionValue );
				}
				//
				if ( weight > bestweight ) {
					t = 0;
					if ( ltg && !li->timeout ) {
						//get the travel time from the goal to the long term goal
						t = AAS_AreaTravelTimeToGoalArea( li->goalareanum, li->goalorigin, ltg->areanum, travelflags );
					} //end if
					//if the travel back is possible and doesn't take too long
					if ( t <= ltg_time ) {
						bestweight = weight;
						bestitem = li;
					} //end if
				} //end if
			} //end if
		} //end if
	} //end for
	//if no goal item found
	if ( !bestitem )
		return qfalse;
	//create a bot goal for this item
	iteminfo = &ic->iteminfo[bestitem->iteminfo];
	VectorCopy( bestitem->goalorigin, goal.origin );
	VectorCopy( iteminfo->mins, goal.mins );
	VectorCopy( iteminfo->maxs, goal.maxs );
	goal.areanum = bestitem->goalareanum;
	goal.entitynum = bestitem->entitynum;
	goal.number = bestitem->number;
	goal.flags = GFL_ITEM;
	if ( bestitem->timeout )
		goal.flags |= GFL_DROPPED;
	if ( bestitem->flags & IFL_ROAM )
		goal.flags |= GFL_ROAM;
	goal.iteminfo = bestitem->iteminfo;
	//if it's a dropped item
	if ( bestitem->timeout ) {
		avoidtime = AVOID_DROPPED_TIME;
	} //end if
	else {
		avoidtime = iteminfo->respawntime;
		if ( !avoidtime )
			avoidtime = AVOID_DEFAULT_TIME;
		if ( avoidtime < AVOID_MINIMUM_TIME )
			avoidtime = AVOID_MINIMUM_TIME;
	} //end else
	//add the chosen goal to the goals to avoid for a while
	BotAddToAvoidGoals( gs, bestitem->number, avoidtime );
	//push the goal on the stack
	BotPushGoal( goalstate, &goal );
	//
	return qtrue;
} //end of the function BotChooseNBGItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotTouchingGoal( const vec3_t origin, const bot_goal_t *goal ) {
	int i;
	vec3_t boxmins, boxmaxs;
	vec3_t absmins, absmaxs;
	const vec3_t safety_maxs = { 0, 0, 0 }; //{4, 4, 10};
	const vec3_t safety_mins = { 0, 0, 0 }; //{-4, -4, 0};

	AAS_PresenceTypeBoundingBox( PRESENCE_NORMAL, boxmins, boxmaxs );
	VectorSubtract( goal->mins, boxmaxs, absmins );
	VectorSubtract( goal->maxs, boxmins, absmaxs );
	VectorAdd( absmins, goal->origin, absmins );
	VectorAdd( absmaxs, goal->origin, absmaxs );
	//make the box a little smaller for safety
	VectorSubtract( absmaxs, safety_maxs, absmaxs );
	VectorSubtract( absmins, safety_mins, absmins );

	for ( i = 0; i < 3; i++ ) {
		if ( origin[i] < absmins[i] || origin[i] > absmaxs[i] )
			return qfalse;
	} //end for
	return qtrue;
} //end of the function BotTouchingGoal
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotItemGoalInVisButNotVisible( int viewer, vec3_t eye, vec3_t viewangles [[maybe_unused]], bot_goal_t *goal ) {
	aas_entityinfo_t entinfo;
	bsp_trace_t trace;
	vec3_t middle;

	if ( !( goal->flags & GFL_ITEM ) )
		return qfalse;
	//
	VectorAdd( goal->mins, goal->mins, middle );
	( ( middle )[0] = (float)( ( middle )[0] * ( 0.5 ) ), ( middle )[1] = (float)( ( middle )[1] * ( 0.5 ) ), ( middle )[2] = (float)( ( middle )[2] * ( 0.5 ) ) );
	VectorAdd( goal->origin, middle, middle );
	//
	trace = AAS_Trace( eye, NULL, NULL, middle, viewer, CONTENTS_SOLID );
	//if the goal middle point is visible
	if ( trace.fraction >= 1 ) {
		//the goal entity number doesn't have to be valid
		//just assume it's valid
		if ( goal->entitynum <= 0 )
			return qfalse;
		//
		//if the entity data isn't valid
		AAS_EntityInfo( goal->entitynum, &entinfo );
		//NOTE: for some wacko reason entities are sometimes
		// not updated
		//if (!entinfo.valid) return qtrue;
		if ( entinfo.ltime < AAS_Time() - 0.5 )
			return qtrue;
	} //end if
	return qfalse;
} //end of the function BotItemGoalInVisButNotVisible
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotResetGoalState( int goalstate ) {
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return;
	Com_Memset( gs->goalstack, 0, MAX_GOALSTACK * sizeof( bot_goal_t ) );
	gs->goalstacktop = 0;
	BotResetAvoidGoals( goalstate );
} //end of the function BotResetGoalState
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotLoadItemWeights( int goalstate, const char *filename ) {
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return BLERR_CANNOTLOADITEMWEIGHTS;
	//load the weight configuration
	gs->itemweightconfig = ReadWeightConfig( filename );
	if ( !gs->itemweightconfig ) {
		botimport.Print( PRT_FATAL, "couldn't load weights\n" );
		return BLERR_CANNOTLOADITEMWEIGHTS;
	} //end if
	//if there's no item configuration
	if ( !itemconfig )
		return BLERR_CANNOTLOADITEMWEIGHTS;
	//create the item weight index
	gs->itemweightindex = ItemWeightIndex( gs->itemweightconfig, itemconfig );
	//everything went ok
	return BLERR_NOERROR;
} //end of the function BotLoadItemWeights
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotFreeItemWeights( int goalstate ) {
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle( goalstate );
	if ( !gs )
		return;
	if ( gs->itemweightconfig )
		FreeWeightConfig( gs->itemweightconfig );
	if ( gs->itemweightindex )
		FreeMemory( gs->itemweightindex );
} //end of the function BotFreeItemWeights
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotAllocGoalState( int client ) {
	int i;

	for ( i = 1; i <= MAX_CLIENTS; i++ ) {
		if ( !botgoalstates[i] ) {
			botgoalstates[i] = (bot_goalstate_t *)GetClearedMemory(sizeof(bot_goalstate_t));
			botgoalstates[i]->client = client;
			return i;
		} //end if
	} //end for
	return 0;
} //end of the function BotAllocGoalState
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
void BotFreeGoalState( int handle ) {
	if ( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "goal state handle %d out of range\n", handle );
		return;
	} //end if
	if ( !botgoalstates[handle] ) {
		botimport.Print( PRT_FATAL, "invalid goal state handle %d\n", handle );
		return;
	} //end if
	BotFreeItemWeights( handle );
	FreeMemory( botgoalstates[handle] );
	botgoalstates[handle] = NULL;
} //end of the function BotFreeGoalState
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotSetupGoalAI( void ) {
	const char *filename;

	//check if teamplay is on
	g_gametype = (int)( LibVarValue( "g_gametype", "0" ) );
	//item configuration file
	filename = LibVarString( "itemconfig", "items.c" );
	//load the item configuration
	itemconfig = LoadItemConfig( filename );
	if ( !itemconfig ) {
		botimport.Print( PRT_FATAL, "couldn't load item config\n" );
		return BLERR_CANNOTLOADITEMCONFIG;
	} //end if
	//
	droppedweight = LibVar( "droppedweight", "1000" );
	//everything went ok
	return BLERR_NOERROR;
} //end of the function BotSetupGoalAI
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotShutdownGoalAI( void ) {
	int i;

	if ( itemconfig )
		FreeMemory( itemconfig );
	itemconfig = NULL;
	if ( levelitemheap )
		FreeMemory( levelitemheap );
	levelitemheap = NULL;
	allocatedLevelItems = 0;
	freelevelitems = NULL;
	levelitems = NULL;
	numlevelitems = 0;

	BotFreeInfoEntities();

	for ( i = 1; i <= MAX_CLIENTS; i++ ) {
		if ( botgoalstates[i] ) {
			BotFreeGoalState( i );
		} //end if
	} //end for
} //end of the function BotShutdownGoalAI

static constexpr stateField_t goalStateFields[] = {
	{ "client", offsetof( bot_goalstate_t, client ), 1, stateType_t::Int32 },
	{ "lastreachabilityarea", offsetof( bot_goalstate_t, lastreachabilityarea ), 1, stateType_t::Int32 },
	{ "goalstacktop", offsetof( bot_goalstate_t, goalstacktop ), 1, stateType_t::Int32 },
	{ "avoidgoals", offsetof( bot_goalstate_t, avoidgoals ), MAX_AVOIDGOALS, stateType_t::Int32 },
	{ "avoidgoaltimes", offsetof( bot_goalstate_t, avoidgoaltimes ), MAX_AVOIDGOALS, stateType_t::Float32 },
	{ "goalstack[0].origin", offsetof( bot_goalstate_t, goalstack[0].origin ), 3, stateType_t::Float32 },
	{ "goalstack[0].areanum", offsetof( bot_goalstate_t, goalstack[0].areanum ), 1, stateType_t::Int32 },
	{ "goalstack[0].mins", offsetof( bot_goalstate_t, goalstack[0].mins ), 3, stateType_t::Float32 },
	{ "goalstack[0].maxs", offsetof( bot_goalstate_t, goalstack[0].maxs ), 3, stateType_t::Float32 },
	{ "goalstack[0].entitynum", offsetof( bot_goalstate_t, goalstack[0].entitynum ), 1, stateType_t::Int32 },
	{ "goalstack[0].number", offsetof( bot_goalstate_t, goalstack[0].number ), 1, stateType_t::Int32 },
	{ "goalstack[0].flags", offsetof( bot_goalstate_t, goalstack[0].flags ), 1, stateType_t::Int32 },
	{ "goalstack[0].iteminfo", offsetof( bot_goalstate_t, goalstack[0].iteminfo ), 1, stateType_t::Int32 },
	{ "goalstack[1].origin", offsetof( bot_goalstate_t, goalstack[1].origin ), 3, stateType_t::Float32 },
	{ "goalstack[1].areanum", offsetof( bot_goalstate_t, goalstack[1].areanum ), 1, stateType_t::Int32 },
	{ "goalstack[1].mins", offsetof( bot_goalstate_t, goalstack[1].mins ), 3, stateType_t::Float32 },
	{ "goalstack[1].maxs", offsetof( bot_goalstate_t, goalstack[1].maxs ), 3, stateType_t::Float32 },
	{ "goalstack[1].entitynum", offsetof( bot_goalstate_t, goalstack[1].entitynum ), 1, stateType_t::Int32 },
	{ "goalstack[1].number", offsetof( bot_goalstate_t, goalstack[1].number ), 1, stateType_t::Int32 },
	{ "goalstack[1].flags", offsetof( bot_goalstate_t, goalstack[1].flags ), 1, stateType_t::Int32 },
	{ "goalstack[1].iteminfo", offsetof( bot_goalstate_t, goalstack[1].iteminfo ), 1, stateType_t::Int32 },
	{ "goalstack[2].origin", offsetof( bot_goalstate_t, goalstack[2].origin ), 3, stateType_t::Float32 },
	{ "goalstack[2].areanum", offsetof( bot_goalstate_t, goalstack[2].areanum ), 1, stateType_t::Int32 },
	{ "goalstack[2].mins", offsetof( bot_goalstate_t, goalstack[2].mins ), 3, stateType_t::Float32 },
	{ "goalstack[2].maxs", offsetof( bot_goalstate_t, goalstack[2].maxs ), 3, stateType_t::Float32 },
	{ "goalstack[2].entitynum", offsetof( bot_goalstate_t, goalstack[2].entitynum ), 1, stateType_t::Int32 },
	{ "goalstack[2].number", offsetof( bot_goalstate_t, goalstack[2].number ), 1, stateType_t::Int32 },
	{ "goalstack[2].flags", offsetof( bot_goalstate_t, goalstack[2].flags ), 1, stateType_t::Int32 },
	{ "goalstack[2].iteminfo", offsetof( bot_goalstate_t, goalstack[2].iteminfo ), 1, stateType_t::Int32 },
	{ "goalstack[3].origin", offsetof( bot_goalstate_t, goalstack[3].origin ), 3, stateType_t::Float32 },
	{ "goalstack[3].areanum", offsetof( bot_goalstate_t, goalstack[3].areanum ), 1, stateType_t::Int32 },
	{ "goalstack[3].mins", offsetof( bot_goalstate_t, goalstack[3].mins ), 3, stateType_t::Float32 },
	{ "goalstack[3].maxs", offsetof( bot_goalstate_t, goalstack[3].maxs ), 3, stateType_t::Float32 },
	{ "goalstack[3].entitynum", offsetof( bot_goalstate_t, goalstack[3].entitynum ), 1, stateType_t::Int32 },
	{ "goalstack[3].number", offsetof( bot_goalstate_t, goalstack[3].number ), 1, stateType_t::Int32 },
	{ "goalstack[3].flags", offsetof( bot_goalstate_t, goalstack[3].flags ), 1, stateType_t::Int32 },
	{ "goalstack[3].iteminfo", offsetof( bot_goalstate_t, goalstack[3].iteminfo ), 1, stateType_t::Int32 },
	{ "goalstack[4].origin", offsetof( bot_goalstate_t, goalstack[4].origin ), 3, stateType_t::Float32 },
	{ "goalstack[4].areanum", offsetof( bot_goalstate_t, goalstack[4].areanum ), 1, stateType_t::Int32 },
	{ "goalstack[4].mins", offsetof( bot_goalstate_t, goalstack[4].mins ), 3, stateType_t::Float32 },
	{ "goalstack[4].maxs", offsetof( bot_goalstate_t, goalstack[4].maxs ), 3, stateType_t::Float32 },
	{ "goalstack[4].entitynum", offsetof( bot_goalstate_t, goalstack[4].entitynum ), 1, stateType_t::Int32 },
	{ "goalstack[4].number", offsetof( bot_goalstate_t, goalstack[4].number ), 1, stateType_t::Int32 },
	{ "goalstack[4].flags", offsetof( bot_goalstate_t, goalstack[4].flags ), 1, stateType_t::Int32 },
	{ "goalstack[4].iteminfo", offsetof( bot_goalstate_t, goalstack[4].iteminfo ), 1, stateType_t::Int32 },
	{ "goalstack[5].origin", offsetof( bot_goalstate_t, goalstack[5].origin ), 3, stateType_t::Float32 },
	{ "goalstack[5].areanum", offsetof( bot_goalstate_t, goalstack[5].areanum ), 1, stateType_t::Int32 },
	{ "goalstack[5].mins", offsetof( bot_goalstate_t, goalstack[5].mins ), 3, stateType_t::Float32 },
	{ "goalstack[5].maxs", offsetof( bot_goalstate_t, goalstack[5].maxs ), 3, stateType_t::Float32 },
	{ "goalstack[5].entitynum", offsetof( bot_goalstate_t, goalstack[5].entitynum ), 1, stateType_t::Int32 },
	{ "goalstack[5].number", offsetof( bot_goalstate_t, goalstack[5].number ), 1, stateType_t::Int32 },
	{ "goalstack[5].flags", offsetof( bot_goalstate_t, goalstack[5].flags ), 1, stateType_t::Int32 },
	{ "goalstack[5].iteminfo", offsetof( bot_goalstate_t, goalstack[5].iteminfo ), 1, stateType_t::Int32 },
	{ "goalstack[6].origin", offsetof( bot_goalstate_t, goalstack[6].origin ), 3, stateType_t::Float32 },
	{ "goalstack[6].areanum", offsetof( bot_goalstate_t, goalstack[6].areanum ), 1, stateType_t::Int32 },
	{ "goalstack[6].mins", offsetof( bot_goalstate_t, goalstack[6].mins ), 3, stateType_t::Float32 },
	{ "goalstack[6].maxs", offsetof( bot_goalstate_t, goalstack[6].maxs ), 3, stateType_t::Float32 },
	{ "goalstack[6].entitynum", offsetof( bot_goalstate_t, goalstack[6].entitynum ), 1, stateType_t::Int32 },
	{ "goalstack[6].number", offsetof( bot_goalstate_t, goalstack[6].number ), 1, stateType_t::Int32 },
	{ "goalstack[6].flags", offsetof( bot_goalstate_t, goalstack[6].flags ), 1, stateType_t::Int32 },
	{ "goalstack[6].iteminfo", offsetof( bot_goalstate_t, goalstack[6].iteminfo ), 1, stateType_t::Int32 },
	{ "goalstack[7].origin", offsetof( bot_goalstate_t, goalstack[7].origin ), 3, stateType_t::Float32 },
	{ "goalstack[7].areanum", offsetof( bot_goalstate_t, goalstack[7].areanum ), 1, stateType_t::Int32 },
	{ "goalstack[7].mins", offsetof( bot_goalstate_t, goalstack[7].mins ), 3, stateType_t::Float32 },
	{ "goalstack[7].maxs", offsetof( bot_goalstate_t, goalstack[7].maxs ), 3, stateType_t::Float32 },
	{ "goalstack[7].entitynum", offsetof( bot_goalstate_t, goalstack[7].entitynum ), 1, stateType_t::Int32 },
	{ "goalstack[7].number", offsetof( bot_goalstate_t, goalstack[7].number ), 1, stateType_t::Int32 },
	{ "goalstack[7].flags", offsetof( bot_goalstate_t, goalstack[7].flags ), 1, stateType_t::Int32 },
	{ "goalstack[7].iteminfo", offsetof( bot_goalstate_t, goalstack[7].iteminfo ), 1, stateType_t::Int32 },
};
static constexpr stateSchema_t goalStateSchema = { "botlib.goal", 1, 1, sizeof( bot_goalstate_t ), goalStateFields, sizeof( goalStateFields ) / sizeof( goalStateFields[0] ) };
struct goalPoolSave_t {
	uint32_t present[MAX_CLIENTS + 1], indices[MAX_CLIENTS + 1], itemConfig;
	int32_t cachedWeights[MAX_CLIENTS + 1], items;
	uint8_t itemHash[32], indexHash[MAX_CLIENTS + 1][32];
};
static_assert( sizeof( goalPoolSave_t ) == 2900 );
static constexpr stateField_t goalPoolFields[] = {
	{ "present", offsetof( goalPoolSave_t, present ), MAX_CLIENTS + 1, stateType_t::UInt32 },
	{ "indices", offsetof( goalPoolSave_t, indices ), MAX_CLIENTS + 1, stateType_t::UInt32 },
	{ "itemConfig", offsetof( goalPoolSave_t, itemConfig ), 1, stateType_t::UInt32 },
	{ "cachedWeights", offsetof( goalPoolSave_t, cachedWeights ), MAX_CLIENTS + 1, stateType_t::Int32 },
	{ "items", offsetof( goalPoolSave_t, items ), 1, stateType_t::Int32 },
	{ "itemHash", offsetof( goalPoolSave_t, itemHash ), 32, stateType_t::Bytes },
	{ "indexHash", offsetof( goalPoolSave_t, indexHash ), ( MAX_CLIENTS + 1 ) * 32, stateType_t::Bytes }
};
static constexpr stateSchema_t goalPoolSchema = { "botlib.goalPool", 1, 1, sizeof( goalPoolSave_t ), goalPoolFields, 7 };
static bool GoalPoolIdentity( goalPoolSave_t *saved ) {
	*saved = {};
	for ( auto &index : saved->cachedWeights )
		index = -1;
	if ( botgoalstates[0] )
		return false;
	saved->itemConfig = itemconfig != nullptr;
	if ( itemconfig ) {
		// The loaded descriptor is pointer-free and has no padding on supported ABIs.
		static_assert( sizeof( iteminfo_t ) == 32 + 2 * MAX_STRINGFIELD + 11 * 4 );
		saved->items = itemconfig->numiteminfo;
		if ( saved->items < 0 || saved->items > 4096 || ( saved->items && !itemconfig->iteminfo ) )
			return false;
		calc_sha_256( saved->itemHash, saved->items ? (const void *)itemconfig->iteminfo : "", size_t( saved->items ) * sizeof( iteminfo_t ) );
	}
	for ( int i = 1; i <= MAX_CLIENTS; ++i ) {
		const auto *goal = botgoalstates[i];
		if ( !goal )
			continue;
		saved->present[i] = 1;
		saved->cachedWeights[i] = Bot_WeightCacheIndex( goal->itemweightconfig );
		if ( saved->cachedWeights[i] == -2 )
			for ( int j = 1; j < i; ++j )
				if ( botgoalstates[j] && botgoalstates[j]->itemweightconfig == goal->itemweightconfig )
					return false;
		saved->indices[i] = goal->itemweightindex != nullptr;
		if ( goal->itemweightindex ) {
			if ( !itemconfig || !goal->itemweightconfig )
				return false;
			for ( int j = 0; j < saved->items; ++j )
				if ( goal->itemweightindex[j] < -1 || goal->itemweightindex[j] >= goal->itemweightconfig->numweights )
					return false;
			calc_sha_256( saved->indexHash[i], goal->itemweightindex, size_t( saved->items ) * sizeof( int32_t ) );
		}
	}
	return true;
}
static bool ValidGoalState( const bot_goalstate_t &saved ) {
	if ( saved.client < 0 || saved.client >= MAX_CLIENTS || saved.goalstacktop < 0 || saved.goalstacktop >= MAX_GOALSTACK )
		return false;
	for ( float time : saved.avoidgoaltimes )
		if ( !std::isfinite( time ) )
			return false;
	for ( const auto &goal : saved.goalstack ) {
		if ( goal.areanum < 0 || goal.entitynum < -1 || goal.entitynum >= MAX_GENTITIES )
			return false;
		for ( int i = 0; i < 3; ++i )
			if ( !std::isfinite( goal.origin[i] ) || !std::isfinite( goal.mins[i] ) || !std::isfinite( goal.maxs[i] ) || goal.mins[i] > goal.maxs[i] )
				return false;
	}
	return true;
}
bool Bot_WriteGoalState( stateWriter_t *writer ) {
	if ( !writer )
		return false;
	goalPoolSave_t pool;
	if ( !GoalPoolIdentity( &pool ) ) {
		writer->failed = true;
		return false;
	}
	if ( !State_Append( writer, goalPoolSchema, 0, &pool ) )
		return false;
	for ( uint32_t i = 1; i <= MAX_CLIENTS; ++i )
		if ( pool.present[i] ) {
			if ( !ValidGoalState( *botgoalstates[i] ) ) {
				writer->failed = true;
				return false;
			}
			if ( !State_Append( writer, goalStateSchema, i, botgoalstates[i] ) )
				return false;
			if ( pool.cachedWeights[i] == -2 && !Bot_WriteWeightState( writer, 128 + i, botgoalstates[i]->itemweightconfig ) )
				return false;
		}
	return true;
}
bool Bot_ReadGoalState( const stateReader_t &reader, bool apply ) {
	goalPoolSave_t pool, current;
	uint32_t version;
	if ( !State_Find( reader, goalPoolSchema, 0, &pool, &version ) || !GoalPoolIdentity( &current ) || memcmp( &pool, &current, sizeof( pool ) ) )
		return false;
	static bot_goalstate_t saved[MAX_CLIENTS + 1];
	for ( uint32_t i = 1; i <= MAX_CLIENTS; ++i )
		if ( pool.present[i] ) {
			if ( !State_Find( reader, goalStateSchema, i, &saved[i], &version ) || !ValidGoalState( saved[i] ) )
				return false;
			if ( pool.cachedWeights[i] == -2 && !Bot_ReadWeightState( reader, 128 + i, botgoalstates[i]->itemweightconfig, false ) )
				return false;
		}
	if ( apply )
		for ( uint32_t i = 1; i <= MAX_CLIENTS; ++i )
			if ( pool.present[i] ) {
				if ( pool.cachedWeights[i] == -2 && !Bot_ReadWeightState( reader, 128 + i, botgoalstates[i]->itemweightconfig, true ) )
					return false;
				saved[i].itemweightconfig = botgoalstates[i]->itemweightconfig;
				saved[i].itemweightindex = botgoalstates[i]->itemweightindex;
				*botgoalstates[i] = saved[i];
			}
	return true;
}

static constexpr int MAX_SAVED_LEVEL_ITEMS = 4096;
struct levelItemsHeaderSave_t {
	int32_t count, number, gametype, live, free;
};
struct levelItemsSave_t {
	int32_t number[MAX_SAVED_LEVEL_ITEMS], iteminfo[MAX_SAVED_LEVEL_ITEMS], flags[MAX_SAVED_LEVEL_ITEMS],
		goalareanum[MAX_SAVED_LEVEL_ITEMS], entitynum[MAX_SAVED_LEVEL_ITEMS], next[MAX_SAVED_LEVEL_ITEMS], prev[MAX_SAVED_LEVEL_ITEMS];
	float weight[MAX_SAVED_LEVEL_ITEMS], timeout[MAX_SAVED_LEVEL_ITEMS], origin[MAX_SAVED_LEVEL_ITEMS][3], goalorigin[MAX_SAVED_LEVEL_ITEMS][3];
};
static_assert( sizeof( levelItemsHeaderSave_t ) == 20 && sizeof( levelItemsSave_t ) == 245760 );
static levelItemsSave_t savedLevelItems;
static constexpr stateField_t levelItemsHeaderFields[] = {
	{ "count", offsetof( levelItemsHeaderSave_t, count ), 1, stateType_t::Int32 },
	{ "number", offsetof( levelItemsHeaderSave_t, number ), 1, stateType_t::Int32 },
	{ "gametype", offsetof( levelItemsHeaderSave_t, gametype ), 1, stateType_t::Int32 },
	{ "live", offsetof( levelItemsHeaderSave_t, live ), 1, stateType_t::Int32 },
	{ "free", offsetof( levelItemsHeaderSave_t, free ), 1, stateType_t::Int32 }
};
static constexpr stateSchema_t levelItemsHeaderSchema = { "botlib.levelItemsHeader", 1, 1, sizeof( levelItemsHeaderSave_t ), levelItemsHeaderFields, 5 };
static int LevelItemSlot( const levelitem_t *item ) {
	if ( !item )
		return -1;
	for ( int i = 0; i < allocatedLevelItems; ++i )
		if ( item == &levelitemheap[i] )
			return i;
	return -2;
}
static bool ValidLevelItems( const levelItemsHeaderSave_t &header, bool *live ) {
	if ( header.count < 0 || header.count > MAX_SAVED_LEVEL_ITEMS || header.count != allocatedLevelItems ||
		 ( bool( header.count ) != bool( levelitemheap ) ) || header.number < 0 || header.number > header.count || header.gametype != g_gametype ||
		 header.live < -1 || header.live >= header.count || header.free < -1 || header.free >= header.count )
		return false;
	uint8_t visited[MAX_SAVED_LEVEL_ITEMS]{};
	for ( int i = 0; i < header.count; ++i ) {
		live[i] = false;
		if ( savedLevelItems.next[i] < -1 || savedLevelItems.next[i] >= header.count )
			return false;
	}
	for ( int list = 0; list < 2; ++list ) {
		int previous = -1;
		for ( int slot = list ? header.free : header.live; slot >= 0; slot = savedLevelItems.next[slot] ) {
			if ( visited[slot] )
				return false;
			visited[slot] = 1;
			if ( !list ) {
				live[slot] = true;
				if ( savedLevelItems.prev[slot] != previous || !itemconfig || savedLevelItems.iteminfo[slot] < 0 ||
					 savedLevelItems.iteminfo[slot] >= itemconfig->numiteminfo || savedLevelItems.entitynum[slot] < 0 ||
					 savedLevelItems.entitynum[slot] >= MAX_GENTITIES || savedLevelItems.goalareanum[slot] < 0 ||
					 !std::isfinite( savedLevelItems.weight[slot] ) || !std::isfinite( savedLevelItems.timeout[slot] ) )
					return false;
				for ( int axis = 0; axis < 3; ++axis )
					if ( !std::isfinite( savedLevelItems.origin[slot][axis] ) || !std::isfinite( savedLevelItems.goalorigin[slot][axis] ) )
						return false;
			}
			previous = slot;
		}
	}
	for ( int i = 0; i < header.count; ++i )
		if ( !visited[i] )
			return false;
	return true;
}
static bool LevelItemRecords( stateWriter_t *writer, const stateReader_t *reader, uint32_t count ) {
	if ( !count )
		return true;
	const stateField_t fields[] = {
		{ "number", offsetof( levelItemsSave_t, number ), count, stateType_t::Int32 },
		{ "iteminfo", offsetof( levelItemsSave_t, iteminfo ), count, stateType_t::Int32 },
		{ "flags", offsetof( levelItemsSave_t, flags ), count, stateType_t::Int32 },
		{ "goalareanum", offsetof( levelItemsSave_t, goalareanum ), count, stateType_t::Int32 },
		{ "entitynum", offsetof( levelItemsSave_t, entitynum ), count, stateType_t::Int32 },
		{ "next", offsetof( levelItemsSave_t, next ), count, stateType_t::Int32 },
		{ "prev", offsetof( levelItemsSave_t, prev ), count, stateType_t::Int32 },
		{ "weight", offsetof( levelItemsSave_t, weight ), count, stateType_t::Float32 },
		{ "timeout", offsetof( levelItemsSave_t, timeout ), count, stateType_t::Float32 },
		{ "origin", offsetof( levelItemsSave_t, origin ), count * 3, stateType_t::Float32 },
		{ "goalorigin", offsetof( levelItemsSave_t, goalorigin ), count * 3, stateType_t::Float32 },
	};
	const stateSchema_t schema = { "botlib.levelItems", 1, 1, sizeof( levelItemsSave_t ), fields, 11 };
	uint32_t version;
	return writer ? State_Append( writer, schema, 0, &savedLevelItems ) : State_Find( *reader, schema, 0, &savedLevelItems, &version );
}
bool Bot_WriteLevelItemState( stateWriter_t *writer ) {
	if ( !writer )
		return false;
	if ( allocatedLevelItems < 0 || allocatedLevelItems > MAX_SAVED_LEVEL_ITEMS || ( allocatedLevelItems && !levelitemheap ) ) {
		writer->failed = true;
		return false;
	}
	const levelItemsHeaderSave_t header{ allocatedLevelItems, numlevelitems, g_gametype, LevelItemSlot( levelitems ), LevelItemSlot( freelevelitems ) };
	savedLevelItems = {};
	for ( int i = 0; i < allocatedLevelItems; ++i ) {
		savedLevelItems.next[i] = LevelItemSlot( levelitemheap[i].next );
		savedLevelItems.prev[i] = -1;
	}
	bool live[MAX_SAVED_LEVEL_ITEMS]{};
	for ( int slot = header.live; slot != -1; ) {
		if ( slot < 0 || slot >= allocatedLevelItems || live[slot] ) {
			writer->failed = true;
			return false;
		}
		live[slot] = true;
		const auto &item = levelitemheap[slot];
		savedLevelItems.number[slot] = item.number;
		savedLevelItems.iteminfo[slot] = item.iteminfo;
		savedLevelItems.flags[slot] = item.flags;
		savedLevelItems.goalareanum[slot] = item.goalareanum;
		savedLevelItems.entitynum[slot] = item.entitynum;
		savedLevelItems.weight[slot] = item.weight;
		savedLevelItems.timeout[slot] = item.timeout;
		memcpy( savedLevelItems.origin[slot], item.origin, sizeof( item.origin ) );
		memcpy( savedLevelItems.goalorigin[slot], item.goalorigin, sizeof( item.goalorigin ) );
		savedLevelItems.prev[slot] = LevelItemSlot( item.prev );
		slot = savedLevelItems.next[slot];
	}
	if ( !ValidLevelItems( header, live ) ) {
		writer->failed = true;
		return false;
	}
	return State_Append( writer, levelItemsHeaderSchema, 0, &header ) && LevelItemRecords( writer, nullptr, uint32_t( header.count ) );
}
bool Bot_ReadLevelItemState( const stateReader_t &reader, bool apply ) {
	levelItemsHeaderSave_t header;
	uint32_t version;
	if ( !State_Find( reader, levelItemsHeaderSchema, 0, &header, &version ) || header.count < 0 || header.count > MAX_SAVED_LEVEL_ITEMS ||
		 !LevelItemRecords( nullptr, &reader, uint32_t( header.count ) ) )
		return false;
	bool live[MAX_SAVED_LEVEL_ITEMS];
	if ( !ValidLevelItems( header, live ) )
		return false;
	if ( apply ) {
		for ( int i = 0; i < header.count; ++i ) {
			auto &item = levelitemheap[i];
			memset( &item, 0, sizeof( item ) );
			item.next = savedLevelItems.next[i] < 0 ? nullptr : &levelitemheap[savedLevelItems.next[i]];
			if ( !live[i] )
				continue; // Allocation clears free-node payload before its next use.
			item.prev = savedLevelItems.prev[i] < 0 ? nullptr : &levelitemheap[savedLevelItems.prev[i]];
			item.number = savedLevelItems.number[i];
			item.iteminfo = savedLevelItems.iteminfo[i];
			item.flags = savedLevelItems.flags[i];
			item.goalareanum = savedLevelItems.goalareanum[i];
			item.entitynum = savedLevelItems.entitynum[i];
			item.weight = savedLevelItems.weight[i];
			item.timeout = savedLevelItems.timeout[i];
			memcpy( item.origin, savedLevelItems.origin[i], sizeof( item.origin ) );
			memcpy( item.goalorigin, savedLevelItems.goalorigin[i], sizeof( item.goalorigin ) );
		}
		levelitems = header.live < 0 ? nullptr : &levelitemheap[header.live];
		freelevelitems = header.free < 0 ? nullptr : &levelitemheap[header.free];
		numlevelitems = header.number;
	}
	return true;
}

static constexpr stateField_t mapLocationFields[] = {
	{ "origin", offsetof( maplocation_t, origin ), 3, stateType_t::Float32 },
	{ "areanum", offsetof( maplocation_t, areanum ), 1, stateType_t::Int32 },
	{ "name", offsetof( maplocation_t, name ), MAX_EPAIRKEY, stateType_t::String }
};
static constexpr stateSchema_t mapLocationSchema = { "botlib.mapLocation", 1, 1, sizeof( maplocation_t ), mapLocationFields, 3 };
static constexpr stateField_t campSpotFields[] = {
	{ "origin", offsetof( campspot_t, origin ), 3, stateType_t::Float32 },
	{ "areanum", offsetof( campspot_t, areanum ), 1, stateType_t::Int32 },
	{ "name", offsetof( campspot_t, name ), MAX_EPAIRKEY, stateType_t::String },
	{ "range", offsetof( campspot_t, range ), 1, stateType_t::Float32 },
	{ "weight", offsetof( campspot_t, weight ), 1, stateType_t::Float32 },
	{ "wait", offsetof( campspot_t, wait ), 1, stateType_t::Float32 },
	{ "random", offsetof( campspot_t, random ), 1, stateType_t::Float32 }
};
static constexpr stateSchema_t campSpotSchema = { "botlib.campSpot", 1, 1, sizeof( campspot_t ), campSpotFields, 7 };
static constexpr stateField_t goalMapHashField = { "hash", 0, 32, stateType_t::Bytes };
static constexpr stateSchema_t goalMapHashSchema = { "botlib.goalMap", 1, 1, 32, &goalMapHashField, 1 };
static bool GoalMapHash( uint8_t *digest ) {
	Sha_256 hash;
	sha_256_init( &hash, digest );
	unsigned char bytes[2048];
	uint32_t count = 0;
	for ( auto *location = maplocations; location; location = location->next ) {
		if ( ++count > MAX_SAVED_LEVEL_ITEMS || location->areanum < 0 )
			return false;
		for ( float value : location->origin )
			if ( !std::isfinite( value ) )
				return false;
		const size_t size = State_Write( mapLocationSchema, location, bytes, sizeof( bytes ) );
		if ( !size )
			return false;
		sha_256_write( &hash, bytes, size );
	}
	sha_256_write( &hash, &count, sizeof( count ) );
	count = 0;
	for ( auto *camp = campspots; camp; camp = camp->next ) {
		if ( ++count > MAX_SAVED_LEVEL_ITEMS || camp->areanum <= 0 || !std::isfinite( camp->range ) || !std::isfinite( camp->weight ) ||
			 !std::isfinite( camp->wait ) || !std::isfinite( camp->random ) )
			return false;
		for ( float value : camp->origin )
			if ( !std::isfinite( value ) )
				return false;
		const size_t size = State_Write( campSpotSchema, camp, bytes, sizeof( bytes ) );
		if ( !size )
			return false;
		sha_256_write( &hash, bytes, size );
	}
	sha_256_write( &hash, &count, sizeof( count ) );
	sha_256_close( &hash );
	return true;
}
bool Bot_WriteGoalMapState( stateWriter_t *writer ) {
	if ( !writer )
		return false;
	uint8_t digest[32];
	if ( !GoalMapHash( digest ) ) {
		writer->failed = true;
		return false;
	}
	return State_Append( writer, goalMapHashSchema, 0, digest );
}
bool Bot_ReadGoalMapState( const stateReader_t &reader ) {
	uint8_t saved[32], loaded[32];
	uint32_t version;
	return State_Find( reader, goalMapHashSchema, 0, saved, &version ) && GoalMapHash( loaded ) && !memcmp( saved, loaded, sizeof( saved ) );
}
