#include "client.h"
#include "../public/state_replication_public.h"
#include "../public/ui_native_public.h"
#include <cmath>

struct clientCheckpoint_t {
	int32_t time, oldTime, frameTime, clock, extrapolated, newSnapshots, weapon;
	float angles[3], sensitivity;
};
static_assert( sizeof( clientCheckpoint_t ) == 44 && offsetof( clientCheckpoint_t, angles ) == 28 );
static constexpr stateField_t clientCheckpointFields[] = {
	{ "time", offsetof( clientCheckpoint_t, time ), 1, stateType_t::Int32 },
	{ "oldTime", offsetof( clientCheckpoint_t, oldTime ), 1, stateType_t::Int32 },
	{ "frameTime", offsetof( clientCheckpoint_t, frameTime ), 1, stateType_t::Int32 },
	{ "clock", offsetof( clientCheckpoint_t, clock ), 1, stateType_t::Int32 },
	{ "extrapolated", offsetof( clientCheckpoint_t, extrapolated ), 1, stateType_t::Int32 },
	{ "newSnapshots", offsetof( clientCheckpoint_t, newSnapshots ), 1, stateType_t::Int32 },
	{ "weapon", offsetof( clientCheckpoint_t, weapon ), 1, stateType_t::Int32 },
	{ "angles", offsetof( clientCheckpoint_t, angles ), 3, stateType_t::Float32 },
	{ "sensitivity", offsetof( clientCheckpoint_t, sensitivity ), 1, stateType_t::Float32 }
};
static constexpr stateSchema_t clientCheckpointSchema{ "client.input", 1, 1, sizeof( clientCheckpoint_t ), clientCheckpointFields, ARRAY_LEN( clientCheckpointFields ) };
static constexpr stateSchema_t clientCommandSchema{ "client.usercmd", 1, 1, sizeof( usercmd_t ), usercmdSaveFields, ARRAY_LEN( usercmdSaveFields ) };
static bool ValidClientCheckpoint( const clientCheckpoint_t &saved, const usercmd_t &command ) {
	const int32_t clocks[] = { saved.time, saved.oldTime, saved.frameTime, saved.clock, command.serverTime };
	for ( int32_t time : clocks )
		if ( time < 0 || time >= 0x78000000 )
			return false;
	for ( float angle : saved.angles )
		if ( !std::isfinite( angle ) )
			return false;
	return saved.extrapolated >= 0 && saved.extrapolated <= 1 && saved.newSnapshots >= 0 && saved.newSnapshots <= 1 &&
		   saved.weapon >= 0 && saved.weapon <= 255 && std::isfinite( saved.sensitivity ) && saved.sensitivity >= 0;
}
bool CL_CheckpointReady() {
	return cls.state == CA_ACTIVE && cl.snap.valid;
}
bool CL_WriteCheckpoint( stateWriter_t *writer ) {
	const int64_t clock = int64_t( cls.realtime ) + cl.serverTimeDelta;
	if ( !writer || !CL_CheckpointReady() || clock < 0 || clock >= 0x78000000 )
		return false;
	const clientCheckpoint_t saved{ cl.serverTime, cl.oldServerTime, cl.oldFrameServerTime, int32_t( clock ), cl.extrapolatedSnapshot, cl.newSnapshots,
		cl.cgameUserCmdValue, { cl.viewangles[0], cl.viewangles[1], cl.viewangles[2] }, cl.cgameSensitivity };
	const auto &command = cl.cmds[cl.cmdNumber & CMD_MASK];
	return ValidClientCheckpoint( saved, command ) && State_Append( writer, clientCheckpointSchema, 0, &saved ) && State_Append( writer, clientCommandSchema, 0, &command );
}
bool CL_ReadCheckpoint( const stateReader_t &reader, bool apply, bool paused ) {
	clientCheckpoint_t saved;
	usercmd_t command;
	uint32_t version;
	if ( !State_Find( reader, clientCheckpointSchema, 0, &saved, &version ) || !State_Find( reader, clientCommandSchema, 0, &command, &version ) ||
		 !ValidClientCheckpoint( saved, command ) )
		return false;
	const int64_t delta = int64_t( saved.clock ) - cls.realtime;
	if ( delta < INT32_MIN || delta > INT32_MAX )
		return false;
	if ( !apply )
		return true;
	if ( !CL_CheckpointReady() )
		return false;
	cl.serverTime = saved.time;
	cl.oldServerTime = saved.oldTime;
	cl.oldFrameServerTime = saved.frameTime;
	cl.serverTimeDelta = int32_t( delta );
	cl.extrapolatedSnapshot = qboolean( saved.extrapolated );
	cl.newSnapshots = qboolean( saved.newSnapshots );
	VectorCopy( saved.angles, cl.viewangles );
	cl.cgameUserCmdValue = saved.weapon;
	cl.cgameSensitivity = saved.sensitivity;
	// Loopback bypasses packet throttling. Keep the pending command, with fresh
	// connection sequence numbers; duplicated old commands are discarded normally.
	for ( auto &queued : cl.cmds )
		queued = command;
	if ( paused )
		NativeUI_SetActiveMenu( UIMENU_INGAME );
	return true;
}
