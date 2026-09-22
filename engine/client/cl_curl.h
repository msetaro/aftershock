/*
===========================================================================
Copyright (C) 2006 Tony J. White (tjw@tjw.org)

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


#ifndef __QCURL_H__
#define __QCURL_H__

#include "../platform/curl_public.h"
extern cvar_t *cl_cURLLib;

qboolean CL_cURL_Init( void );
void CL_cURL_Shutdown( void );
void CL_cURL_BeginDownload( const char *localName, const char *remoteURL );
void CL_cURL_PerformDownload( void );
void CL_cURL_Cleanup( void );

typedef struct download_s {
	char URL[MAX_OSPATH];
	char Name[MAX_OSPATH];
	char gameDir[MAX_OSPATH];
	char TempName[MAX_OSPATH * 2 + 14]; // gameDir + PATH_SEP + Name + ".00000000.tmp"
	char progress[MAX_OSPATH + 64];
	CURL *cURL;
	CURLM *cURLM;
	fileHandle_t fHandle;
	int Size;
	int Count;
	qboolean headerCheck;
	qboolean mapAutoDownload;

	struct func_s {
		char *( *version )( void );
		char *( *easy_escape )( CURL *curl, const char *string, int length );
		void ( *free )( char *ptr );

		CURL *( *easy_init )( void );
		CURLcode ( *easy_setopt )( CURL *curl, CURLoption option, ... );
		CURLcode ( *easy_perform )( CURL *curl );
		void ( *easy_cleanup )( CURL *curl );
		CURLcode ( *easy_getinfo )( CURL *curl, CURLINFO info, ... );
		const char *( *easy_strerror )( CURLcode );

		CURLM *( *multi_init )( void );
		CURLMcode ( *multi_add_handle )( CURLM *multi_handle, CURL *curl_handle );
		CURLMcode ( *multi_remove_handle )( CURLM *multi_handle, CURL *curl_handle );
		CURLMcode ( *multi_perform )( CURLM *multi_handle, int *running_handles );
		CURLMcode ( *multi_cleanup )( CURLM *multi_handle );
		CURLMsg *( *multi_info_read )( CURLM *multi_handle, int *msgs_in_queue );
		const char *( *multi_strerror )( CURLMcode );

		void *lib;
	} func;
} download_t;

#endif // __QCURL_H__
