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
#ifndef FILESYSTEM_PUBLIC_H
#define FILESYSTEM_PUBLIC_H

#include "q_shared.h"

// Raw OS-path streams used by the existing filter and bot developer tools.
// Keep stdio return values and encoding; qpaths continue to use FS_Read/FS_Write.
FILE * FS_OSOpen( const char *path, const char *mode );
size_t FS_OSRead( void *buffer, size_t size, size_t count, FILE *file );
size_t FS_OSWrite( const void *buffer, size_t size, size_t count, FILE *file );
int FS_OSClose( FILE *file );
int FS_OSSeek( FILE *file, long offset, int origin );
long FS_OSTell( FILE *file );
int FS_OSFlush( FILE *file );
int FS_OSVPrintf( FILE *file, const char *format, va_list args );
int QDECL FS_OSPrintf( FILE *file, const char *format, ... ) __attribute__((format(printf, 2, 3)));

#endif
