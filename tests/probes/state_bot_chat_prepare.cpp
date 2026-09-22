#include "../../engine/botlib/be_ai_chat.cpp"
#include "state_bot_weight_io.h"
int botDeveloper;
float AAS_Time() {
	return 50;
}
int Q_Rand() {
	return 0;
}
static void ClearContent() {
	for ( auto *state : botchatstates )
		if ( state && state->chat ) {
			bool cached = false;
			for ( const auto *cache : ichatdata )
				cached |= cache && cache->chat == state->chat;
			if ( !cached )
				FreeMemory( state->chat );
			state->chat = nullptr;
		}
	for ( auto *&cache : ichatdata )
		if ( cache ) {
			FreeMemory( cache->chat );
			FreeMemory( cache );
			cache = nullptr;
		}
}
int main() {
	botimport.FS_FOpenFile = Open;
	botimport.FS_Read = Read;
	botimport.FS_FCloseFile = Close;
	botimport.Print = Print;
	sourceText = "chat \"test\" { type \"greeting\" { \"hello\"; \"next\"; } }";
	auto *shared = BotLoadInitialChat( "checkpoint_w.c", "test" );
	auto *owned = BotLoadInitialChat( "checkpoint_w.c", "test" );
	assert(shared && owned);
	ichatdata[7] = (bot_ichatdata_t *)GetClearedMemory( sizeof( bot_ichatdata_t ) );
	ichatdata[7]->chat = shared;
	strcpy( ichatdata[7]->filename, shared->filename );
	strcpy( ichatdata[7]->chatname, shared->chatname );
	bot_chatstate_t actors[3]{};
	actors[0].chat = actors[1].chat = shared;
	actors[2].chat = owned;
	botchatstates[3] = &actors[0];
	botchatstates[5] = &actors[1];
	botchatstates[MAX_CLIENTS] = &actors[2];
	shared->types->firstchatmessage->time = 63.5f;
	owned->types->firstchatmessage->time = 49.25f;
	bot_consolemessage_t heap[3]{};
	allocatedConsoleMessages = 3;
	consolemessageheap = heap;
	heap[0].next = &heap[1];
	heap[1].prev = &heap[0];
	heap[1].next = &heap[2];
	heap[2].prev = &heap[1];
	freeconsolemessages = heap;
	BotQueueConsoleMessage( MAX_CLIENTS, 0, "pending" );
	static unsigned char bytes[65536];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(Bot_WriteChatQueueState(&writer) && Bot_WriteChatContentState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	char expected[MAX_MESSAGE_SIZE];
	strcpy( expected, BotChooseInitialChatMessage( &actors[0], "greeting" ) );
	assert(!Bot_PrepareChatQueueState(reader) && !Bot_PrepareChatContentState(reader));
	ClearContent();
	memset( botchatstates, 0, sizeof( botchatstates ) );
	memset( heap, 0, sizeof( heap ) );
	freeconsolemessages = nullptr;
	assert(allocations==0 && Bot_PrepareChatQueueState(reader));
	assert(botchatstates[3] && botchatstates[5] && botchatstates[MAX_CLIENTS] && !botchatstates[1]);
	bot_consolemessage_qvm_t message;
	assert(BotNextConsoleMessage(MAX_CLIENTS,&message)==1 && !strcmp(message.message,"pending"));
	assert(Bot_PrepareChatContentState(reader));
	assert(ichatdata[7] && !ichatdata[0]);
	assert(botchatstates[3]->chat==botchatstates[5]->chat && botchatstates[3]->chat==ichatdata[7]->chat);
	assert(botchatstates[MAX_CLIENTS]->chat!=ichatdata[7]->chat);
	assert(botchatstates[MAX_CLIENTS]->chat->types->firstchatmessage->time==49.25f);
	assert(!strcmp(BotChooseInitialChatMessage(botchatstates[3],"greeting"),expected));
	static unsigned char incomplete[65536];
	stateWriter_t partial{ incomplete, sizeof( incomplete ) };
	chatContentPool_t pool;
	assert(ChatContentPool(&pool));
	assert(State_Append(&partial,chatContentPoolSchema,0,&pool));
	assert(ChatContentRecord(&partial,nullptr,7,ichatdata[7]->chat,ichatdata[7],false,false));
	assert(ChatContentRecord(&partial,nullptr,2*MAX_CLIENTS+1,nullptr,nullptr,true,false));
	stateReader_t missing;
	assert(State_Open(incomplete,State_Finish(&partial),&missing));
	ClearContent();
	const int actorAllocations = allocations;
	assert(!Bot_PrepareChatContentState(missing) && allocations==actorAllocations);

	sourceText = "chat \"test\" { type \"greeting\" { \"changed\"; \"next\"; } }";
	assert(!Bot_PrepareChatContentState(reader) && allocations==actorAllocations);
	for ( const auto *cache : ichatdata )
		assert(!cache);
	for ( auto *&state : botchatstates )
		if ( state ) {
			assert(!state->chat);
			FreeMemory( state );
			state = nullptr;
		}
	assert(allocations==0);
	// Missing actor data must not publish actors or touch the existing queue heap.
	chatQueueHeader_t header;
	uint32_t version;
	assert(State_Find(reader,chatQueueHeaderSchema,0,&header,&version));
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,chatQueueHeaderSchema,0,&header));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	const auto before = heap[0];
	assert(!Bot_PrepareChatQueueState(reader) && allocations==0);
	assert(!memcmp(&before,&heap[0],sizeof(before)));
	puts( "PASS: chat construction preserves exact actor/cache slots, private content, timers and queued messages" );
}
