#include "../../engine/platform/sys_runtime.cpp"
#include <stdio.h>

static uint64_t appliedMask;
qboolean Sys_SetAffinityMask( const uint64_t mask ) {
	appliedMask = mask;
	return qtrue;
}

int main( void ) {
	static const struct {
		const char *text;
		uint64_t expected;
	} cases[] = {
		{ "0", 0 }, { "3", 3 }, { "0x10", 16 },
		{ "0x8000000000000000", UINT64_C( 0x8000000000000000 ) },
		{ "A", 255 }, { "P", 15 }, { "E", 240 },
		{ "1+2", 3 }, { "3-1", 2 }, { "1+2-1", 2 },
		{ "P+E", 255 }, { "A-P", 240 }, { "A-A", 0 },
		{ "1-1+2", 2 }, { "0x10+1-0x10", 1 },
		{ "0x8000000000000000+1", UINT64_C( 0x8000000000000001 ) }
	};
	int failures = 0;
	affinityMask = 255;
	pCoreMask = 15;
	eCoreMask = 240;
	for ( const auto &test : cases ) {
		uint64_t value = 0;
		const char *end = parseAffinityMask( test.text, &value, 0 );
		if ( *end || value != test.expected ) {
			fprintf( stderr, "%s: expected %llu, got %llu\n", test.text,
				(unsigned long long)test.expected, (unsigned long long)value );
			++failures;
		}
		appliedMask = 0;
		Sys_ApplyAffinityMask( test.text );
		uint64_t expected = test.expected & affinityMask ? test.expected : affinityMask;
		if ( appliedMask != expected ) {
			fprintf( stderr, "%s: public affinity path differs\n", test.text );
			++failures;
		}
	}
	return failures ? 1 : 0;
}
