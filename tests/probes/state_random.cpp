#include "../../engine/public/state_public.h"
#define NATIVE_NAMESPACE game
#define NATIVE_SOURCE "bg/bg_lib.cpp"
#include "../../game/module.cpp"

int main() {
	game::srand( 0xfedcba98U );
	uint32_t reference = 0xfedcba98U;
	for ( int i = 0; i < 4096; ++i ) {
		reference = 69069U * reference + 1U;
		assert( game::rand() == int( reference & 0x7fffU ) );
	}
	const uint32_t seed = game::Q_GetRandomSeed();
	assert( seed == reference );
	const stateField_t field = { "seed", 0, 1, stateType_t::UInt32 };
	const stateSchema_t schema = { "game_random", 1, 1, sizeof( seed ), &field, 1 };
	unsigned char bytes[256];
	const size_t size = State_Write( schema, &seed, bytes, sizeof( bytes ) );
	assert( size );
	int expected[256];
	for ( int &value : expected )
		value = game::rand();
	game::srand( 17 );
	uint32_t restored = 0, version;
	assert( State_Read( schema, bytes, size, &restored, &version ) && version == 1 );
	game::srand( restored );
	for ( int value : expected )
		assert( game::rand() == value );
	puts( "PASS: native RNG continuation preserves the original sequence and saved high bits" );
}
