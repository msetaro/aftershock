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


#ifdef USE_CURL
#include "curl_public.h"
static uint32_t curlReferences;

#ifdef USE_CURL_DLOPEN
static bool curlLoaded;

char *( *qcurl_version )( void );
curl_slist *( *qcurl_slist_append )( curl_slist *, const char * );
void ( *qcurl_slist_free_all )( curl_slist * );

CURL *( *qcurl_easy_init )( void );
CURLcode ( *qcurl_easy_setopt )( CURL *curl, CURLoption option, ... );
CURLcode ( *qcurl_easy_perform )( CURL *curl );
void ( *qcurl_easy_cleanup )( CURL *curl );
CURLcode ( *qcurl_easy_getinfo )( CURL *curl, CURLINFO info, ... );
CURL *( *qcurl_easy_duphandle )( CURL *curl );
void ( *qcurl_easy_reset )( CURL *curl );
const char *( *qcurl_easy_strerror )( CURLcode );

CURLM *( *qcurl_multi_init )( void );
CURLMcode ( *qcurl_multi_add_handle )( CURLM *multi_handle,
	CURL *curl_handle );
CURLMcode ( *qcurl_multi_remove_handle )( CURLM *multi_handle,
	CURL *curl_handle );
CURLMcode ( *qcurl_multi_fdset )( CURLM *multi_handle,
	fd_set *read_fd_set,
	fd_set *write_fd_set,
	fd_set *exc_fd_set,
	int *max_fd );
CURLMcode ( *qcurl_multi_perform )( CURLM *multi_handle,
	int *running_handles );
CURLMcode ( *qcurl_multi_cleanup )( CURLM *multi_handle );
CURLMsg *( *qcurl_multi_info_read )( CURLM *multi_handle,
	int *msgs_in_queue );
const char *( *qcurl_multi_strerror )( CURLMcode );

static void *cURLLib = NULL;

/*
=================
GPA
=================
*/
static void *GPA( const char *str ) {
	void *rv;

	rv = Sys_LoadFunction( cURLLib, str );
	if ( !rv ) {
		Com_Printf( "Can't load symbol %s\n", str );
		curlLoaded = qfalse;
		return NULL;
	} else {
		Com_DPrintf( "Loaded symbol %s (0x%p)\n", str, rv );
		return rv;
	}
}
#endif /* USE_CURL_DLOPEN */

/*
=================
CL_cURL_Init
=================
*/
bool Sys_CurlAcquire( const char *library ) {
	if ( curlReferences ) {
		++curlReferences;
		return true;
	}
#ifdef USE_CURL_DLOPEN


	Com_Printf( "Loading \"%s\"...", library );
	if ( ( cURLLib = Sys_LoadLibrary( library ) ) == 0 ) {
#ifdef _WIN32
		return qfalse;
#else
		char fn[1024];

		Com_sprintf( fn, sizeof( fn ), "%s/%s", Sys_Pwd(), library );

		if ( ( cURLLib = Sys_LoadLibrary( fn ) ) == 0 ) {
#ifdef ALTERNATE_CURL_LIB
			// On some linux distributions there is no libcurl.so.3, but only libcurl.so.4. That one works too.
			if ( ( cURLLib = Sys_LoadLibrary( ALTERNATE_CURL_LIB ) ) == 0 ) {
				return qfalse;
			}
#else
			return qfalse;
#endif
		}
#endif /* _WIN32 */
	}

	curlLoaded = qtrue;

	qcurl_version = (char *(*)())GPA( "curl_version" );
	qcurl_slist_append = ( curl_slist * (*)(curl_slist *, const char *)) GPA( "curl_slist_append" );
	qcurl_slist_free_all = (void ( * )( curl_slist * ))GPA( "curl_slist_free_all" );

	qcurl_easy_init = ( CURL * (*)() ) GPA( "curl_easy_init" );
	qcurl_easy_setopt = (CURLcode ( * )( CURL *, CURLoption, ... ))GPA( "curl_easy_setopt" );
	qcurl_easy_perform = (CURLcode ( * )( CURL * ))GPA( "curl_easy_perform" );
	qcurl_easy_cleanup = (void ( * )( CURL * ))GPA( "curl_easy_cleanup" );
	qcurl_easy_getinfo = (CURLcode ( * )( CURL *, CURLINFO, ... ))GPA( "curl_easy_getinfo" );
	qcurl_easy_duphandle = ( CURL * (*)(CURL *)) GPA( "curl_easy_duphandle" );
	qcurl_easy_reset = (void ( * )( CURL * ))GPA( "curl_easy_reset" );
	qcurl_easy_strerror = (const char *(*)( CURLcode ))GPA( "curl_easy_strerror" );

	qcurl_multi_init = ( CURLM * (*)() ) GPA( "curl_multi_init" );
	qcurl_multi_add_handle = (CURLMcode ( * )( CURLM *, CURL * ))GPA( "curl_multi_add_handle" );
	qcurl_multi_remove_handle = (CURLMcode ( * )( CURLM *, CURL * ))GPA( "curl_multi_remove_handle" );
	qcurl_multi_fdset = (CURLMcode ( * )( CURLM *, fd_set *, fd_set *, fd_set *, int * ))GPA( "curl_multi_fdset" );
	qcurl_multi_perform = (CURLMcode ( * )( CURLM *, int * ))GPA( "curl_multi_perform" );
	qcurl_multi_cleanup = (CURLMcode ( * )( CURLM * ))GPA( "curl_multi_cleanup" );
	qcurl_multi_info_read = ( CURLMsg * (*)(CURLM *, int *)) GPA( "curl_multi_info_read" );
	qcurl_multi_strerror = (const char *(*)( CURLMcode ))GPA( "curl_multi_strerror" );

	if ( !curlLoaded ) {
		curlReferences = 1;
		Sys_CurlRelease();
		Com_Printf( "FAIL One or more symbols not found\n" );
		return qfalse;
	}
	Com_Printf( "OK\n" );

	++curlReferences;
	return true;
#else
	(void)library;
	++curlReferences;
	return true;
#endif /* USE_CURL_DLOPEN */
}

/*
=================
CL_cURL_Shutdown
=================
*/
void Sys_CurlRelease() {
	if ( !curlReferences || --curlReferences )
		return;
#ifdef USE_CURL_DLOPEN
	if ( cURLLib ) {
		Sys_UnloadLibrary( cURLLib );
		cURLLib = NULL;
	}
	qcurl_version = NULL;
	qcurl_slist_append = NULL;
	qcurl_slist_free_all = NULL;

	qcurl_easy_init = NULL;
	qcurl_easy_setopt = NULL;
	qcurl_easy_perform = NULL;
	qcurl_easy_cleanup = NULL;
	qcurl_easy_getinfo = NULL;
	qcurl_easy_duphandle = NULL;
	qcurl_easy_reset = NULL;

	qcurl_multi_init = NULL;
	qcurl_multi_add_handle = NULL;
	qcurl_multi_remove_handle = NULL;
	qcurl_multi_fdset = NULL;
	qcurl_multi_perform = NULL;
	qcurl_multi_cleanup = NULL;
	qcurl_multi_info_read = NULL;
	qcurl_multi_strerror = NULL;
#endif /* USE_CURL_DLOPEN */
}


#endif
