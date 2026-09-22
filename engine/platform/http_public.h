#ifndef HTTP_PUBLIC_H
#define HTTP_PUBLIC_H
#include <stdint.h>

constexpr uint32_t HTTP_MAX_RESPONSE = 32768;
struct httpResult_t {
	int status;
	uint32_t size;
	bool failed;
	char body[HTTP_MAX_RESPONSE + 1];
};
// Main-thread only, one outstanding request. Inputs are copied before returning.
// HTTPS only, verified certificates, no redirects. Empty CA uses system trust.
// Poll returns one completion; a transport failure has failed=true. No logging.
bool Sys_HTTPStart( const char *url, const char *method, const char *token, const char *body, const char *ca );
bool Sys_HTTPPoll( httpResult_t *result );
void Sys_HTTPCancel();
#endif
