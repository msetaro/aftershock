#include "../../code/qcommon/q_shared.h"
#include "../../code/qcommon/qcommon.h"
#include "../../code/qcommon/vm_local.h"

static int expected[3];

static intptr_t QDECL entry( int command, int a, int b, int c ) {
	if ( command != 42 || a != expected[0] || b != expected[1] || c != expected[2] ) {
		fprintf( stderr, "FAIL: native entry received stale arguments\n" );
		return -1;
	}
	return 17;
}

extern "C" int __wrap_main( void ) {
	vm_t vm = {};
	vm.entryPoint = entry;
	const int counts[] = { 3, 0, 1, 2 };
	for ( int count : counts ) {
		for ( int i = 0; i < 3; i++ ) expected[i] = i < count ? 11 + i : 0;
		intptr_t result;
		switch ( count ) {
		case 0: result = VM_Call( &vm, 0, 42 ); break;
		case 1: result = VM_Call( &vm, 1, 42, 11 ); break;
		case 2: result = VM_Call( &vm, 2, 42, 11, 12 ); break;
		default: result = VM_Call( &vm, 3, 42, 11, 12, 13 ); break;
		}
		if ( result != 17 || vm.callLevel != 0 ) return 1;
	}
	puts( "PASS: native dispatch initializes unused slots for counts 0 through 3" );
	return 0;
}
