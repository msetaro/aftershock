#include "../../engine/botlib/be_ai_chat.cpp"
#include <assert.h>
botlib_import_t botimport;
static float chatTime = 12.5f;
float AAS_Time() {
	return chatTime;
}
void Q_strncpyz( char *dest, const char *source, int capacity ) {
	assert(capacity>0 && strlen(source)<size_t(capacity));
	strcpy( dest, source );
}
static bot_consolemessage_t original[MAX_SAVED_CONSOLE_MESSAGES], restored[MAX_SAVED_CONSOLE_MESSAGES];
static unsigned char bytes[4 * 1024 * 1024];
static void InitPool( int count ) {
	allocatedConsoleMessages = count;
	consolemessageheap = original;
	memset( original, 0, sizeof( original ) );
	for ( int i = 0; i < count; ++i ) {
		original[i].next = i + 1 == count ? nullptr : &original[i + 1];
		original[i].prev = i ? &original[i - 1] : nullptr;
	}
	freeconsolemessages = original;
}
int main() {
	bot_chatstate_t actors[2]{}, loaded[2]{};
	botchatstates[1] = &actors[0];
	botchatstates[3] = &actors[1];
	actors[0].gender = CHAT_GENDERMALE;
	actors[0].client = 1;
	strcpy( actors[0].name, "Checkpoint bot" );
	strcpy( actors[0].chatmessage, "pending response" );
	actors[1].client = 3;
	InitPool( 70 );
	for ( int i = 0; i < 67; ++i )
		BotQueueConsoleMessage( i % 2 ? 3 : 1, i % 2, "queued message" );
	BotRemoveConsoleMessage( 1, 1 );
	actors[1].handle = 8192;
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(Bot_WriteChatQueueState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	chatQueueHeader_t header;
	uint32_t version;
	assert(State_Find(reader,chatQueueHeaderSchema,0,&header,&version));
	BotQueueConsoleMessage( 3, 1, "after checkpoint" );
	const int expected = ChatMessageSlot( actors[1].lastmessage );
	assert(expected==0 && actors[1].lastmessage->handle==1);
	consolemessageheap = restored;
	botchatstates[1] = &loaded[0];
	botchatstates[3] = &loaded[1];
	bot_chat_t chat{};
	loaded[0].chat = &chat;
	assert(Bot_ReadChatQueueState(reader,false) && loaded[0].numconsolemessages==0);
	assert(Bot_ReadChatQueueState(reader,true));
	assert(loaded[0].chat==&chat && !strcmp(loaded[0].chatmessage,"pending response"));
	assert(loaded[0].firstmessage==&restored[2] && loaded[1].lastmessage==&restored[65]);
	bot_consolemessage_qvm_t message;
	assert(BotNextConsoleMessage(1,&message)==2 && message.time==chatTime && !strcmp(message.message,"queued message"));
	BotQueueConsoleMessage( 3, 1, "after checkpoint" );
	assert(ChatMessageSlot(loaded[1].lastmessage)==expected && loaded[1].lastmessage->handle==1);
	assert(!strcmp(loaded[1].lastmessage->message,"after checkpoint"));
	BotRemoveConsoleMessage( 1, 2 );
	assert(BotNextConsoleMessage(1,&message)==3);
	// Valid envelope with the last text chunk absent must not alter any live queue.
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,chatQueueHeaderSchema,0,&header));
	assert(ChatQueueLinks(&writer,nullptr,uint32_t(header.count)));
	assert(State_Append(&writer,chatActorSchema,1,&savedChatActors[1]));
	assert(State_Append(&writer,chatActorSchema,3,&savedChatActors[3]));
	assert(ChatQueueChunk(&writer,nullptr,0,64,false));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	const auto before = loaded[0];
	const auto first = *loaded[0].firstmessage;
	assert(!Bot_ReadChatQueueState(reader,true));
	assert(!memcmp(&before,&loaded[0],sizeof(before)) && !memcmp(&first,loaded[0].firstmessage,sizeof(first)));
	// Live/free overlap and a free-list cycle reject saves instead of truncating.
	freeconsolemessages = loaded[0].firstmessage;
	writer = { bytes, sizeof( bytes ) };
	assert(!Bot_WriteChatQueueState(&writer) && !State_Finish(&writer));
	botchatstates[1] = nullptr;
	botchatstates[3] = nullptr;
	InitPool( 70 );
	original[69].next = &original[4];
	writer = { bytes, sizeof( bytes ) };
	assert(!Bot_WriteChatQueueState(&writer) && !State_Finish(&writer));
	// Exercise the actual maximum pool, including its last chunk. Reverse the free
	// list so a single queued message occupies the last slot without filling it.
	InitPool( MAX_SAVED_CONSOLE_MESSAGES );
	for ( int i = 0; i < allocatedConsoleMessages; ++i ) {
		original[i].next = i ? &original[i - 1] : nullptr;
		original[i].prev = i + 1 == allocatedConsoleMessages ? nullptr : &original[i + 1];
	}
	freeconsolemessages = &original[allocatedConsoleMessages - 1];
	actors[0] = {};
	botchatstates[1] = &actors[0];
	BotQueueConsoleMessage( 1, 0, "last slot" );
	writer = { bytes, sizeof( bytes ) };
	assert(Bot_WriteChatQueueState(&writer));
	assert(writer.records==4);
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	consolemessageheap = restored;
	loaded[0] = {};
	botchatstates[1] = &loaded[0];
	assert(Bot_ReadChatQueueState(reader,true));
	assert(loaded[0].firstmessage==&restored[MAX_SAVED_CONSOLE_MESSAGES-1]);
	BotQueueConsoleMessage( 1, 0, "next slot" );
	assert(loaded[0].lastmessage==&restored[MAX_SAVED_CONSOLE_MESSAGES-2]);
	puts( "PASS: relocated chat queues preserve pending text, handles and next allocation across the maximum pool" );
}
