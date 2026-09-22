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
 * name:		be_ai_weight.h
 *
 * desc:		fuzzy weights
 *
 * $Archive: /source/code/botlib/be_ai_weight.h $
 *
 *****************************************************************************/

#define WT_BALANCE			1
#define MAX_WEIGHTS			128

//fuzzy separator
typedef struct fuzzyseperator_s {
	int index;
	int value;
	int type;
	float weight;
	float minweight;
	float maxweight;
	struct fuzzyseperator_s *child;
	struct fuzzyseperator_s *next;
} fuzzyseperator_t;

//fuzzy weight
typedef struct weight_s {
	char *name;
	struct fuzzyseperator_s *firstseperator;
} weight_t;

//weight configuration
typedef struct weightconfig_s {
	int numweights;
	weight_t weights[MAX_WEIGHTS];
	char filename[MAX_QPATH];
} weightconfig_t;

//reads a weight configuration
weightconfig_t *ReadWeightConfig( const char *filename );
//free a weight configuration
void FreeWeightConfig( weightconfig_t *config );
//writes a weight configuration, returns true if successful
qboolean WriteWeightConfig( char *filename, weightconfig_t *config );
//find the fuzzy weight with the given name
int FindFuzzyWeight( const weightconfig_t *wc, const char *name );
//returns the fuzzy weight for the given inventory and weight
float FuzzyWeight( int *inventory, weightconfig_t *wc, int weightnum );
float FuzzyWeightUndecided( int *inventory, weightconfig_t *wc, int weightnum );
//scales the weight with the given name
void ScaleWeight( weightconfig_t *config, char *name, float scale );
//scale the balance range
void ScaleBalanceRange( weightconfig_t *config, float scale );
//evolves the weight configuration
void EvolveWeightConfig( weightconfig_t *config );
//interbreed the weight configurations and stores the interbreeded one in configout
void InterbreedWeightConfigs( weightconfig_t *config1, weightconfig_t *config2, weightconfig_t *configout );
//frees cached weight configurations
void BotShutdownWeights( void );

#include "../public/state_public.h"
// Slots are assigned by the checkpoint owner: cache 0..127, goals 128+handle,
// weapons 256+handle. Names/topology must match the normally reloaded content.
bool Bot_WriteWeightState( stateWriter_t *writer, uint32_t slot, const weightconfig_t *config );
bool Bot_ReadWeightState( const stateReader_t &reader, uint32_t slot, weightconfig_t *config, bool apply );
// -1 null, -2 privately owned, otherwise the shared cache slot.
int Bot_WeightCacheIndex( const weightconfig_t *config );
bool Bot_WriteWeightCacheState( stateWriter_t *writer );
bool Bot_ReadWeightCacheState( const stateReader_t &reader, bool apply );

// Preparation owns new allocations; the destination/cache must be empty.
bool Bot_CreateWeightState( const stateReader_t &reader, uint32_t slot, weightconfig_t **output );
bool Bot_PrepareWeightCacheState( const stateReader_t &reader );

weightconfig_t *Bot_WeightCacheAt( int index );
void Bot_FreePrivateWeightState( weightconfig_t *config );
