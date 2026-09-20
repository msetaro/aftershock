#ifndef NET_HISTORY_PUBLIC_H
#define NET_HISTORY_PUBLIC_H

#include <stdint.h>
#include <type_traits>

inline constexpr uint32_t NET_HISTORY_FRAMES = 64;
inline constexpr uint32_t NET_HISTORY_ENTITIES = 1024;
inline constexpr uint32_t NET_HISTORY_BOXES = 2048;
inline constexpr uint32_t NET_MAX_REWIND_MS = 1000;

struct netBox_t {
	float mins[3], maxs[3];
};
struct netHistoryEntity_t {
	uint64_t generation; // Changes at spawn, respawn or teleport; zero is absent.
	uint32_t firstBox, boxCount;
};
struct netHistoryFrame_t {
	uint32_t time, boxCount;
	netHistoryEntity_t entities[NET_HISTORY_ENTITIES];
	netBox_t boxes[NET_HISTORY_BOXES];
};
// Caller owns storage. Records are copied; queries borrow until the next store/reset.
struct netHistory_t {
	uint32_t next, count;
	netHistoryFrame_t frames[NET_HISTORY_FRAMES];
};
struct netHistoryQuery_t {
	const netHistoryFrame_t *older, *newer;
	uint32_t time, rewindMs;
	float fraction;
	bool clamped;
};
struct netBoxHit_t {
	float fraction, normal[3];
	uint32_t box;
	bool startSolid, allSolid;
};
static_assert( sizeof( netBox_t ) == 24 && std::is_trivially_copyable_v<netBox_t> );
static_assert( sizeof( netHistoryEntity_t ) == 16 && std::is_trivially_copyable_v<netHistoryEntity_t> );
static_assert( std::is_trivially_destructible_v<netHistory_t> && sizeof( netHistory_t ) < 5 * 1024 * 1024 );

void NET_HistoryReset( netHistory_t *history );
bool NET_HistoryStore( netHistory_t *history, const netHistoryFrame_t *frame );
bool NET_HistoryQuery( const netHistory_t *history, uint32_t now, uint32_t viewTime, uint32_t maximumRewind, netHistoryQuery_t *query );
uint32_t NET_HistoryEntity( const netHistoryQuery_t *query, uint32_t entity, uint64_t generation, netBox_t *boxes, uint32_t capacity );
bool NET_TraceBoxes( const float start[3], const float end[3], const netBox_t *boxes, uint32_t count, float maximumFraction, netBoxHit_t *hit );

#endif
