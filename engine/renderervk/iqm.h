/*
===========================================================================
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

#ifndef __IQM_H__
#define __IQM_H__

#define IQM_MAGIC "INTERQUAKEMODEL"
#define IQM_VERSION 2

#define IQM_MAX_JOINTS		128

typedef struct iqmheader {
	char magic[16];
	uint32_t version;
	uint32_t filesize;
	uint32_t flags;
	uint32_t num_text, ofs_text;
	uint32_t num_meshes, ofs_meshes;
	uint32_t num_vertexarrays, num_vertexes, ofs_vertexarrays;
	uint32_t num_triangles, ofs_triangles, ofs_adjacency;
	uint32_t num_joints, ofs_joints;
	uint32_t num_poses, ofs_poses;
	uint32_t num_anims, ofs_anims;
	uint32_t num_frames, num_framechannels, ofs_frames, ofs_bounds;
	uint32_t num_comment, ofs_comment;
	uint32_t num_extensions, ofs_extensions;
} iqmHeader_t;

typedef struct iqmmesh {
	uint32_t name;
	uint32_t material;
	uint32_t first_vertex, num_vertexes;
	uint32_t first_triangle, num_triangles;
} iqmMesh_t;

enum {
	IQM_POSITION = 0,
	IQM_TEXCOORD = 1,
	IQM_NORMAL = 2,
	IQM_TANGENT = 3,
	IQM_BLENDINDEXES = 4,
	IQM_BLENDWEIGHTS = 5,
	IQM_COLOR = 6,
	IQM_CUSTOM = 0x10
};

enum {
	IQM_BYTE = 0,
	IQM_UBYTE = 1,
	IQM_SHORT = 2,
	IQM_USHORT = 3,
	IQM_INT = 4,
	IQM_UINT = 5,
	IQM_HALF = 6,
	IQM_FLOAT = 7,
	IQM_DOUBLE = 8,
};

typedef struct iqmtriangle {
	uint32_t vertex[3];
} iqmTriangle_t;

typedef struct iqmjoint {
	uint32_t name;
	int32_t parent;
	float translate[3], rotate[4], scale[3];
} iqmJoint_t;

typedef struct iqmpose {
	int32_t parent;
	uint32_t mask;
	float channeloffset[10];
	float channelscale[10];
} iqmPose_t;

typedef struct iqmanim {
	uint32_t name;
	uint32_t first_frame, num_frames;
	float framerate;
	uint32_t flags;
} iqmAnim_t;

enum {
	IQM_LOOP = 1 << 0
};

typedef struct iqmvertexarray {
	uint32_t type;
	uint32_t flags;
	uint32_t format;
	uint32_t size;
	uint32_t offset;
} iqmVertexArray_t;

typedef struct iqmbounds {
	float bbmin[3], bbmax[3];
	float xyradius, radius;
} iqmBounds_t;


// Preserve the existing file record layouts.
static_assert( sizeof( iqmHeader_t ) == 124 && alignof( iqmHeader_t ) == 4 &&
			   std::is_trivially_copyable_v<iqmHeader_t> && std::is_standard_layout_v<iqmHeader_t> );
static_assert( sizeof( iqmMesh_t ) == 24 && alignof( iqmMesh_t ) == 4 &&
			   std::is_trivially_copyable_v<iqmMesh_t> && std::is_standard_layout_v<iqmMesh_t> );
static_assert( sizeof( iqmTriangle_t ) == 12 && alignof( iqmTriangle_t ) == 4 &&
			   std::is_trivially_copyable_v<iqmTriangle_t> && std::is_standard_layout_v<iqmTriangle_t> );
static_assert( sizeof( iqmJoint_t ) == 48 && alignof( iqmJoint_t ) == 4 &&
			   std::is_trivially_copyable_v<iqmJoint_t> && std::is_standard_layout_v<iqmJoint_t> );
static_assert( sizeof( iqmPose_t ) == 88 && alignof( iqmPose_t ) == 4 &&
			   std::is_trivially_copyable_v<iqmPose_t> && std::is_standard_layout_v<iqmPose_t> );
static_assert( sizeof( iqmAnim_t ) == 20 && alignof( iqmAnim_t ) == 4 &&
			   std::is_trivially_copyable_v<iqmAnim_t> && std::is_standard_layout_v<iqmAnim_t> );
static_assert( sizeof( iqmVertexArray_t ) == 20 && alignof( iqmVertexArray_t ) == 4 &&
			   std::is_trivially_copyable_v<iqmVertexArray_t> && std::is_standard_layout_v<iqmVertexArray_t> );
static_assert( sizeof( iqmBounds_t ) == 32 && alignof( iqmBounds_t ) == 4 &&
			   std::is_trivially_copyable_v<iqmBounds_t> && std::is_standard_layout_v<iqmBounds_t> );

#endif
