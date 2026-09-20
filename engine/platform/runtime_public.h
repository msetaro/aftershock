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
// common.c -- misc functions used in client and server

#ifndef RUNTIME_PUBLIC_H
#define RUNTIME_PUBLIC_H

#include "../qcommon/q_shared.h"

void Sys_GetProcessorId( char *vendor );
#ifdef USE_AFFINITY_MASK
void Sys_InitAffinity( void );
void Sys_ApplyAffinityMask( const char *str );
#endif
#if defined( _WIN32 ) && defined( _DEBUG )
void Sys_DebugBreak( void );
#endif
time_t Sys_Time( time_t *result );
char *Sys_CTime( const time_t *value );
struct tm *Sys_LocalTime( const time_t *value );

#endif
