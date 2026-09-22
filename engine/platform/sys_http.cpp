#include "http_public.h"
#ifdef USE_CURL
#include "curl_public.h"
#include <cstring>
#include <cstdio>

// Foreign curl varargs/getinfo use the platform C ABI word.
using httpCurlWord_t = decltype( 0L );
static CURL *request;
static CURLM *requests;
static curl_slist *headers;
static bool acquired;
static httpResult_t response;
static char requestBody[8193];

static void Release() {
	if ( request && requests )
		qcurl_multi_remove_handle( requests, request );
	if ( request )
		qcurl_easy_cleanup( request );
	if ( requests )
		qcurl_multi_cleanup( requests );
	if ( headers )
		qcurl_slist_free_all( headers );
	request = nullptr;
	requests = nullptr;
	headers = nullptr;
	memset( requestBody, 0, sizeof( requestBody ) );
	if ( acquired )
		Sys_CurlRelease();
	acquired = false;
}
void Sys_HTTPCancel() {
	Release();
	response = {};
}
static size_t Write( void *data, size_t size, size_t count, void * ) {
	if ( size && count > ( HTTP_MAX_RESPONSE - response.size ) / size )
		return 0;
	const size_t bytes = size * count;
	memcpy( response.body + response.size, data, bytes );
	response.size += uint32_t( bytes );
	response.body[response.size] = 0;
	return bytes;
}
static bool Header( const char *text ) {
	curl_slist *replacement = qcurl_slist_append( headers, text );
	if ( !replacement )
		return false;
	headers = replacement;
	return true;
}
bool Sys_HTTPStart( const char *url, const char *method, const char *token, const char *body, const char *ca ) {
	if ( acquired || !url || !method || !token || !body || !ca || strlen( url ) > 1023 || strlen( ca ) > 1023 || strlen( token ) > 128 || strlen( body ) > 8192 )
		return false;
	if ( strncmp( url, "https://", 8 ) || !url[8] || strpbrk( url, "@#\\\r\n\t " ) )
		return false;
	for ( const unsigned char *p = (const unsigned char *)token; *p; ++p )
		if ( !( ( *p >= 'a' && *p <= 'z' ) || ( *p >= 'A' && *p <= 'Z' ) || ( *p >= '0' && *p <= '9' ) || *p == '-' || *p == '_' || *p == '.' ) )
			return false;
	if ( strcmp( method, "GET" ) && strcmp( method, "POST" ) && strcmp( method, "PUT" ) && strcmp( method, "DELETE" ) )
		return false;
	if ( !strcmp( method, "GET" ) && *body )
		return false;
	response = {};
	acquired = Sys_CurlAcquire( DEFAULT_CURL_LIB );
	if ( !acquired )
		return false;
	request = qcurl_easy_init();
	requests = qcurl_multi_init();
	char authorization[160];
	snprintf( authorization, sizeof( authorization ), "Authorization: Bearer %s", token );
	if ( !request || !requests || !Header( "Accept: application/json" ) || !Header( "Content-Type: application/json" ) || ( *token && !Header( authorization ) ) ) {
		Sys_HTTPCancel();
		return false;
	}
	memcpy( requestBody, body, strlen( body ) + 1 );
	bool valid =
		qcurl_easy_setopt( request, CURLOPT_URL, url ) == CURLE_OK &&
		qcurl_easy_setopt( request, CURLOPT_HTTPHEADER, headers ) == CURLE_OK &&
		qcurl_easy_setopt( request, CURLOPT_CUSTOMREQUEST, method ) == CURLE_OK &&
		qcurl_easy_setopt( request, CURLOPT_FOLLOWLOCATION, 0L ) == CURLE_OK &&
		qcurl_easy_setopt( request, CURLOPT_SSL_VERIFYPEER, 1L ) == CURLE_OK &&
		qcurl_easy_setopt( request, CURLOPT_SSL_VERIFYHOST, 2L ) == CURLE_OK &&
		qcurl_easy_setopt( request, CURLOPT_SSLVERSION, httpCurlWord_t( CURL_SSLVERSION_TLSv1_2 ) ) == CURLE_OK &&
		qcurl_easy_setopt( request, CURLOPT_NOSIGNAL, 1L ) == CURLE_OK &&
		qcurl_easy_setopt( request, CURLOPT_CONNECTTIMEOUT_MS, 5000L ) == CURLE_OK &&
		qcurl_easy_setopt( request, CURLOPT_TIMEOUT_MS, 10000L ) == CURLE_OK &&
		qcurl_easy_setopt( request, CURLOPT_WRITEFUNCTION, Write ) == CURLE_OK;
#if CURL_AT_LEAST_VERSION( 7, 85, 0 )
	valid = valid && qcurl_easy_setopt( request, CURLOPT_PROTOCOLS_STR, "https" ) == CURLE_OK;
#else
	valid = valid && qcurl_easy_setopt( request, CURLOPT_PROTOCOLS, httpCurlWord_t( CURLPROTO_HTTPS ) ) == CURLE_OK;
#endif
	if ( *ca )
		valid = valid && qcurl_easy_setopt( request, CURLOPT_CAINFO, ca ) == CURLE_OK;
	if ( strcmp( method, "GET" ) )
		valid = valid && qcurl_easy_setopt( request, CURLOPT_POSTFIELDS, requestBody ) == CURLE_OK;
	if ( !valid || qcurl_multi_add_handle( requests, request ) != CURLM_OK ) {
		Sys_HTTPCancel();
		return false;
	}
	return true;
}
bool Sys_HTTPPoll( httpResult_t *result ) {
	if ( !request || !result )
		return false;
	int running = 0;
	const CURLMcode code = qcurl_multi_perform( requests, &running );
	if ( code == CURLM_CALL_MULTI_PERFORM )
		return false;
	if ( code == CURLM_OK && running )
		return false;
	response.failed = code != CURLM_OK;
	int remaining;
	CURLMsg *message = qcurl_multi_info_read( requests, &remaining );
	response.failed = response.failed || !message || message->msg != CURLMSG_DONE || message->data.result != CURLE_OK;
	httpCurlWord_t status = 0;
	if ( qcurl_easy_getinfo( request, CURLINFO_RESPONSE_CODE, &status ) != CURLE_OK )
		response.failed = true;
	response.status = status >= 100 && status <= 599 ? int( status ) : 0;
	*result = response;
	Sys_HTTPCancel();
	return true;
}
#else
bool Sys_HTTPStart( const char *, const char *, const char *, const char *, const char * ) {
	return false;
}
bool Sys_HTTPPoll( httpResult_t * ) {
	return false;
}
void Sys_HTTPCancel() {
}
#endif
