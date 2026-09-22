#include "server.h"
#include "../platform/save_public.h"
#include "../botlib/botlib_public.h"
#include "../public/g_native_public.h"
#include "../public/state_replication_public.h"
#include <cmath>

extern botlib_export_t *botlib_export;
extern int bot_enable;

// ponytail: explicit saves use a bounded 64 MiB buffer; revisit only if measured
// full-game captures exceed it. No allocation occurs on the ordinary frame path.
static constexpr size_t CHECKPOINT_CAPACITY = 64 * 1024 * 1024;
struct checkpointHeader_t {
	char map[MAX_QPATH], game[MAX_QPATH];
	int32_t checksum, time, serverTime, residual, restartTime, maxclients, localClient;
	int32_t pure; // Read-only legacy v1 metadata; v2 owns pure mode in server cvars.
	uint32_t protocol;
};
static_assert( sizeof( checkpointHeader_t ) == 164 && offsetof( checkpointHeader_t, protocol ) == 160 );
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
	{ "protocol", offsetof( checkpointHeader_t, protocol ), 1, stateType_t::UInt32, 2 }
};
static constexpr stateSchema_t checkpointHeaderSchema{ "checkpoint", 2, 1, sizeof( checkpointHeader_t ), checkpointHeaderFields, ARRAY_LEN( checkpointHeaderFields ) };
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
	"sv_fps", "sv_maxclients", "sv_pure", "sv_cheats", "sv_snapshotBudget",
	"sv_minRate", "sv_maxRate", "sv_lanForceRate", "sv_padPackets", "sv_levelTimeReset",
	"cm_noAreas", "cm_noCurves", "cm_playerCurveClip", "timescale", "cl_paused", "sv_paused"
};
static struct {
	void *data;
	stateReader_t reader;
	checkpointHeader_t header;
	checkpointClient_t clients[MAX_CLIENTS];
	usercmd_t commands[MAX_CLIENTS];
	uint32_t started;
	bool ready, sent;
	char path[MAX_QPATH];
} checkpointLoad;

bool SV_CheckpointLoading() {
	return checkpointLoad.data != nullptr;
}
void SV_ClearCheckpoint() {
	if ( checkpointLoad.data )
		Z_Free( checkpointLoad.data );
	checkpointLoad = {};
}
int SV_CheckpointClientSlot() {
	return checkpointLoad.ready ? checkpointLoad.header.localClient : -1;
}
bool SV_CheckpointConnect( client_t *client ) {
	const int slot = SV_CheckpointClientSlot();
	if ( slot < 0 || client != &svs.clients[slot] || client->netchan.remoteAddress.type != NA_LOOPBACK )
		return false;
	const auto &saved = checkpointLoad.clients[slot];
	Q_strncpyz( client->userinfo, saved.userinfo, sizeof( client->userinfo ) );
	Q_strncpyz( client->name, saved.name, sizeof( client->name ) );
	client->ping = saved.ping;
	client->rate = saved.rate;
	client->snapshotMsec = saved.snapshotMsec;
	return true;
}
bool SV_CheckpointEnter( client_t *client ) {
	if ( !SV_CheckpointConnect( client ) )
		return false;
	client->lastUsercmd = checkpointLoad.commands[checkpointLoad.header.localClient];
	return true;
}
static bool CheckpointPath( const char *name, bool map ) {
	if ( !memchr( name, 0, MAX_QPATH ) || ( map && !name[0] ) || name[0] == '/' || strstr( name, ".." ) )
		return false;
	for ( const char *p = name; *p; ++p )
		if ( !( ( *p >= 'a' && *p <= 'z' ) || ( *p >= 'A' && *p <= 'Z' ) || ( *p >= '0' && *p <= '9' ) ||
				 *p == '_' || *p == '-' || ( map && *p == '/' && p[1] && p[1] != '/' ) ) )
			return false;
	return true;
}
static bool ValidCheckpointHeader( const checkpointHeader_t &header ) {
	return CheckpointPath( header.map, true ) && CheckpointPath( header.game, false ) &&
		   header.time >= 0 && header.time < 0x78000000 && header.serverTime >= 0 && header.serverTime < 0x78000000 &&
		   header.residual >= 0 && header.residual <= 1000000 && header.restartTime >= 0 && header.restartTime < 0x78000000 &&
		   header.maxclients >= 1 && header.maxclients <= MAX_CLIENTS && header.localClient >= 0 && header.localClient < header.maxclients &&
		   header.pure >= -1 && header.pure <= 1 && header.protocol == AFTERSHOCK_NET_VERSION;
}
static bool ValidCheckpointClient( const checkpointHeader_t &header, int slot, const checkpointClient_t &client, const usercmd_t &command ) {
	if ( slot < 0 || slot >= header.maxclients || client.active > 1 || client.bot > client.active ||
		 !memchr( client.userinfo, 0, sizeof( client.userinfo ) ) || !memchr( client.name, 0, sizeof( client.name ) ) ||
		 !Info_Validate( client.userinfo ) || client.identity || client.identityState != IDENTITY_ANONYMOUS ||
		 client.ping < 0 || client.rate < 0 || client.snapshotMsec < 0 || client.snapshotMsec > 1000 ||
		 command.serverTime < 0 || command.serverTime >= 0x78000000 )
		return false;
	// Authentication is connection-owned; this format accepts only local anonymous play.
	if ( slot == header.localClient )
		return client.active && !client.bot && client.snapshotMsec;
	return !client.active || ( client.bot && client.snapshotMsec );
}
static bool ReadCheckpointHeader( const stateReader_t &reader, checkpointHeader_t *header ) {
	*header = {};
	header->pure = -1;
	uint32_t version;
	if ( !State_Find( reader, checkpointHeaderSchema, 0, header, &version ) )
		return false;
	if ( version == 1 ) {
		// The frozen v1 format predates this field and uses protocol revision 2.
		// Keep that explicit value so a later protocol change requires a migration.
		header->protocol = 2;
		const stateField_t field{ "pure", 0, 1, stateType_t::Int32 };
		const stateSchema_t legacy{ "checkpoint", 1, 1, sizeof( header->pure ), &field, 1 };
		if ( !State_Find( reader, legacy, 0, &header->pure, &version ) || header->pure < 0 )
			return false;
	}
	return ValidCheckpointHeader( *header );
}
static bool ReadServerCheckpoint( const stateReader_t &reader, checkpointHeader_t *header ) {
	uint32_t version;
	qRandomState_t random;
	if ( !ReadCheckpointHeader( reader, header ) ||
		 !State_Find( reader, checkpointRandomSchema, 0, &random, &version ) || random.draws > Q_RANDOM_MAX_DRAWS )
		return false;
	for ( int32_t value : random.signature )
		if ( value < 0 )
			return false;
	for ( int i = 0; i < header->maxclients; ++i ) {
		checkpointClient_t client;
		usercmd_t command;
		if ( !State_Find( reader, checkpointClientSchema, uint32_t( i ), &client, &version ) ||
			 !State_Find( reader, checkpointCommandSchema, uint32_t( i ), &command, &version ) || !ValidCheckpointClient( *header, i, client, command ) )
			return false;
	}
	char value[MAX_GAMESTATE_CHARS];
	const stateField_t field{ "value", 0, sizeof( value ), stateType_t::String };
	const stateSchema_t schema{ "engine.configstring", 1, 1, sizeof( value ), &field, 1 };
	size_t characters = 1;
	for ( uint32_t i = 0; i < MAX_CONFIGSTRINGS; ++i ) {
		if ( !State_Find( reader, schema, i, value, &version ) )
			return false;
		if ( value[0] )
			characters += strlen( value ) + 1;
		if ( characters > MAX_GAMESTATE_CHARS )
			return false;
	}
	for ( uint32_t i = 0; i < MAX_GENTITIES; ++i ) {
		svEntity_t entity{};
		if ( !State_Find( reader, checkpointInterestSchema, i, &entity, &version ) || entity.replicationPriority < 0 || entity.replicationPriority > 3 ||
			 !std::isfinite( entity.interestRadius ) || entity.interestRadius < 0 || entity.interestRadius > 65536 )
			return false;
	}
	return true;
}
static bool CheckpointHeader( checkpointHeader_t *header ) {
	if ( SV_CheckpointLoading() || !com_sv_running->integer || sv.state != SS_GAME || sv.restarting || com_dedicated->integer ||
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
	header->protocol = AFTERSHOCK_NET_VERSION;
	return ValidCheckpointHeader( *header );
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
		if ( !ValidCheckpointClient( header, int( i ), saved, client.lastUsercmd ) || !State_Append( writer, checkpointClientSchema, i, &saved ) || !State_Append( writer, checkpointCommandSchema, i, &client.lastUsercmd ) )
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
		if ( entity.replicationPriority < 0 || entity.replicationPriority > 3 || !std::isfinite( entity.interestRadius ) || entity.interestRadius < 0 || entity.interestRadius > 65536 ||
			 !State_Append( writer, checkpointInterestSchema, i, &entity ) )
			return false;
	}
	for ( uint32_t i = 0; i < ARRAY_LEN( checkpointCvars ); ++i )
		if ( !Cvar_WriteState( writer, "engine.serverCvars", i, checkpointCvars[i] ) )
			return false;
#ifndef DEDICATED
	if ( !CL_WriteCheckpoint( writer ) )
		return false;
#endif
	return Cvar_WriteServerState( writer ) && CM_WritePortalState( writer ) && SV_WriteWorldState( writer );
}
static bool ValidateCheckpointCvars( const stateReader_t &reader ) {
	for ( uint32_t i = 0; i < ARRAY_LEN( checkpointCvars ); ++i )
		if ( !Cvar_ReadState( reader, "engine.serverCvars", i, checkpointCvars[i], false ) )
			return false;
	return Game_ReadCheckpointCvars( &reader, 0 ) != 0;
}
static bool SaveCheckpoint( const char *name ) {
	char path[MAX_QPATH];
	void *data = Z_Malloc( CHECKPOINT_CAPACITY );
	stateWriter_t writer{ data, CHECKPOINT_CAPACITY };
	const bool server = WriteServerCheckpoint( &writer );
	const bool game = server && Game_WriteCheckpoint( &writer );
	const uint32_t now = uint32_t( Sys_Milliseconds() );
	const bool bots = game && BotLib_WriteState( &writer, now );
	const size_t size = bots ? State_Finish( &writer ) : 0;
	stateReader_t reader;
	checkpointHeader_t header;
	const bool readable = size && State_Open( data, size, &reader ) && ReadServerCheckpoint( reader, &header ) && Cvar_CheckStateCapacity( reader, ValidateCheckpointCvars ) && Game_ReadCheckpoint( &reader, 0 ) &&
						  BotLib_ReadState( reader, now, false ) && CM_ReadPortalState( reader, false ) && SV_ReadWorldState( reader, false );
	const bool success = readable && Sys_SaveRevision( saveKind_t::Game, name, data, int( size ), path, sizeof( path ) );
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

static bool RestoreCheckpointCvars( bool prepare ) {
	const auto &reader = checkpointLoad.reader;
	for ( uint32_t i = 0; i < ARRAY_LEN( checkpointCvars ); ++i )
		if ( !Cvar_ReadState( reader, "engine.serverCvars", i, checkpointCvars[i], true, prepare ) )
			return false;
	return Game_ReadCheckpointCvars( &reader, prepare ? 2 : 1 ) != 0;
}
static bool RestoreCheckpointServer() {
	const auto &reader = checkpointLoad.reader;
	const auto &header = checkpointLoad.header;
	uint32_t version;
	sv.time = header.time;
	svs.time = header.serverTime;
	sv.timeResidual = header.residual;
	sv.restartTime = header.restartTime;
	for ( int i = 0; i < header.maxclients; ++i ) {
		auto &saved = checkpointLoad.clients[i];
		if ( !State_Find( reader, checkpointClientSchema, uint32_t( i ), &saved, &version ) ||
			 !State_Find( reader, checkpointCommandSchema, uint32_t( i ), &checkpointLoad.commands[i], &version ) )
			return false;
		if ( !saved.bot )
			continue;
		auto &client = svs.clients[i];
		client.state = CS_ACTIVE;
		client.netchan.remoteAddress.type = NA_BOT;
		client.gentity = SV_GentityNum( i );
		client.lastPacketTime = svs.time;
		client.lastSnapshotTime = svs.time - 9999;
		client.lastUsercmd = checkpointLoad.commands[i];
		client.ping = saved.ping;
		client.rate = saved.rate;
		client.snapshotMsec = saved.snapshotMsec;
		client.country = "BOT";
		Q_strncpyz( client.userinfo, saved.userinfo, sizeof( client.userinfo ) );
		Q_strncpyz( client.name, saved.name, sizeof( client.name ) );
	}
	char value[MAX_GAMESTATE_CHARS];
	const stateField_t field{ "value", 0, sizeof( value ), stateType_t::String };
	const stateSchema_t schema{ "engine.configstring", 1, 1, sizeof( value ), &field, 1 };
	for ( uint32_t i = 0; i < MAX_CONFIGSTRINGS; ++i ) {
		if ( !State_Find( reader, schema, i, value, &version ) )
			return false;
		// Keep the newly negotiated server ID and pure-content transport context.
		if ( i != CS_SYSTEMINFO )
			SV_SetConfigstring( int( i ), value );
	}
	SV_ClearWorld();
	Com_Memset( sv.svEntities, 0, sizeof( sv.svEntities ) );
	Com_Memset( sv.baselineUsed, 0, sizeof( sv.baselineUsed ) );
	for ( uint32_t i = 0; i < MAX_GENTITIES; ++i ) {
		svEntity_t saved{};
		if ( !State_Find( reader, checkpointInterestSchema, i, &saved, &version ) )
			return false;
		sv.svEntities[i].replicationPriority = saved.replicationPriority;
		sv.svEntities[i].interestRadius = saved.interestRadius;
	}
	if ( !SV_ReadWorldState( reader, true ) )
		return false;
	SV_CreateBaseline();
	return CM_ReadPortalState( reader, true );
}
static bool LoadCheckpoint( const char *path ) {
	if ( com_dedicated->integer || SV_CheckpointLoading() || strncmp( path, "saves/", 6 ) )
		return false;
	checkpointHeader_t current;
	if ( com_sv_running->integer && !CheckpointHeader( &current ) )
		return false;
	const int length = Sys_ReadSave( path, nullptr, 0 );
	if ( length <= 0 || size_t( length ) > CHECKPOINT_CAPACITY )
		return false;
	void *data = Z_Malloc( size_t( length ) );
	stateReader_t reader;
	checkpointHeader_t header;
	const bool valid = Sys_ReadSave( path, data, length ) == length && State_Open( data, size_t( length ), &reader ) &&
					   ReadServerCheckpoint( reader, &header ) && !strcmp( header.game, Cvar_VariableString( "fs_game" ) ) &&
					   Cvar_CheckStateCapacity( reader, ValidateCheckpointCvars ) && Cvar_ReadServerState( reader, false );
	if ( !valid ) {
		Z_Free( data );
		return false;
	}
#ifndef DEDICATED
	if ( !CL_ReadCheckpoint( reader, false, false ) ) {
		Z_Free( data );
		return false;
	}
#endif
	char mapPath[MAX_QPATH + 16];
	Com_sprintf( mapPath, sizeof( mapPath ), "maps/%s.bsp", header.map );
	if ( FS_ReadFile( mapPath, nullptr ) <= 0 ) {
		Z_Free( data );
		return false;
	}
	qRandomState_t random;
	uint32_t version;
	const auto previousRandom = Q_GetRandomState();
	const bool compatible = State_Find( reader, checkpointRandomSchema, 0, &random, &version ) && Q_RestoreRandomState( &random );
	const bool preserved = Q_RestoreRandomState( &previousRandom ) != 0;
	if ( !compatible || !preserved ) {
		Z_Free( data );
		return false;
	}
	SV_Shutdown( "Loading checkpoint" );
	checkpointLoad.data = data;
	checkpointLoad.reader = reader;
	checkpointLoad.header = header;
	Q_strncpyz( checkpointLoad.path, path, sizeof( checkpointLoad.path ) );
	if ( !RestoreCheckpointCvars( true ) || !BotLib_PrepareSettings( reader ) ) {
		SV_ClearCheckpoint();
		return false;
	}
	// Normal map initialization builds content owners, but no gameplay frame or
	// automatic bot spawn may run before the recorded world replaces the defaults.
	SV_SpawnServer( header.map, qtrue );
	if ( sv.maxclients != header.maxclients || sv_mapChecksum->integer != header.checksum || ( header.pure >= 0 && sv.pure != header.pure ) ) {
		SV_Shutdown( "Checkpoint map context differs" );
		return false;
	}
	if ( bot_enable ) {
		for ( int i = 0; i < 1000 && !botlib_export->aas.AAS_Initialized(); ++i )
			if ( botlib_export->BotLibStartFrame( 0 ) )
				break;
	}
	const uint32_t now = uint32_t( Sys_Milliseconds() );
	if ( !BotLib_PrepareState( reader, now ) || !Game_ReadCheckpoint( &reader, 0 ) || !CM_ReadPortalState( reader, false ) || !SV_ReadWorldState( reader, false ) ||
		 !Game_ReadCheckpoint( &reader, 1 ) || !BotLib_ReadState( reader, now, true ) || !RestoreCheckpointServer() ) {
		SV_Shutdown( "Checkpoint owner reconstruction failed" );
		return false;
	}
	Cvar_Set( "cl_paused", "0" );
	Cvar_Set( "sv_paused", "0" );
	checkpointLoad.started = uint32_t( Sys_Milliseconds() );
	checkpointLoad.ready = true;
	return true;
}
void SV_LoadGame_f() {
	if ( Cmd_Argc() != 2 || !LoadCheckpoint( Cmd_Argv( 1 ) ) )
		Com_Printf( "Usage: loadgame saves/NAME.REVISION.asstate (matching installed content required)\n" );
}
bool SV_CheckpointFrame() {
	if ( !SV_CheckpointLoading() )
		return false;
	if ( !checkpointLoad.ready )
		return true;
	if ( uint32_t( Sys_Milliseconds() ) - checkpointLoad.started > 120000 ) {
		SV_Shutdown( "Checkpoint local reconnect timed out" );
		return true;
	}
	auto &client = svs.clients[checkpointLoad.header.localClient];
	if ( client.state != CS_ACTIVE )
		return true;
	if ( !checkpointLoad.sent ) {
		SV_IssueNewSnapshot();
		SV_SendClientMessages();
		checkpointLoad.sent = true;
		return true;
	}
#ifndef DEDICATED
	if ( !CL_CheckpointReady() )
		return true;
	const auto &reader = checkpointLoad.reader;
	qRandomState_t random;
	uint32_t version;
	if ( !BotLib_ReadState( reader, uint32_t( Sys_Milliseconds() ), true ) ||
		 !RestoreCheckpointCvars( false ) || !Cvar_ReadServerState( reader, true ) ||
		 !State_Find( reader, checkpointRandomSchema, 0, &random, &version ) ) {
		SV_Shutdown( "Checkpoint final state rejected" );
		return true;
	}
	if ( !CL_ReadCheckpoint( reader, true, Cvar_VariableIntegerValue( "cl_paused" ) != 0 ) ) {
		SV_Shutdown( "Checkpoint local input rejected" );
		return true;
	}
	if ( !Game_RestoreCheckpointRandom( &reader ) || !Q_RestoreRandomState( &random ) ) {
		SV_Shutdown( "Checkpoint random stream is incompatible" );
		return true;
	}
	Com_Printf( "Game loaded: %s\n", checkpointLoad.path );
	SV_ClearCheckpoint();
#endif
	return true;
}
