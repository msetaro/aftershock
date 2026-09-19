#include "g_local.h"
#include "botlib.h"
#include <assert.h>

void BotInputToUserCommand( bot_input_t *input, usercmd_t *command, int delta[3], int time );

void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up ) {
	(void)up;
	VectorClear( forward );
	forward[angles[PITCH] == -90 ? 2 : 0] = 1;
	VectorClear( right );
	right[1] = -1;
}

int main( void ) {
	static const struct {
		float direction;
		int positive, negative;
	} cases[] = {
		{ 2, -2, 2 }, { -2, 2, -2 }, { 1, 127, -127 }, { -1, -127, 127 },
		{ 0, 0, 0 }, { 0.5f, 63, -63 }, { -0.5f, -63, 63 },
		{ 50, -50, 50 }, { -50, 50, -50 }
	};
	unsigned i;
	int vertical;
	for ( vertical = 0; vertical < 2; vertical++ ) {
		for ( i = 0; i < sizeof( cases ) / sizeof( cases[0] ); i++ ) {
			bot_input_t input = { 0 };
			usercmd_t command;
			int delta[3] = { 0 };
			input.speed = 400;
			input.weapon = 2;
			input.dir[vertical ? 2 : 0] = cases[i].direction;
			input.dir[1] = cases[i].direction;
			input.viewangles[PITCH] = vertical ? -90 : 0;
			BotInputToUserCommand( &input, &command, delta, 1234 );
			assert( command.serverTime == 1234 && command.weapon == 2 );
			assert( command.forwardmove == cases[i].positive );
			assert( command.rightmove == cases[i].negative );
			assert( command.upmove == (vertical ? cases[i].positive : 0) );
		}
	}
	return 0;
}
