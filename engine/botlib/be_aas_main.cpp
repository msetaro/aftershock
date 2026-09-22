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
 * name:		be_aas_main.c
 *
 * desc:		AAS
 *
 * $Archive: /MissionPack/code/botlib/be_aas_main.c $
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "l_memory.h"
#include "l_libvar.h"
#include "l_utils.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_log.h"
#include "aasfile.h"
#include "botlib_public.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"
#include "../../third_party/sha256/sha-256.h"
#include <cmath>

aas_t aasworld;

libvar_t *saveroutingcache;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void QDECL AAS_Error( const char *fmt, ... ) {
	char str[1024];
	va_list arglist;

	va_start( arglist, fmt );
	Q_vsnprintf( str, sizeof( str ), fmt, arglist );
	va_end( arglist );
	botimport.Print( PRT_FATAL, "%s", str );
} //end of the function AAS_Error
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Loaded( void ) {
	return aasworld.loaded;
} //end of the function AAS_Loaded
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Initialized( void ) {
	return aasworld.initialized;
} //end of the function AAS_Initialized
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void AAS_SetInitialized( void ) {
	aasworld.initialized = qtrue;
	botimport.Print( PRT_MESSAGE, "AAS initialized.\n" );
#ifdef DEBUG
	//create all the routing cache
	//AAS_CreateAllRoutingCache();
	//
	//AAS_RoutingInfo();
#endif
} //end of the function AAS_SetInitialized
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ContinueInit( float time ) {
	//if no AAS file loaded
	if ( !aasworld.loaded )
		return;
	//if AAS is already initialized
	if ( aasworld.initialized )
		return;
	//calculate reachability, if not finished return
	if ( AAS_ContinueInitReachability( time ) )
		return;
	//initialize clustering for the new map
	AAS_InitClustering();
	//if reachability has been calculated and an AAS file should be written
	//or there is a forced data optimization
	if ( aasworld.savefile || ( (int)LibVarGetValue( "forcewrite" ) ) ) {
		//optimize the AAS data
		if ( (int)LibVarValue( "aasoptimize", "0" ) )
			AAS_Optimize();
		//save the AAS file
		if ( AAS_WriteAASFile( aasworld.filename ) ) {
			botimport.Print( PRT_MESSAGE, "%s written successfully\n", aasworld.filename );
		} //end if
		else {
			botimport.Print( PRT_ERROR, "couldn't write %s\n", aasworld.filename );
		} //end else
	} //end if
	//initialize the routing
	AAS_InitRouting();
	//at this point AAS is initialized
	AAS_SetInitialized();
} //end of the function AAS_ContinueInit
//===========================================================================
// called at the start of every frame
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_StartFrame( float time ) {
	aasworld.time = time;
	//unlink all entities that were not updated last frame
	AAS_UnlinkInvalidEntities();
	//invalidate the entities
	AAS_InvalidateEntities();
	//initialize AAS
	AAS_ContinueInit( time );
	//
	aasworld.frameroutingupdates = 0;
	//
	if ( botDeveloper ) {
		if ( LibVarGetValue( "showcacheupdates" ) ) {
			AAS_RoutingInfo();
			LibVarSet( "showcacheupdates", "0" );
		} //end if
		if ( LibVarGetValue( "showmemoryusage" ) ) {
			PrintUsedMemorySize();
			LibVarSet( "showmemoryusage", "0" );
		} //end if
		if ( LibVarGetValue( "memorydump" ) ) {
			PrintMemoryLabels();
			LibVarSet( "memorydump", "0" );
		} //end if
	} //end if
	//
	if ( saveroutingcache->value ) {
		AAS_WriteRouteCache();
		LibVarSet( "saveroutingcache", "0" );
	} //end if
	//
	aasworld.numframes++;
	return BLERR_NOERROR;
} //end of the function AAS_StartFrame
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float AAS_Time( void ) {
	return aasworld.time;
} //end of the function AAS_Time
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj ) {
	vec3_t pVec, vec;

	VectorSubtract( point, vStart, pVec );
	VectorSubtract( vEnd, vStart, vec );
	VectorNormalize( vec );
	// project onto the directional vector for this segment
	VectorMA( vStart, DotProduct( pVec, vec ), vec, vProj );
} //end of the function AAS_ProjectPointOntoVector
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static int AAS_LoadFiles( const char *mapname ) {
	int errnum;
	char aasfile[MAX_PATH];
	//	char bspfile[MAX_PATH];

	Q_strncpyz( aasworld.mapname, mapname, sizeof( aasworld.mapname ) );
	//NOTE: first reset the entity links into the AAS areas and BSP leaves
	// the AAS link heap and BSP link heap are reset after respectively the
	// AAS file and BSP file are loaded
	AAS_ResetEntityLinks();
	// load bsp info
	AAS_LoadBSPFile();

	//load the aas file
	Com_sprintf( aasfile, sizeof( aasfile ), "maps/%s.aas", mapname );
	errnum = AAS_LoadAASFile( aasfile );
	if ( errnum != BLERR_NOERROR )
		return errnum;

	botimport.Print( PRT_MESSAGE, "loaded %s\n", aasfile );
	Q_strncpyz( aasworld.filename, aasfile, sizeof( aasworld.filename ) );
	return BLERR_NOERROR;
} //end of the function AAS_LoadFiles
//===========================================================================
// called every time a map changes
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_LoadMap( const char *mapname ) {
	int errnum;

	//if no mapname is provided then the string indexes are updated
	if ( !mapname ) {
		return 0;
	} //end if
	//
	aasworld.initialized = qfalse;
	//NOTE: free the routing caches before loading a new map because
	// to free the caches the old number of areas, number of clusters
	// and number of areas in a clusters must be available
	AAS_FreeRoutingCaches();
	//load the map
	errnum = AAS_LoadFiles( mapname );
	if ( errnum != BLERR_NOERROR ) {
		aasworld.loaded = qfalse;
		return errnum;
	} //end if
	//
	AAS_InitSettings();
	//initialize the AAS link heap for the new map
	AAS_InitAASLinkHeap();
	//initialize the AAS linked entities for the new map
	AAS_InitAASLinkedEntities();
	//initialize reachability for the new map
	AAS_InitReachability();
	//initialize the alternative routing
	AAS_InitAlternativeRouting();
	//everything went ok
	return 0;
} //end of the function AAS_LoadMap
//===========================================================================
// called when the library is first loaded
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Setup( void ) {
	aasworld.maxclients = LibVarInteger( "maxclients", "128", 0, MAX_CLIENTS );
	aasworld.maxentities = LibVarInteger( "maxentities", "1024", 0, MAX_GENTITIES );
	// as soon as it's set to 1 the routing cache will be saved
	saveroutingcache = LibVar( "saveroutingcache", "0" );
	//allocate memory for the entities
	if ( aasworld.entities )
		FreeMemory( aasworld.entities );
	aasworld.entities = (aas_entity_t *)GetClearedHunkMemory(aasworld.maxentities * sizeof(aas_entity_t));
	//invalidate all the entities
	AAS_InvalidateEntities();
	//force some recalculations
	//LibVarSet("forceclustering", "1");			//force clustering calculation
	//LibVarSet("forcereachability", "1");		//force reachability calculation
	aasworld.numframes = 0;
	return BLERR_NOERROR;
} //end of the function AAS_Setup
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_Shutdown( void ) {
	AAS_ShutdownAlternativeRouting();
	//
	AAS_DumpBSPData();
	//free routing caches
	AAS_FreeRoutingCaches();
	//free aas link heap
	AAS_FreeAASLinkHeap();
	//free aas linked entities
	AAS_FreeAASLinkedEntities();
	//free the aas data
	AAS_DumpAASData();
	//free the entities
	if ( aasworld.entities )
		FreeMemory( aasworld.entities );
	//clear the aasworld structure
	Com_Memset( &aasworld, 0, sizeof( aas_t ) );
	//aas has not been initialized
	aasworld.initialized = qfalse;
	//NOTE: as soon as a new .bsp file is loaded the .bsp file memory is
	// freed and reallocated, so there's no need to free that memory here
	//print shutdown
	botimport.Print( PRT_MESSAGE, "AAS shutdown.\n" );
} //end of the function AAS_Shutdown

struct aasWorldSave_t {
	int32_t loaded, initialized, savefile, bspchecksum;
	float time;
	int32_t numframes;
	char filename[MAX_PATH], mapname[MAX_PATH];
	int32_t numreachabilityareas;
	float reachabilitytime;
	int32_t maxentities, maxclients, frameroutingupdates;
	uint8_t geometry[32];
};
static constexpr stateField_t aasWorldFields[] = {
	{ "loaded", offsetof( aasWorldSave_t, loaded ), 1, stateType_t::Int32 },
	{ "initialized", offsetof( aasWorldSave_t, initialized ), 1, stateType_t::Int32 },
	{ "savefile", offsetof( aasWorldSave_t, savefile ), 1, stateType_t::Int32 },
	{ "bspchecksum", offsetof( aasWorldSave_t, bspchecksum ), 1, stateType_t::Int32 },
	{ "time", offsetof( aasWorldSave_t, time ), 1, stateType_t::Float32 },
	{ "numframes", offsetof( aasWorldSave_t, numframes ), 1, stateType_t::Int32 },
	{ "filename", offsetof( aasWorldSave_t, filename ), MAX_PATH, stateType_t::String },
	{ "mapname", offsetof( aasWorldSave_t, mapname ), MAX_PATH, stateType_t::String },
	{ "numreachabilityareas", offsetof( aasWorldSave_t, numreachabilityareas ), 1, stateType_t::Int32 },
	{ "reachabilitytime", offsetof( aasWorldSave_t, reachabilitytime ), 1, stateType_t::Float32 },
	{ "maxentities", offsetof( aasWorldSave_t, maxentities ), 1, stateType_t::Int32 },
	{ "maxclients", offsetof( aasWorldSave_t, maxclients ), 1, stateType_t::Int32 },
	{ "frameroutingupdates", offsetof( aasWorldSave_t, frameroutingupdates ), 1, stateType_t::Int32 },
	{ "geometry", offsetof( aasWorldSave_t, geometry ), 32, stateType_t::Bytes },
};
static constexpr stateSchema_t aasWorldSchema = { "botlib.aasWorld", 1, 1, sizeof( aasWorldSave_t ), aasWorldFields, sizeof( aasWorldFields ) / sizeof( *aasWorldFields ) };
// File records are fixed-width, little-endian and pointer-free. Existing layout
// assertions cover these arrays; reachability has two tail padding bytes to omit.
static_assert( offsetof( aas_reachability_t, traveltime ) == 40 );
static bool AASHashArray( Sha_256 *hash, const void *data, int32_t count, size_t stride, size_t bytes ) {
	if ( count < 0 || ( count && !data ) )
		return false;
	sha_256_write( hash, &count, sizeof( count ) );
	auto *cursor = static_cast<const unsigned char *>( data );
	for ( int32_t i = 0; i < count; ++i, cursor += stride )
		sha_256_write( hash, cursor, bytes );
	return true;
}
static bool AASGeometryHash( uint8_t *digest ) {
	Sha_256 hash;
	sha_256_init( &hash, digest );
	if ( !AASHashArray( &hash, aasworld.bboxes, aasworld.numbboxes, sizeof( *aasworld.bboxes ), sizeof( *aasworld.bboxes ) ) )
		return false;
	if ( !AASHashArray( &hash, aasworld.vertexes, aasworld.numvertexes, sizeof( *aasworld.vertexes ), sizeof( *aasworld.vertexes ) ) )
		return false;
	if ( !AASHashArray( &hash, aasworld.planes, aasworld.numplanes, sizeof( *aasworld.planes ), sizeof( *aasworld.planes ) ) )
		return false;
	if ( !AASHashArray( &hash, aasworld.edges, aasworld.numedges, sizeof( *aasworld.edges ), sizeof( *aasworld.edges ) ) )
		return false;
	if ( !AASHashArray( &hash, aasworld.edgeindex, aasworld.edgeindexsize, sizeof( *aasworld.edgeindex ), sizeof( *aasworld.edgeindex ) ) )
		return false;
	if ( !AASHashArray( &hash, aasworld.faces, aasworld.numfaces, sizeof( *aasworld.faces ), sizeof( *aasworld.faces ) ) )
		return false;
	if ( !AASHashArray( &hash, aasworld.faceindex, aasworld.faceindexsize, sizeof( *aasworld.faceindex ), sizeof( *aasworld.faceindex ) ) )
		return false;
	if ( !AASHashArray( &hash, aasworld.areas, aasworld.numareas, sizeof( *aasworld.areas ), sizeof( *aasworld.areas ) ) )
		return false;
	if ( !AASHashArray( &hash, aasworld.reachability, aasworld.reachabilitysize, sizeof( *aasworld.reachability ), offsetof( aas_reachability_t, traveltime ) + sizeof( uint16_t ) ) )
		return false;
	if ( !AASHashArray( &hash, aasworld.nodes, aasworld.numnodes, sizeof( *aasworld.nodes ), sizeof( *aasworld.nodes ) ) )
		return false;
	if ( !AASHashArray( &hash, aasworld.portals, aasworld.numportals, sizeof( *aasworld.portals ), sizeof( *aasworld.portals ) ) )
		return false;
	if ( !AASHashArray( &hash, aasworld.portalindex, aasworld.portalindexsize, sizeof( *aasworld.portalindex ), sizeof( *aasworld.portalindex ) ) )
		return false;
	if ( !AASHashArray( &hash, aasworld.clusters, aasworld.numclusters, sizeof( *aasworld.clusters ), sizeof( *aasworld.clusters ) ) )
		return false;
	if ( aasworld.numareasettings != aasworld.numareas || !aasworld.areasettings )
		return false;
	for ( int i = 0; i < aasworld.numareasettings; ++i ) {
		auto settings = aasworld.areasettings[i];
		settings.areaflags &= ~AREA_DISABLED;
		sha_256_write( &hash, &settings, sizeof( settings ) );
	}
	sha_256_close( &hash );
	return true;
}
static bool ValidAASWorld( const aasWorldSave_t &saved ) {
	return aasworld.numareas >= 1 && aasworld.numareas <= 65536 && saved.loaded == 1 && saved.initialized == 1 && aasworld.loaded == 1 && aasworld.initialized == 1 &&
		   saved.savefile >= 0 && saved.savefile <= 1 && saved.numframes >= 0 && std::isfinite( saved.time ) && std::isfinite( saved.reachabilitytime ) &&
		   saved.bspchecksum == aasworld.bspchecksum && !strcmp( saved.filename, aasworld.filename ) && !strcmp( saved.mapname, aasworld.mapname ) &&
		   saved.numreachabilityareas >= 0 && saved.numreachabilityareas <= aasworld.numareas + 2 && saved.frameroutingupdates >= 0 &&
		   saved.maxentities == aasworld.maxentities && saved.maxclients == aasworld.maxclients &&
		   saved.maxentities >= 1 && saved.maxentities <= MAX_GENTITIES && saved.maxclients >= 0 && saved.maxclients <= MAX_CLIENTS;
}
static bool AASDisabledAreas( stateWriter_t *writer, const stateReader_t *reader, bool apply ) {
	// Same area ceiling as the spatial-link checkpoint, with one bit per area.
	static uint32_t disabled[65536];
	if ( aasworld.numareas < 1 || aasworld.numareas > 65536 || !aasworld.areasettings )
		return false;
	const stateField_t field = { "disabled", 0, uint32_t( aasworld.numareas ), stateType_t::UInt32 };
	const stateSchema_t schema = { "botlib.aasDisabled", 1, 1, sizeof( disabled ), &field, 1 };
	if ( writer ) {
		for ( int i = 0; i < aasworld.numareas; ++i )
			disabled[i] = bool( aasworld.areasettings[i].areaflags & AREA_DISABLED );
		return State_Append( writer, schema, 0, disabled );
	}
	uint32_t version;
	if ( !State_Find( *reader, schema, 0, disabled, &version ) )
		return false;
	for ( int i = 0; i < aasworld.numareas; ++i )
		if ( disabled[i] > 1 )
			return false;
	if ( apply )
		for ( int i = 0; i < aasworld.numareas; ++i )
			aasworld.areasettings[i].areaflags = ( aasworld.areasettings[i].areaflags & ~AREA_DISABLED ) | ( disabled[i] ? AREA_DISABLED : 0 );
	return true;
}
bool AAS_WriteWorldState( stateWriter_t *writer ) {
	if ( !writer )
		return false;
	aasWorldSave_t saved{};
	saved.loaded = aasworld.loaded;
	saved.initialized = aasworld.initialized;
	saved.savefile = aasworld.savefile;
	saved.bspchecksum = aasworld.bspchecksum;
	saved.time = aasworld.time;
	saved.numframes = aasworld.numframes;
	saved.numreachabilityareas = aasworld.numreachabilityareas;
	saved.reachabilitytime = aasworld.reachabilitytime;
	saved.maxentities = aasworld.maxentities;
	saved.maxclients = aasworld.maxclients;
	saved.frameroutingupdates = aasworld.frameroutingupdates;
	if ( !memchr( aasworld.filename, 0, sizeof( aasworld.filename ) ) || !memchr( aasworld.mapname, 0, sizeof( aasworld.mapname ) ) ) {
		writer->failed = true;
		return false;
	}
	strcpy( saved.filename, aasworld.filename );
	strcpy( saved.mapname, aasworld.mapname );
	if ( !ValidAASWorld( saved ) || !AASGeometryHash( saved.geometry ) || !State_Append( writer, aasWorldSchema, 0, &saved ) || !AASDisabledAreas( writer, nullptr, false ) ) {
		writer->failed = true;
		return false;
	}
	return true;
}
bool AAS_ReadWorldState( const stateReader_t &reader, bool apply ) {
	aasWorldSave_t saved;
	uint32_t version;
	uint8_t geometry[32];
	if ( !State_Find( reader, aasWorldSchema, 0, &saved, &version ) || !ValidAASWorld( saved ) || !AASGeometryHash( geometry ) ||
		 memcmp( saved.geometry, geometry, sizeof( geometry ) ) || !AASDisabledAreas( nullptr, &reader, false ) )
		return false;
	if ( apply ) {
		aasworld.savefile = saved.savefile;
		aasworld.time = saved.time;
		aasworld.numframes = saved.numframes;
		aasworld.numreachabilityareas = saved.numreachabilityareas;
		aasworld.reachabilitytime = saved.reachabilitytime;
		aasworld.frameroutingupdates = saved.frameroutingupdates;
		return AASDisabledAreas( nullptr, &reader, true );
	}
	return true;
}
