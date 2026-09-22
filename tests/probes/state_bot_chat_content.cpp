#include "../../engine/botlib/be_ai_chat.cpp"
#include <assert.h>
botlib_import_t botimport;
float AAS_Time() {
	return 50.0f;
}
int Q_Rand() {
	return 0;
}
int Q_stricmp( const char *left, const char *right ) {
	return strcmp( left, right );
}
int main() {
	char hello[] = "hello", next[] = "next", answer[] = "answer", word[] = "word";
	bot_chatmessage_t lines[3]{ { hello, 60, nullptr }, { next, 25, nullptr }, { answer, 80, nullptr } };
	lines[0].next = &lines[1];
	bot_chattype_t type{};
	strcpy( type.name, "greeting" );
	type.numchatmessages = 2;
	type.firstchatmessage = lines;
	bot_chat_t chat{ &type, {}, {} };
	bot_ichatdata_t cache{};
	cache.chat = &chat;
	strcpy( cache.filename, "bots/test_t.c" );
	strcpy( cache.chatname, "test" );
	ichatdata[3] = &cache;
	bot_chatstate_t actors[3]{};
	actors[0].chat = &chat;
	actors[1].chat = &chat;
	botchatstates[1] = &actors[0];
	botchatstates[4] = &actors[1];
	botchatstates[6] = &actors[2];
	bot_chattype_t privateType{};
	strcpy( privateType.name, "response" );
	privateType.numchatmessages = 1;
	privateType.firstchatmessage = &lines[2];
	bot_chat_t privateChat{ &privateType, {}, {} };
	actors[2].chat = &privateChat;
	bot_chatmessage_t replyLine{ answer, 49, nullptr };
	bot_matchstring_t matchString{ word, nullptr };
	bot_matchpiece_t piece{ MT_STRING, &matchString, 0, nullptr };
	bot_replychatkey_t key{ RCKFL_VARIABLES, nullptr, &piece, nullptr };
	bot_replychat_t reply{ &key, 2.5f, 1, &replyLine, nullptr };
	replychats = &reply;
	bot_synonym_t synonym{ word, 1, nullptr };
	bot_synonymlist_t synonymList{ 7, 1, &synonym, nullptr };
	synonyms = &synonymList;
	bot_randomstring_t randomString{ word, nullptr };
	bot_randomlist_t randomList{ hello, 1, &randomString, nullptr };
	randomstrings = &randomList;
	bot_matchtemplate_t match{ 9, 2, 3, &piece, nullptr };
	matchtemplates = &match;
	static unsigned char bytes[65536];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(Bot_WriteChatContentState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	const char *expected = BotChooseInitialChatMessage( &actors[0], "greeting" );
	assert(!strcmp(expected,"next") && lines[1].time==70);
	char loadedHello[] = "hello", loadedNext[] = "next";
	bot_chatmessage_t loaded[3]{ { loadedHello, 0, nullptr }, { loadedNext, 0, nullptr }, { answer, 0, nullptr } };
	loaded[0].next = &loaded[1];
	bot_chattype_t loadedType = type;
	loadedType.firstchatmessage = loaded;
	bot_chat_t loadedChat{ &loadedType, {}, {} };
	bot_ichatdata_t loadedCache = cache;
	loadedCache.chat = &loadedChat;
	ichatdata[3] = &loadedCache;
	actors[0].chat = &loadedChat;
	actors[1].chat = &loadedChat;
	privateType.firstchatmessage = &loaded[2];
	bot_chatmessage_t loadedReply{ answer, 0, nullptr };
	reply.firstchatmessage = &loadedReply;
	assert(Bot_ReadChatContentState(reader,false) && loaded[1].time==0);
	assert(Bot_ReadChatContentState(reader,true));
	assert(loaded[0].time==60 && loaded[1].time==25 && loaded[2].time==80 && loadedReply.time==49);
	assert(!strcmp(BotChooseInitialChatMessage(&actors[1],"greeting"),expected) && loaded[1].time==70);
	loaded[0].time = 123;
	match.context = 10;
	assert(!Bot_ReadChatContentState(reader,true) && loaded[0].time==123);
	match.context = 9;
	loadedCache.chatname[0] = 'X';
	assert(!Bot_ReadChatContentState(reader,true) && loaded[0].time==123);
	loadedCache.chatname[0] = 't';
	loaded[1].chatmessage = word;
	assert(!Bot_ReadChatContentState(reader,true) && loaded[0].time==123);
	loaded[1].chatmessage = loadedNext;
	// Omit the last (global reply/content) record: earlier owners cannot mutate.
	chatContentPool_t pool;
	assert(ChatContentPool(&pool));
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,chatContentPoolSchema,0,&pool));
	assert(ChatContentRecord(&writer,nullptr,3,&loadedChat,&loadedCache,false,false));
	assert(ChatContentRecord(&writer,nullptr,MAX_CLIENTS+6,&privateChat,nullptr,false,false));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	loaded[0].time = 456;
	assert(!Bot_ReadChatContentState(reader,true) && loaded[0].time==456);
	loaded[1].next = &loaded[0];
	writer = { bytes, sizeof( bytes ) };
	assert(!Bot_WriteChatContentState(&writer) && !State_Finish(&writer));
	loaded[1].next = nullptr;
	loaded[1].time = std::numeric_limits<float>::quiet_NaN();
	writer = { bytes, sizeof( bytes ) };
	assert(!Bot_WriteChatContentState(&writer) && !State_Finish(&writer));
	loaded[1].time = 25;
	bot_ichatdata_t alias = loadedCache;
	ichatdata[4] = &alias;
	writer = { bytes, sizeof( bytes ) };
	assert(!Bot_WriteChatContentState(&writer) && !State_Finish(&writer));
	ichatdata[4] = nullptr;
	puts( "PASS: shared/private chat and reply timers retain line selection, immutable content identity and transactional reads" );
}
