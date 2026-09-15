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

#include "video_public.h"
#ifdef _WIN32
#include <windows.h>
#include "../qcommon/qcommon_public.h"

#define WIN32_HANDLE_VALID(h) ((h) && (h) != INVALID_HANDLE_VALUE)

qboolean Sys_OpenVideoPipe( sysVideoPipe_t *pipe, const char *ospath, const char *pipeFormat, const char *caller )
{
	char cmd[MAX_OSPATH*2];
	char namedPipeName[128];		// base length is 15 chars for "\\.\pipe\LOCAL\", rest is for "q3a-*" suffix reserved
	char logName[MAX_OSPATH*2 + 8];	// fileName + strlen("-log.txt")
	int namedPipeRand[1];			// one 32bit random id should be enough to avoid collisions
	SECURITY_ATTRIBUTES sAttr;

	// we can't use "2> " stderr log file redirection with named pipes
	// so will create and inherit corresponding file handles
	const char* cmd_fmt2 = "ffmpeg -threads 0 -f avi -i %s -y %s \"%s\"";

	Com_sprintf( logName, sizeof( logName ), "%s-log.txt", ospath );
	// make sure log file dir exists before file creation
	FS_CreatePath( logName );

	// create security attributes to inherit log file handle
	memset( &sAttr, 0x0, sizeof( sAttr ) );
	sAttr.nLength = sizeof( SECURITY_ATTRIBUTES );
	sAttr.bInheritHandle = TRUE;

	pipe->hStdErr = CreateFileA( logName, GENERIC_WRITE, FILE_SHARE_READ, &sAttr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

	// generate random pipe suffix
	Sys_RandomBytes( (byte*)namedPipeRand, sizeof( namedPipeRand ) );
	Com_sprintf( namedPipeName, sizeof( namedPipeName ), "\\\\.\\pipe\\LOCAL\\q3a-%x", namedPipeRand[0] );

	pipe->hNamedPipe = CreateNamedPipeA( namedPipeName, PIPE_ACCESS_OUTBOUND, PIPE_TYPE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS, 1, 0, 0, 0, NULL );
	if ( pipe->hNamedPipe != INVALID_HANDLE_VALUE )
	{
		STARTUPINFOA si = { sizeof( STARTUPINFOA ) };
		PROCESS_INFORMATION pi = { 0 };
		BOOL bResult;

		// hide ffmpeg console window
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;

		// enable stdout/stderr redirection for a log file
		if ( pipe->hStdErr != INVALID_HANDLE_VALUE )
		{
			si.dwFlags |= STARTF_USESTDHANDLES;
			si.hStdInput = GetStdHandle( STD_INPUT_HANDLE );
			si.hStdOutput = pipe->hStdErr;
			si.hStdError = pipe->hStdErr;
		}

		// create ffmpeg command line using name pipe and aviPipeFormat
		Com_sprintf( cmd, sizeof( cmd ), cmd_fmt2, namedPipeName, pipeFormat, ospath );

		// create ffmpeg process
		bResult = CreateProcessA( NULL, cmd, NULL,	NULL, si.dwFlags & STARTF_USESTDHANDLES ? TRUE : FALSE, 
			0, NULL, NULL, &si, &pi );

		if ( bResult == TRUE )
		{
			pipe->hProcess = pi.hProcess;
			pipe->hThread = pi.hThread;
			// wait till ffmpeg client connects to the pipe
			ConnectNamedPipe( pipe->hNamedPipe, NULL );
		}
		else
		{
			int err = (int)GetLastError();
			if ( err == ERROR_FILE_NOT_FOUND ) {
				Com_Printf( S_COLOR_ERROR "%s: ffmpeg binary not found!\n", caller );
			} else {
				Com_Printf( S_COLOR_ERROR "%s: ffmpeg startup error %d\n", caller, err );
			}

			// cleanup pipe and log handle
			CloseHandle( pipe->hNamedPipe );
			pipe->hNamedPipe = INVALID_HANDLE_VALUE;
			if ( WIN32_HANDLE_VALID( pipe->hStdErr ) )
			{
				CloseHandle( pipe->hStdErr );
				pipe->hStdErr = INVALID_HANDLE_VALUE;
			}

			return qfalse;
		}
	}
	else
	{
		Com_Printf( S_COLOR_ERROR "%s: error %i creating named pipe %s\n", caller, (int)GetLastError(), namedPipeName );
		// cleanup log handle
		if ( WIN32_HANDLE_VALID( pipe->hStdErr ) )
		{
			CloseHandle( pipe->hStdErr );
			pipe->hStdErr = INVALID_HANDLE_VALUE;
		}
		return qfalse;
	}
	return qtrue;
}

qboolean Sys_WriteVideoPipe( sysVideoPipe_t *pipe, const void *buf, unsigned int len )
{
	if ( WIN32_HANDLE_VALID( pipe->hNamedPipe ) ) {
		DWORD n = 0;
		WriteFile( pipe->hNamedPipe, buf, len, &n, NULL );
		if ( n != len ) {
			// ffmpeg died most likely, we should close all handles here to avoid recursive errors
			Com_Error( ERR_DROP, "Failed to write avi file to pipe" );
		}
		return qtrue;
	}
	return qfalse;
}

void Sys_CloseVideoPipe( sysVideoPipe_t *pipe )
{
	if ( WIN32_HANDLE_VALID( pipe->hNamedPipe ) )
	{
		FlushFileBuffers( pipe->hNamedPipe );
		DisconnectNamedPipe( pipe->hNamedPipe );
		CloseHandle( pipe->hNamedPipe );
		if ( WIN32_HANDLE_VALID( pipe->hProcess ) )
		{
			WaitForSingleObject( pipe->hProcess, INFINITE );
			CloseHandle( pipe->hProcess );
		}
		CloseHandle( pipe->hThread );
		if ( WIN32_HANDLE_VALID( pipe->hStdErr ) )
			CloseHandle( pipe->hStdErr );
		pipe->hNamedPipe = INVALID_HANDLE_VALUE;
		pipe->hProcess = INVALID_HANDLE_VALUE;
		pipe->hThread = INVALID_HANDLE_VALUE;
		pipe->hStdErr = INVALID_HANDLE_VALUE;
	}
}
#endif
