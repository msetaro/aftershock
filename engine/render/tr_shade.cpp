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
// tr_shade.c

#include "tr_local.h"
#include "tr_cooked.h"

void RB_BindPipeline( uint32_t pipeline ) {
	R_CheckRHI( RHI_BindPipeline( pipeline ), "pipeline binding" );
}

void RB_BindIndex( void ) {
#ifdef USE_VBO
	if ( tess.vboIndex ) {
		RHI_BindIndexData( 0, nullptr );
		return;
	}
#endif
	RHI_BindIndexData( tess.numIndexes, tess.indexes );
}

void RB_DrawGeometry( rhiDepthRange_t depthRange, qboolean indexed ) {
	rhiRasterState_t raster;
	RB_GetRaster( depthRange, &raster );
	if ( !RHI_PrepareDraw( &raster, &tr.whiteImage->texture ) )
		return;
#ifdef USE_VBO
	if ( tess.vboIndex ) {
		VBO_RenderIBOItems();
		return;
	}
#endif
	if ( indexed )
		RHI_DrawBoundIndices();
	else
		RHI_Draw( tess.numVertexes );
}

void RB_BindGeometry( uint32_t flags ) {
	rhiVertexStream_t streams[RHI_MAX_VERTEX_STREAMS] = {};

	const uint32_t streamFlags[RHI_MAX_VERTEX_STREAMS] = { TESS_XYZ, TESS_RGBA0, TESS_ST0, TESS_ST1, TESS_ST2, TESS_NNN, TESS_RGBA1, TESS_RGBA2 };
	uint32_t mask = 0;
	for ( uint32_t i = 0; i < RHI_MAX_VERTEX_STREAMS; i++ ) {
		if ( flags & streamFlags[i] )
			mask |= 1U << i;
	}

	if ( ( flags & ( TESS_XYZ | TESS_RGBA0 | TESS_ST0 | TESS_ST1 | TESS_ST2 | TESS_NNN | TESS_RGBA1 | TESS_RGBA2 ) ) == 0 )
		return;

#ifdef USE_VBO
	if ( tess.vboIndex ) {

		if ( flags & TESS_XYZ ) { // 0
			streams[0].offset = tess.shader->vboOffset + 0;
		}

		if ( flags & TESS_RGBA0 ) { // 1
			streams[1].offset = tess.shader->stages[tess.vboStage]->rgb_offset[0];
		}

		if ( flags & TESS_ST0 ) { // 2
			streams[2].offset = tess.shader->stages[tess.vboStage]->tex_offset[0];
		}

		if ( flags & TESS_ST1 ) { // 3
			streams[3].offset = tess.shader->stages[tess.vboStage]->tex_offset[1];
		}

		if ( flags & TESS_ST2 ) { // 4
			streams[4].offset = tess.shader->stages[tess.vboStage]->tex_offset[2];
		}

		if ( flags & TESS_NNN ) { // 5
			streams[5].offset = tess.shader->normalOffset;
		}

		if ( flags & TESS_RGBA1 ) { // 6
			streams[6].offset = tess.shader->stages[tess.vboStage]->rgb_offset[1];
		}

		if ( flags & TESS_RGBA2 ) { // 7
			streams[7].offset = tess.shader->stages[tess.vboStage]->rgb_offset[2];
		}

		RHI_BindVertexStreams( rhiGeometryBuffer_t::World, mask, streams );

	} else
#endif // USE_VBO
	{

		if ( flags & TESS_XYZ ) {
			streams[0].size = tess.numVertexes * sizeof( tess.xyz[0] );
			streams[0].data = &tess.xyz[0];
		}

		if ( flags & TESS_RGBA0 ) {
			streams[1].size = tess.numVertexes * sizeof( color4ub_t );
			streams[1].data = tess.svars.colors[0][0].rgba;
		}

		if ( flags & TESS_ST0 ) {
			streams[2].size = tess.numVertexes * sizeof( vec2_t );
			streams[2].data = tess.svars.texcoordPtr[0];
		}

		if ( flags & TESS_ST1 ) {
			streams[3].size = tess.numVertexes * sizeof( vec2_t );
			streams[3].data = tess.svars.texcoordPtr[1];
		}

		if ( flags & TESS_ST2 ) {
			streams[4].size = tess.numVertexes * sizeof( vec2_t );
			streams[4].data = tess.svars.texcoordPtr[2];
		}

		if ( flags & TESS_NNN ) {
			streams[5].size = tess.numVertexes * sizeof( tess.normal[0] );
			streams[5].data = tess.normal;
		}

		if ( flags & TESS_RGBA1 ) {
			streams[6].size = tess.numVertexes * sizeof( color4ub_t );
			streams[6].data = tess.svars.colors[1][0].rgba;
		}

		if ( flags & TESS_RGBA2 ) {
			streams[7].size = tess.numVertexes * sizeof( color4ub_t );
			streams[7].data = tess.svars.colors[2][0].rgba;
		}

		RHI_BindVertexStreams( rhiGeometryBuffer_t::Frame, mask, streams );
	}
}


void RB_BindLighting( int stage, int bundle ) {
	rhiVertexStream_t streams[RHI_MAX_VERTEX_STREAMS] = {};

#ifdef USE_VBO
	if ( tess.vboIndex ) {

		streams[0].offset = tess.shader->vboOffset + 0;
		streams[1].offset = tess.shader->stages[stage]->tex_offset[bundle];
		streams[2].offset = tess.shader->normalOffset;

		RHI_BindVertexStreams( rhiGeometryBuffer_t::World, 7, streams );

	} else
#endif // USE_VBO
	{

		streams[0].size = tess.numVertexes * sizeof( tess.xyz[0] );
		streams[0].data = &tess.xyz[0];
		streams[1].size = tess.numVertexes * sizeof( vec2_t );
		streams[1].data = tess.svars.texcoordPtr[bundle];
		streams[2].size = tess.numVertexes * sizeof( tess.normal[0] );
		streams[2].data = tess.normal;

		RHI_BindVertexStreams( rhiGeometryBuffer_t::Frame, 7, streams );
	}
}


/*

  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/


/*
==================
R_DrawElements
==================
*/
#ifndef USE_VULKAN
void R_DrawElements( int numIndexes, const glIndex_t *indexes ) {
	qglDrawElements( GL_TRIANGLES, numIndexes, GL_INDEX_TYPE, indexes );
}
#endif


/*
=============================================================

SURFACE SHADERS

=============================================================
*/

shaderCommands_t tess;
#ifndef USE_VULKAN
static qboolean setArraysOnce;
#endif

/*
=================
R_BindAnimatedImage
=================
*/
static void R_BindAnimatedImage( const textureBundle_t *bundle ) {
	int64_t index;
	double v;

	if ( bundle->isVideoMap ) {
		ri.CIN_RunCinematic( bundle->videoMapHandle );
		ri.CIN_UploadCinematic( bundle->videoMapHandle );
		return;
	}

	if ( bundle->isScreenMap /*&& backEnd.viewParms.frameSceneNum == 1*/ ) {
		if ( !backEnd.screenMapDone )
			GL_Bind( tr.blackImage );
		else
			RHI_BindScreenMap( glState.currenttmu + RHI_BINDING_TEXTURE_BASE );
		return;
	}

	if ( bundle->numImageAnimations <= 1 ) {
		GL_Bind( bundle->image[0] );
		return;
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
	//v = tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE;
	//index = v;
	//index >>= FUNCTABLE_SIZE2;

	v = tess.shaderTime * bundle->imageAnimationSpeed; // fix for frameloss bug -EC-
	index = (int64_t)( v );

	if ( index < 0 ) {
		index = 0; // may happen with shader time offsets
	}
	index %= bundle->numImageAnimations;

	GL_Bind( bundle->image[index] );
}


/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris( const shaderCommands_t *input ) {
#ifdef USE_VULKAN
	uint32_t pipeline;

	if ( r_showtris->integer == 1 && backEnd.drawConsole )
		return;

	if ( tess.numIndexes == 0 )
		return;

	if ( r_fastsky->integer && input->shader->isSky )
		return;

#ifdef USE_VBO
	if ( tess.vboIndex ) {
#ifdef USE_PMLIGHT
		if ( tess.dlightPass )
			pipeline = backEnd.viewParms.portalView == PV_MIRROR ? r_pipelines.tris_mirror_debug_red_pipeline : r_pipelines.tris_debug_red_pipeline;
		else
#endif
			pipeline = backEnd.viewParms.portalView == PV_MIRROR ? r_pipelines.tris_mirror_debug_green_pipeline : r_pipelines.tris_debug_green_pipeline;
	} else
#endif
	{
#ifdef USE_PMLIGHT
		if ( tess.dlightPass )
			pipeline = backEnd.viewParms.portalView == PV_MIRROR ? r_pipelines.tris_mirror_debug_red_pipeline : r_pipelines.tris_debug_red_pipeline;
		else
#endif
			pipeline = backEnd.viewParms.portalView == PV_MIRROR ? r_pipelines.tris_mirror_debug_pipeline : r_pipelines.tris_debug_pipeline;
	}

	RB_BindPipeline( pipeline );
	RB_DrawGeometry( DEPTH_RANGE_ZERO, qtrue );

#else
	if ( r_showtris->integer == 1 && backEnd.drawConsole )
		return;

	GL_ClientState( 0, CLS_NONE );
	qglDisable( GL_TEXTURE_2D );

	qglColor4f( 1, 1, 1, 1 );

	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );
	qglDepthRange( 0, 0 );

	qglVertexPointer( 3, GL_FLOAT, sizeof( input->xyz[0] ), input->xyz );

	if ( qglLockArraysEXT ) {
		qglLockArraysEXT( 0, input->numVertexes );
	}

	R_DrawElements( input->numIndexes, input->indexes );

	if ( qglUnlockArraysEXT ) {
		qglUnlockArraysEXT();
	}

	qglEnable( GL_TEXTURE_2D );

	qglDepthRange( 0, 1 );
#endif
}


/*
================
DrawNormals

Draws vertex normals for debugging
================
*/
static void DrawNormals( const shaderCommands_t *input [[maybe_unused]] ) {
	int i;
#ifdef USE_VULKAN
#ifdef USE_VBO
	if ( tess.vboIndex )
		return; // must be handled specially
#endif

	GL_Bind( tr.whiteImage );

	tess.numIndexes = 0;
	for ( i = 0; i < tess.numVertexes; i++ ) {
		( ( tess.xyz[i + tess.numVertexes] )[0] = (float)( ( tess.xyz[i] )[0] + ( tess.normal[i] )[0] * ( 2.0 ) ), ( tess.xyz[i + tess.numVertexes] )[1] = (float)( ( tess.xyz[i] )[1] + ( tess.normal[i] )[1] * ( 2.0 ) ), ( tess.xyz[i + tess.numVertexes] )[2] = (float)( ( tess.xyz[i] )[2] + ( tess.normal[i] )[2] * ( 2.0 ) ) );
		tess.indexes[tess.numIndexes + 0] = i;
		tess.indexes[tess.numIndexes + 1] = i + tess.numVertexes;
		tess.numIndexes += 2;
	}
	tess.numVertexes *= 2;
	Com_Memset( tess.svars.colors[0][0].rgba, tr.identityLightByte, tess.numVertexes * sizeof( color4ub_t ) );

	RB_BindPipeline( r_pipelines.normals_debug_pipeline );
	RB_BindIndex();
	RB_BindGeometry( TESS_XYZ | TESS_ST0 | TESS_RGBA0 );
	RB_DrawGeometry( DEPTH_RANGE_ZERO, qtrue );
#else
	GL_ClientState( 0, CLS_NONE );

	qglDisable( GL_TEXTURE_2D );
	qglColor4f( 1, 1, 1, 1 );

	qglDepthRange( 0, 0 ); // never occluded

	GL_State( GLS_DEPTHMASK_TRUE );

	for ( i = tess.numVertexes - 1; i >= 0; i-- ) {
		VectorMA( tess.xyz[i], 2.0, tess.normal[i], tess.xyz[i * 2 + 1] );
		VectorCopy( tess.xyz[i], tess.xyz[i * 2] );
	}

	qglVertexPointer( 3, GL_FLOAT, sizeof( tess.xyz[0] ), tess.xyz );

	if ( qglLockArraysEXT ) {
		qglLockArraysEXT( 0, tess.numVertexes * 2 );
	}

	qglDrawArrays( GL_LINES, 0, tess.numVertexes * 2 );

	if ( qglUnlockArraysEXT ) {
		qglUnlockArraysEXT();
	}

	qglEnable( GL_TEXTURE_2D );

	qglDepthRange( 0, 1 );
#endif
}


/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface( shader_t *shader, int fogNum ) {

	shader_t *state;

#ifdef USE_VBO
	if ( shader->isStaticShader && !shader->remappedShader && !backEnd.viewParms.shadowView && !backEnd.refdef.numSceneLights && !backEnd.refdef.sun.enabled ) {
		tess.allowVBO = qtrue;
	} else {
		tess.allowVBO = qfalse;
	}
#endif

	if ( shader->remappedShader ) {
		state = shader->remappedShader;
	} else {
		state = shader;
	}

#ifdef USE_PMLIGHT
	if ( tess.fogNum != fogNum ) {
		tess.dlightUpdateParams = qtrue;
	}
#endif

#ifdef USE_TESS_NEEDS_NORMAL
#ifdef USE_PMLIGHT
	tess.needsNormal = backEnd.refdef.numSceneLights || backEnd.refdef.sun.enabled || state->needsNormal || tess.dlightPass || r_shownormals->integer;
#else
	tess.needsNormal = backEnd.refdef.numSceneLights || backEnd.refdef.sun.enabled || state->needsNormal || r_shownormals->integer;
#endif
#endif

#ifdef USE_TESS_NEEDS_ST2
	tess.needsST2 = state->needsST2;
#endif

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.shader = state;
	if ( state->metallicRoughness || backEnd.refdef.numSceneLights || backEnd.refdef.sun.enabled )
		Com_Memset( tess.tangent, 0, sizeof( tess.tangent ) );
	tess.fogNum = fogNum;

#ifdef USE_LEGACY_DLIGHTS
	tess.dlightBits = 0; // will be OR'd in by surface functions
#endif
	tess.xstages = state->stages;
	tess.numPasses = state->numUnfoggedPasses;

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if ( tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime ) {
		tess.shaderTime = tess.shader->clampTime;
	}
}


/*
===================
DrawMultitextured

output = t0 * t1 or t0 + t1

t0 = most upstream according to spec
t1 = most downstream according to spec
===================
*/
#ifndef USE_VULKAN
static void DrawMultitextured( const shaderCommands_t *input, int stage ) {
	const shaderStage_t *pStage;

	pStage = tess.xstages[stage];

	GL_State( pStage->stateBits );

	if ( !setArraysOnce ) {
		R_ComputeColors( 0, tess.svars.colors[0], pStage );
		R_ComputeTexCoords( 0, &pStage->bundle[0] );
		R_ComputeTexCoords( 1, &pStage->bundle[1] );
		GL_ClientState( 0, CLS_TEXCOORD_ARRAY | CLS_COLOR_ARRAY );

		qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoordPtr[0] );
		qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, input->svars.colors[0].rgba );

		GL_ClientState( 1, CLS_TEXCOORD_ARRAY );
		qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoordPtr[1] );
	}

	//
	// base
	//
	GL_SelectTexture( 0 );
	R_BindAnimatedImage( &pStage->bundle[0] );

	//
	// lightmap/secondary pass
	//
	GL_SelectTexture( 1 );
	qglEnable( GL_TEXTURE_2D );
	R_BindAnimatedImage( &pStage->bundle[1] );

	if ( r_lightmap->integer ) {
		GL_TexEnv( GL_REPLACE );
	} else {
		GL_TexEnv( pStage->mtEnv );
	}

	R_DrawElements( input->numIndexes, input->indexes );

	//
	// disable texturing on TEXTURE1, then select TEXTURE0
	//
	//GL_ClientState( 1, CLS_NONE );

	qglDisable( GL_TEXTURE_2D );
	GL_SelectTexture( 0 );
}
#endif


#ifdef USE_LEGACY_DLIGHTS
/*
===================
ProjectDlightTexture

Perform dynamic lighting with another rendering pass
===================
*/
#ifdef USE_VULKAN
static qboolean ProjectDlightTexture( void ) {
#else
static void ProjectDlightTexture( void ) {
#endif
	int i, l;
	vec3_t origin;
	float *texCoords;
	byte *colors;
	byte clipBits[SHADER_MAX_VERTEXES];
#ifdef USE_VULKAN
	uint32_t pipeline;
	qboolean rebindIndex = qfalse;
#else
	float texCoordsArray[SHADER_MAX_VERTEXES][2];
	byte colorArray[SHADER_MAX_VERTEXES][4];
#endif
	glIndex_t hitIndexes[SHADER_MAX_INDEXES];
	int numIndexes;
	float scale;
	float radius;
	float modulate = 0.0f;
	const dlight_t *dl;

	if ( !backEnd.refdef.num_dlights ) {
#ifdef USE_VULKAN
		return rebindIndex;
#else
		return;
#endif
	}

	for ( l = 0; (unsigned int)l < backEnd.refdef.num_dlights; l++ ) {

		if ( !( tess.dlightBits & ( 1 << l ) ) ) {
			continue; // this surface definitely doesn't have any of this light
		}

#ifdef USE_VULKAN
		texCoords = (float *)&tess.svars.texcoords[0][0];
		tess.svars.texcoordPtr[0] = tess.svars.texcoords[0];
		colors = tess.svars.colors[0][0].rgba;
#else
		texCoords = texCoordsArray[0];
		colors = colorArray[0];
#endif

		dl = &backEnd.refdef.dlights[l];
		VectorCopy( dl->transformed, origin );
		radius = dl->radius;
		scale = 1.0f / radius;

		for ( i = 0; i < tess.numVertexes; i++, texCoords += 2, colors += 4 ) {
			int clip = 0;
			vec3_t dist;

			VectorSubtract( origin, tess.xyz[i], dist );

			backEnd.pc.c_dlightVertexes++;

			texCoords[0] = 0.5f + dist[0] * scale;
			texCoords[1] = 0.5f + dist[1] * scale;

			if ( !r_dlightBacks->integer &&
				 // dist . tess.normal[i]
				 ( dist[0] * tess.normal[i][0] +
					 dist[1] * tess.normal[i][1] +
					 dist[2] * tess.normal[i][2] ) < 0.0f ) {
				clip = 63;
			} else {
				if ( texCoords[0] < 0.0f ) {
					clip |= 1;
				} else if ( texCoords[0] > 1.0f ) {
					clip |= 2;
				}
				if ( texCoords[1] < 0.0f ) {
					clip |= 4;
				} else if ( texCoords[1] > 1.0f ) {
					clip |= 8;
				}

				// modulate the strength based on the height and color
				if ( dist[2] > radius ) {
					clip |= 16;
					modulate = 0.0f;
				} else if ( dist[2] < -radius ) {
					clip |= 32;
					modulate = 0.0f;
				} else {
					//*((int*)&dist[2]) &= 0x7FFFFFFF;
					dist[2] = fabsf( dist[2] );
					if ( dist[2] < radius * 0.5f ) {
						modulate = 1.0 * 255.0;
					} else {
						modulate = (float)( 2.0f * ( radius - dist[2] ) * scale * 255.0 );
					}
				}
			}
			clipBits[i] = (unsigned char)( clip );
			colors[0] = (unsigned char)( dl->color[0] * modulate );
			colors[1] = (unsigned char)( dl->color[1] * modulate );
			colors[2] = (unsigned char)( dl->color[2] * modulate );
			colors[3] = 255;
		}

		// build a list of triangles that need light
		numIndexes = 0;
		for ( i = 0; i < tess.numIndexes; i += 3 ) {
			glIndex_t a, b, c;

			a = tess.indexes[i];
			b = tess.indexes[i + 1];
			c = tess.indexes[i + 2];
			if ( clipBits[a] & clipBits[b] & clipBits[c] ) {
				continue; // not lighted
			}
			hitIndexes[numIndexes] = a;
			hitIndexes[numIndexes + 1] = b;
			hitIndexes[numIndexes + 2] = c;
			numIndexes += 3;
		}

		if ( numIndexes == 0 ) {
			continue;
		}

#ifndef USE_VULKAN
		GL_ClientState( 1, CLS_NONE );
		GL_ClientState( 0, CLS_TEXCOORD_ARRAY | CLS_COLOR_ARRAY );

		qglTexCoordPointer( 2, GL_FLOAT, 0, texCoordsArray[0] );
		qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, colorArray );
#endif

		GL_Bind( tr.dlightImage );

#ifdef USE_VULKAN
		if ( numIndexes != tess.numIndexes ) {
			// re-bind index buffer for later fog pass
			rebindIndex = qtrue;
		}
		pipeline = r_pipelines.dlight_pipelines[dl->additive > 0 ? 1 : 0][tess.shader->cullType][tess.shader->polygonOffset];
		RB_BindPipeline( pipeline );
		RHI_BindIndexData( numIndexes, hitIndexes );
		RB_BindGeometry( TESS_RGBA0 | TESS_ST0 );
		RB_DrawGeometry( DEPTH_RANGE_NORMAL, qtrue );
#else
		// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
		// where they aren't rendered

		if ( dl->additive ) {
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
		} else {
			GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
		}

		R_DrawElements( numIndexes, hitIndexes );
#endif
		backEnd.pc.c_totalIndexes += numIndexes;
		backEnd.pc.c_dlightIndexes += numIndexes;
	}

#ifdef USE_VULKAN
	return rebindIndex;
#endif
}

#endif // USE_LEGACY_DLIGHTS

void VK_SetFogParams( shaderUniform_t *uniform, int *fogStage );
static shaderUniform_t uniform;

/*
===================
RB_FogPass

Blends a fog texture on top of everything else
===================
*/
#ifdef USE_VULKAN
static void RB_FogPass( qboolean rebindIndex ) {
	uint32_t pipeline = r_pipelines.fog_pipelines[tess.shader->fogPass - 1][tess.shader->cullType][tess.shader->polygonOffset];
#ifdef USE_FOG_ONLY
	int fog_stage;

	// fog parameters
	RB_BindPipeline( pipeline );
	if ( rebindIndex ) {
		RB_BindIndex();
	}
	VK_SetFogParams( &uniform, &fog_stage );
	RHI_UploadUniform( &uniform, sizeof( uniform ) );
	RHI_BindTexture( RHI_BINDING_FOG_ONLY, &tr.fogImage->texture );
	RB_DrawGeometry( DEPTH_RANGE_NORMAL, qtrue );
#else
	const fog_t *fog = tr.world->fogs + tess.fogNum;
	int i;

	for ( i = 0; i < tess.numVertexes; i++ ) {
		tess.svars.colors[0][i] = fog->colorInt;
	}

	RB_CalcFogTexCoords( (float *)tess.svars.texcoords[0] );
	tess.svars.texcoordPtr[0] = tess.svars.texcoords[0];
	GL_Bind( tr.fogImage );

	RB_BindPipeline( pipeline );
	if ( rebindIndex ) {
		RB_BindIndex();
	}
	RB_BindGeometry( TESS_ST0 | TESS_RGBA0 );
	RB_DrawGeometry( DEPTH_RANGE_NORMAL, qtrue );
#endif
#else
static void RB_FogPass( void ) {
	const fog_t *fog;
	int i;

	RB_CalcFogTexCoords( (float *)tess.svars.texcoords[0] );

	GL_ClientState( 1, CLS_NONE );
	GL_ClientState( 0, CLS_TEXCOORD_ARRAY | CLS_COLOR_ARRAY );

	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors[0].rgba );
	qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[0] );

	GL_SelectTexture( 0 );
	GL_Bind( tr.fogImage );

	if ( tess.shader->fogPass == FP_EQUAL ) {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
	} else {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	}

	R_DrawElements( tess.numIndexes, tess.indexes );
#endif
}


/*
===============
R_ComputeColors
===============
*/
void R_ComputeColors( const int b, color4ub_t *dest, const shaderStage_t *pStage ) {
	int i;

	if ( tess.numVertexes == 0 )
		return;

	//
	// rgbGen
	//
	switch ( pStage->bundle[b].rgbGen ) {
	case CGEN_IDENTITY:
		Com_Memset( dest, 0xff, tess.numVertexes * 4 );
		break;
	default:
	case CGEN_IDENTITY_LIGHTING:
		Com_Memset( dest, tr.identityLightByte, tess.numVertexes * 4 );
		break;
	case CGEN_LIGHTING_DIFFUSE:
		RB_CalcDiffuseColor( (unsigned char *)dest );
		break;
	case CGEN_EXACT_VERTEX:
		Com_Memcpy( dest, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
		break;
	case CGEN_CONST:
		for ( i = 0; i < tess.numVertexes; i++ ) {
			dest[i] = pStage->bundle[b].constantColor;
		}
		break;
	case CGEN_VERTEX:
		if ( tr.identityLight == 1 ) {
			Com_Memcpy( dest, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
		} else {
			for ( i = 0; i < tess.numVertexes; i++ ) {
				dest[i].rgba[0] = (unsigned char)( tess.vertexColors[i].rgba[0] * tr.identityLight );
				dest[i].rgba[1] = (unsigned char)( tess.vertexColors[i].rgba[1] * tr.identityLight );
				dest[i].rgba[2] = (unsigned char)( tess.vertexColors[i].rgba[2] * tr.identityLight );
				dest[i].rgba[3] = tess.vertexColors[i].rgba[3];
			}
		}
		break;
	case CGEN_ONE_MINUS_VERTEX:
		if ( tr.identityLight == 1 ) {
			for ( i = 0; i < tess.numVertexes; i++ ) {
				dest[i].rgba[0] = 255 - tess.vertexColors[i].rgba[0];
				dest[i].rgba[1] = 255 - tess.vertexColors[i].rgba[1];
				dest[i].rgba[2] = 255 - tess.vertexColors[i].rgba[2];
			}
		} else {
			for ( i = 0; i < tess.numVertexes; i++ ) {
				dest[i].rgba[0] = (unsigned char)( ( 255 - tess.vertexColors[i].rgba[0] ) * tr.identityLight );
				dest[i].rgba[1] = (unsigned char)( ( 255 - tess.vertexColors[i].rgba[1] ) * tr.identityLight );
				dest[i].rgba[2] = (unsigned char)( ( 255 - tess.vertexColors[i].rgba[2] ) * tr.identityLight );
			}
		}
		break;
	case CGEN_FOG: {
		const fog_t *fog = tr.world->fogs + tess.fogNum;

		for ( i = 0; i < tess.numVertexes; i++ ) {
			dest[i] = fog->colorInt;
		}
	} break;
	case CGEN_WAVEFORM:
		RB_CalcWaveColor( &pStage->bundle[b].rgbWave, dest->rgba );
		break;
	case CGEN_ENTITY:
		RB_CalcColorFromEntity( dest->rgba );
		break;
	case CGEN_ONE_MINUS_ENTITY:
		RB_CalcColorFromOneMinusEntity( dest->rgba );
		break;
	}

	//
	// alphaGen
	//
	switch ( pStage->bundle[b].alphaGen ) {
	case AGEN_SKIP:
		break;
	case AGEN_IDENTITY:
		if ( ( pStage->bundle[b].rgbGen == CGEN_VERTEX && tr.identityLight != 1 ) ||
			 pStage->bundle[b].rgbGen != CGEN_VERTEX ) {
			for ( i = 0; i < tess.numVertexes; i++ ) {
				dest[i].rgba[3] = 255;
			}
		}
		break;
	case AGEN_CONST:
		for ( i = 0; i < tess.numVertexes; i++ ) {
			dest[i].rgba[3] = pStage->bundle[b].constantColor.rgba[3];
		}
		break;
	case AGEN_WAVEFORM:
		RB_CalcWaveAlpha( &pStage->bundle[b].alphaWave, dest->rgba );
		break;
	case AGEN_LIGHTING_SPECULAR:
		RB_CalcSpecularAlpha( dest->rgba );
		break;
	case AGEN_ENTITY:
		RB_CalcAlphaFromEntity( dest->rgba );
		break;
	case AGEN_ONE_MINUS_ENTITY:
		RB_CalcAlphaFromOneMinusEntity( dest->rgba );
		break;
	case AGEN_VERTEX:
		for ( i = 0; i < tess.numVertexes; i++ ) {
			dest[i].rgba[3] = tess.vertexColors[i].rgba[3];
		}
		break;
	case AGEN_ONE_MINUS_VERTEX:
		for ( i = 0; i < tess.numVertexes; i++ ) {
			dest[i].rgba[3] = 255 - tess.vertexColors[i].rgba[3];
		}
		break;
	case AGEN_PORTAL: {
		for ( i = 0; i < tess.numVertexes; i++ ) {
			unsigned char alpha;
			float len;
			vec3_t v;

			VectorSubtract( tess.xyz[i], backEnd.viewParms.orientation.origin, v );
			len = VectorLength( v ) * tess.shader->portalRangeR;

			if ( len > 1 ) {
				alpha = 0xff;
			} else {
				alpha = (unsigned char)( len * 0xff );
			}

			dest[i].rgba[3] = alpha;
		}
	} break;
	}

	//
	// fog adjustment for colors to fade out as fog increases
	//
	if ( tess.fogNum ) {
		switch ( pStage->bundle[b].adjustColorsForFog ) {
		case ACFF_MODULATE_RGB:
			RB_CalcModulateColorsByFog( dest->rgba );
			break;
		case ACFF_MODULATE_ALPHA:
			RB_CalcModulateAlphasByFog( dest->rgba );
			break;
		case ACFF_MODULATE_RGBA:
			RB_CalcModulateRGBAsByFog( dest->rgba );
			break;
		case ACFF_NONE:
			break;
		}
	}
}


/*
===============
R_ComputeTexCoords
===============
*/
void R_ComputeTexCoords( const int b, const textureBundle_t *bundle ) {
	int i;
	int tm;
	vec2_t *src, *dst;

	if ( !tess.numVertexes )
		return;

	src = dst = tess.svars.texcoords[b];

	//
	// generate the texture coordinates
	//
	switch ( bundle->tcGen ) {
	case TCGEN_IDENTITY:
		src = tess.texCoords00;
		break;
	case TCGEN_TEXTURE:
		src = tess.texCoords[0];
		break;
	case TCGEN_LIGHTMAP:
		src = tess.texCoords[1];
		break;
	case TCGEN_VECTOR:
		for ( i = 0; i < tess.numVertexes; i++ ) {
			dst[i][0] = DotProduct( tess.xyz[i], bundle->tcGenVectors[0] );
			dst[i][1] = DotProduct( tess.xyz[i], bundle->tcGenVectors[1] );
		}
		break;
	case TCGEN_FOG:
		RB_CalcFogTexCoords( (float *)dst );
		break;
	case TCGEN_ENVIRONMENT_MAPPED:
		RB_CalcEnvironmentTexCoords( (float *)dst );
		break;
	case TCGEN_ENVIRONMENT_MAPPED_FP:
		RB_CalcEnvironmentTexCoordsFP( (float *)dst, bundle->isScreenMap );
		break;
	case TCGEN_BAD:
		return;
	}

	//
	// alter texture coordinates
	//
	for ( tm = 0; tm < bundle->numTexMods; tm++ ) {
		switch ( bundle->texMods[tm].type ) {
		case TMOD_NONE:
			tm = TR_MAX_TEXMODS; // break out of for loop
			break;

		case TMOD_TURBULENT:
			RB_CalcTurbulentTexCoords( &bundle->texMods[tm].wave, (float *)src, (float *)dst );
			src = dst;
			break;

		case TMOD_ENTITY_TRANSLATE:
			RB_CalcScrollTexCoords( backEnd.currentEntity->e.shaderTexCoord, (float *)src, (float *)dst );
			src = dst;
			break;

		case TMOD_SCROLL:
			RB_CalcScrollTexCoords( bundle->texMods[tm].scroll, (float *)src, (float *)dst );
			src = dst;
			break;

		case TMOD_SCALE:
			RB_CalcScaleTexCoords( bundle->texMods[tm].scaleOffset.scale, (float *)src, (float *)dst );
			src = dst;
			break;

		case TMOD_OFFSET:
			for ( i = 0; i < tess.numVertexes; i++ ) {
				dst[i][0] = src[i][0] + bundle->texMods[tm].scaleOffset.offset[0];
				dst[i][1] = src[i][1] + bundle->texMods[tm].scaleOffset.offset[1];
			}
			src = dst;
			break;

		case TMOD_SCALE_OFFSET:
			for ( i = 0; i < tess.numVertexes; i++ ) {
				dst[i][0] = ( src[i][0] * bundle->texMods[tm].scaleOffset.scale[0] ) + bundle->texMods[tm].scaleOffset.offset[0];
				dst[i][1] = ( src[i][1] * bundle->texMods[tm].scaleOffset.scale[1] ) + bundle->texMods[tm].scaleOffset.offset[1];
			}
			src = dst;
			break;

		case TMOD_OFFSET_SCALE:
			for ( i = 0; i < tess.numVertexes; i++ ) {
				dst[i][0] = ( src[i][0] + bundle->texMods[tm].scaleOffset.offset[0] ) * bundle->texMods[tm].scaleOffset.scale[0];
				dst[i][1] = ( src[i][1] + bundle->texMods[tm].scaleOffset.offset[1] ) * bundle->texMods[tm].scaleOffset.scale[1];
			}
			src = dst;
			break;

		case TMOD_STRETCH:
			RB_CalcStretchTexCoords( &bundle->texMods[tm].wave, (float *)src, (float *)dst );
			src = dst;
			break;

		case TMOD_TRANSFORM:
			RB_CalcTransformTexCoords( &bundle->texMods[tm], (float *)src, (float *)dst );
			src = dst;
			break;

		case TMOD_ROTATE:
			RB_CalcRotateTexCoords( bundle->texMods[tm].rotateSpeed, (float *)src, (float *)dst );
			src = dst;
			break;

		default:
			ri.Error( ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'", bundle->texMods[tm].type, tess.shader->name );
			break;
		}
	}

	tess.svars.texcoordPtr[b] = src;
}


/*
** RB_IterateStagesGeneric
*/
#ifdef USE_VULKAN
static void RB_IterateStagesGeneric( const shaderCommands_t *input, qboolean fogCollapse )
#else
static void RB_IterateStagesGeneric( const shaderCommands_t *input )
#endif
{
	const shaderStage_t *pStage;
	int tess_flags;
	int stage, i;

#ifdef USE_VULKAN
	uint32_t pipeline;
	int fog_stage;
	qboolean pushUniform;

	RB_BindIndex();

	tess_flags = input->shader->tessFlags;

	pushUniform = qfalse;

#ifdef USE_FOG_COLLAPSE
	if ( fogCollapse ) {
		VK_SetFogParams( &uniform, &fog_stage );
		VectorCopy( backEnd.orientation.viewOrigin, uniform.eyePos );
		RHI_BindTexture( RHI_BINDING_FOG_COLLAPSE, &tr.fogImage->texture );
		pushUniform = qtrue;
	} else
#endif
	{
		fog_stage = 0;
		if ( tess_flags & TESS_VPOS ) {
			VectorCopy( backEnd.orientation.viewOrigin, uniform.eyePos );
			tess_flags &= ~TESS_VPOS;
			pushUniform = qtrue;
		}
	}
#endif // USE_VULKAN

	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ ) {
		pStage = tess.xstages[stage];
		if ( !pStage )
			break;

#ifdef USE_VBO
		tess.vboStage = stage;
#endif

#ifdef USE_VULKAN
		tess_flags |= pStage->tessFlags;

		for ( i = 0; (uint32_t)i < pStage->numTexBundles; i++ ) {
			if ( pStage->bundle[i].image[0] != NULL ) {
				GL_SelectTexture( i );
				R_BindAnimatedImage( &pStage->bundle[i] );
				if ( tess_flags & ( TESS_ST0 << i ) ) {
					R_ComputeTexCoords( i, &pStage->bundle[i] );
				}
				if ( tess_flags & ( TESS_RGBA0 << i ) ) {
					R_ComputeColors( i, tess.svars.colors[i], pStage );
				}
				if ( tess_flags & ( TESS_ENT0 << i ) && backEnd.currentEntity ) {
					uniform.ent.color[i][0] = (float)( backEnd.currentEntity->e.shader.rgba[0] / 255.0 );
					uniform.ent.color[i][1] = (float)( backEnd.currentEntity->e.shader.rgba[1] / 255.0 );
					uniform.ent.color[i][2] = (float)( backEnd.currentEntity->e.shader.rgba[2] / 255.0 );
					uniform.ent.color[i][3] = (float)( pStage->bundle[i].alphaGen == AGEN_IDENTITY ? 1.0 : ( backEnd.currentEntity->e.shader.rgba[3] / 255.0 ) );
					pushUniform = qtrue;
				}
			}
		}

		if ( pushUniform ) {
			pushUniform = qfalse;
			RHI_UploadUniform( &uniform, sizeof( uniform ) );
		}

		GL_SelectTexture( 0 );

		if ( r_lightmap->integer && pStage->bundle[1].lightmap != LIGHTMAP_INDEX_NONE ) {
			//GL_SelectTexture( 0 );
			GL_Bind( tr.whiteImage ); // replace diffuse texture with a white one thus effectively render only lightmap
		}

		if ( backEnd.viewParms.portalView == PV_MIRROR ) {
			pipeline = pStage->vk_mirror_pipeline[fog_stage];
		} else {
			pipeline = pStage->vk_pipeline[fog_stage];
		}

		RB_BindPipeline( pipeline );
		RB_BindGeometry( tess_flags );
		RB_DrawGeometry( tess.depthRange, qtrue );

		if ( pStage->depthFragment ) {
			if ( backEnd.viewParms.portalView == PV_MIRROR )
				pipeline = pStage->vk_mirror_pipeline_df;
			else
				pipeline = pStage->vk_pipeline_df;
			RB_BindPipeline( pipeline );
			RB_DrawGeometry( tess.depthRange, qtrue );
		}
#else
		R_ComputeColors( 0, tess.svars.colors[0].rgba, pStage );

		R_ComputeTexCoords( 0, &pStage->bundle[0] );

		//
		// do multitexture
		//
		if ( pStage->bundle[1].image[0] != NULL ) {
			DrawMultitextured( input, stage );
		} else {
			if ( !setArraysOnce ) {
				R_ComputeTexCoords( 0, &pStage->bundle[0] );
				R_ComputeColors( 0, tess.svars.colors[0], pStage );

				GL_ClientState( 1, CLS_NONE );
				GL_ClientState( 0, CLS_TEXCOORD_ARRAY | CLS_COLOR_ARRAY );

				qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoordPtr[0] );
				qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, input->svars.colors[0].rgba );
			}

			//
			// set state
			//
			R_BindAnimatedImage( &pStage->bundle[0] );

			GL_State( pStage->stateBits );

			//
			// draw
			//
			R_DrawElements( input->numIndexes, input->indexes );
		}
#endif

		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer && ( pStage->bundle[0].lightmap != LIGHTMAP_INDEX_NONE || pStage->bundle[1].lightmap != LIGHTMAP_INDEX_NONE ) )
			break;

		tess_flags = 0;
	}

#ifdef USE_VULKAN
	if ( pushUniform ) {
		RHI_UploadUniform( &uniform, sizeof( uniform ) );
	}
	if ( tess_flags ) // fog-only shaders?
		RB_BindGeometry( tess_flags );
#endif
}


#ifdef USE_VULKAN

void VK_SetFogParams( shaderUniform_t *params, int *fogStage ) {
	if ( tess.fogNum && tess.shader->fogPass ) {
		const fogProgramParms_t *fp = RB_CalcFogProgramParms();
		// vertex data
		Vector4Copy( fp->fogDistanceVector, params->fogDistanceVector );
		Vector4Copy( fp->fogDepthVector, params->fogDepthVector );
		params->fogEyeT[0] = fp->eyeT;
		if ( fp->eyeOutside ) {
			params->fogEyeT[1] = 0.0; // fog eye out
		} else {
			params->fogEyeT[1] = 1.0; // fog eye in
		}
		// fragment data
		Vector4Copy( fp->fogColor, params->fogColor );
		*fogStage = 1;
	} else {
		*fogStage = 0;
	}
}


#ifdef USE_PMLIGHT
static void VK_SetLightParams( shaderUniform_t *params, const dlight_t *dl ) {
	float radius;

#ifdef USE_VULKAN
	if ( !glConfig.deviceSupportsGamma && !RHI_GetCapabilities().fboActive )
#else
	if ( !glConfig.deviceSupportsGamma )
#endif
		VectorScale( dl->color, 2 * powf( r_intensity->value, r_gamma->value ), params->light.color );
	else
		VectorCopy( dl->color, params->light.color );

	radius = dl->radius;

	// vertex data
	VectorCopy( backEnd.orientation.viewOrigin, params->eyePos );
	params->eyePos[3] = 0.0f;
	VectorCopy( dl->transformed, params->light.pos );
	params->light.pos[3] = 0.0f;

	// fragment data
	params->light.color[3] = 1.0f / Square( radius );

	if ( dl->linear ) {
		vec4_t ab;
		VectorSubtract( dl->transformed2, dl->transformed, ab );
		ab[3] = 1.0f / DotProduct( ab, ab );
		Vector4Copy( ab, params->light.vector );
	}
}
#endif


#ifdef USE_PMLIGHT
void VK_LightingPass( void ) {
	static uint32_t uniform_offset;
	static int fog_stage;
	uint32_t pipeline;
	const shaderStage_t *pStage;
	cullType_t cull;
	int abs_light;

	if ( tess.shader->lightingStage < 0 )
		return;

	pStage = tess.xstages[tess.shader->lightingStage];

	// we may need to update programs for fog transitions
	if ( tess.dlightUpdateParams ) {

		// fog parameters
		VK_SetFogParams( &uniform, &fog_stage );
		// light parameters
		VK_SetLightParams( &uniform, tess.light );

		uniform_offset = RHI_UploadUniform( &uniform, sizeof( uniform ) );

		tess.dlightUpdateParams = qfalse;
	}

	if ( uniform_offset == (uint32_t)( ~0 ) )
		return; // no space left...

	cull = tess.shader->cullType;
	if ( backEnd.viewParms.portalView == PV_MIRROR ) {
		switch ( cull ) {
		case CT_FRONT_SIDED:
			cull = CT_BACK_SIDED;
			break;
		case CT_BACK_SIDED:
			cull = CT_FRONT_SIDED;
			break;
		default:
			break;
		}
	}

	abs_light = /* (pStage->stateBits & GLS_ATEST_BITS) && */ ( cull == CT_TWO_SIDED ) ? 1 : 0;

	if ( fog_stage )
		RHI_BindTexture( RHI_BINDING_FOG_DLIGHT, &tr.fogImage->texture );

	if ( tess.light->linear )
		pipeline = r_pipelines.dlight1_pipelines_x[cull][tess.shader->polygonOffset][fog_stage][abs_light];
	else
		pipeline = r_pipelines.dlight_pipelines_x[cull][tess.shader->polygonOffset][fog_stage][abs_light];

	GL_SelectTexture( 0 );
	R_BindAnimatedImage( &pStage->bundle[tess.shader->lightingBundle] );

#ifdef USE_VBO
	if ( tess.vboIndex == 0 )
#endif
	{
		R_ComputeTexCoords( tess.shader->lightingBundle, &pStage->bundle[tess.shader->lightingBundle] );
	}

	RB_BindPipeline( pipeline );
	RB_BindIndex();
	RB_BindLighting( tess.shader->lightingStage, tess.shader->lightingBundle );
	RB_DrawGeometry( tess.depthRange, qtrue );
}
#endif // USE_PMLIGHT


// Matches direct.glsl; all data lives in the existing frame uniform arena.
struct directUniform_t {
	float model[16], shadowMatrix[6][16];
	vec4_t eye, lightPosition, lightDirection, lightColor;
	vec4_t lightShape, shadowSettings, splits, viewForward;
	vec4_t color, surface, metal;
};
static_assert( sizeof( directUniform_t ) == 624 );
static_assert( offsetof( directUniform_t, eye ) == 448 );

static void RB_DirectLights( void ) {
	if ( ( !backEnd.refdef.numSceneLights && !backEnd.refdef.sun.enabled ) || tess.shader->isSky ||
		 tess.shader->sort > (float)SS_SEE_THROUGH || ( !tess.shader->metallicRoughness && ( tess.shader->surfaceFlags & SURF_NODLIGHT ) ) )
		return;
	directUniform_t params = {};
	for ( int axis = 0; axis < 3; ++axis )
		VectorCopy( backEnd.orientation.axis[axis], &params.model[axis * 4] );
	VectorCopy( backEnd.orientation.origin, &params.model[12] );
	params.model[15] = 1;
	VectorCopy( backEnd.viewParms.orientation.origin, params.eye );
	VectorCopy( backEnd.viewParms.orientation.axis[0], params.viewForward );
	Vector4Set( params.color, 1, 1, 1, 1 );
	params.shadowSettings[0] = (float)r_shadowOcclusion->integer;
	params.shadowSettings[1] = r_shadowBias->value;
	tess.svars.texcoordPtr[0] = tess.texCoords[0];
	if ( tess.shader->metallicRoughness ) {
		materialParams_t material;
		if ( !R_ResolveMaterialParams( &tess.shader->materialParams, &backEnd.currentEntity->materialOverride, &material ) || ( material.flags & ( 2 | 4 ) ) )
			return;
		Vector4Copy( material.color, params.color );
		VectorSet( params.surface, material.roughness, material.normalScale, material.alphaCutoff );
		params.metal[0] = material.metallic;
		params.metal[1] = 1;
		for ( int i = 0; i < 3; ++i )
			RHI_BindTexture( RHI_BINDING_TEXTURE0 + i, &tess.xstages[0]->bundle[i].image[0]->texture );
	} else {
		const textureBundle_t *diffuse = nullptr;
		for ( int stage = 0; stage < MAX_SHADER_STAGES && tess.xstages[stage] && !diffuse; ++stage )
			for ( uint32_t bundle = 0; bundle < tess.xstages[stage]->numTexBundles; ++bundle )
				if ( tess.xstages[stage]->bundle[bundle].image[0] && tess.xstages[stage]->bundle[bundle].lightmap == LIGHTMAP_INDEX_NONE ) {
					diffuse = &tess.xstages[stage]->bundle[bundle];
					break;
				}
		if ( !diffuse )
			return;
		GL_SelectTexture( 0 );
		R_BindAnimatedImage( diffuse );
		R_ComputeTexCoords( 0, diffuse );
		RHI_BindTexture( RHI_BINDING_TEXTURE0 + 1, &tr.whiteImage->texture );
		RHI_BindTexture( RHI_BINDING_TEXTURE0 + 2, &tr.whiteImage->texture );
	}
	RB_BindPipeline( r_pipelines.directLight[tess.shader->cullType][backEnd.viewParms.portalView == PV_MIRROR][tess.shader->polygonOffset != 0] );
	RB_BindIndex();
	RB_BindGeometry( TESS_XYZ | TESS_ST0 | TESS_NNN );
	rhiVertexStream_t streams[RHI_MAX_VERTEX_STREAMS] = {};
	streams[6].data = tess.tangent;
	streams[6].size = tess.numVertexes * sizeof( tess.tangent[0] );
	RHI_BindVertexStreams( rhiGeometryBuffer_t::Frame, 1U << 6, streams );
	// ponytail: bounded forward draws (16 local tiles + sun); clustered culling
	// is only needed if the measured reference scene exceeds its GPU budget.
	for ( int index = 0; index < backEnd.refdef.numSceneLights + ( backEnd.refdef.sun.enabled ? 1 : 0 ); ++index ) {
		const bool sun = index == backEnd.refdef.numSceneLights;
		if ( !RHI_BindShadowAtlas( sun ? 1u : 0u, RHI_BINDING_BAKED_LIGHT ) )
			continue;
		if ( sun ) {
			memcpy( params.shadowMatrix, backEnd.refdef.sun.matrices, sizeof( backEnd.refdef.sun.matrices ) );
			Vector4Copy( backEnd.refdef.sun.splits, params.splits );
			VectorCopy( tr.sunDirection, params.lightDirection );
			Vector4Set( params.lightPosition, 0, 0, 0, 1 );
			Vector4Set( params.lightColor, 1, 1, 1, r_shadowSun->value );
			Vector4Set( params.lightShape, 2, 0, 0, 0 );
		} else {
			const auto &light = backEnd.refdef.sceneLights[index];
			memcpy( params.shadowMatrix, light.matrices, sizeof( params.shadowMatrix ) );
			VectorCopy( light.light.origin, params.lightPosition );
			params.lightPosition[3] = light.light.radius;
			VectorCopy( light.light.direction, params.lightDirection );
			params.lightDirection[3] = cosf( DEG2RAD( light.light.outerCone ) );
			VectorCopy( light.light.color, params.lightColor );
			params.lightColor[3] = light.light.intensity;
			params.lightShape[0] = light.light.type == sceneLightType_t::Point ? 0.0f : 1.0f;
			params.lightShape[1] = cosf( DEG2RAD( light.light.innerCone ) );
			params.lightShape[2] = (float)light.firstTile;
		}
		if ( RHI_UploadUniform( &params, sizeof( params ) ) != RHI_INVALID_OFFSET )
			RB_DrawGeometry( tess.depthRange, qtrue );
	}
}

static void RB_StageIteratorShadow( void ) {
	if ( tess.shader->isSky || tess.shader->sort > (float)SS_SEE_THROUGH )
		return;
	RB_DeformTessGeometry();
	vec4_t mask = { 1, 0.5f, 0, 0 };
	const shaderStage_t *masked = nullptr;
	if ( tess.shader->metallicRoughness ) {
		materialParams_t material;
		if ( !R_ResolveMaterialParams( &tess.shader->materialParams, &backEnd.currentEntity->materialOverride, &material ) || ( material.flags & 4 ) )
			return;
		if ( material.flags & 8 ) {
			masked = tess.xstages[0];
			mask[0] = material.color[3];
			mask[1] = material.alphaCutoff;
			mask[2] = 1;
		}
	} else {
		for ( int stage = 0; stage < MAX_SHADER_STAGES && tess.xstages[stage]; ++stage ) {
			const uint32_t mode = tess.xstages[stage]->stateBits & GLS_ATEST_BITS;
			if ( mode ) {
				masked = tess.xstages[stage];
				mask[2] = mode == GLS_ATEST_GT_0 ? 3.0f : mode == GLS_ATEST_LT_80 ? 2.0f
																				  : 1.0f;
				break;
			}
		}
	}
	memset( tess.svars.colors[0], 255, tess.numVertexes * sizeof( color4ub_t ) );
	tess.svars.texcoordPtr[0] = tess.texCoords[0];
	GL_SelectTexture( 0 );
	if ( masked ) {
		R_BindAnimatedImage( &masked->bundle[0] );
		R_ComputeTexCoords( 0, &masked->bundle[0] );
		if ( !tess.shader->metallicRoughness )
			R_ComputeColors( 0, tess.svars.colors[0], masked );
	} else {
		GL_Bind( tr.whiteImage );
	}
	if ( RHI_UploadUniform( mask, sizeof( mask ) ) == RHI_INVALID_OFFSET )
		return;
	RB_BindPipeline( r_pipelines.shadowCaster[tess.shader->cullType] );
	RB_BindIndex();
	RB_BindGeometry( TESS_XYZ | TESS_ST0 | TESS_RGBA0 );
	RB_DrawGeometry( DEPTH_RANGE_NORMAL, qtrue );
}

void RB_StageIteratorPbr( void ) {
	RB_DeformTessGeometry();
	materialParams_t material;
	if ( !R_ResolveMaterialParams( &tess.shader->materialParams, &backEnd.currentEntity->materialOverride, &material ) )
		return;
	pbrUniform_t params = {};
	VectorCopy( backEnd.orientation.viewOrigin, params.eye );
	vec3_t ambient, directed, direction;
	if ( backEnd.currentEntity == &tr.worldEntity ) {
		vec3_t point = {};
		for ( int i = 0; i < tess.numVertexes; ++i )
			VectorAdd( point, tess.xyz[i], point );
		VectorScale( point, 1.0f / (float)MAX( tess.numVertexes, 1 ), point );
		if ( !R_LightForPoint( point, ambient, directed, direction ) ) {
			VectorSet( ambient, 150, 150, 150 );
			VectorSet( directed, 150, 150, 150 );
			VectorCopy( tr.sunDirection, direction );
		}
	} else {
		VectorCopy( backEnd.currentEntity->ambientLight, ambient );
		VectorCopy( backEnd.currentEntity->directedLight, directed );
		VectorCopy( backEnd.currentEntity->lightDir, direction );
	}
	VectorScale( ambient, 1.0f / 255.0f, params.ambient );
	VectorScale( directed, 1.0f / 255.0f, params.directed );
	VectorCopy( direction, params.lightDirection );
	const bool baked = tess.shader->lightmapIndex >= 0 && tr.bakedLightmaps;
	params.directed[3] = (float)r_directionalLightmaps->integer;
	memcpy( params.color, material.color, sizeof( params.color ) );
	VectorCopy( material.emissive, params.emissiveMetallic );
	params.emissiveMetallic[3] = material.metallic;
	VectorSet( params.surface, material.roughness, material.normalScale, material.alphaCutoff );
	params.surface[3] = (float)material.flags;
	if ( RHI_UploadUniform( &params, sizeof( params ) ) == RHI_INVALID_OFFSET )
		return;
	const shaderStage_t *stage = tess.xstages[0];
	for ( int i = 0; i < 3; ++i )
		RHI_BindTexture( RHI_BINDING_TEXTURE0 + i, &stage->bundle[i].image[0]->texture );
	if ( baked )
		RHI_BindTexture( RHI_BINDING_BAKED_LIGHT, &tr.bakedLightmaps[tess.shader->lightmapIndex]->texture );
	RB_BindPipeline( backEnd.viewParms.portalView == PV_MIRROR ? stage->vk_mirror_pipeline[0] : stage->vk_pipeline[0] );
	tess.svars.texcoordPtr[0] = tess.texCoords[0];
	tess.svars.texcoordPtr[1] = tess.texCoords[1];
	RB_BindIndex();
	RB_BindGeometry( TESS_XYZ | TESS_ST0 | TESS_NNN | ( baked ? TESS_ST1 : 0 ) );
	rhiVertexStream_t streams[RHI_MAX_VERTEX_STREAMS] = {};
	streams[6].data = tess.tangent;
	streams[6].size = tess.numVertexes * sizeof( tess.tangent[0] );
	RHI_BindVertexStreams( rhiGeometryBuffer_t::Frame, 1U << 6, streams );
	RB_DrawGeometry( tess.depthRange, qtrue );
	RB_DirectLights();
	if ( tess.fogNum && tess.shader->fogPass )
		RB_FogPass( qfalse );
}

void RB_StageIteratorGeneric( void ) {
#ifdef USE_VULKAN
	qboolean rebindIndex = qfalse;
#endif
	qboolean fogCollapse = qfalse;

#ifdef USE_VBO
	if ( tess.vboIndex != 0 ) {
		VBO_PrepareQueues();
		tess.vboStage = 0;
	} else
#endif
		RB_DeformTessGeometry();

#ifdef USE_PMLIGHT
	if ( tess.dlightPass ) {
		VK_LightingPass();
		return;
	}
#endif

#ifdef USE_FOG_COLLAPSE
	fogCollapse = (qboolean)( tess.fogNum && tess.shader->fogPass && tess.shader->fogCollapse && !backEnd.refdef.numSceneLights && !backEnd.refdef.sun.enabled );
#endif

	// call shader function
	RB_IterateStagesGeneric( &tess, fogCollapse );

	// now do any dynamic lighting needed
#ifdef USE_LEGACY_DLIGHTS
#ifdef USE_PMLIGHT
	if ( r_dlightMode->integer == 0 )
#endif
		if ( tess.dlightBits && tess.shader->sort <= (float)SS_OPAQUE && !( tess.shader->surfaceFlags & ( SURF_NODLIGHT | SURF_SKY ) ) ) {
			if ( !fogCollapse ) {
#ifdef USE_VULKAN
				rebindIndex = ProjectDlightTexture();
#else
			ProjectDlightTexture();
#endif
			}
		}
#endif // USE_LEGACY_DLIGHTS

	RB_DirectLights();

	// now do fog
	if ( tess.fogNum && tess.shader->fogPass && !fogCollapse ) {
#ifdef USE_VULKAN
		RB_FogPass( rebindIndex );
#else
		RB_FogPass();
#endif
	}
}

#else

/*
** RB_StageIteratorGeneric
*/
void RB_StageIteratorGeneric( void ) {
	const shaderCommands_t *input;
	shader_t *shader;

	RB_DeformTessGeometry();

	input = &tess;
	shader = input->shader;

	//
	// set face culling appropriately
	//
	GL_Cull( shader->cullType );

	// set polygon offset if necessary
	if ( shader->polygonOffset ) {
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
	}

	//
	// if there is only a single pass then we can enable color
	// and texture arrays before we compile, otherwise we need
	// to avoid compiling those arrays since they will change
	// during multipass rendering
	//
	if ( tess.numPasses > 1 ) {
		setArraysOnce = qfalse;

		GL_ClientState( 1, CLS_NONE );
		GL_ClientState( 0, CLS_NONE );
	} else {
		// FIXME: we can't do that if going to lighting/fog later?
		setArraysOnce = qtrue;

		GL_ClientState( 0, CLS_COLOR_ARRAY | CLS_TEXCOORD_ARRAY );

		if ( tess.xstages[0] ) {
			R_ComputeColors( 0, tess.svars.colors, tess.xstages[0] );
			qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors[0].rgba );
			R_ComputeTexCoords( 0, &tess.xstages[0]->bundle[0] );
			qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoordPtr[0] );
			if ( shader->multitextureEnv ) {
				GL_ClientState( 1, CLS_TEXCOORD_ARRAY );
				R_ComputeTexCoords( 1, &tess.xstages[0]->bundle[1] );
				qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoordPtr[1] );
			} else {
				GL_ClientState( 1, CLS_NONE );
			}
		}
	}

	qglVertexPointer( 3, GL_FLOAT, sizeof( input->xyz[0] ), input->xyz ); // padded for SIMD

	//
	// lock XYZ
	//
	if ( qglLockArraysEXT ) {
		qglLockArraysEXT( 0, input->numVertexes );
	}

	//
	// call shader function
	//
	RB_IterateStagesGeneric( input );

	//
	// now do any dynamic lighting needed
	//
	if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE && !( tess.shader->surfaceFlags & ( SURF_NODLIGHT | SURF_SKY ) ) ) {
		ProjectDlightTexture();
	}

	//
	// now do fog
	//
	if ( tess.fogNum && tess.shader->fogPass ) {
		RB_FogPass();
	}

	//
	// unlock arrays
	//
	if ( qglUnlockArraysEXT ) {
		qglUnlockArraysEXT();
	}

	GL_ClientState( 1, CLS_NONE );

	//
	// reset polygon offset
	//
	if ( shader->polygonOffset ) {
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}
}
#endif // !USE_VULKAN


/*
** RB_EndSurface
*/
void RB_EndSurface( void ) {
	const shaderCommands_t *input;

	input = &tess;

	if ( input->numIndexes == 0 ) {
		//VBO_UnBind();
		return;
	}

	if ( input->numIndexes > SHADER_MAX_INDEXES ) {
		ri.Error( ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit" );
	}

	if ( input->numVertexes > SHADER_MAX_VERTEXES ) {
		ri.Error( ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit" );
	}

	if ( tess.shader == tr.shadowShader ) {
		RB_ShadowTessEnd();
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if ( r_debugSort->integer && r_debugSort->integer < tess.shader->sort && !backEnd.doneSurfaces ) {
#ifdef USE_VBO
		tess.vboIndex = 0; //VBO_UnBind();
#endif
		return;
	}

	//
	// update performance counters
	//
#ifdef USE_PMLIGHT
	if ( tess.dlightPass ) {
		backEnd.pc.c_lit_batches++;
		backEnd.pc.c_lit_vertices += tess.numVertexes;
		backEnd.pc.c_lit_indices += tess.numIndexes;
	} else
#endif
	{
		backEnd.pc.c_shaders++;
		backEnd.pc.c_vertexes += tess.numVertexes;
		backEnd.pc.c_indexes += tess.numIndexes;
	}
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	//
	// call off to shader specific tess end function
	//
	if ( backEnd.viewParms.shadowView )
		RB_StageIteratorShadow();
	else
		tess.shader->optimalStageIteratorFunc();

	//
	// draw debugging stuff
	//
	if ( !backEnd.viewParms.shadowView && r_showtris->integer ) {
		DrawTris( input );
	}
	if ( !backEnd.viewParms.shadowView && r_shownormals->integer ) {
		DrawNormals( input );
	}

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;
	tess.numVertexes = 0;

#ifdef USE_VBO
	tess.vboIndex = 0;
	//VBO_ClearQueue();
#endif
}
