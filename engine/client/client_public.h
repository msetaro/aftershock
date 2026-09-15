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

#ifndef CLIENT_PUBLIC_H
#define CLIENT_PUBLIC_H

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon_public.h"
#include "../renderercommon/tr_public.h"
#include "../qcommon/keys_public.h"

extern	refexport_t		re;		// interface to refresh .dll
extern	cvar_t	*cl_aviFrameRate;
extern	cvar_t	*com_maxfps;
extern	cvar_t	*vid_xpos;
extern	cvar_t	*vid_ypos;
extern	cvar_t	*r_noborder;
extern	cvar_t	*r_allowSoftwareGL;
extern	cvar_t	*r_swapInterval;
extern	cvar_t	*r_glDriver;
extern	cvar_t	*r_displayRefresh;
extern	cvar_t	*r_fullscreen;
extern	cvar_t	*r_mode;
extern	cvar_t	*r_modeFullscreen;
extern	cvar_t	*r_colorbits;
extern	cvar_t	*cl_stencilbits;
extern	cvar_t	*cl_depthbits;
extern	cvar_t	*cl_drawBuffer;
qboolean CL_NoDelay( void );
qboolean CL_GetModeInfo( int *width, int *height, float *windowAspect, int mode, const char *modeFS, int dw, int dh, qboolean fullscreen );
int Key_GetCatcher( void );
void CL_WriteAVIAudioFrame( const byte *pcmBuffer, int size );
qboolean CL_VideoRecording( void );
void	HandleEvents( void );
void	GLimp_InitGamma(glconfig_t *config);
void	GLimp_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256]);
void	GLimp_Init( glconfig_t *config );
void	GLimp_Shutdown( qboolean unloadDLL );
void	GLimp_EndFrame( void );
void	*GL_GetProcAddress( const char *name );
void	VKimp_Init( glconfig_t *config );
void	VKimp_Shutdown( qboolean unloadDLL );
void	*VK_GetInstanceProcAddr( VkInstance instance, const char *name );
qboolean VK_CreateSurface( VkInstance instance, VkSurfaceKHR* pSurface );

int CL_FrameCount( void );
void CL_SoundStopped( void );
void CL_SoundRegistrationCleared( void );
void CL_AdvanceVideoAudio( int speed, float mixOffset, int *soundtime, int *paintedtime );
int CL_VideoAudioEndTime( void );

#endif
