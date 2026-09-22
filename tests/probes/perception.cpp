#include "../../engine/navigation/perception_public.h"
#include <cassert>
#include <cmath>
#include <type_traits>
#include <cstdio>

int main() {
	static_assert( std::is_trivially_copyable_v<aiSenseState_t> );
	const aiSenseSettings_t settings = { 1000, 0.5f, 0.2f, 200 };
	const float eye[3] = {}, forward[3] = { 1, 0, 0 };
	aiCandidate_t targets[3] = {};
	for ( auto &target : targets ) {
		target.position[0] = 100;
		target.alive = target.hostile = target.clearSight = true;
		target.referenceDistance = 100;
		target.maxDistance = 1000;
		target.rolloff = 1;
		target.model = S_DISTANCE_INVERSE;
	}
	targets[0].entity = 4;
	targets[1].entity = 2;
	targets[2].entity = 1;
	targets[2].hostile = false;
	targets[2].position[0] = 1;
	aiSenseState_t state{};
	auto seen = AI_Sense( settings, eye, forward, targets, 3, 20, &state );
	assert(seen.target == 2 && seen.visible && !seen.heard); // Stable tie, ignore friendly.
	targets[0].clearSight = targets[1].clearSight = false;
	seen = AI_Sense( settings, eye, forward, targets, 3, 20, &state );
	assert(seen.target == 2 && !seen.visible && !seen.heard); // Remember last seen position.
	targets[0].position[0] = -200;
	targets[0].loudness = 1;
	targets[0].blockedSound = true;
	seen = AI_Sense( settings, eye, forward, targets, 3, 20, &state );
	assert(!seen.heard); // Inverse-distance .5 times audio's occluded .35 < .2 threshold.
	targets[0].blockedSound = false;
	seen = AI_Sense( settings, eye, forward, targets, 3, 20, &state );
	assert(seen.target == 4 && !seen.visible && seen.heard);
	assert(std::fabs(seen.gain-0.5f) < 0.000001f && seen.position[0] == -200);
	const aiSenseState_t saved = state;
	targets[0].loudness = 0;
	targets[0].clearSight = true;
	seen = AI_Sense( settings, eye, forward, targets, 3, 199, &state );
	assert(seen.target == 4 && !seen.visible && !seen.heard); // Behind the sight cone.
	seen = AI_Sense( settings, eye, forward, targets, 3, 1, &state );
	assert(seen.target == -1);
	aiSenseState_t restored = saved;
	seen = AI_Sense( settings, eye, forward, targets, 3, 200, &restored );
	assert(seen.target == -1 && restored.age == state.age);
	const aiCoverPoint_t covers[] = {
		{ { 10, 0, 0 }, 6, false, true }, // Unreachable.
		{ { 20, 0, 0 }, 7, true, false }, // Exposed to the threat.
		{ { 150, 0, 0 }, 9, true, true },
		{ { 100, 0, 0 }, 8, true, true }
	};
	float position[3];
	assert(AI_SelectCover(eye,covers,4,200,position) == 8 && position[0] == 100);
	assert(AI_SelectCover(eye,covers,4,50,position) == -1);
	std::puts( "PASS: sight cone/occlusion, audio-model hearing, stable targets, memory and reachable protected cover" );
}
