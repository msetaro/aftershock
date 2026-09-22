#include "../../engine/server/sv_checkpoint.cpp"
#include <cassert>

int main() {
	static unsigned char bytes[65536];
	checkpointHeader_t header{};
	strcpy( header.map, "oa_dm1" );
	strcpy( header.game, "baseoa" );
	header.time = 5000;
	header.serverTime = 9000;
	header.residual = 20;
	header.maxclients = 4;
	header.localClient = 2;
	header.pure = 1;
	assert(ValidCheckpointHeader(header));
	auto bad = header;
	bad.localClient = 4;
	assert(!ValidCheckpointHeader(bad));
	bad = header; bad.residual = -1;
	assert(!ValidCheckpointHeader(bad));
	bad = header; strcpy(bad.map,"../outside");
	assert(!ValidCheckpointHeader(bad));
	bad = header; strcpy(bad.game,"baseoa;quit");
	assert(!ValidCheckpointHeader(bad));
	bad = header; bad.time = INT32_MAX;
	assert(!ValidCheckpointHeader(bad));
	checkpointClient_t saved{};
	saved.active = 1;
	strcpy(saved.userinfo,"\\name\\Player");
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
	saved.active = 1; saved.bot = 1; saved.snapshotMsec = 50;
	stateWriter_t writer{ bytes, sizeof(bytes) };
	assert(State_Append(&writer,checkpointHeaderSchema,0,&header));
	assert(State_Append(&writer,checkpointClientSchema,1,&saved));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	checkpointHeader_t decoded;
	uint32_t version;
	assert(State_Find(reader,checkpointHeaderSchema,0,&decoded,&version));
	assert(ValidCheckpointHeader(decoded) && !memcmp(&header,&decoded,sizeof(header)));
	assert(State_Find(reader,checkpointClientSchema,1,&saved,&version));
	assert(ValidCheckpointClient(decoded,1,saved,command));
	puts("PASS: checkpoint map, clock and local-player/bot slot validation");
}
