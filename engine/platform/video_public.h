/*
===========================================================================
Copyright (C) 2005-2006 Tim Angus

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

#ifndef VIDEO_PUBLIC_H
#define VIDEO_PUBLIC_H

#include "../qcommon/q_shared.h"

#ifdef _WIN32
typedef struct {
	void *hNamedPipe;
	void *hProcess;
	void *hThread;
	void *hStdErr;
} sysVideoPipe_t;

qboolean Sys_OpenVideoPipe( sysVideoPipe_t *pipe, const char *ospath, const char *pipeFormat, const char *caller );
qboolean Sys_WriteVideoPipe( sysVideoPipe_t *pipe, const void *buf, unsigned int len );
void Sys_CloseVideoPipe( sysVideoPipe_t *pipe );
#endif

#endif
