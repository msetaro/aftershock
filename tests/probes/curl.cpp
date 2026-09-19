/* Real option forwarding and a local-file transfer; no network access. */
#include "../../engine/client/cl_curl.cpp"
#include <assert.h>

void QDECL Com_DPrintf( const char *fmt, ... ) {
	(void)fmt;
}

int main( int argc, char **argv ) {
	char data[64] = { 0 }, *private_data = NULL;
	CURL *curl;
	FILE *output;

	assert( argc == 2 );
	output = tmpfile();
	assert( output );
	curl = curl_easy_init();
	assert( curl );
	assert( qcurl_easy_setopt_warn( curl, CURLOPT_URL, argv[1] ) == CURLE_OK );
	assert( qcurl_easy_setopt_warn( curl, CURLOPT_WRITEDATA, (void *)output ) == CURLE_OK );
	assert( qcurl_easy_setopt_warn( curl, CURLOPT_PRIVATE, (void *)output ) == CURLE_OK );
	assert( curl_easy_getinfo( curl, CURLINFO_PRIVATE, &private_data ) == CURLE_OK );
	assert( private_data == (char *)output );
	assert( qcurl_easy_setopt_warn( curl, CURLOPT_NOBODY, 1L ) == CURLE_OK );
	assert( curl_easy_perform( curl ) == CURLE_OK );
	assert( ftell( output ) == 0 );
	assert( qcurl_easy_setopt_warn( curl, CURLOPT_NOBODY, 0L ) == CURLE_OK );
	assert( qcurl_easy_setopt_warn( curl, CURLOPT_MAXFILESIZE_LARGE, (curl_off_t)3 ) == CURLE_OK );
	assert( curl_easy_perform( curl ) == CURLE_FILESIZE_EXCEEDED );
	assert( ftell( output ) >= 0 && ftell( output ) <= 3 );
	rewind( output );
	assert( qcurl_easy_setopt_warn( curl, CURLOPT_MAXFILESIZE_LARGE, (curl_off_t)1024 ) == CURLE_OK );
	assert( curl_easy_perform( curl ) == CURLE_OK );
	rewind( output );
	assert( fread( data, 1, sizeof( data ), output ) == strlen( "aftershock curl options\n" ) );
	assert( strcmp( data, "aftershock curl options\n" ) == 0 );
	curl_easy_cleanup( curl );
	fclose( output );
	puts( "PASS: curl long, pointer and offset options; local-file transfer matched" );
	return 0;
}
