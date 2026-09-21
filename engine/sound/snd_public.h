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


#ifndef SND_PUBLIC_H
#define SND_PUBLIC_H

#include "../qcommon/q_shared.h"

void S_Init( void );
void S_Shutdown( void );
void S_LoadWorldAudio(); // After the client collision map is loaded.

// if origin is NULL, the sound will be dynamically sourced from the entity
void S_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx );
void S_StartLocalSound( sfxHandle_t sfx, int channelNum );

void S_StartBackgroundTrack( const char *intro, const char *loop );
void S_StopBackgroundTrack( void );

// cinematics and voice-over-network will send raw samples
// 1.0 volume will be direct output of source samples
void S_RawSamples( int samples, int rate, int width, int channels,
	const byte *data, float volume );

// stop all sounds and the background track
void S_StopAllSounds( void );

// all continuous looping sounds must be added before calling S_Update
void S_ClearLoopingSounds( qboolean killall );
void S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void S_StopLoopingSound( int entityNum );

// recompute the relative volumes for all running sounds
// relative to the given entityNum / orientation
void S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater );

// let the sound system know where an entity currently is
void S_UpdateEntityPosition( int entityNum, const vec3_t origin );

void S_Update( int msec );

void S_DisableSounds( void );

void S_BeginRegistration( void );

// RegisterSound will always return a valid sample, even if it
// has to create a placeholder.  This prevents continuous filesystem
// checks for missing files
sfxHandle_t S_RegisterSound( const char *sample, qboolean compressed );

void S_DisplayFreeMemory( void );

void S_ClearSoundBuffer( void );

void SNDDMA_Activate( void );

typedef struct {
	unsigned int channels;
	unsigned int samples; // mono samples in buffer
	int fullsamples; // samples with all channels in buffer (samples divided by channels)
	int submission_chunk; // don't mix less than this #
	int samplebits;
	int isfloat;
	int speed;
	byte *buffer;
	const char *driver;
} dma_t;

extern byte *dma_buffer2;

// initializes cycling through a DMA buffer and returns information on it
qboolean SNDDMA_Init( void );

// gets the current DMA position
int SNDDMA_GetDMAPos( void );

// shutdown the DMA xfer.
void SNDDMA_Shutdown( void );

void SNDDMA_BeginPainting( void );

void SNDDMA_Submit( void );

extern int s_soundtime;
extern int s_paintedtime;
extern int s_rawend;
extern dma_t dma;
extern cvar_t *s_volume;
#define WAV_FORMAT_PCM			0x0001
#define WAVE_FORMAT_IEEE_FLOAT	0x0003

#endif
