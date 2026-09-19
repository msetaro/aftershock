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

#include <stdint.h>
#include <type_traits>


//NOTE:	int =	default signed
//				default long

#define AASID						(('S'<<24)+('A'<<16)+('A'<<8)+'E')
#define AASVERSION_OLD				4
#define AASVERSION					5

//presence types
#define PRESENCE_NONE				1
#define PRESENCE_NORMAL				2
#define PRESENCE_CROUCH				4

//travel types
#define MAX_TRAVELTYPES				32
#define TRAVEL_INVALID				1		//temporary not possible
#define TRAVEL_WALK					2		//walking
#define TRAVEL_CROUCH				3		//crouching
#define TRAVEL_BARRIERJUMP			4		//jumping onto a barrier
#define TRAVEL_JUMP					5		//jumping
#define TRAVEL_LADDER				6		//climbing a ladder
#define TRAVEL_WALKOFFLEDGE			7		//walking of a ledge
#define TRAVEL_SWIM					8		//swimming
#define TRAVEL_WATERJUMP			9		//jump out of the water
#define TRAVEL_TELEPORT				10		//teleportation
#define TRAVEL_ELEVATOR				11		//travel by elevator
#define TRAVEL_ROCKETJUMP			12		//rocket jumping required for travel
#define TRAVEL_BFGJUMP				13		//bfg jumping required for travel
#define TRAVEL_GRAPPLEHOOK			14		//grappling hook required for travel
#define TRAVEL_DOUBLEJUMP			15		//double jump
#define TRAVEL_RAMPJUMP				16		//ramp jump
#define TRAVEL_STRAFEJUMP			17		//strafe jump
#define TRAVEL_JUMPPAD				18		//jump pad
#define TRAVEL_FUNCBOB				19		//func bob

//additional travel flags
#define TRAVELTYPE_MASK				0xFFFFFF
#define TRAVELFLAG_NOTTEAM1			(1 << 24)
#define TRAVELFLAG_NOTTEAM2			(2 << 24)

//face flags
#define FACE_SOLID					1		//just solid at the other side
#define FACE_LADDER					2		//ladder
#define FACE_GROUND					4		//standing on ground when in this face
#define FACE_GAP					8		//gap in the ground
#define FACE_LIQUID					16		//face separating two areas with liquid
#define FACE_LIQUIDSURFACE			32		//face separating liquid and air
#define FACE_BRIDGE					64		//can walk over this face if bridge is closed

//area contents
#define AREACONTENTS_WATER				1
#define AREACONTENTS_LAVA				2
#define AREACONTENTS_SLIME				4
#define AREACONTENTS_CLUSTERPORTAL		8
#define AREACONTENTS_TELEPORTAL			16
#define AREACONTENTS_ROUTEPORTAL		32
#define AREACONTENTS_TELEPORTER			64
#define AREACONTENTS_JUMPPAD			128
#define AREACONTENTS_DONOTENTER			256
#define	AREACONTENTS_VIEWPORTAL			512
#define AREACONTENTS_MOVER				1024
#define AREACONTENTS_NOTTEAM1			2048
#define AREACONTENTS_NOTTEAM2			4096
//number of model of the mover inside this area
#define AREACONTENTS_MODELNUMSHIFT		24
#define AREACONTENTS_MAXMODELNUM		0xFF
#define AREACONTENTS_MODELNUM			(AREACONTENTS_MAXMODELNUM << AREACONTENTS_MODELNUMSHIFT)

//area flags
#define AREA_GROUNDED				1		//bot can stand on the ground
#define AREA_LADDER					2		//area contains one or more ladder faces
#define AREA_LIQUID					4		//area contains a liquid
#define AREA_DISABLED				8		//area is disabled for routing when set
#define AREA_BRIDGE					16		//area ontop of a bridge

//aas file header lumps
#define AAS_LUMPS					14
#define AASLUMP_BBOXES				0
#define AASLUMP_VERTEXES			1
#define AASLUMP_PLANES				2
#define AASLUMP_EDGES				3
#define AASLUMP_EDGEINDEX			4
#define AASLUMP_FACES				5
#define AASLUMP_FACEINDEX			6
#define AASLUMP_AREAS				7
#define AASLUMP_AREASETTINGS		8
#define AASLUMP_REACHABILITY		9
#define AASLUMP_NODES				10
#define AASLUMP_PORTALS				11
#define AASLUMP_PORTALINDEX			12
#define AASLUMP_CLUSTERS			13

//========== bounding box =========

//bounding box
typedef struct aas_bbox_s
{
	int32_t presencetype;
	int32_t flags;
	vec3_t mins, maxs;
} aas_bbox_t;

//============ settings ===========

//reachability to another area
typedef struct aas_reachability_s
{
	int32_t areanum;						//number of the reachable area
	int32_t facenum;						//number of the face towards the other area
	int32_t edgenum;						//number of the edge towards the other area
	vec3_t start;						//start point of inter area movement
	vec3_t end;							//end point of inter area movement
	int32_t traveltype;					//type of travel required to get to the area
	uint16_t traveltime;//travel time of the inter area movement
} aas_reachability_t;

//area settings
typedef struct aas_areasettings_s
{
	//could also add all kind of statistic fields
	int32_t contents;						//contents of the area
	int32_t areaflags;						//several area flags
	int32_t presencetype;					//how a bot can be present in this area
	int32_t cluster;						//cluster the area belongs to, if negative it's a portal
	int32_t clusterareanum;				//number of the area in the cluster
	int32_t numreachableareas;			//number of reachable areas from this one
	int32_t firstreachablearea;			//first reachable area in the reachable area index
} aas_areasettings_t;

//cluster portal
typedef struct aas_portal_s
{
	int32_t areanum;						//area that is the actual portal
	int32_t frontcluster;					//cluster at front of portal
	int32_t backcluster;					//cluster at back of portal
	int32_t clusterareanum[2];			//number of the area in the front and back cluster
} aas_portal_t;

//cluster portal index
typedef int32_t aas_portalindex_t;

//cluster
typedef struct aas_cluster_s
{
	int32_t numareas;						//number of areas in the cluster
	int32_t numreachabilityareas;			//number of areas with reachabilities
	int32_t numportals;						//number of cluster portals
	int32_t firstportal;					//first cluster portal in the index
} aas_cluster_t;

//============ 3d definition ============

typedef vec3_t aas_vertex_t;

//just a plane in the third dimension
typedef struct aas_plane_s
{
	vec3_t normal;						//normal vector of the plane
	float dist;							//distance of the plane (normal vector * distance = point in plane)
	int32_t type;
} aas_plane_t;

//edge
typedef struct aas_edge_s
{
	int32_t v[2];							//numbers of the vertexes of this edge
} aas_edge_t;

//edge index, negative if vertexes are reversed
typedef int32_t aas_edgeindex_t;

//a face bounds an area, often it will also separate two areas
typedef struct aas_face_s
{
	int32_t planenum;						//number of the plane this face is in
	int32_t faceflags;						//face flags (no use to create face settings for just this field)
	int32_t numedges;						//number of edges in the boundary of the face
	int32_t firstedge;						//first edge in the edge index
	int32_t frontarea;						//area at the front of this face
	int32_t backarea;						//area at the back of this face
} aas_face_t;

//face index, stores a negative index if backside of face
typedef int32_t aas_faceindex_t;

//area with a boundary of faces
typedef struct aas_area_s
{
	int32_t areanum;						//number of this area
	//3d definition
	int32_t numfaces;						//number of faces used for the boundary of the area
	int32_t firstface;						//first face in the face index used for the boundary of the area
	vec3_t mins;						//mins of the area
	vec3_t maxs;						//maxs of the area
	vec3_t center;						//'center' of the area
} aas_area_t;

//nodes of the bsp tree
typedef struct aas_node_s
{
	int32_t planenum;
	int32_t children[2];					//child nodes of this node, or areas as leaves when negative
										//when a child is zero it's a solid leaf
} aas_node_t;

//=========== aas file ===============

//header lump
typedef struct
{
	int32_t fileofs;
	int32_t filelen;
} aas_lump_t;

//aas file header
typedef struct aas_header_s
{
	int32_t ident;
	int32_t version;
	int32_t bspchecksum;
	//data entries
	aas_lump_t lumps[AAS_LUMPS];
} aas_header_t;


//====== additional information ======
/*

-	when a node child is a solid leaf the node child number is zero
-	two adjacent areas (sharing a plane at opposite sides) share a face
	this face is a portal between the areas
-	when an area uses a face from the faceindex with a positive index
	then the face plane normal points into the area
-	the face edges are stored counter clockwise using the edgeindex
-	two adjacent convex areas (sharing a face) only share One face
	this is a simple result of the areas being convex
-	the areas can't have a mixture of ground and gap faces
	other mixtures of faces in one area are allowed
-	areas with the AREACONTENTS_CLUSTERPORTAL in the settings have
	the cluster number set to the negative portal number
-	edge zero is a dummy
-	face zero is a dummy
-	area zero is a dummy
-	node zero is a dummy
*/

// Preserve the existing file record layouts.
static_assert( sizeof( aas_bbox_t ) == 32 && alignof( aas_bbox_t ) == 4 &&
	std::is_trivially_copyable_v<aas_bbox_t> && std::is_standard_layout_v<aas_bbox_t> );
static_assert( sizeof( aas_reachability_t ) == 44 && alignof( aas_reachability_t ) == 4 &&
	std::is_trivially_copyable_v<aas_reachability_t> && std::is_standard_layout_v<aas_reachability_t> );
static_assert( sizeof( aas_areasettings_t ) == 28 && alignof( aas_areasettings_t ) == 4 &&
	std::is_trivially_copyable_v<aas_areasettings_t> && std::is_standard_layout_v<aas_areasettings_t> );
static_assert( sizeof( aas_portal_t ) == 20 && alignof( aas_portal_t ) == 4 &&
	std::is_trivially_copyable_v<aas_portal_t> && std::is_standard_layout_v<aas_portal_t> );
static_assert( sizeof( aas_cluster_t ) == 16 && alignof( aas_cluster_t ) == 4 &&
	std::is_trivially_copyable_v<aas_cluster_t> && std::is_standard_layout_v<aas_cluster_t> );
static_assert( sizeof( aas_plane_t ) == 20 && alignof( aas_plane_t ) == 4 &&
	std::is_trivially_copyable_v<aas_plane_t> && std::is_standard_layout_v<aas_plane_t> );
static_assert( sizeof( aas_edge_t ) == 8 && alignof( aas_edge_t ) == 4 &&
	std::is_trivially_copyable_v<aas_edge_t> && std::is_standard_layout_v<aas_edge_t> );
static_assert( sizeof( aas_face_t ) == 24 && alignof( aas_face_t ) == 4 &&
	std::is_trivially_copyable_v<aas_face_t> && std::is_standard_layout_v<aas_face_t> );
static_assert( sizeof( aas_area_t ) == 48 && alignof( aas_area_t ) == 4 &&
	std::is_trivially_copyable_v<aas_area_t> && std::is_standard_layout_v<aas_area_t> );
static_assert( sizeof( aas_node_t ) == 12 && alignof( aas_node_t ) == 4 &&
	std::is_trivially_copyable_v<aas_node_t> && std::is_standard_layout_v<aas_node_t> );
static_assert( sizeof( aas_lump_t ) == 8 && alignof( aas_lump_t ) == 4 &&
	std::is_trivially_copyable_v<aas_lump_t> && std::is_standard_layout_v<aas_lump_t> );
static_assert( sizeof( aas_header_t ) == 124 && alignof( aas_header_t ) == 4 &&
	std::is_trivially_copyable_v<aas_header_t> && std::is_standard_layout_v<aas_header_t> );
