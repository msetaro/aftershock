#include "../../engine/navigation/behavior_public.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>

int main( int argc, char **argv ) {
	assert(argc == 2);
	FILE *file = std::fopen( argv[1], "rb" );
	assert(file);
	assert(std::fseek(file, 0, SEEK_END) == 0);
	const size_t size = size_t( std::ftell( file ) );
	std::rewind( file );
	void *data = std::malloc( size );
	assert(data && std::fread(data, 1, size, file) == size);
	std::fclose( file );
	aiBehavior_t behavior{};
	assert(AI_ReadBehavior(data,size,&behavior));
	std::free( data );
	static_assert( std::is_trivially_copyable_v<aiBehavior_t> );
	static_assert( std::is_trivially_copyable_v<aiState_t> );
	aiState_t state{};
	aiInputs_t input{};
	input.health = 1;
	assert(AI_UpdateBehavior(behavior,input,20,&state) == AI_PATROL);
	assert(!std::strcmp(AI_StateName(behavior,state),"patrol"));
	input.heard = true;
	assert(AI_UpdateBehavior(behavior,input,20,&state) == AI_INVESTIGATE);
	input.visible = true;
	assert(AI_UpdateBehavior(behavior,input,20,&state) == AI_ATTACK);
	input.health = 0.2f;
	assert(AI_UpdateBehavior(behavior,input,200,&state) == AI_ATTACK);
	assert(AI_UpdateBehavior(behavior,input,300,&state) == AI_COVER);
	assert(!std::strcmp(AI_StateName(behavior,state),"cover"));
	input.visible = false;
	assert(AI_UpdateBehavior(behavior,input,20,&state) == AI_INVESTIGATE); // Parent transition.
	input.visible = true;
	assert(AI_UpdateBehavior(behavior,input,20,&state) == AI_ATTACK);
	const aiState_t saved = state;
	assert(AI_UpdateBehavior(behavior,input,500,&state) == AI_COVER);
	aiState_t restored = saved;
	assert(AI_UpdateBehavior(behavior,input,500,&restored) == AI_COVER);
	assert(state.current == restored.current && state.elapsed == restored.elapsed && state.transitions == restored.transitions);
	input.covered = true;
	assert(AI_UpdateBehavior(behavior,input,999,&state) == AI_COVER);
	assert(AI_UpdateBehavior(behavior,input,1,&state) == AI_ATTACK); // Leaf dwell time.
	input.visible = input.heard = false;
	assert(AI_UpdateBehavior(behavior,input,20,&state) == AI_INVESTIGATE);
	for ( int i = 0; i < 4; ++i )
		assert(AI_UpdateBehavior(behavior,input,1000,&state) == AI_INVESTIGATE);
	assert(AI_UpdateBehavior(behavior,input,999,&state) == AI_INVESTIGATE);
	assert(AI_UpdateBehavior(behavior,input,1,&state) == AI_PATROL);
	std::puts( "PASS: authored leaf/parent transitions, minimum dwell, timeout and POD continuation" );
}
