#include "../../engine/server/sv_checkpoint.cpp"
#include <cassert>

int main() {
	static unsigned char bytes[1024 * 1024];
	checkpointHeader_t header{};
	strcpy( header.map, "oa_dm1" );
	strcpy( header.game, "baseoa" );
	header.time = 5000;
	header.serverTime = 9000;
	header.residual = 20;
	header.maxclients = 4;
	header.localClient = 2;
	header.pure = 1;
	header.protocol = AFTERSHOCK_NET_VERSION;
	assert(ValidCheckpointHeader(header));
	auto bad = header;
	bad.protocol++;
	assert(!ValidCheckpointHeader(bad));
	bad = header;
	bad.localClient = 4;
	assert(!ValidCheckpointHeader(bad));
	bad = header;
	bad.residual = -1;
	assert(!ValidCheckpointHeader(bad));
	bad = header;
	strcpy( bad.map, "../outside" );
	assert(!ValidCheckpointHeader(bad));
	bad = header;
	strcpy( bad.game, "baseoa;quit" );
	assert(!ValidCheckpointHeader(bad));
	bad = header;
	bad.time = INT32_MAX;
	assert(!ValidCheckpointHeader(bad));
	checkpointClient_t saved{};
	saved.active = 1;
	strcpy( saved.userinfo, "\\name\\Player" );
	saved.snapshotMsec = 50;
	usercmd_t command{};
	assert(ValidCheckpointClient(header,2,saved,command));
	assert(!ValidCheckpointClient(header,1,saved,command));
	saved.bot = 1;
	assert(ValidCheckpointClient(header,1,saved,command));
	assert(!ValidCheckpointClient(header,2,saved,command));
	saved.identityState = IDENTITY_VERIFIED;
	assert(!ValidCheckpointClient(header,1,saved,command));
	saved.identityState = IDENTITY_ANONYMOUS;
	saved.active = 2;
	assert(!ValidCheckpointClient(header,1,saved,command));
	saved = {};
	assert(ValidCheckpointClient(header,1,saved,command));
	assert(!ValidCheckpointClient(header,2,saved,command));
	saved.active = 1;
	saved.bot = 1;
	saved.snapshotMsec = 50;
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(State_Append(&writer,checkpointHeaderSchema,0,&header));
	assert(State_Append(&writer,checkpointClientSchema,1,&saved));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	checkpointHeader_t decoded;
	uint32_t version;
	assert(ReadCheckpointHeader(reader,&decoded));
	header.pure = -1; // v2 removes this redundant legacy header field.
	assert(ValidCheckpointHeader(decoded) && !memcmp(&header,&decoded,sizeof(header)));
	assert(State_Find(reader,checkpointClientSchema,1,&saved,&version));
	assert(ValidCheckpointClient(decoded,1,saved,command));
	assert(!ReadServerCheckpoint(reader,&decoded)); // Required engine records are absent.
	for ( int invalid = 0; invalid < 2; ++invalid ) {
		writer = { bytes, sizeof( bytes ) };
		const qRandomState_t random{};
		assert(State_Append(&writer,checkpointHeaderSchema,0,&header));
		assert(State_Append(&writer,checkpointRandomSchema,0,&random));
		for ( int i = 0; i < header.maxclients; ++i ) {
			saved = {};
			saved.active = i == 1 || i == header.localClient;
			saved.bot = i == 1;
			saved.snapshotMsec = saved.active ? 50 : 0;
			assert(State_Append(&writer,checkpointClientSchema,uint32_t(i),&saved));
			assert(State_Append(&writer,checkpointCommandSchema,uint32_t(i),&command));
		}
		const stateField_t field{ "value", 0, 1, stateType_t::String };
		const stateSchema_t config{ "engine.configstring", 1, 1, 1, &field, 1 };
		for ( uint32_t i = 0; i < MAX_CONFIGSTRINGS; ++i )
			assert(State_Append(&writer,config,i,""));
		for ( uint32_t i = 0; i < MAX_GENTITIES; ++i ) {
			svEntity_t entity{};
			entity.replicationPriority = invalid && i == MAX_GENTITIES - 1 ? 4 : 0;
			assert(State_Append(&writer,checkpointInterestSchema,i,&entity));
		}
		assert(State_Open(bytes,State_Finish(&writer),&reader));
		assert(ReadServerCheckpoint(reader,&decoded)==!invalid);
	}
	puts( "PASS: checkpoint map, clock and local-player/bot slot validation" );
}
