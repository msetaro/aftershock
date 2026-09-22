#include "server.h"
#include "../botlib/botlib_public.h"
#include "../public/g_native_public.h"
#include "../public/state_replication_public.h"
#include <cmath>

// ponytail: explicit saves use a bounded 64 MiB buffer; revisit only if measured
// full-game captures exceed it. No allocation occurs on the ordinary frame path.
static constexpr size_t CHECKPOINT_CAPACITY = 64 * 1024 * 1024;
struct checkpointHeader_t {
	char map[MAX_QPATH], game[MAX_QPATH];
	int32_t checksum, time, serverTime, residual, restartTime, maxclients, localClient, pure;
};
static constexpr stateField_t checkpointHeaderFields[] = {
	{ "map", offsetof( checkpointHeader_t, map ), MAX_QPATH, stateType_t::String },
	{ "game", offsetof( checkpointHeader_t, game ), MAX_QPATH, stateType_t::String },
	{ "checksum", offsetof( checkpointHeader_t, checksum ), 1, stateType_t::Int32 },
	{ "time", offsetof( checkpointHeader_t, time ), 1, stateType_t::Int32 },
	{ "serverTime", offsetof( checkpointHeader_t, serverTime ), 1, stateType_t::Int32 },
	{ "residual", offsetof( checkpointHeader_t, residual ), 1, stateType_t::Int32 },
	{ "restartTime", offsetof( checkpointHeader_t, restartTime ), 1, stateType_t::Int32 },
	{ "maxclients", offsetof( checkpointHeader_t, maxclients ), 1, stateType_t::Int32 },
	{ "localClient", offsetof( checkpointHeader_t, localClient ), 1, stateType_t::Int32 },
	{ "pure", offsetof( checkpointHeader_t, pure ), 1, stateType_t::Int32 }
};
static constexpr stateSchema_t checkpointHeaderSchema{ "checkpoint", 1, 1, sizeof( checkpointHeader_t ), checkpointHeaderFields, ARRAY_LEN( checkpointHeaderFields ) };
static constexpr stateField_t checkpointRandomFields[] = {
	{ "draws", offsetof( qRandomState_t, draws ), 1, stateType_t::UInt64 },
	{ "seed", offsetof( qRandomState_t, seed ), 1, stateType_t::UInt32 },
	{ "signature", offsetof( qRandomState_t, signature ), 8, stateType_t::Int32 }
};
static constexpr stateSchema_t checkpointRandomSchema{ "engine.random", 1, 1, sizeof( qRandomState_t ), checkpointRandomFields, ARRAY_LEN( checkpointRandomFields ) };
static constexpr stateSchema_t checkpointCommandSchema{ "engine.usercmd", 1, 1, sizeof( usercmd_t ), usercmdSaveFields, ARRAY_LEN( usercmdSaveFields ) };
struct checkpointClient_t {
	uint32_t active, bot;
	char userinfo[MAX_INFO_STRING], name[MAX_NAME_LENGTH];
	int32_t ping, rate, snapshotMsec;
	uint64_t identity;
	uint32_t identityState;
};
static constexpr stateField_t checkpointClientFields[] = {
	{ "active", offsetof( checkpointClient_t, active ), 1, stateType_t::UInt32 },
	{ "bot", offsetof( checkpointClient_t, bot ), 1, stateType_t::UInt32 },
	{ "userinfo", offsetof( checkpointClient_t, userinfo ), MAX_INFO_STRING, stateType_t::String },
	{ "name", offsetof( checkpointClient_t, name ), MAX_NAME_LENGTH, stateType_t::String },
	{ "ping", offsetof( checkpointClient_t, ping ), 1, stateType_t::Int32 },
	{ "rate", offsetof( checkpointClient_t, rate ), 1, stateType_t::Int32 },
	{ "snapshotMsec", offsetof( checkpointClient_t, snapshotMsec ), 1, stateType_t::Int32 },
	{ "identity", offsetof( checkpointClient_t, identity ), 1, stateType_t::UInt64 },
	{ "identityState", offsetof( checkpointClient_t, identityState ), 1, stateType_t::UInt32 }
};
static constexpr stateSchema_t checkpointClientSchema{ "engine.client", 1, 1, sizeof( checkpointClient_t ), checkpointClientFields, ARRAY_LEN( checkpointClientFields ) };
static constexpr stateField_t checkpointInterestFields[] = {
	{ "priority", offsetof( svEntity_t, replicationPriority ), 1, stateType_t::Int32 },
	{ "radius", offsetof( svEntity_t, interestRadius ), 1, stateType_t::Float32 }
};
static constexpr stateSchema_t checkpointInterestSchema{ "engine.interest", 1, 1, sizeof( svEntity_t ), checkpointInterestFields, ARRAY_LEN( checkpointInterestFields ) };
static const char *const checkpointCvars[] = {
	"sv_fps", "sv_maxclients", "sv_pure", "sv_cheats", "sv_gametype", "sv_snapshotBudget",
	"sv_minRate", "sv_maxRate", "sv_lanForceRate", "sv_padPackets", "sv_levelTimeReset",
	"cm_noAreas", "cm_noCurves", "cm_playerCurveClip", "timescale", "cl_paused", "sv_paused"
};
static bool CheckpointHeader( checkpointHeader_t *header ) {
	if ( !com_sv_running->integer || sv.state != SS_GAME || sv.restarting || com_dedicated->integer ||
		 sv.maxclients < 1 || sv.maxclients > MAX_CLIENTS || !svs.clients )
		return false;
	*header = {};
	header->localClient = -1;
	for ( int i = 0; i < sv.maxclients; ++i ) {
		const auto &client = svs.clients[i];
		if ( client.state == CS_FREE )
			continue;
		if ( client.state != CS_ACTIVE || client.downloading || client.download )
			return false;
		if ( client.netchan.remoteAddress.type == NA_BOT )
			continue;
		if ( client.netchan.remoteAddress.type != NA_LOOPBACK || header->localClient >= 0 )
			return false;
		header->localClient = i;
	}
	if ( header->localClient < 0 || strlen( sv_mapname->string ) >= MAX_QPATH || strlen( Cvar_VariableString( "fs_game" ) ) >= MAX_QPATH )
		return false;
	Q_strncpyz( header->map, sv_mapname->string, sizeof( header->map ) );
	Q_strncpyz( header->game, Cvar_VariableString( "fs_game" ), sizeof( header->game ) );
	header->checksum = sv_mapChecksum->integer;
	header->time = sv.time;
	header->serverTime = svs.time;
	header->residual = sv.timeResidual;
	header->restartTime = sv.restartTime;
	header->maxclients = sv.maxclients;
	header->pure = sv.pure;
	return true;
}
static bool WriteServerCheckpoint( stateWriter_t *writer ) {
	checkpointHeader_t header;
	const auto random = Q_GetRandomState();
	if ( !CheckpointHeader( &header ) || random.signature[0] < 0 || random.draws > 100000000 ||
		 !State_Append( writer, checkpointHeaderSchema, 0, &header ) || !State_Append( writer, checkpointRandomSchema, 0, &random ) )
		return false;
	for ( uint32_t i = 0; i < uint32_t( sv.maxclients ); ++i ) {
		const auto &client = svs.clients[i];
		checkpointClient_t saved{};
		saved.active = client.state == CS_ACTIVE;
		if ( saved.active ) {
			saved.bot = client.netchan.remoteAddress.type == NA_BOT;
			Q_strncpyz( saved.userinfo, client.userinfo, sizeof( saved.userinfo ) );
			Q_strncpyz( saved.name, client.name, sizeof( saved.name ) );
			saved.ping = client.ping;
			saved.rate = client.rate;
			saved.snapshotMsec = client.snapshotMsec;
			saved.identity = client.identityId;
			saved.identityState = uint32_t( client.identityState );
		}
		if ( !State_Append( writer, checkpointClientSchema, i, &saved ) || !State_Append( writer, checkpointCommandSchema, i, &client.lastUsercmd ) )
			return false;
	}
	for ( uint32_t i = 0; i < MAX_CONFIGSTRINGS; ++i ) {
		const char *value = sv.configstrings[i] ? sv.configstrings[i] : "";
		const size_t size = strlen( value ) + 1;
		if ( size > MAX_GAMESTATE_CHARS )
			return false;
		const stateField_t field{ "value", 0, uint32_t( size ), stateType_t::String };
		const stateSchema_t schema{ "engine.configstring", 1, 1, uint32_t( size ), &field, 1 };
		if ( !State_Append( writer, schema, i, value ) )
			return false;
	}
	for ( uint32_t i = 0; i < MAX_GENTITIES; ++i ) {
		const auto &entity = sv.svEntities[i];
		if ( entity.replicationPriority < 0 || !std::isfinite( entity.interestRadius ) || entity.interestRadius < 0 ||
			 !State_Append( writer, checkpointInterestSchema, i, &entity ) )
			return false;
	}
	for ( uint32_t i = 0; i < ARRAY_LEN( checkpointCvars ); ++i )
		if ( !Cvar_WriteState( writer, "engine.serverCvars", i, checkpointCvars[i] ) )
			return false;
	return Cvar_WriteServerState( writer ) && CM_WritePortalState( writer );
}
static bool SaveCheckpoint( const char *name ) {
	if ( !name[0] || strlen( name ) > 31 )
		return false;
	for ( const char *p = name; *p; ++p )
		if ( !( *p >= 'a' && *p <= 'z' ) && !( *p >= '0' && *p <= '9' ) && *p != '_' && *p != '-' )
			return false;
	char path[MAX_QPATH];
	int revision;
	for ( revision = 0; revision < 1000; ++revision ) {
		Com_sprintf( path, sizeof( path ), "saves/%s.%03d.asstate", name, revision );
		if ( !FS_FileExists( path ) )
			break;
	}
	if ( revision == 1000 )
		return false;
	void *data = Z_Malloc( CHECKPOINT_CAPACITY );
	stateWriter_t writer{ data, CHECKPOINT_CAPACITY };
	const bool server = WriteServerCheckpoint( &writer );
	const bool game = server && Game_WriteCheckpoint( &writer );
	const uint32_t now = uint32_t( Sys_Milliseconds() );
	const bool bots = game && BotLib_WriteState( &writer, now );
	const size_t size = bots ? State_Finish( &writer ) : 0;
	stateReader_t reader;
	const bool readable = size && State_Open( data, size, &reader ) && Game_ReadCheckpoint( &reader, 0 ) &&
						  BotLib_ReadState( reader, now, false ) && CM_ReadPortalState( reader, false );
	const fileHandle_t file = readable ? FS_FOpenFileWrite( path ) : 0;
	const bool success = file && FS_Write( data, int( size ), file ) == int( size );
	if ( file )
		FS_FCloseFile( file );
	Z_Free( data );
	if ( success )
		Com_Printf( "Game saved: %s (%zu bytes)\n", path, size );
	else
		Com_Printf( "Checkpoint capture failed (%s).\n", !server ? "server" : !game ? "game"
																		  : !bots	? "botlib"
																					: "storage" );
	return success;
}
void SV_SaveGame_f() {
	if ( Cmd_Argc() != 2 || !SaveCheckpoint( Cmd_Argv( 1 ) ) )
		Com_Printf( "Usage: savegame NAME (one active local player; immutable numbered revisions)\n" );
}
