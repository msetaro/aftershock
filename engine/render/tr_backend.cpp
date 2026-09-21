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
#include "tr_local.h"
#include <algorithm>
#include "tr_cooked.h"

backEndData_t *backEndData;
backEndState_t backEnd;

float r_modelview[16];

static qboolean RB_FindScreenMapDrawSurfs( void ) {
	const void *curCmd = &backEndData->commands.cmds;
	const drawBufferCommand_t *db_cmd;
	const drawSurfsCommand_t *ds_cmd;

	for ( ;; ) {
		curCmd = PADP( curCmd, sizeof( void * ) );
		switch ( *(const int *)curCmd ) {
		case RC_DRAW_BUFFER:
			db_cmd = (const drawBufferCommand_t *)curCmd;
			curCmd = (const void *)( db_cmd + 1 );
			break;
		case RC_DRAW_SURFS:
			ds_cmd = (const drawSurfsCommand_t *)curCmd;
			if ( ds_cmd->viewParms.shadowView ) {
				curCmd = (const void *)( ds_cmd + 1 );
				break;
			}
			return ds_cmd->refdef.needScreenMap;
		default:
			return qfalse;
		}
	}
}


static void RB_BeginFrame( void ) {
	bool started;
	R_CheckRHI( RHI_BeginFrame( RB_FindScreenMapDrawSurfs() != qfalse, &started ), "begin frame" );
	if ( started )
		backEnd.screenMapDone = qfalse;
}

static void RB_EndFrame( void ) {
	rhiFrameEnd_t result;
	R_CheckRHI( RHI_EndFrame( r_bloom->integer && !backEnd.doneBloom && backEnd.doneSurfaces, backEnd.screenshotMask != 0, &result ), "end frame" );
	if ( result.bloomApplied )
		backEnd.doneBloom = qtrue;
	// Presentation may take undefined time; retain the pre-present CPU measurement.
	if ( result.submitted )
		backEnd.pc.msec = ri.Milliseconds() - backEnd.pc.msec;
}


static void RB_GetViewportRect( rhiRect_t *r ) {
	const rhiRenderArea_t area = RHI_GetRenderArea();
	if ( backEnd.projection2D ) {
		r->offset.x = 0;
		r->offset.y = 0;
		r->extent.width = area.width;
		r->extent.height = area.height;
	} else {
		r->offset.x = (int32_t)( backEnd.viewParms.viewportX * area.scaleX );
		r->offset.y = (int32_t)( area.height - ( backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight ) * area.scaleY );
		r->extent.width = (uint32_t)( (float)backEnd.viewParms.viewportWidth * area.scaleX );
		r->extent.height = (uint32_t)( (float)backEnd.viewParms.viewportHeight * area.scaleY );
	}
}

static void RB_GetViewport( rhiViewport_t *viewport, rhiDepthRange_t depth_range ) {
	rhiRect_t r;

	RB_GetViewportRect( &r );

	viewport->x = (float)r.offset.x;
	viewport->y = (float)r.offset.y;
	viewport->width = (float)r.extent.width;
	viewport->height = (float)r.extent.height;

	switch ( depth_range ) {
	default:
#ifdef USE_REVERSED_DEPTH
		//case DEPTH_RANGE_NORMAL:
		viewport->minDepth = 0.0f;
		viewport->maxDepth = 1.0f;
		break;
	case DEPTH_RANGE_ZERO:
		viewport->minDepth = 1.0f;
		viewport->maxDepth = 1.0f;
		break;
	case DEPTH_RANGE_ONE:
		viewport->minDepth = 0.0f;
		viewport->maxDepth = 0.0f;
		break;
	case DEPTH_RANGE_WEAPON:
		viewport->minDepth = 0.6f;
		viewport->maxDepth = 1.0f;
		break;
#else
		//case DEPTH_RANGE_NORMAL:
		viewport->minDepth = 0.0f;
		viewport->maxDepth = 1.0f;
		break;
	case DEPTH_RANGE_ZERO:
		viewport->minDepth = 0.0f;
		viewport->maxDepth = 0.0f;
		break;
	case DEPTH_RANGE_ONE:
		viewport->minDepth = 1.0f;
		viewport->maxDepth = 1.0f;
		break;
	case DEPTH_RANGE_WEAPON:
		viewport->minDepth = 0.0f;
		viewport->maxDepth = 0.3f;
		break;
#endif
	}
}

static void RB_GetScissorRect( rhiRect_t *r ) {

	if ( backEnd.viewParms.shadowView ) {
		RB_GetViewportRect( r );
		return;
	}

	if ( backEnd.viewParms.portalView != PV_NONE ) {
		r->offset.x = backEnd.viewParms.scissorX;
		r->offset.y = glConfig.vidHeight - backEnd.viewParms.scissorY - backEnd.viewParms.scissorHeight;
		r->extent.width = backEnd.viewParms.scissorWidth;
		r->extent.height = backEnd.viewParms.scissorHeight;
	} else {
		RB_GetViewportRect( r );

		if ( r->offset.x < 0 )
			r->offset.x = 0;
		if ( r->offset.y < 0 )
			r->offset.y = 0;

		if ( r->offset.x + r->extent.width > (uint32_t)glConfig.vidWidth )
			r->extent.width = glConfig.vidWidth - r->offset.x;
		if ( r->offset.y + r->extent.height > (uint32_t)glConfig.vidHeight )
			r->extent.height = glConfig.vidHeight - r->offset.y;
	}
}


void RB_GetRaster( rhiDepthRange_t depthRange, rhiRasterState_t *raster ) {
	raster->depthRange = depthRange;
	RB_GetScissorRect( &raster->scissor );
	RB_GetViewport( &raster->viewport, depthRange );
}

static void RB_ClearColorBuffer( const float *color ) {
	rhiRect_t rect;
	RB_GetScissorRect( &rect );
	RHI_ClearColor( color, &rect );
}

static void RB_ClearDepthBuffer( bool stencil ) {
	rhiRect_t rect;
	RB_GetScissorRect( &rect );
	RHI_ClearDepth( stencil && glConfig.stencilBits > 0, &rect );
}


static void RB_GetMVP( float *mvp ) {
	if ( backEnd.projection2D ) {
		float mvp0 = 2.0f / glConfig.vidWidth;
		float mvp5 = 2.0f / glConfig.vidHeight;

		mvp[0] = mvp0;
		mvp[1] = 0.0f;
		mvp[2] = 0.0f;
		mvp[3] = 0.0f;
		mvp[4] = 0.0f;
		mvp[5] = mvp5;
		mvp[6] = 0.0f;
		mvp[7] = 0.0f;
#ifdef USE_REVERSED_DEPTH
		mvp[8] = 0.0f;
		mvp[9] = 0.0f;
		mvp[10] = 0.0f;
		mvp[11] = 0.0f;
		mvp[12] = -1.0f;
		mvp[13] = -1.0f;
		mvp[14] = 1.0f;
		mvp[15] = 1.0f;
#else
		mvp[8] = 0.0f;
		mvp[9] = 0.0f;
		mvp[10] = 1.0f;
		mvp[11] = 0.0f;
		mvp[12] = -1.0f;
		mvp[13] = -1.0f;
		mvp[14] = 0.0f;
		mvp[15] = 1.0f;
#endif
	} else {
		const float *p = backEnd.viewParms.projectionMatrix;
		float proj[16];
		Com_Memcpy( proj, p, 64 );

		// update q3's proj matrix (opengl) to vulkan conventions: z - [0, 1] instead of [-1, 1] and invert y direction
		proj[5] = -p[5];
		//proj[10] = ( p[10] - 1.0f ) / 2.0f;
		//proj[14] = p[14] / 2.0f;
		myGlMultMatrix( r_modelview, proj, mvp );
	}
}


void RB_UpdateMVP( const float *m ) {
	float transform[16];
	if ( m )
		Com_Memcpy( transform, m, sizeof( transform ) );
	else
		RB_GetMVP( transform );
	RHI_PushTransform( transform );
}

static void RB_Bloom( void ) {
	if ( RHI_GetFrameState().screenMapPass )
		return;
	if ( backEnd.doneBloom || !backEnd.doneSurfaces || !RHI_GetCapabilities().fboActive )
		return;
	float transform[16];
	RB_GetMVP( transform );
	RHI_Bloom( transform );
	backEnd.doneBloom = qtrue;
}


#ifndef USE_VULKAN
static const float s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};


const float *GL_Ortho( const float left, const float right, const float bottom, const float top, const float znear, const float zfar ) {
	static float m[16] = { 0 };

	m[0] = 2.0f / ( right - left );
	m[5] = 2.0f / ( top - bottom );
	m[10] = -2.0f / ( zfar - znear );
	m[12] = -( right + left ) / ( right - left );
	m[13] = -( top + bottom ) / ( top - bottom );
	m[14] = -( zfar + znear ) / ( zfar - znear );
	m[15] = 1.0f;

	return m;
}
#endif


/*
** GL_Bind
*/
void GL_Bind( image_t *image ) {
#ifdef USE_VULKAN
	if ( !image ) {
		ri.Printf( PRINT_WARNING, "GL_Bind: NULL image\n" );
		image = tr.defaultImage;
	}

	if ( r_nobind->integer && tr.dlightImage ) { // performance evaluation option
		image = tr.dlightImage;
	}

	//if ( glState.currenttextures[glState.currenttmu] != texnum ) {
	image->frameUsed = tr.frameCount;
	RHI_BindTexture( glState.currenttmu + RHI_BINDING_TEXTURE_BASE, &image->texture );

	//}
#else
	GLuint texnum;

	if ( !image ) {
		ri.Printf( PRINT_WARNING, "GL_Bind: NULL image\n" );
		texnum = tr.defaultImage->texnum;
	} else {
		texnum = image->texnum;
	}

	if ( r_nobind->integer && tr.dlightImage ) { // performance evaluation option
		texnum = tr.dlightImage->texnum;
	}

	if ( glState.currenttextures[glState.currenttmu] != texnum ) {
		if ( image ) {
			image->frameUsed = tr.frameCount;
		}
		glState.currenttextures[glState.currenttmu] = texnum;
		qglBindTexture( GL_TEXTURE_2D, texnum );
	}
#endif
}


/*
** GL_SelectTexture
*/
void GL_SelectTexture( int unit ) {
#ifndef USE_VULKAN
	if ( glState.currenttmu == unit ) {
		return;
	}
#endif

	if ( unit >= glConfig.numTextureUnits ) {
		ri.Error( ERR_DROP, "GL_SelectTexture: unit = %i", unit );
	}
#ifndef USE_VULKAN
	qglActiveTextureARB( GL_TEXTURE0_ARB + unit );
#endif
	glState.currenttmu = unit;
}


/*
** GL_SelectClientTexture
*/
#ifndef USE_VULKAN
static void GL_SelectClientTexture( int unit ) {
	if ( glState.currentArray == unit ) {
		return;
	}

	if ( unit >= glConfig.numTextureUnits ) {
		ri.Error( ERR_DROP, "GL_SelectClientTexture: unit = %i", unit );
	}

	qglClientActiveTextureARB( GL_TEXTURE0_ARB + unit );

	glState.currentArray = unit;
}
#endif


/*
** GL_Cull
*/
void GL_Cull( cullType_t cullType ) {
	if ( glState.faceCulling == cullType ) {
		return;
	}

	glState.faceCulling = cullType;
#ifndef USE_VULKAN
	if ( cullType == CT_TWO_SIDED ) {
		qglDisable( GL_CULL_FACE );
	} else {
		qboolean cullFront;
		qglEnable( GL_CULL_FACE );

		cullFront = ( cullType == CT_FRONT_SIDED );
		if ( backEnd.viewParms.portalView == PV_MIRROR ) {
			cullFront = !cullFront;
		}

		qglCullFace( cullFront ? GL_FRONT : GL_BACK );
	}
#endif
}


/*
** GL_TexEnv
*/
void GL_TexEnv( GLint env [[maybe_unused]] ) {
#ifndef USE_VULKAN
	if ( env == glState.texEnv[glState.currenttmu] )
		return;

	glState.texEnv[glState.currenttmu] = env;

	switch ( env ) {
	case GL_MODULATE:
	case GL_REPLACE:
	case GL_DECAL:
	case GL_ADD:
		qglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, env );
		break;
	default:
		ri.Error( ERR_DROP, "GL_TexEnv: invalid env '%d' passed", env );
		break;
	}
#endif
}


/*
** GL_State
**
** This routine is responsible for setting the most commonly changed state
** in Q3.
*/
void GL_State( unsigned stateBits [[maybe_unused]] ) {
#ifndef USE_VULKAN
	unsigned diff = stateBits ^ glState.glStateBits;

	if ( !diff ) {
		return;
	}

	//
	// check depthFunc bits
	//
	if ( diff & GLS_DEPTHFUNC_EQUAL ) {
		if ( stateBits & GLS_DEPTHFUNC_EQUAL ) {
			qglDepthFunc( GL_EQUAL );
		} else {
			qglDepthFunc( GL_LEQUAL );
		}
	}

	//
	// check blend bits
	//
	if ( diff & GLS_BLEND_BITS ) {
		GLenum srcFactor = GL_ONE, dstFactor = GL_ONE;

		if ( stateBits & GLS_BLEND_BITS ) {
			switch ( stateBits & GLS_SRCBLEND_BITS ) {
			case GLS_SRCBLEND_ZERO:
				srcFactor = GL_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = GL_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = GL_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = GL_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = GL_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = GL_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ALPHA_SATURATE:
				srcFactor = GL_SRC_ALPHA_SATURATE;
				break;
			default:
				ri.Error( ERR_DROP, "GL_State: invalid src blend state bits" );
				break;
			}

			switch ( stateBits & GLS_DSTBLEND_BITS ) {
			case GLS_DSTBLEND_ZERO:
				dstFactor = GL_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = GL_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = GL_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = GL_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = GL_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				ri.Error( ERR_DROP, "GL_State: invalid dst blend state bits" );
				break;
			}

			qglEnable( GL_BLEND );
			qglBlendFunc( srcFactor, dstFactor );
		} else {
			qglDisable( GL_BLEND );
		}
	}

	//
	// check depthmask
	//
	if ( diff & GLS_DEPTHMASK_TRUE ) {
		if ( stateBits & GLS_DEPTHMASK_TRUE ) {
			qglDepthMask( GL_TRUE );
		} else {
			qglDepthMask( GL_FALSE );
		}
	}

	//
	// fill/line mode
	//
	if ( diff & GLS_POLYMODE_LINE ) {
		if ( stateBits & GLS_POLYMODE_LINE ) {
			qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		} else {
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	//
	// depthtest
	//
	if ( diff & GLS_DEPTHTEST_DISABLE ) {
		if ( stateBits & GLS_DEPTHTEST_DISABLE ) {
			qglDisable( GL_DEPTH_TEST );
		} else {
			qglEnable( GL_DEPTH_TEST );
		}
	}

	//
	// alpha test
	//
	if ( diff & GLS_ATEST_BITS ) {
		switch ( stateBits & GLS_ATEST_BITS ) {
		case 0:
			qglDisable( GL_ALPHA_TEST );
			break;
		case GLS_ATEST_GT_0:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GREATER, 0.0f );
			break;
		case GLS_ATEST_LT_80:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_LESS, 0.5f );
			break;
		case GLS_ATEST_GE_80:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GEQUAL, 0.5f );
			break;
		default:
			ri.Error( ERR_DROP, "GL_State: invalid alpha test bits" );
			break;
		}
	}

	glState.glStateBits = stateBits;
#endif // USE_VULKAN
}


#ifndef USE_VULKAN
void GL_ClientState( int unit, unsigned stateBits ) {
	unsigned diff = stateBits ^ glState.glClientStateBits[unit];

	if ( diff == 0 ) {
		if ( stateBits ) {
			GL_SelectClientTexture( unit );
		}
		return;
	}

	GL_SelectClientTexture( unit );

	if ( diff & CLS_COLOR_ARRAY ) {
		if ( stateBits & CLS_COLOR_ARRAY )
			qglEnableClientState( GL_COLOR_ARRAY );
		else
			qglDisableClientState( GL_COLOR_ARRAY );
	}

	if ( diff & CLS_NORMAL_ARRAY ) {
		if ( stateBits & CLS_NORMAL_ARRAY )
			qglEnableClientState( GL_NORMAL_ARRAY );
		else
			qglDisableClientState( GL_NORMAL_ARRAY );
	}

	if ( diff & CLS_TEXCOORD_ARRAY ) {
		if ( stateBits & CLS_TEXCOORD_ARRAY )
			qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		else
			qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	glState.glClientStateBits[unit] = stateBits;
}
#endif


static void RB_SetGL2D( void );

/*
================
RB_Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
static void RB_Hyperspace( void ) {
	color4ub_t c;

	if ( !backEnd.isHyperspace ) {
		// do initialization shit
	}

	if ( tess.shader != tr.whiteShader ) {
		RB_EndSurface();
		RB_SetGL2D();
		RB_BeginSurface( tr.whiteShader, 0 );
	}

#ifdef USE_VBO
	VBO_UnBind();
#endif

	RB_SetGL2D();

	if ( r_teleporterFlash->integer == 0 ) {
		c.rgba[0] = c.rgba[1] = c.rgba[2] = 0; // fade to black
	} else {
		c.rgba[0] = c.rgba[1] = c.rgba[2] = ( backEnd.refdef.time & 255 ); // fade to white
	}
	c.rgba[3] = 255;

	RB_AddQuadStamp2( (float)( backEnd.refdef.x ), (float)( backEnd.refdef.y ), (float)( backEnd.refdef.width ), (float)( backEnd.refdef.height ),
		0.0, 0.0, 0.0, 0.0, c );

	RB_EndSurface();

	tess.numIndexes = 0;
	tess.numVertexes = 0;

	backEnd.isHyperspace = qtrue;
}


static void SetViewportAndScissor( void ) {
#ifdef USE_VULKAN
	//Com_Memcpy( r_modelview, backEnd.or.modelMatrix, 64 );
	//RB_UpdateMVP();
	// force depth range and viewport/scissor updates
	RHI_InvalidateViewport();
#else
	qglMatrixMode( GL_PROJECTION );
	qglLoadMatrixf( backEnd.viewParms.projectionMatrix );
	qglMatrixMode( GL_MODELVIEW );

	// set the window clipping
	qglViewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
	qglScissor( backEnd.viewParms.scissorX, backEnd.viewParms.scissorY,
		backEnd.viewParms.scissorWidth, backEnd.viewParms.scissorHeight );
#endif
}


/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
static void RB_BeginDrawingView( void ) {
#ifndef USE_VULKAN
	int clearBits = 0;
#endif

	// sync with gl if needed
	if ( r_finish->integer == 1 && !glState.finishCalled ) {
#ifdef USE_VULKAN
		R_CheckRHI( RHI_WaitQueue(), "wait queue" );
#else
		qglFinish();
#endif
		glState.finishCalled = qtrue;
	} else if ( r_finish->integer == 0 ) {
		glState.finishCalled = qtrue;
	}

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;

	//
	// set the modelview matrix for the viewer
	//
	SetViewportAndScissor();

#ifdef USE_VULKAN
	RB_ClearDepthBuffer( qtrue );
#else
	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );

	// clear relevant buffers
	clearBits = GL_DEPTH_BUFFER_BIT;

	if ( r_shadows->integer == 2 ) {
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}
	if ( 0 && r_fastsky->integer && !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) ) {
		clearBits |= GL_COLOR_BUFFER_BIT; // FIXME: only if sky shaders have been used
#ifdef _DEBUG
		qglClearColor( 0.8f, 0.7f, 0.4f, 1.0f ); // FIXME: get color of sky
#else
		qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f ); // FIXME: get color of sky
#endif
	}
	qglClear( clearBits );
#endif

	if ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) {
		RB_Hyperspace();
		backEnd.projection2D = qfalse;
		SetViewportAndScissor();
	} else {
		backEnd.isHyperspace = qfalse;
	}

	glState.faceCulling = (cullType_t)( -1 ); // force face culling to set next time

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;
}

#ifdef USE_PMLIGHT
static void RB_LightingPass( void );
#endif

/*
==================
RB_RenderDrawSurfList
==================
*/
static void RB_RenderDrawSurfList( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	shader_t *shader, *oldShader;
	int fogNum;
	int entityNum, oldEntityNum;
	int dlighted;
	qboolean depthRange, isCrosshair;
#ifndef USE_VULKAN
	qboolean oldDepthRange, wasCrosshair;
#endif
	int i;
	drawSurf_t *drawSurf;
	unsigned int oldSort;
#ifdef USE_PMLIGHT
	float oldShaderSort;
#endif
	double originalTime; // -EC-

	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;

	// draw everything
	oldEntityNum = -1;
	backEnd.currentEntity = &tr.worldEntity;
	oldShader = NULL;
#ifndef USE_VULKAN
	oldDepthRange = qfalse;
	wasCrosshair = qfalse;
#endif
	oldSort = MAX_UINT;
#ifdef USE_PMLIGHT
	oldShaderSort = -1;
#endif
	depthRange = qfalse;

	backEnd.pc.c_surfaces += numDrawSurfs;

	for ( i = 0, drawSurf = drawSurfs; i < numDrawSurfs; i++, drawSurf++ ) {
		if ( drawSurf->sort == oldSort ) {
			// fast path, same as previous sort
			rb_surfaceTable[*drawSurf->surface]( drawSurf->surface );
			continue;
		}

		R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted );
#ifdef USE_VULKAN
		if ( RHI_GetFrameState().screenMapPass && entityNum != REFENTITYNUM_WORLD && backEnd.refdef.entities[entityNum].e.renderfx & RF_DEPTHHACK ) {
			continue;
		}
#endif
		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from separate
		// entities merged into a single batch, like smoke and blood puff sprites
		if ( ( ( oldSort ^ drawSurf->sort ) & ~QSORT_REFENTITYNUM_MASK ) || !shader->entityMergable ) {
			//if ( oldShader != NULL ) {
			RB_EndSurface();
			//}
#ifdef USE_PMLIGHT
#define INSERT_POINT SS_FOG
			if ( backEnd.refdef.numLitSurfs && oldShaderSort < (float)INSERT_POINT && shader->sort >= (float)INSERT_POINT ) {
				//RB_BeginDrawingLitSurfs(); // no need, already setup in RB_BeginDrawingView()
#ifdef USE_VULKAN
				RB_LightingPass();
#else
				if ( depthRange ) {
					qglDepthRange( 0, 1 );
					RB_LightingPass();
					qglDepthRange( 0, 0.3 );
				} else {
					RB_LightingPass();
				}
#endif
				oldEntityNum = -1; // force matrix setup
			}
			oldShaderSort = shader->sort;
#endif
			RB_BeginSurface( shader, fogNum );
			oldShader = shader;
		}

		oldSort = drawSurf->sort;

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum ) {
			depthRange = isCrosshair = qfalse;

			if ( entityNum != REFENTITYNUM_WORLD ) {
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
				if ( backEnd.currentEntity->intShaderTime )
					backEnd.refdef.floatTime = originalTime - (double)( backEnd.currentEntity->e.shaderTime.i ) * 0.001;
				else
					backEnd.refdef.floatTime = originalTime - (double)backEnd.currentEntity->e.shaderTime.f;

				// set up the transformation matrix
				R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation );
				// set up the dynamic lighting if needed
#ifdef USE_LEGACY_DLIGHTS
#ifdef USE_PMLIGHT
				if ( !r_dlightMode->integer )
#endif
					if ( backEnd.currentEntity->needDlights ) {
						R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orientation );
					}
#endif // USE_LEGACY_DLIGHTS
				if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK ) {
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;

					if ( backEnd.currentEntity->e.renderfx & RF_CROSSHAIR )
						isCrosshair = qtrue;
				}
			} else {
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.orientation = backEnd.viewParms.world;
#ifdef USE_LEGACY_DLIGHTS
#ifdef USE_PMLIGHT
				if ( !r_dlightMode->integer )
#endif
					R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orientation );
#endif // USE_LEGACY_DLIGHTS
			}

			// we have to reset the shaderTime as well otherwise image animations on
			// the world (like water) continue with the wrong frame
			tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

#ifdef USE_VULKAN
			Com_Memcpy( r_modelview, backEnd.orientation.modelMatrix, 64 );
			tess.depthRange = depthRange ? DEPTH_RANGE_WEAPON : DEPTH_RANGE_NORMAL;
			RB_UpdateMVP( NULL );
#else
			qglLoadMatrixf( backEnd.orientation.modelMatrix );
#endif

			//
			// change depthrange. Also change projection matrix so first person weapon does not look like coming
			// out of the screen.
			//
#ifndef USE_VULKAN
			if ( oldDepthRange != depthRange || wasCrosshair != isCrosshair ) {
				if ( depthRange ) {
					if ( backEnd.viewParms.stereoFrame != STEREO_CENTER ) {
						if ( isCrosshair ) {
							if ( oldDepthRange ) {
								// was not a crosshair but now is, change back proj matrix
								qglMatrixMode( GL_PROJECTION );
								qglLoadMatrixf( backEnd.viewParms.projectionMatrix );
								qglMatrixMode( GL_MODELVIEW );
							}
						} else {
							viewParms_t temp = backEnd.viewParms;

							R_SetupProjection( &temp, r_znear->value, qfalse );

							qglMatrixMode( GL_PROJECTION );
							qglLoadMatrixf( temp.projectionMatrix );
							qglMatrixMode( GL_MODELVIEW );
						}
					}

					if ( !oldDepthRange )
						qglDepthRange( 0, 0.3 );
				} else {
					if ( !wasCrosshair && backEnd.viewParms.stereoFrame != STEREO_CENTER ) {
						qglMatrixMode( GL_PROJECTION );
						qglLoadMatrixf( backEnd.viewParms.projectionMatrix );
						qglMatrixMode( GL_MODELVIEW );
					}

					qglDepthRange( 0, 1 );
				}
				oldDepthRange = depthRange;
				wasCrosshair = isCrosshair;
			}
#endif

			oldEntityNum = entityNum;
		}

		// add the triangles for this surface
		rb_surfaceTable[*drawSurf->surface]( drawSurf->surface );
	}

	// draw the contents of the last shader batch
	if ( oldShader != NULL ) {
		RB_EndSurface();
	}

	backEnd.refdef.floatTime = originalTime;

	// go back to the world modelview matrix
#ifdef USE_VULKAN
	Com_Memcpy( r_modelview, backEnd.viewParms.world.modelMatrix, 64 );
	tess.depthRange = DEPTH_RANGE_NORMAL;
	//RB_UpdateMVP();
#else
	qglLoadMatrixf( backEnd.viewParms.world.modelMatrix );
	if ( depthRange ) {
		qglDepthRange( 0, 1 );
	}
#endif
}


#ifdef USE_PMLIGHT
/*
=================
RB_BeginDrawingLitView
=================
*/
static void RB_BeginDrawingLitSurfs( void ) {
	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;

	//
	// set the modelview matrix for the viewer
	//
	SetViewportAndScissor();

	glState.faceCulling = (cullType_t)( -1 ); // force face culling to set next time
}


/*
==================
RB_RenderLitSurfList
==================
*/
static void RB_RenderLitSurfList( dlight_t *dl ) {
	shader_t *shader, *oldShader;
	int fogNum;
	int entityNum, oldEntityNum;
#ifndef USE_VULKAN
	qboolean oldDepthRange, wasCrosshair;
#endif
	qboolean depthRange, isCrosshair;
	const litSurf_t *litSurf;
	unsigned int oldSort;
	double originalTime; // -EC-

	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;

	// draw everything
	oldEntityNum = -1;
	backEnd.currentEntity = &tr.worldEntity;
	oldShader = NULL;
#ifndef USE_VULKAN
	oldDepthRange = qfalse;
	wasCrosshair = qfalse;
#endif
	oldSort = MAX_UINT;
	depthRange = qfalse;

	tess.dlightUpdateParams = qtrue;

	for ( litSurf = dl->head; litSurf; litSurf = litSurf->next ) {
		//if ( litSurf->sort == sort ) {
		if ( litSurf->sort == oldSort ) {
			// fast path, same as previous sort
			rb_surfaceTable[*litSurf->surface]( litSurf->surface );
			continue;
		}

		R_DecomposeLitSort( litSurf->sort, &entityNum, &shader, &fogNum );
#ifdef USE_VULKAN
		if ( RHI_GetFrameState().screenMapPass && entityNum != REFENTITYNUM_WORLD && backEnd.refdef.entities[entityNum].e.renderfx & RF_DEPTHHACK ) {
			continue;
		}
#endif
		// anything BEFORE opaque is sky/portal, anything AFTER it should never have been added
		//assert( shader->sort == SS_OPAQUE );
		// !!! but MIRRORS can trip that assert, so just do this for now
		//if ( shader->sort < SS_OPAQUE )
		//	continue;

		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from separate
		// entities merged into a single batch, like smoke and blood puff sprites
		if ( ( ( oldSort ^ litSurf->sort ) & ~QSORT_REFENTITYNUM_MASK ) || !shader->entityMergable ) {
			if ( oldShader != NULL ) {
				RB_EndSurface();
			}
			RB_BeginSurface( shader, fogNum );
			oldShader = shader;
		}

		oldSort = litSurf->sort;

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum ) {
			depthRange = isCrosshair = qfalse;

			if ( entityNum != REFENTITYNUM_WORLD ) {
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];

				if ( backEnd.currentEntity->intShaderTime )
					backEnd.refdef.floatTime = originalTime - (double)( backEnd.currentEntity->e.shaderTime.i ) * 0.001;
				else
					backEnd.refdef.floatTime = originalTime - (double)backEnd.currentEntity->e.shaderTime.f;

				// set up the transformation matrix
				R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation );

				if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK ) {
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;

					if ( backEnd.currentEntity->e.renderfx & RF_CROSSHAIR )
						isCrosshair = qtrue;
				}
			} else {
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.orientation = backEnd.viewParms.world;
			}

			// we have to reset the shaderTime as well otherwise image animations on
			// the world (like water) continue with the wrong frame
			tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

			// set up the dynamic lighting
			R_TransformDlights( 1, dl, &backEnd.orientation );
			tess.dlightUpdateParams = qtrue;

#ifdef USE_VULKAN
			tess.depthRange = depthRange ? DEPTH_RANGE_WEAPON : DEPTH_RANGE_NORMAL;
			Com_Memcpy( r_modelview, backEnd.orientation.modelMatrix, 64 );
			RB_UpdateMVP( NULL );
#else
			qglLoadMatrixf( backEnd.orientation.modelMatrix );

			//
			// change depthrange. Also change projection matrix so first person weapon does not look like coming
			// out of the screen.
			//

			if ( oldDepthRange != depthRange || wasCrosshair != isCrosshair ) {
				if ( depthRange ) {
					if ( backEnd.viewParms.stereoFrame != STEREO_CENTER ) {
						if ( isCrosshair ) {
							if ( oldDepthRange ) {
								// was not a crosshair but now is, change back proj matrix
								qglMatrixMode( GL_PROJECTION );
								qglLoadMatrixf( backEnd.viewParms.projectionMatrix );
								qglMatrixMode( GL_MODELVIEW );
							}
						} else {
							viewParms_t temp = backEnd.viewParms;

							R_SetupProjection( &temp, r_znear->value, qfalse );

							qglMatrixMode( GL_PROJECTION );
							qglLoadMatrixf( temp.projectionMatrix );
							qglMatrixMode( GL_MODELVIEW );
						}
					}

					if ( !oldDepthRange )
						qglDepthRange( 0, 0.3 );
				} else {
					if ( !wasCrosshair && backEnd.viewParms.stereoFrame != STEREO_CENTER ) {
						qglMatrixMode( GL_PROJECTION );
						qglLoadMatrixf( backEnd.viewParms.projectionMatrix );
						qglMatrixMode( GL_MODELVIEW );
					}

					qglDepthRange( 0, 1 );
				}
				oldDepthRange = depthRange;
				wasCrosshair = isCrosshair;
			}
#endif

			oldEntityNum = entityNum;
		}

		// add the triangles for this surface
		rb_surfaceTable[*litSurf->surface]( litSurf->surface );
	}

	// draw the contents of the last shader batch
	if ( oldShader != NULL ) {
		RB_EndSurface();
	}

	backEnd.refdef.floatTime = originalTime;

	// go back to the world modelview matrix
#ifdef USE_VULKAN
	Com_Memcpy( r_modelview, backEnd.viewParms.world.modelMatrix, 64 );
	tess.depthRange = DEPTH_RANGE_NORMAL;
	//RB_UpdateMVP();
#else
	qglLoadMatrixf( backEnd.viewParms.world.modelMatrix );
	if ( depthRange ) {
		qglDepthRange( 0, 1 );
	}
#endif // !USE_VULKAN
}
#endif // USE_PMLIGHT


/*
============================================================================

RENDER BACK END FUNCTIONS

============================================================================
*/

/*
================
RB_SetGL2D
================
*/
static void RB_SetGL2D( void ) {

	if ( backEnd.projection2D ) {
		return;
	}

	backEnd.projection2D = qtrue;

#ifdef USE_VULKAN
	RB_UpdateMVP( NULL );

	// force depth range and viewport/scissor updates
	RHI_InvalidateViewport();
#else
	// set 2D virtual screen size
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglMatrixMode( GL_PROJECTION );
	qglLoadMatrixf( GL_Ortho( 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1 ) );
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity();

	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	GL_Cull( CT_TWO_SIDED );
	qglDisable( GL_CLIP_PLANE0 );
#endif

	// set time for 2D shaders
	backEnd.refdef.time = ri.Milliseconds();
	backEnd.refdef.floatTime = (double)backEnd.refdef.time * 0.001; // -EC-: cast to double
}


/*
=============
RE_StretchRaw

FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw( int x, int y, int w, int h, int cols, int rows, byte *data, int client, qboolean dirty ) {
	int i, j;
	int start, end;

	if ( !tr.registered ) {
		return;
	}

	start = 0;
	if ( r_speeds->integer ) {
		start = ri.Milliseconds();
	}

	// make sure rows and cols are powers of 2
	for ( i = 0; ( 1 << i ) < cols; i++ ) {
	}
	for ( j = 0; ( 1 << j ) < rows; j++ ) {
	}

	if ( ( 1 << i ) != cols || ( 1 << j ) != rows ) {
		ri.Error( ERR_DROP, "%s(): size not a power of 2: %i by %i", __func__, cols, rows );
	}

	RE_UploadCinematic( w, h, cols, rows, data, client, dirty );

	if ( r_speeds->integer ) {
		end = ri.Milliseconds();
		ri.Printf( PRINT_ALL, "RE_UploadCinematic( %i, %i ): %i msec\n", cols, rows, end - start );
	}

	tr.cinematicShader->stages[0]->bundle[0].image[0] = tr.scratchImage[client];
	RE_StretchPic( (float)( x ), (float)( y ), (float)( w ), (float)( h ), 0.5f / (float)( cols ), 0.5f / (float)( rows ), 1.0f - 0.5f / (float)( cols ), (float)( 1.0f - 0.5 / rows ), tr.cinematicShader->index );
}


void RE_UploadCinematic( int w [[maybe_unused]], int h [[maybe_unused]], int cols, int rows, byte *data, int client, qboolean dirty ) {

	image_t *image;

	if ( !tr.scratchImage[client] ) {
		tr.scratchImage[client] = R_CreateImage( va( "*scratch%i", client ), NULL, data, cols, rows, (imgFlags_t)( IMGFLAG_CLAMPTOEDGE | IMGFLAG_RGB | IMGFLAG_NOSCALE ) );
		return;
	}

	image = tr.scratchImage[client];

#ifndef USE_VULKAN
	GL_Bind( image );
#endif

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != image->width || rows != image->height ) {
		image->width = image->uploadWidth = cols;
		image->height = image->uploadHeight = rows;
#ifdef USE_VULKAN
		R_CheckRHI( RHI_CreateTexture( &image->texture, cols, rows, 1, image->internalFormat, image->wrapClampMode, image->imgName ), "CreateTexture" );
		R_UploadTexture( &image->texture, image->internalFormat, 0, 0, cols, rows, 1, data, cols * rows * 4, qfalse );
#else
		qglTexImage2D( GL_TEXTURE_2D, 0, image->internalFormat, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_clamp_mode );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_clamp_mode );
#endif
	} else if ( dirty ) {
		// otherwise, just subimage upload it so that drivers can tell we are going to be changing
		// it and don't try and do a texture compression
#ifdef USE_VULKAN
		R_UploadTexture( &image->texture, image->internalFormat, 0, 0, cols, rows, 1, data, cols * rows * 4, qtrue );
#else
		qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
#endif
	}
}


/*
=============
RB_SetColor
=============
*/
static const void *RB_SetColor( const void *data ) {
	const setColorCommand_t *cmd;

	cmd = (const setColorCommand_t *)data;

	backEnd.color2D.rgba[0] = (unsigned char)( cmd->color[0] * 255 );
	backEnd.color2D.rgba[1] = (unsigned char)( cmd->color[1] * 255 );
	backEnd.color2D.rgba[2] = (unsigned char)( cmd->color[2] * 255 );
	backEnd.color2D.rgba[3] = (unsigned char)( cmd->color[3] * 255 );

	return (const void *)( cmd + 1 );
}


/*
=============
RB_StretchPic
=============
*/
static const void *RB_StretchPic( const void *data ) {
	const stretchPicCommand_t *cmd;
	shader_t *shader;

	cmd = (const stretchPicCommand_t *)data;

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		RB_EndSurface();
		backEnd.currentEntity = &backEnd.entity2D;
		RB_SetGL2D(); // set correct shader time before RB_BeginSurface() on 3D->2D transition
		RB_BeginSurface( shader, 0 );
	}

#ifdef USE_VBO
	VBO_UnBind();
#endif

	RB_SetGL2D();

#ifdef USE_VULKAN
	if ( r_bloom->integer ) {
		RB_Bloom();
	}
#endif

	RB_AddQuadStamp2( cmd->x, cmd->y, cmd->w, cmd->h, cmd->s1, cmd->t1, cmd->s2, cmd->t2, backEnd.color2D );
	return (const void *)( cmd + 1 );
}


#ifdef USE_PMLIGHT
static void RB_LightingPass( void ) {
	dlight_t *dl;
	int i;

#ifdef USE_VBO
	//VBO_Flush();
	//tess.allowVBO = qfalse; // for now
#endif

	tess.dlightPass = qtrue;

	for ( i = 0; (unsigned int)i < backEnd.viewParms.num_dlights; i++ ) {
		dl = &backEnd.viewParms.dlights[i];
		if ( dl->head ) {
			tess.light = dl;
			RB_RenderLitSurfList( dl );
		}
	}

	tess.dlightPass = qfalse;

	backEnd.viewParms.num_dlights = 0;
}
#endif


static void transform_to_eye_space( const vec3_t v, vec3_t v_eye ) {
	const float *m = backEnd.viewParms.world.modelMatrix;
	v_eye[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12];
	v_eye[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13];
	v_eye[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14];
};


/*
================
RB_DebugPolygon
================
*/
static void RB_DebugPolygon( int color, int numPoints, float *points ) {
	vec3_t pa;
	vec3_t pb;
	vec3_t p;
	vec3_t q;
	vec3_t n;
	int i;

	if ( numPoints < 3 ) {
		return;
	}

	transform_to_eye_space( &points[0], pa );
	transform_to_eye_space( &points[3], pb );
	VectorSubtract( pb, pa, p );

	for ( i = 2; i < numPoints; i++ ) {
		transform_to_eye_space( &points[3 * i], pb );
		VectorSubtract( pb, pa, q );
		CrossProduct( q, p, n );
		if ( VectorLength( n ) > 1e-5 ) {
			break;
		}
	}

	if ( DotProduct( n, pa ) >= 0 ) {
		return; // discard backfacing polygon
	}

#ifdef USE_VULKAN
	// Solid shade.
	for ( i = 0; i < numPoints; i++ ) {
		VectorCopy( &points[3 * i], tess.xyz[i] );

		tess.svars.colors[0][i].rgba[0] = ( color & 1 ) ? 255 : 0;
		tess.svars.colors[0][i].rgba[1] = ( color & 2 ) ? 255 : 0;
		tess.svars.colors[0][i].rgba[2] = ( color & 4 ) ? 255 : 0;
		tess.svars.colors[0][i].rgba[3] = 255;
	}
	tess.numVertexes = numPoints;

	tess.numIndexes = 0;
	for ( i = 1; i < numPoints - 1; i++ ) {
		tess.indexes[tess.numIndexes + 0] = 0;
		tess.indexes[tess.numIndexes + 1] = i;
		tess.indexes[tess.numIndexes + 2] = i + 1;
		tess.numIndexes += 3;
	}

	RB_BindIndex();
	RB_BindPipeline( r_pipelines.surface_debug_pipeline_solid );
	RB_BindGeometry( TESS_XYZ | TESS_RGBA0 | TESS_ST0 );
	RB_DrawGeometry( DEPTH_RANGE_NORMAL, qtrue );

	// Outline.
	Com_Memset( tess.svars.colors[0], tr.identityLightByte, numPoints * 2 * sizeof( color4ub_t ) );

	for ( i = 0; i < numPoints; i++ ) {
		VectorCopy( &points[3 * i], tess.xyz[2 * i] );
		VectorCopy( &points[3 * ( ( i + 1 ) % numPoints )], tess.xyz[2 * i + 1] );
	}
	tess.numVertexes = numPoints * 2;
	tess.numIndexes = 0;

	RB_BindPipeline( r_pipelines.surface_debug_pipeline_outline );
	RB_BindGeometry( TESS_XYZ | TESS_RGBA0 );
	RB_DrawGeometry( DEPTH_RANGE_ZERO, qfalse );
	tess.numVertexes = 0;
#else
	GL_SelectTexture( 0 );
	qglDisable( GL_TEXTURE_2D );

	GL_ClientState( 0, CLS_NONE );
	qglVertexPointer( 3, GL_FLOAT, 0, points );

	// draw solid shade
	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	qglColor4f( color & 1, ( color >> 1 ) & 1, ( color >> 2 ) & 1, 1 );
	qglDrawArrays( GL_TRIANGLE_FAN, 0, numPoints );

	// draw wireframe outline
	qglDepthRange( 0, 0 );
	qglColor4f( 1, 1, 1, 1 );
	qglDrawArrays( GL_LINE_LOOP, 0, numPoints );
	qglDepthRange( 0, 1 );

	qglEnable( GL_TEXTURE_2D );
#endif
}


/*
====================
RB_DebugGraphics

Visualization aid for movement clipping debugging
====================
*/
static void RB_DebugGraphics( void ) {

	if ( !r_debugSurface->integer ) {
		return;
	}

	GL_Bind( tr.whiteImage );
#ifdef USE_VULKAN
	RB_UpdateMVP( NULL );
#else
	GL_Cull( CT_FRONT_SIDED );
#endif
	ri.CM_DrawDebugSurface( RB_DebugPolygon );
}


/*
=============
RB_DrawSurfs
=============
*/
static void RB_PresentationEffects() {
	if ( ( !r_softParticles->integer && !r_decals->integer ) || ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) ||
		 backEnd.viewParms.portalView != PV_NONE || backEnd.refdef.switchRenderPass )
		return;
	const srfPoly_t *polys[FX_MAX_PARTICLES];
	uint32_t count = 0;
	for ( int i = 0; i < backEnd.refdef.numPolys && count < FX_MAX_PARTICLES; ++i )
		if ( backEnd.refdef.polys[i].softDistance > 0 )
			polys[count++] = &backEnd.refdef.polys[i];
	if ( !count && !backEnd.refdef.numDecals )
		return;
	std::sort( polys, polys + count, []( const srfPoly_t *a, const srfPoly_t *b ) {
		float distance = 0;
		for ( int axis = 0; axis < 3; ++axis )
			distance += ( a->verts[0].xyz[axis] + a->verts[2].xyz[axis] - b->verts[0].xyz[axis] - b->verts[2].xyz[axis] ) * backEnd.viewParms.orientation.axis[0][axis];
		return distance > 0;
	} );
	rhiRect_t viewport;
	RB_GetViewportRect( &viewport );
	if ( !RHI_BeginEffects( &viewport ) )
		return;
	float projection[16], transform[16];
	Com_Memcpy( projection, backEnd.viewParms.projectionMatrix, sizeof( projection ) );
	projection[5] = -projection[5];
	myGlMultMatrix( backEnd.viewParms.world.modelMatrix, projection, transform );
	RB_DrawDecals( &viewport, transform );
	RHI_EffectsScissor( &viewport );
	for ( uint32_t i = 0; i < count; ++i ) {
		const auto *poly = polys[i];
		const auto *stage = R_GetShaderByHandle( poly->hShader )->stages[0];
		auto *image = stage->bundle[0].image[0];
		rhiParticle_t draw{};
		for ( int vertex = 0; vertex < 4; ++vertex ) {
			for ( int row = 0; row < 4; ++row ) {
				draw.clip[vertex][row] = transform[12 + row];
				for ( int axis = 0; axis < 3; ++axis )
					draw.clip[vertex][row] += transform[axis * 4 + row] * poly->verts[vertex].xyz[axis];
			}
			for ( int axis = 0; axis < 2; ++axis )
				draw.uv[vertex][axis] = poly->verts[vertex].st[axis];
			const byte channel = poly->verts[0].modulate.rgba[vertex];
			draw.color[vertex] = (float)( vertex < 3 && stage->bundle[0].rgbGen == CGEN_VERTEX ? (byte)( channel * tr.identityLight ) : channel ) / 255;
		}
		draw.depth[0] = projection[10];
		draw.depth[1] = projection[14];
		draw.depth[2] = poly->softDistance;
		image->frameUsed = tr.frameCount;
		R_EffectSoftDraw( RHI_DrawParticle( &draw, &image->texture, ( stage->stateBits & GLS_DSTBLEND_BITS ) == GLS_DSTBLEND_ONE ) );
	}
	RHI_EndEffects();
}

static const void *RB_DrawSurfs( const void *data ) {
	const drawSurfsCommand_t *cmd;

	// finish any 2D drawing if needed
	RB_EndSurface();

	cmd = (const drawSurfsCommand_t *)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

#ifdef USE_VBO
	VBO_UnBind();
#endif
	if ( backEnd.viewParms.shadowView ) {
		if ( backEnd.viewParms.shadowFirst && !RHI_BeginShadowPass( backEnd.viewParms.shadowView - 1 ) )
			ri.Error( ERR_DROP, "Shadow atlas is unavailable" );
		backEnd.projection2D = qfalse;
		SetViewportAndScissor();
		RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );
		if ( backEnd.viewParms.shadowLast )
			RHI_EndShadowPass();
		return (const void *)( cmd + 1 );
	}

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView();

	RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );

#ifdef USE_VBO
	VBO_UnBind();
#endif

	if ( r_drawSun->integer ) {
		RB_DrawSun( 0.1f, tr.sunShader );
	}

	// darken down any stencil shadows
	RB_ShadowFinish();

	// add light flares on lights that aren't obscured
	RB_RenderFlares();

#ifdef USE_PMLIGHT
	if ( backEnd.refdef.numLitSurfs ) {
		RB_BeginDrawingLitSurfs();
		RB_LightingPass();
	}
#endif

	if ( r_ssao->integer && !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) &&
		 backEnd.viewParms.portalView == PV_NONE && !cmd->refdef.switchRenderPass ) {
		rhiRect_t viewport;
		RB_GetViewportRect( &viewport );
		RHI_Occlusion( backEnd.viewParms.projectionMatrix, &viewport, r_ssaoRadius->value, r_ssaoStrength->value );
	}

	RB_PresentationEffects();

	// draw main system development information (surface outlines, etc)
	RB_DebugGraphics();

#ifdef USE_VULKAN
	if ( cmd->refdef.switchRenderPass ) {
		RHI_EndPass();
		RHI_BeginMainPass();
		backEnd.screenMapDone = qtrue;
	}
#endif

	//TODO Maybe check for rdf_noworld stuff but q3mme has full 3d ui
	backEnd.doneSurfaces = qtrue; // for bloom

	return (const void *)( cmd + 1 );
}


/*
=============
RB_DrawBuffer
=============
*/
static const void *RB_DrawBuffer( const void *data ) {
	const drawBufferCommand_t *cmd;

	cmd = (const drawBufferCommand_t *)data;

#ifdef USE_VULKAN
	RB_BeginFrame();

	tess.depthRange = DEPTH_RANGE_NORMAL;

	// force depth range and viewport/scissor updates
	RHI_InvalidateViewport();

	if ( r_clear->integer && RHI_GetCapabilities().clearAttachment ) {
		const vec4_t color = { 1, 0, 0.5, 1 };
		backEnd.projection2D = qtrue; // to ensure we have viewport that occupies entire window
		RB_ClearColorBuffer( color );
		backEnd.projection2D = qfalse;
	}
#else
	qglDrawBuffer( cmd->buffer );

	// clear screen for debugging
	if ( r_clear->integer ) {
		qglClearColor( 1, 0, 0.5, 1 );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}
#endif

	return (const void *)( cmd + 1 );
}


/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/
#ifdef USE_VULKAN
void RB_ShowImages( void ) {
	int i;

	RB_SetGL2D();

	// draw full-screen quad
	tess.numVertexes = 4;

	tess.svars.colors[0][0].u32 = ~0U; // 255-255-255-255
	tess.svars.colors[0][1].u32 = ~0U;
	tess.svars.colors[0][2].u32 = ~0U;
	tess.svars.colors[0][3].u32 = ~0U;

	tess.svars.texcoords[0][0][0] = 0.0f;
	tess.svars.texcoords[0][0][1] = 0.0f;

	tess.svars.texcoords[0][1][0] = 1.0f;
	tess.svars.texcoords[0][1][1] = 0.0f;

	tess.svars.texcoords[0][2][0] = 0.0f;
	tess.svars.texcoords[0][2][1] = 1.0f;

	tess.svars.texcoords[0][3][0] = 1.0f;
	tess.svars.texcoords[0][3][1] = 1.0f;

	tess.svars.texcoordPtr[0] = tess.svars.texcoords[0];

	tess.xyz[0][0] = 0.0f;
	tess.xyz[0][1] = 0.0f;

	tess.xyz[1][0] = (float)glConfig.vidWidth;
	tess.xyz[1][1] = 0.0f;

	tess.xyz[2][0] = 0.0f;
	tess.xyz[2][1] = (float)glConfig.vidHeight;

	tess.xyz[3][0] = (float)glConfig.vidWidth;
	tess.xyz[3][1] = (float)glConfig.vidHeight;

	RB_BindPipeline( r_pipelines.images_debug_pipeline2 );
	RB_BindGeometry( TESS_XYZ | TESS_RGBA0 | TESS_ST0 );
	RB_DrawGeometry( DEPTH_RANGE_NORMAL, qfalse );

	for ( i = 0; i < tr.numImages; i++ ) {
		image_t *image = tr.images[i];

		float w = (float)( glConfig.vidWidth / 20 );
		float h = (float)( glConfig.vidHeight / 15 );
		float x = i % 20 * w;
		float y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		tess.xyz[0][0] = x;
		tess.xyz[0][1] = y;

		tess.xyz[1][0] = x + w;
		tess.xyz[1][1] = y;

		tess.xyz[2][0] = x;
		tess.xyz[2][1] = y + h;

		tess.xyz[3][0] = x + w;
		tess.xyz[3][1] = y + h;

		GL_Bind( image );
		RB_BindPipeline( r_pipelines.images_debug_pipeline );
		RB_BindGeometry( TESS_XYZ );
		RB_DrawGeometry( DEPTH_RANGE_NORMAL, qfalse );
	}

	tess.numIndexes = 0;
	tess.numVertexes = 0;
}
#else
void RB_ShowImages( void ) {
	int i;
	image_t *image;
	float x, y, w, h;
	int start, end;
	const vec2_t t[4] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
	vec3_t v[4];

	RB_SetGL2D();

	qglClear( GL_COLOR_BUFFER_BIT );

	qglFinish();

	GL_ClientState( 0, CLS_TEXCOORD_ARRAY );
	qglTexCoordPointer( 2, GL_FLOAT, 0, t );

	start = ri.Milliseconds();

	for ( i = 0; i < tr.numImages; i++ ) {
		image = tr.images[i];
		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		GL_Bind( image );

		VectorSet( v[0], x, y, 0 );
		VectorSet( v[1], x + w, y, 0 );
		VectorSet( v[2], x, y + h, 0 );
		VectorSet( v[3], x + w, y + h, 0 );

		qglVertexPointer( 3, GL_FLOAT, 0, v );
		qglDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	}

	qglFinish();

	end = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "%i msec to draw all images\n", end - start );
}
#endif


/*
=============
RB_ColorMask
=============
*/
static const void *RB_ColorMask( const void *data ) {
	const colorMaskCommand_t *cmd = (const colorMaskCommand_t *)data;
#ifdef USE_VULKAN
	// TODO: implement! ZZZZZZZZZZZ
#else
	qglColorMask( cmd->rgba[0], cmd->rgba[1], cmd->rgba[2], cmd->rgba[3] );
#endif

	return (const void *)( cmd + 1 );
}


/*
=============
RB_ClearDepth
=============
*/
static const void *RB_ClearDepth( const void *data ) {
	const clearDepthCommand_t *cmd = (const clearDepthCommand_t *)data;

	RB_EndSurface();

#ifdef USE_VULKAN
	RB_ClearDepthBuffer( r_shadows->integer == 2 ? qtrue : qfalse );
#else
	qglClear( GL_DEPTH_BUFFER_BIT );
#endif

	return (const void *)( cmd + 1 );
}


/*
=============
RB_ClearColor
=============
*/
static const void *RB_ClearColor( const void *data ) {
	const clearColorCommand_t *cmd = (const clearColorCommand_t *)data;

#ifdef USE_VULKAN
	backEnd.projection2D = qtrue;
	RB_ClearColorBuffer( colorBlack );
	backEnd.projection2D = qfalse;
#else
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	qglClear( GL_COLOR_BUFFER_BIT );
#endif

	return (const void *)( cmd + 1 );
}


/*
=============
RB_FinishBloom
=============
*/
static const void *RB_FinishBloom( const void *data ) {
	const finishBloomCommand_t *cmd = (const finishBloomCommand_t *)data;

	RB_EndSurface();

#ifdef USE_VULKAN
	if ( r_bloom->integer ) {
		RB_Bloom();
	}
#endif

	// texture swapping test
	if ( r_showImages->integer ) {
		RB_ShowImages();
	}

	backEnd.drawConsole = qtrue;

	return (const void *)( cmd + 1 );
}


static const void *RB_SwapBuffers( const void *data ) {

	const swapBuffersCommand_t *cmd;

	// finish any 2D drawing if needed
	RB_EndSurface();

	// texture swapping test
	if ( r_showImages->integer && !backEnd.drawConsole ) {
		RB_ShowImages();
	}

	cmd = (const swapBuffersCommand_t *)data;

	tr.needScreenMap = 0;

#ifdef USE_VULKAN
	RB_EndFrame();

	if ( backEnd.doneSurfaces && !glState.finishCalled ) {
		R_CheckRHI( RHI_WaitQueue(), "wait queue" );
	}
#else
	if ( backEnd.doneSurfaces && !glState.finishCalled ) {
		qglFinish();
	}
#endif

#ifdef USE_VULKAN
	if ( backEnd.screenshotMask && RHI_GetFrameState().submitted ) {
#else
	if ( backEnd.screenshotMask && tr.frameCount > 1 ) {
#endif
		if ( backEnd.screenshotMask & SCREENSHOT_TGA && backEnd.screenshotTGA[0] ) {
			RB_TakeScreenshot( 0, 0, gls.captureWidth, gls.captureHeight, backEnd.screenshotTGA );
			if ( !backEnd.screenShotTGAsilent ) {
				ri.Printf( PRINT_ALL, "Wrote %s\n", backEnd.screenshotTGA );
			}
		}
		if ( backEnd.screenshotMask & SCREENSHOT_JPG && backEnd.screenshotJPG[0] ) {
			RB_TakeScreenshotJPEG( 0, 0, gls.captureWidth, gls.captureHeight, backEnd.screenshotJPG );
			if ( !backEnd.screenShotJPGsilent ) {
				ri.Printf( PRINT_ALL, "Wrote %s\n", backEnd.screenshotJPG );
			}
		}
		if ( backEnd.screenshotMask & SCREENSHOT_BMP && ( backEnd.screenshotBMP[0] || ( backEnd.screenshotMask & SCREENSHOT_BMP_CLIPBOARD ) ) ) {
			RB_TakeScreenshotBMP( 0, 0, gls.captureWidth, gls.captureHeight, backEnd.screenshotBMP, backEnd.screenshotMask & SCREENSHOT_BMP_CLIPBOARD );
			if ( !backEnd.screenShotBMPsilent ) {
				ri.Printf( PRINT_ALL, "Wrote %s\n", backEnd.screenshotBMP );
			}
		}
#ifdef AFTERSHOCK_DEVTOOLS
		if ( backEnd.screenshotMask & SCREENSHOT_PNG && backEnd.screenshotPNG[0] ) {
			RB_TakeScreenshotPNG( gls.captureWidth, gls.captureHeight, backEnd.screenshotPNG );
			backEnd.screenshotPNG[0] = 0;
		}
#endif
		if ( backEnd.screenshotMask & SCREENSHOT_AVI ) {
			RB_TakeVideoFrameCmd( &backEnd.vcmd );
		}

		backEnd.screenshotJPG[0] = '\0';
		backEnd.screenshotTGA[0] = '\0';
		backEnd.screenshotBMP[0] = '\0';
		backEnd.screenshotMask = 0;
	}

#ifdef USE_VULKAN
	const rhiStatus_t presentStatus = RHI_PresentFrame();
	if ( presentStatus == rhiStatus_t::DeviceLost )
		ri.Printf( PRINT_DEVELOPER, "GPU presentation: device lost\n" ); // Preserve the existing continuation policy.
	else
		R_CheckRHI( presentStatus, "PresentFrame" );
#else
	ri.GLimp_EndFrame();
#endif

	backEnd.projection2D = qfalse;
	backEnd.doneSurfaces = qfalse;
	backEnd.drawConsole = qfalse;
#ifdef USE_VULKAN
	backEnd.doneBloom = qfalse;
#endif

	return (const void *)( cmd + 1 );
}


/*
====================
RB_ExecuteRenderCommands
====================
*/
void RB_ExecuteRenderCommands( const void *data ) {

	backEnd.pc.msec = ri.Milliseconds();

	while ( 1 ) {
		data = PADP( data, sizeof( void * ) );

		switch ( *(const int *)data ) {
		case RC_SET_COLOR:
			data = RB_SetColor( data );
			break;
		case RC_STRETCH_PIC:
			data = RB_StretchPic( data );
			break;
		case RC_DRAW_SURFS:
			data = RB_DrawSurfs( data );
			break;
		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer( data );
			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers( data );
			break;
		case RC_FINISHBLOOM:
			data = RB_FinishBloom( data );
			break;
		case RC_COLORMASK:
			data = RB_ColorMask( data );
			break;
		case RC_CLEARDEPTH:
			data = RB_ClearDepth( data );
			break;
		case RC_CLEARCOLOR:
			data = RB_ClearColor( data );
			break;
#ifdef AFTERSHOCK_DEVTOOLS
		case RC_DEVELOPER_UI: {
			const developerUiCommand_t *command = (const developerUiCommand_t *)data;
			RB_DrawDeveloperUI( &command->draw );
			data = command + 1;
			break;
		}
#endif
		case RC_END_OF_LIST:
		default:
			// stop rendering
#ifdef USE_VULKAN
			RB_EndFrame();
//			if (com_errorEntered && (begin_frame_called && !end_frame_called)) {
//				RB_EndFrame();
//			}
#else
			backEnd.pc.msec = ri.Milliseconds() - backEnd.pc.msec;
#endif
			return;
		}
	}
}

#ifdef AFTERSHOCK_DEVTOOLS
bool RE_GetDeveloperModel( int index, devModel_t *model ) {
	*model = {};
	if ( index < 0 || index >= tr.numModels )
		return false;
	const model_t *source = tr.models[index];
	Q_strncpyz( model->name, source->name, sizeof( model->name ) );
	model->type = (int32_t)source->type;
	model->bytes = source->dataSize;
	model->reloads = R_CookedModelReloads( index );
	model->lods = (uint32_t)source->numLods;
	memcpy( model->lodDraws, source->lodDraws, sizeof( model->lodDraws ) );
	if ( source->type == MOD_MESH )
		model->frames = source->md3[0]->numFrames;
	else if ( source->type == MOD_MDR )
		model->frames = ( (const mdrHeader_t *)source->modelData )->numFrames;
	else if ( source->type == MOD_IQM )
		model->frames = ( (const iqmData_t *)source->modelData )->num_frames;
	return true;
}

bool RE_GetDeveloperImage( int index, devImage_t *image ) {
	*image = {};
	if ( index < 0 || index >= tr.numImages )
		return false;
	const image_t *source = tr.images[index];
	Q_strncpyz( image->name, source->imgName, sizeof( image->name ) );
	image->texture = (uint32_t)index + 1;
	image->width = source->width;
	image->height = source->height;
	image->uploadWidth = source->uploadWidth;
	image->uploadHeight = source->uploadHeight;
	image->flags = (uint32_t)source->flags;
	image->format = (uint32_t)source->internalFormat;
	image->reloads = R_CookedImageReloads( index );
	return true;
}

bool RE_GetDeveloperMaterial( int index, devMaterial_t *material ) {
	*material = {};
	if ( index < 0 || index >= tr.numShaders )
		return false;
	const shader_t *source = tr.shaders[index];
	Q_strncpyz( material->name, source->name, sizeof( material->name ) );
	material->sort = source->sort;
	material->reloads = R_CookedMaterialReloads( index );
	material->stages = source->numUnfoggedPasses;
	material->cull = (int32_t)source->cullType;
	material->surfaceFlags = source->surfaceFlags;
	material->contentFlags = source->contentFlags;
	material->explicitDefinition = source->explicitlyDefined != 0;
	material->fallback = source->defaultShader != 0;
	material->metallicRoughness = source->metallicRoughness;
	material->params = source->materialParams;
	static_assert( MAX_SHADER_STAGES == 8 && NUM_TEXTURE_BUNDLES == 3 );
	for ( int stage = 0; stage < source->numUnfoggedPasses; ++stage ) {
		if ( !source->stages[stage] )
			continue; // Missing-image stages are not instantiated by GeneratePermanentShader.
		material->present[stage] = true;
		material->stateBits[stage] = source->stages[stage]->stateBits;
		for ( int bundle = 0; bundle < NUM_TEXTURE_BUNDLES; ++bundle ) {
			const image_t *image = source->stages[stage]->bundle[bundle].image[0];
			for ( int i = 0; image && i < tr.numImages; ++i ) {
				if ( tr.images[i] == image ) {
					material->textures[stage][bundle] = (uint32_t)i + 1;
					break;
				}
			}
		}
	}
	return true;
}

bool RE_SetDeveloperMaterial( int index, const materialParams_t *params ) {
	if ( index < 0 || index >= tr.numShaders || !params || !tr.shaders[index]->metallicRoughness )
		return false;
	shader_t *shader = tr.shaders[index];
	materialParams_t checked;
	if ( params->flags != shader->materialParams.flags || !R_ResolveMaterialParams( params, nullptr, &checked ) )
		return false;
	// Frontend edits affect this frame's queued draws; pipeline/order is unchanged.
	shader->materialParams = checked;
	return true;
}

uint32_t RE_GetDeveloperTimings( devGpuTiming_t *timings, uint32_t capacity ) {
	const rhiTiming_t *source;
	const uint32_t count = MIN( capacity, RHI_GetTimings( &source ) );
	for ( uint32_t i = 0; i < count; ++i ) {
		Q_strncpyz( timings[i].name, source[i].name, sizeof( timings[i].name ) );
		timings[i].microseconds = source[i].microseconds;
	}
	return count;
}

uint32_t RE_CreateDeveloperTexture( unsigned char *pixels, int width, int height ) {
	if ( !tr.registered || !pixels || width <= 0 || height <= 0 )
		return 0;
	image_t *image = R_CreateImage( "*developer-font", NULL, pixels, width, height,
		(imgFlags_t)( IMGFLAG_CLAMPTOEDGE | IMGFLAG_NO_COMPRESSION | IMGFLAG_NOLIGHTSCALE | IMGFLAG_NOSCALE ) );
	for ( int i = 0; i < tr.numImages; ++i ) {
		if ( tr.images[i] == image )
			return (uint32_t)i + 1;
	}
	return 0;
}

void RB_DrawDeveloperUI( const devUiDraw_t *draw ) {
	if ( !tr.registered || !draw || !draw->vertices || !draw->indices || !draw->commands )
		return;
	if ( tess.numIndexes )
		RB_EndSurface();
	VBO_UnBind();
	RB_SetGL2D();
	rhiPipelineDesc_t description = {};
	description.shader_type = TYPE_SIGNLE_TEXTURE;
	description.state_bits = GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	description.face_culling = CT_TWO_SIDED;
	const uint32_t pipeline = R_FindPipeline( 0, &description, qtrue );
	const rhiRenderArea_t area = RHI_GetRenderArea();
	tess.svars.texcoordPtr[0] = tess.svars.texcoords[0];
	for ( uint32_t c = 0; c < draw->commandCount; ++c ) {
		const devUiCommand_t *command = &draw->commands[c];
		if ( !command->texture || command->texture > (uint32_t)tr.numImages ||
			 command->firstIndex > draw->indexCount || command->indexCount > draw->indexCount - command->firstIndex || command->indexCount % 3 )
			continue;
		rhiRasterState_t raster;
		RB_GetRaster( DEPTH_RANGE_NORMAL, &raster );
		const int x = (int)( Com_Clamp( 0, (float)glConfig.vidWidth, command->clip[0] ) * (float)area.width / (float)glConfig.vidWidth );
		const int y = (int)( Com_Clamp( 0, (float)glConfig.vidHeight, command->clip[1] ) * (float)area.height / (float)glConfig.vidHeight );
		const int right = (int)( Com_Clamp( 0, (float)glConfig.vidWidth, command->clip[2] ) * (float)area.width / (float)glConfig.vidWidth );
		const int bottom = (int)( Com_Clamp( 0, (float)glConfig.vidHeight, command->clip[3] ) * (float)area.height / (float)glConfig.vidHeight );
		if ( right <= x || bottom <= y )
			continue;
		raster.scissor = { { x, y }, { (uint32_t)( right - x ), (uint32_t)( bottom - y ) } };
		GL_SelectTexture( 0 );
		GL_Bind( tr.images[command->texture - 1] );
		// ponytail: expand UI triangles into the existing bounded tess buffer;
		// use a dedicated vertex stream only if tooling measurements justify it.
		for ( uint32_t first = 0; first < command->indexCount; ) {
			const uint32_t count = MIN( command->indexCount - first, (uint32_t)( ( SHADER_MAX_VERTEXES - 1 ) / 3 * 3 ) );
			bool valid = true;
			for ( uint32_t i = 0; i < count; ++i ) {
				const uint32_t index = draw->indices[command->firstIndex + first + i];
				if ( index >= draw->vertexCount ) {
					valid = false;
					break;
				}
				const devUiVertex_t *vertex = &draw->vertices[index];
				tess.xyz[i][0] = vertex->x;
				tess.xyz[i][1] = vertex->y;
				tess.xyz[i][2] = 0;
				tess.xyz[i][3] = 1;
				tess.svars.texcoords[0][i][0] = vertex->u;
				tess.svars.texcoords[0][i][1] = vertex->v;
				Com_Memcpy( tess.svars.colors[0][i].rgba, &vertex->color, 4 );
			}
			if ( !valid )
				break;
			tess.numVertexes = (int)count;
			RB_BindPipeline( pipeline );
			RB_BindGeometry( TESS_XYZ | TESS_RGBA0 | TESS_ST0 );
			RHI_InvalidateViewport();
			if ( RHI_PrepareDraw( &raster, &tr.whiteImage->texture ) )
				RHI_Draw( count );
			first += count;
		}
	}
	tess.numVertexes = tess.numIndexes = 0;
	tess.shader = NULL;
	RHI_InvalidateViewport();
}
#endif
