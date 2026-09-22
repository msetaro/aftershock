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
 * name:		be_aas_entity.c
 *
 * desc:		AAS entities
 *
 * $Archive: /MissionPack/code/botlib/be_aas_entity.c $
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"
#include "l_log.h"
#include "aasfile.h"
#include "botlib_public.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"
#include <cmath>

#define MASK_SOLID		CONTENTS_PLAYERCLIP

//FIXME: these might change
enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER
};

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_UpdateEntity( int entnum, bot_entitystate_t *state ) {
	int relink;
	aas_entity_t *ent;
	vec3_t absmins, absmaxs;

	if ( !aasworld.loaded ) {
		botimport.Print( PRT_MESSAGE, "AAS_UpdateEntity: not loaded\n" );
		return BLERR_NOAASFILE;
	} //end if

	if ( entnum < 0 || entnum >= aasworld.maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_UpdateEntity: entnum %d out of range\n", entnum );
		return BLERR_INVALIDENTITYNUMBER;
	}

	ent = &aasworld.entities[entnum];

	if ( !state ) {
		//unlink the entity
		AAS_UnlinkFromAreas( ent->areas );
		//unlink the entity from the BSP leaves
		AAS_UnlinkFromBSPLeaves( ent->leaves );
		//
		ent->areas = NULL;
		//
		ent->leaves = NULL;
		return BLERR_NOERROR;
	}

	ent->i.update_time = AAS_Time() - ent->i.ltime;
	ent->i.type = state->type;
	ent->i.flags = state->flags;
	ent->i.ltime = AAS_Time();
	VectorCopy( ent->i.origin, ent->i.lastvisorigin );
	VectorCopy( state->old_origin, ent->i.old_origin );
	ent->i.solid = state->solid;
	ent->i.groundent = state->groundent;
	ent->i.modelindex = state->modelindex;
	ent->i.modelindex2 = state->modelindex2;
	ent->i.frame = state->frame;
	ent->i.event = state->event;
	ent->i.eventParm = state->eventParm;
	ent->i.powerups = state->powerups;
	ent->i.weapon = state->weapon;
	ent->i.legsAnim = state->legsAnim;
	ent->i.torsoAnim = state->torsoAnim;
	//number of the entity
	ent->i.number = entnum;
	//updated so set valid flag
	ent->i.valid = qtrue;
	//link everything the first frame
	if ( aasworld.numframes == 1 )
		relink = qtrue;
	else
		relink = qfalse;
	//
	if ( ent->i.solid == SOLID_BSP ) {
		//if the angles of the model changed
		if ( !VectorCompare( state->angles, ent->i.angles ) ) {
			VectorCopy( state->angles, ent->i.angles );
			relink = qtrue;
		} //end if
		//get the mins and maxs of the model
		//FIXME: rotate mins and maxs
		AAS_BSPModelMinsMaxsOrigin( ent->i.modelindex, ent->i.angles, ent->i.mins, ent->i.maxs, NULL );
	} //end if
	else if ( ent->i.solid == SOLID_BBOX ) {
		//if the bounding box size changed
		if ( !VectorCompare( state->mins, ent->i.mins ) ||
			 !VectorCompare( state->maxs, ent->i.maxs ) ) {
			VectorCopy( state->mins, ent->i.mins );
			VectorCopy( state->maxs, ent->i.maxs );
			relink = qtrue;
		} //end if
		VectorCopy( state->angles, ent->i.angles );
	} //end if
	//if the origin changed
	if ( !VectorCompare( state->origin, ent->i.origin ) ) {
		VectorCopy( state->origin, ent->i.origin );
		relink = qtrue;
	} //end if
	//if the entity should be relinked
	if ( relink ) {
		//don't link the world model
		if ( entnum != ENTITYNUM_WORLD ) {
			//absolute mins and maxs
			VectorAdd( ent->i.mins, ent->i.origin, absmins );
			VectorAdd( ent->i.maxs, ent->i.origin, absmaxs );
			//unlink the entity
			AAS_UnlinkFromAreas( ent->areas );
			//relink the entity to the AAS areas (use the larges bbox)
			ent->areas = AAS_LinkEntityClientBBox( absmins, absmaxs, entnum, PRESENCE_NORMAL );
			//unlink the entity from the BSP leaves
			AAS_UnlinkFromBSPLeaves( ent->leaves );
			//link the entity to the world BSP tree
			ent->leaves = AAS_BSPLinkEntity( absmins, absmaxs, entnum, 0 );
		} //end if
	} //end if
	return BLERR_NOERROR;
} //end of the function AAS_UpdateEntity
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_EntityInfo( int entnum, aas_entityinfo_t *info ) {
	if ( !aasworld.initialized ) {
		botimport.Print( PRT_FATAL, "AAS_EntityInfo: aasworld not initialized\n" );
		Com_Memset( info, 0, sizeof( aas_entityinfo_t ) );
		return;
	} //end if

	if ( entnum < 0 || entnum >= aasworld.maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntityInfo: entnum %d out of range\n", entnum );
		Com_Memset( info, 0, sizeof( aas_entityinfo_t ) );
		return;
	} //end if

	Com_Memcpy( info, &aasworld.entities[entnum].i, sizeof( aas_entityinfo_t ) );
} //end of the function AAS_EntityInfo
#if 0
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_EntityOrigin(int entnum, vec3_t origin)
{
	if (entnum < 0 || entnum >= aasworld.maxentities)
	{
		botimport.Print(PRT_FATAL, "AAS_EntityOrigin: entnum %d out of range\n", entnum);
		VectorClear(origin);
		return;
	} //end if

	VectorCopy(aasworld.entities[entnum].i.origin, origin);
} //end of the function AAS_EntityOrigin
#endif
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_EntityModelindex( int entnum ) {
	if ( entnum < 0 || entnum >= aasworld.maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntityModelindex: entnum %d out of range\n", entnum );
		return 0;
	} //end if
	return aasworld.entities[entnum].i.modelindex;
} //end of the function AAS_EntityModelindex
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_EntityType( int entnum ) {
	if ( !aasworld.initialized )
		return 0;

	if ( entnum < 0 || entnum >= aasworld.maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntityType: entnum %d out of range\n", entnum );
		return 0;
	} //end if
	return aasworld.entities[entnum].i.type;
} //end of the AAS_EntityType
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_EntityModelNum( int entnum ) {
	if ( !aasworld.initialized )
		return 0;

	if ( entnum < 0 || entnum >= aasworld.maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntityModelNum: entnum %d out of range\n", entnum );
		return 0;
	} //end if
	return aasworld.entities[entnum].i.modelindex;
} //end of the function AAS_EntityModelNum
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_OriginOfMoverWithModelNum( int modelnum, vec3_t origin ) {
	int i;
	aas_entity_t *ent;

	for ( i = 0; i < aasworld.maxentities; i++ ) {
		ent = &aasworld.entities[i];
		if ( ent->i.type == ET_MOVER ) {
			if ( ent->i.modelindex == modelnum ) {
				VectorCopy( ent->i.origin, origin );
				return qtrue;
			} //end if
		} //end if
	} //end for
	return qfalse;
} //end of the function AAS_OriginOfMoverWithModelNum
#if 0
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_EntitySize(int entnum, vec3_t mins, vec3_t maxs)
{
	aas_entity_t *ent;

	if (!aasworld.initialized) return;

	if (entnum < 0 || entnum >= aasworld.maxentities)
	{
		botimport.Print(PRT_FATAL, "AAS_EntitySize: entnum %d out of range\n", entnum);
		return;
	} //end if

	ent = &aasworld.entities[entnum];
	VectorCopy(ent->i.mins, mins);
	VectorCopy(ent->i.maxs, maxs);
} //end of the function AAS_EntitySize
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_EntityBSPData(int entnum, bsp_entdata_t *entdata)
{
	aas_entity_t *ent;

	ent = &aasworld.entities[entnum];
	VectorCopy(ent->i.origin, entdata->origin);
	VectorCopy(ent->i.angles, entdata->angles);
	VectorAdd(ent->i.origin, ent->i.mins, entdata->absmins);
	VectorAdd(ent->i.origin, ent->i.maxs, entdata->absmaxs);
	entdata->solid = ent->i.solid;
	entdata->modelnum = ent->i.modelindex - 1;
} //end of the function AAS_EntityBSPData
#endif
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ResetEntityLinks( void ) {
	int i;
	for ( i = 0; i < aasworld.maxentities; i++ ) {
		aasworld.entities[i].areas = NULL;
		aasworld.entities[i].leaves = NULL;
	} //end for
} //end of the function AAS_ResetEntityLinks
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InvalidateEntities( void ) {
	int i;
	for ( i = 0; i < aasworld.maxentities; i++ ) {
		aasworld.entities[i].i.valid = qfalse;
		aasworld.entities[i].i.number = i;
	} //end for
} //end of the function AAS_InvalidateEntities
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_UnlinkInvalidEntities( void ) {
	int i;
	aas_entity_t *ent;

	for ( i = 0; i < aasworld.maxentities; i++ ) {
		ent = &aasworld.entities[i];
		if ( !ent->i.valid ) {
			AAS_UnlinkFromAreas( ent->areas );
			ent->areas = NULL;
			AAS_UnlinkFromBSPLeaves( ent->leaves );
			ent->leaves = NULL;
		} //end for
	} //end for
} //end of the function AAS_UnlinkInvalidEntities
#if 0
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_NearestEntity(vec3_t origin, int modelindex)
{
	int i, bestentnum;
	float dist, bestdist;
	aas_entity_t *ent;
	vec3_t dir;

	bestentnum = 0;
	bestdist = 99999;
	for (i = 0; i < aasworld.maxentities; i++)
	{
		ent = &aasworld.entities[i];
		if (ent->i.modelindex != modelindex) continue;
		VectorSubtract(ent->i.origin, origin, dir);
		if (fabs(dir[0]) < 40)
		{
			if (fabs(dir[1]) < 40)
			{
				dist = VectorLength(dir);
				if (dist < bestdist)
				{
					bestdist = dist;
					bestentnum = i;
				} //end if
			} //end if
		} //end if
	} //end for
	return bestentnum;
} //end of the function AAS_NearestEntity
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static int AAS_BestReachableEntityArea(int entnum)
{
	aas_entity_t *ent;

	ent = &aasworld.entities[entnum];
	return AAS_BestReachableLinkArea(ent->areas);
} //end of the function AAS_BestReachableEntityArea
#endif
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_NextEntity( int entnum ) {
	if ( !aasworld.loaded )
		return 0;

	if ( entnum < 0 )
		entnum = -1;
	while ( ++entnum < aasworld.maxentities ) {
		if ( aasworld.entities[entnum].i.valid )
			return entnum;
	} //end while
	return 0;
} //end of the function AAS_NextEntity

static constexpr stateField_t aasEntityFields[] = {
	{ "valid", offsetof( aas_entityinfo_t, valid ), 1, stateType_t::Int32 },
	{ "type", offsetof( aas_entityinfo_t, type ), 1, stateType_t::Int32 },
	{ "flags", offsetof( aas_entityinfo_t, flags ), 1, stateType_t::Int32 },
	{ "ltime", offsetof( aas_entityinfo_t, ltime ), 1, stateType_t::Float32 },
	{ "update_time", offsetof( aas_entityinfo_t, update_time ), 1, stateType_t::Float32 },
	{ "number", offsetof( aas_entityinfo_t, number ), 1, stateType_t::Int32 },
	{ "origin", offsetof( aas_entityinfo_t, origin ), 3, stateType_t::Float32 },
	{ "angles", offsetof( aas_entityinfo_t, angles ), 3, stateType_t::Float32 },
	{ "old_origin", offsetof( aas_entityinfo_t, old_origin ), 3, stateType_t::Float32 },
	{ "lastvisorigin", offsetof( aas_entityinfo_t, lastvisorigin ), 3, stateType_t::Float32 },
	{ "mins", offsetof( aas_entityinfo_t, mins ), 3, stateType_t::Float32 },
	{ "maxs", offsetof( aas_entityinfo_t, maxs ), 3, stateType_t::Float32 },
	{ "groundent", offsetof( aas_entityinfo_t, groundent ), 1, stateType_t::Int32 },
	{ "solid", offsetof( aas_entityinfo_t, solid ), 1, stateType_t::Int32 },
	{ "modelindex", offsetof( aas_entityinfo_t, modelindex ), 1, stateType_t::Int32 },
	{ "modelindex2", offsetof( aas_entityinfo_t, modelindex2 ), 1, stateType_t::Int32 },
	{ "frame", offsetof( aas_entityinfo_t, frame ), 1, stateType_t::Int32 },
	{ "event", offsetof( aas_entityinfo_t, event ), 1, stateType_t::Int32 },
	{ "eventParm", offsetof( aas_entityinfo_t, eventParm ), 1, stateType_t::Int32 },
	{ "powerups", offsetof( aas_entityinfo_t, powerups ), 1, stateType_t::Int32 },
	{ "weapon", offsetof( aas_entityinfo_t, weapon ), 1, stateType_t::Int32 },
	{ "legsAnim", offsetof( aas_entityinfo_t, legsAnim ), 1, stateType_t::Int32 },
	{ "torsoAnim", offsetof( aas_entityinfo_t, torsoAnim ), 1, stateType_t::Int32 }
};
static constexpr stateSchema_t aasEntitySchema = { "botlib.aasEntity", 1, 1, sizeof( aas_entityinfo_t ), aasEntityFields, sizeof( aasEntityFields ) / sizeof( *aasEntityFields ) };
static constexpr stateField_t aasEntityCountField = { "count", 0, 1, stateType_t::Int32 };
static constexpr stateSchema_t aasEntityCountSchema = { "botlib.aasEntities", 1, 1, sizeof( int32_t ), &aasEntityCountField, 1 };
static bool ValidAASEntity( const aas_entityinfo_t &entity, int slot ) {
	if ( entity.valid < 0 || entity.valid > 1 || ( entity.valid && entity.number != slot ) || entity.groundent < -1 || entity.groundent >= MAX_GENTITIES ||
		 entity.solid < SOLID_NOT || entity.solid > SOLID_BSP || entity.modelindex < 0 || entity.modelindex >= MAX_MODELS ||
		 entity.modelindex2 < 0 || entity.modelindex2 >= MAX_MODELS )
		return false;
	for ( const auto &field : aasEntityFields )
		if ( field.type == stateType_t::Float32 ) {
			const auto *values = reinterpret_cast<const unsigned char *>( &entity ) + field.offset;
			for ( uint32_t i = 0; i < field.count; ++i ) {
				float value;
				memcpy( &value, values + i * sizeof( float ), sizeof( value ) );
				if ( !std::isfinite( value ) )
					return false;
			}
		}
	for ( int i = 0; i < 3; ++i )
		if ( entity.mins[i] > entity.maxs[i] )
			return false;
	return true;
}
bool AAS_WriteEntityState( stateWriter_t *writer ) {
	if ( !writer )
		return false;
	const int32_t count = aasworld.entities ? aasworld.maxentities : 0;
	if ( count < 0 || count > MAX_GENTITIES ) {
		writer->failed = true;
		return false;
	}
	if ( !State_Append( writer, aasEntityCountSchema, 0, &count ) )
		return false;
	for ( int i = 0; i < count; ++i ) {
		// The Quake 3 BSP adapter returns no leaf links. Spatial AAS links are owned
		// by AAS_WriteLinkState, independently of whether this frame marked i valid.
		if ( aasworld.entities[i].leaves || !ValidAASEntity( aasworld.entities[i].i, i ) ) {
			writer->failed = true;
			return false;
		}
		if ( !State_Append( writer, aasEntitySchema, uint32_t( i ), &aasworld.entities[i].i ) )
			return false;
	}
	return true;
}
bool AAS_ReadEntityState( const stateReader_t &reader, bool apply ) {
	int32_t count;
	uint32_t version;
	if ( !State_Find( reader, aasEntityCountSchema, 0, &count, &version ) || count < 0 || count > MAX_GENTITIES ||
		 count != ( aasworld.entities ? aasworld.maxentities : 0 ) )
		return false;
	static aas_entityinfo_t saved[MAX_GENTITIES];
	for ( int i = 0; i < count; ++i )
		if ( aasworld.entities[i].leaves || !State_Find( reader, aasEntitySchema, uint32_t( i ), &saved[i], &version ) || !ValidAASEntity( saved[i], i ) )
			return false;
	if ( apply )
		for ( int i = 0; i < count; ++i )
			aasworld.entities[i].i = saved[i];
	return true;
}
