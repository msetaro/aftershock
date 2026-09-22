#define NATIVE_NAMESPACE game
#define NATIVE_SOURCE "game/g_composed.cpp"
#include "../../game/module.cpp"

namespace game {
level_locals_t level;
}
int main() {
	game::G_ResetComposed();
	for ( uint32_t i = 0; i < MAX_GENTITIES; ++i ) {
		auto &state = game::composed[i];
		state.started = UINT32_C( 0xffff0000 ) + i;
		state.nextTouch = state.started + 900;
		state.firstFrame = int( i % 31 );
		state.frames = 31;
		state.frameMS = 100;
		state.waitMS = 250;
		state.sound = int( i % MAX_SOUNDS );
		state.splashRadius = float( i ) * 0.25f;
		state.loopAnimation = ( i % 2 ) == 0;
		state.once = ( i % 3 ) == 0;
		state.loopSound = ( i % 5 ) == 0;
	}
	static game::composedState_t expected[MAX_GENTITIES];
	memcpy( expected, game::composed, sizeof( expected ) );
	static unsigned char bytes[131072];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert( game::G_WriteComposedState(&writer) );
	const size_t size = State_Finish( &writer );
	stateReader_t reader;
	assert( size && State_Open(bytes,size,&reader) );
	game::G_ResetComposed();
	assert( game::G_ReadComposedState(reader,false) );
	assert( !game::composed[0].frames );
	assert( game::G_ReadComposedState(reader,true) );
	assert( !memcmp(expected,game::composed,sizeof(expected)) );
	game::gentity_t entity{};
	entity.s.number = 17;
	game::level.time = int32_t( expected[17].started + 1250 );
	game::ComposedThink( &entity );
	assert( entity.s.frame==expected[17].firstFrame+12 );
	assert( entity.nextthink==game::level.time+FRAMETIME );
	game::composed[17].frameMS = 0;
	writer = { bytes, sizeof( bytes ) };
	assert( !game::G_WriteComposedState(&writer) && !State_Finish(&writer) );
	puts( "PASS: composed animation, trigger and audio clocks restore with identical continuation" );
}
