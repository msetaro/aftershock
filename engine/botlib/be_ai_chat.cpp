/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*****************************************************************************
 * name:		be_ai_chat.c
 *
 * desc:		bot chat AI
 *
 * $Archive: /MissionPack/code/botlib/be_ai_chat.c $
 *
 *****************************************************************************/

#include <inttypes.h>
#include "../qcommon/filesystem_public.h"
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon_public.h"
#include "l_memory.h"
#include "l_libvar.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"
#include "l_log.h"
#include "aasfile.h"
#include "botlib_public.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_ea.h"
#include "be_ai_chat.h"
#include "../../third_party/sha256/sha-256.h"
#include <algorithm>
#include <cmath>
#include <functional>


//escape character
#define ESCAPE_CHAR				0x01	//'_'
//
// "hi ", people, " ", 0, " entered the game"
//becomes:
// "hi _rpeople_ _v0_ entered the game"
//

//match piece types
#define MT_VARIABLE					1		//variable match piece
#define MT_STRING					2		//string match piece
//reply chat key flags
#define RCKFL_AND					1		//key must be present
#define RCKFL_NOT					2		//key must be absent
#define RCKFL_NAME					4		//name of bot must be present
#define RCKFL_STRING				8		//key is a string
#define RCKFL_VARIABLES				16		//key is a match template
#define RCKFL_BOTNAMES				32		//key is a series of botnames
#define RCKFL_GENDERFEMALE			64		//bot must be female
#define RCKFL_GENDERMALE			128		//bot must be male
#define RCKFL_GENDERLESS			256		//bot must be genderless
//time to ignore a chat message after using it
#define CHATMESSAGE_RECENTTIME	20

//the actuall chat messages
typedef struct bot_chatmessage_s {
	char *chatmessage; //chat message string
	float time; //last time used
	struct bot_chatmessage_s *next; //next chat message in a list
} bot_chatmessage_t;
//bot chat type with chat lines
typedef struct bot_chattype_s {
	char name[MAX_CHATTYPE_NAME];
	int numchatmessages;
	bot_chatmessage_t *firstchatmessage;
	struct bot_chattype_s *next;
} bot_chattype_t;
//bot chat lines
typedef struct bot_chat_s {
	bot_chattype_t *types;
	char filename[MAX_QPATH], chatname[MAX_QPATH];
} bot_chat_t;

//random string
typedef struct bot_randomstring_s {
	char *string;
	struct bot_randomstring_s *next;
} bot_randomstring_t;
//list with random strings
typedef struct bot_randomlist_s {
	char *string;
	int numstrings;
	bot_randomstring_t *firstrandomstring;
	struct bot_randomlist_s *next;
} bot_randomlist_t;

//synonym
typedef struct bot_synonym_s {
	char *string;
	float weight;
	struct bot_synonym_s *next;
} bot_synonym_t;
//list with synonyms
typedef struct bot_synonymlist_s {
	uint64_t context;
	float totalweight;
	bot_synonym_t *firstsynonym;
	struct bot_synonymlist_s *next;
} bot_synonymlist_t;

//fixed match string
typedef struct bot_matchstring_s {
	char *string;
	struct bot_matchstring_s *next;
} bot_matchstring_t;

//piece of a match template
typedef struct bot_matchpiece_s {
	int type;
	bot_matchstring_t *firststring;
	int variable;
	struct bot_matchpiece_s *next;
} bot_matchpiece_t;
//match template
typedef struct bot_matchtemplate_s {
	uint64_t context;
	int type;
	int subtype;
	bot_matchpiece_t *first;
	struct bot_matchtemplate_s *next;
} bot_matchtemplate_t;

//reply chat key
typedef struct bot_replychatkey_s {
	int flags;
	char *string;
	bot_matchpiece_t *match;
	struct bot_replychatkey_s *next;
} bot_replychatkey_t;
//reply chat
typedef struct bot_replychat_s {
	bot_replychatkey_t *keys;
	float priority;
	int numchatmessages;
	bot_chatmessage_t *firstchatmessage;
	struct bot_replychat_s *next;
} bot_replychat_t;

//string list
typedef struct bot_stringlist_s {
	char *string;
	struct bot_stringlist_s *next;
} bot_stringlist_t;

//chat state of a bot
typedef struct bot_chatstate_s {
	int gender; //0=it, 1=female, 2=male
	int client; //client number
	char name[32]; //name of the bot
	char chatmessage[MAX_MESSAGE_SIZE];
	int handle;
	//the console messages visible to the bot
	bot_consolemessage_t *firstmessage; //first message is the first typed message
	bot_consolemessage_t *lastmessage; //last message is the last typed message, bottom of console
	//number of console messages stored in the state
	int numconsolemessages;
	//the bot chat lines
	bot_chat_t *chat;
} bot_chatstate_t;

typedef struct {
	bot_chat_t *chat;
	char filename[MAX_QPATH];
	char chatname[MAX_QPATH];
} bot_ichatdata_t;

static bot_ichatdata_t *ichatdata[MAX_CLIENTS];

static bot_chatstate_t *botchatstates[MAX_CLIENTS + 1];
//console message heap
static bot_consolemessage_t *consolemessageheap = NULL;
static int allocatedConsoleMessages;
static bot_consolemessage_t *freeconsolemessages = NULL;
//list with match strings
static bot_matchtemplate_t *matchtemplates = NULL;
//list with synonyms
static bot_synonymlist_t *synonyms = NULL;
//list with random strings
static bot_randomlist_t *randomstrings = NULL;
//reply chats
static bot_replychat_t *replychats = NULL;

//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
static bot_chatstate_t *BotChatStateFromHandle( int handle ) {
	if ( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "chat state handle %d out of range\n", handle );
		return NULL;
	} //end if
	if ( !botchatstates[handle] ) {
		botimport.Print( PRT_FATAL, "invalid chat state %d\n", handle );
		return NULL;
	} //end if
	return botchatstates[handle];
} //end of the function BotChatStateFromHandle
//===========================================================================
// initialize the heap with unused console messages
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void InitConsoleMessageHeap( void ) {
	int i, max_messages;

	if ( consolemessageheap )
		FreeMemory( consolemessageheap );
	//
	max_messages = LibVarInteger( "max_messages", "1024", 2, 65536 );
	consolemessageheap = (bot_consolemessage_t *)GetClearedHunkMemory(max_messages *
												sizeof(bot_consolemessage_t));
	allocatedConsoleMessages = max_messages;
	consolemessageheap[0].prev = NULL;
	consolemessageheap[0].next = &consolemessageheap[1];
	for ( i = 1; i < max_messages - 1; i++ ) {
		consolemessageheap[i].prev = &consolemessageheap[i - 1];
		consolemessageheap[i].next = &consolemessageheap[i + 1];
	} //end for
	consolemessageheap[max_messages - 1].prev = &consolemessageheap[max_messages - 2];
	consolemessageheap[max_messages - 1].next = NULL;
	//pointer to the free console messages
	freeconsolemessages = consolemessageheap;
} //end of the function InitConsoleMessageHeap
//===========================================================================
// allocate one console message from the heap
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static bot_consolemessage_t *AllocConsoleMessage( void ) {
	bot_consolemessage_t *message;
	message = freeconsolemessages;
	if ( freeconsolemessages )
		freeconsolemessages = freeconsolemessages->next;
	if ( freeconsolemessages )
		freeconsolemessages->prev = NULL;
	return message;
} //end of the function AllocConsoleMessage
//===========================================================================
// deallocate one console message from the heap
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void FreeConsoleMessage( bot_consolemessage_t *message ) {
	if ( freeconsolemessages )
		freeconsolemessages->prev = message;
	message->prev = NULL;
	message->next = freeconsolemessages;
	freeconsolemessages = message;
} //end of the function FreeConsoleMessage
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotRemoveConsoleMessage( int chatstate, int handle ) {
	bot_consolemessage_t *m, *nextm;
	bot_chatstate_t *cs;

	cs = BotChatStateFromHandle( chatstate );
	if ( !cs )
		return;

	for ( m = cs->firstmessage; m; m = nextm ) {
		nextm = m->next;
		if ( m->handle == handle ) {
			if ( m->next )
				m->next->prev = m->prev;
			else
				cs->lastmessage = m->prev;
			if ( m->prev )
				m->prev->next = m->next;
			else
				cs->firstmessage = m->next;

			FreeConsoleMessage( m );
			cs->numconsolemessages--;
			break;
		} //end if
	} //end for
} //end of the function BotRemoveConsoleMessage
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotQueueConsoleMessage( int chatstate, int type, const char *message ) {
	bot_consolemessage_t *m;
	bot_chatstate_t *cs;

	cs = BotChatStateFromHandle( chatstate );
	if ( !cs )
		return;

	m = AllocConsoleMessage();
	if ( !m ) {
		botimport.Print( PRT_ERROR, "empty console message heap\n" );
		return;
	} //end if
	cs->handle++;
	if ( cs->handle <= 0 || cs->handle > 8192 )
		cs->handle = 1;
	m->handle = cs->handle;
	m->time = AAS_Time();
	m->type = type;
	Q_strncpyz( m->message, message, sizeof( m->message ) );
	m->next = NULL;
	if ( cs->lastmessage ) {
		cs->lastmessage->next = m;
		m->prev = cs->lastmessage;
		cs->lastmessage = m;
	} //end if
	else {
		cs->lastmessage = m;
		cs->firstmessage = m;
		m->prev = NULL;
	} //end if
	cs->numconsolemessages++;
} //end of the function BotQueueConsoleMessage
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotNextConsoleMessage( int chatstate, bot_consolemessage_qvm_t *cm ) {
	bot_chatstate_t *cs;
	bot_consolemessage_t *firstmsg;

	cs = BotChatStateFromHandle( chatstate );
	if ( !cs )
		return 0;
	if ( ( firstmsg = cs->firstmessage ) != NULL ) {
		if ( cm != NULL ) {
			cm->handle = firstmsg->handle;
			cm->time = firstmsg->time;
			cm->type = firstmsg->type;
			Q_strncpyz( cm->message, firstmsg->message, sizeof( cm->message ) );
			cm->next = 0;
			cm->prev = 0;
		}
		return firstmsg->handle;
	}
	return 0;
}


//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotNumConsoleMessages( int chatstate ) {
	bot_chatstate_t *cs;

	cs = BotChatStateFromHandle( chatstate );
	if ( !cs )
		return 0;
	return cs->numconsolemessages;
} //end of the function BotNumConsoleMessages
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static int IsWhiteSpace( char c ) {
	if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || c == '(' || c == ')' || c == '?' || c == ':' || c == '\'' || c == '/' || c == ',' || c == '.' || c == '[' || c == ']' || c == '-' || c == '_' || c == '+' || c == '=' )
		return qfalse;
	return qtrue;
} //end of the function IsWhiteSpace
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
static void BotRemoveTildes( char *message ) {
	int i;

	//remove all tildes from the chat message
	for ( i = 0; message[i]; i++ ) {
		if ( message[i] == '~' ) {
			memmove( &message[i], &message[i + 1], strlen( &message[i + 1] ) + 1 );
		} //end if
	} //end for
} //end of the function BotRemoveTildes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void UnifyWhiteSpaces( char *string ) {
	char *ptr, *oldptr;

	for ( ptr = oldptr = string; *ptr; oldptr = ptr ) {
		while ( *ptr && IsWhiteSpace( *ptr ) )
			ptr++;
		if ( ptr > oldptr ) {
			//if not at the start and not at the end of the string
			//write only one space
			if ( oldptr > string && *ptr )
				*oldptr++ = ' ';
			//remove all other white spaces
			if ( ptr > oldptr )
				memmove( oldptr, ptr, strlen( ptr ) + 1 );
		} //end if
		while ( *ptr && !IsWhiteSpace( *ptr ) )
			ptr++;
	} //end while
} //end of the function UnifyWhiteSpaces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int StringContains( const char *str1, const char *str2, int casesensitive ) {
	int len, i, j, index;

	if ( str1 == NULL || str2 == NULL )
		return -1;

	len = (int)( strlen( str1 ) - strlen( str2 ) );
	index = 0;
	for ( i = 0; i <= len; i++, str1++, index++ ) {
		for ( j = 0; str2[j]; j++ ) {
			if ( casesensitive ) {
				if ( str1[j] != str2[j] )
					break;
			} //end if
			else {
				if ( locase[(byte)str1[j]] != locase[(byte)str2[j]] )
					break;
			} //end else
		} //end for
		if ( !str2[j] )
			return index;
	} //end for
	return -1;
} //end of the function StringContains
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static const char *StringContainsWord( const char *str1, const char *str2 ) {
	int len, i, j;

	len = (int)( strlen( str1 ) - strlen( str2 ) );
	for ( i = 0; i <= len; i++, str1++ ) {
		//if not at the start of the string
		if ( i ) {
			//skip to the start of the next word
			while ( *str1 != '\0' && *str1 != ' ' && *str1 != '.' && *str1 != ',' && *str1 != '!' )
				str1++;
			if ( *str1 == '\0' )
				break;
			str1++;
		}

		//compare the word
		for ( j = 0; str2[j] != '\0'; j++ ) {
			if ( locase[(byte)str1[j]] != locase[(byte)str2[j]] )
				break;
		}

		//if there was a word match
		if ( str2[j] == '\0' ) {
			//if the first string has an end of word
			if ( str1[j] == '\0' || str1[j] == ' ' || str1[j] == '.' || str1[j] == ',' || str1[j] == '!' )
				return str1;
		}
	}

	return NULL;
} //end of the function StringContainsWord
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void StringReplaceWords( char *string, int size, const char *synonym, const char *replacement ) {
	char *str;
	const char *str2, *endp;
	int replen, synlen;

	synlen = (int)strlen( synonym );
	replen = (int)strlen( replacement );
	endp = string + size;

	//find the synonym in the string
	str = (char *)StringContainsWord( string, synonym );

	//if the synonym occurred in the string
	while ( str && str + replen < endp ) {
		//if the synonym isn't part of the replacement which is already in the string
		//useful for abbreviations
		str2 = StringContainsWord( string, replacement );
		while ( str2 ) {
			if ( str2 <= str && str < str2 + replen )
				break;
			str2 = StringContainsWord( str2 + 1, replacement );
		}

		if ( !str2 ) {
			memmove( str + replen, str + synlen, strlen( str + synlen ) + 1 );
			//append the synonym replacement
			Com_Memcpy( str, replacement, replen );
		}

		//find the next synonym in the string
		str = (char *)StringContainsWord( str + replen, synonym );
	} //end if
} //end of the function StringReplaceWords
#if 0
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotDumpSynonymList(bot_synonymlist_t *synlist)
{
	FILE *fp;
	bot_synonymlist_t *syn;
	bot_synonym_t *synonym;

	fp = Log_FilePointer();
	if (!fp) return;
	for (syn = synlist; syn; syn = syn->next)
	{
	        FS_OSPrintf(fp, "%" PRId64 " : [", (int64_t)(scriptSigned_t)syn->context);
		for (synonym = syn->firstsynonym; synonym; synonym = synonym->next)
		{
			FS_OSPrintf(fp, "(\"%s\", %1.2f)", synonym->string, synonym->weight);
			if (synonym->next) FS_OSPrintf(fp, ", ");
		} //end for
		FS_OSPrintf(fp, "]\n");
	} //end for
} //end of the function BotDumpSynonymList
#endif
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static bot_synonymlist_t *BotLoadSynonyms( const char *filename ) {
	int pass, contextlevel, numsynonyms;
	uint64_t context, contextstack[32];
	char *ptr = NULL;
	size_t size;
	source_t *source;
	token_t token;
	bot_synonymlist_t *synlist, *lastsyn, *syn;
	bot_synonym_t *synonym, *lastsynonym;

	size = 0;
	synlist = NULL; //make compiler happy
	syn = NULL; //make compiler happy
	synonym = NULL; //make compiler happy
	//the synonyms are parsed in two phases
	for ( pass = 0; pass < 2; pass++ ) {
		//
		if ( pass && size )
			ptr = (char *)GetClearedHunkMemory(size);
		//
		PC_SetBaseFolder( BOTFILESBASEFOLDER );
		source = LoadSourceFile( filename );
		if ( !source ) {
			botimport.Print( PRT_ERROR, "couldn't load %s\n", filename );
			return NULL;
		} //end if
		//
		context = 0;
		contextlevel = 0;
		synlist = NULL; //list synonyms
		lastsyn = NULL; //last synonym in the list
		//
		while ( PC_ReadToken( source, &token ) ) {
			if ( token.type == TT_NUMBER ) {
				context |= token.intvalue;
				contextstack[contextlevel] = token.intvalue;
				contextlevel++;
				if ( contextlevel >= 32 ) {
					SourceError( source, "more than 32 context levels" );
					FreeSource( source );
					return NULL;
				} //end if
				if ( !PC_ExpectTokenString( source, "{" ) ) {
					FreeSource( source );
					return NULL;
				} //end if
			} //end if
			else if ( token.type == TT_PUNCTUATION ) {
				if ( !strcmp( token.string, "}" ) ) {
					contextlevel--;
					if ( contextlevel < 0 ) {
						SourceError( source, "too many }" );
						FreeSource( source );
						return NULL;
					} //end if
					context &= ~contextstack[contextlevel];
				} //end if
				else if ( !strcmp( token.string, "[" ) ) {
					size += sizeof( bot_synonymlist_t );
					if ( pass && ptr ) {
						syn = (bot_synonymlist_t *)ptr;
						ptr += sizeof( bot_synonymlist_t );
						syn->context = context;
						syn->firstsynonym = NULL;
						syn->next = NULL;
						if ( lastsyn )
							lastsyn->next = syn;
						else
							synlist = syn;
						lastsyn = syn;
					} //end if
					numsynonyms = 0;
					lastsynonym = NULL;
					while ( 1 ) {
						size_t len;
						if ( !PC_ExpectTokenString( source, "(" ) ||
							 !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ) {
							FreeSource( source );
							return NULL;
						} //end if
						StripDoubleQuotes( token.string );
						len = strlen( token.string );
						if ( len == 0 ) {
							SourceError( source, "empty string" );
							FreeSource( source );
							return NULL;
						} //end if
						len = PAD( len + 1, sizeof( uintptr_t ) );
						size += sizeof( bot_synonym_t ) + len;
						if ( pass && ptr ) {
							synonym = (bot_synonym_t *)ptr;
							ptr += sizeof( bot_synonym_t );
							synonym->string = ptr;
							ptr += len;
							strcpy( synonym->string, token.string );
							//
							if ( lastsynonym )
								lastsynonym->next = synonym;
							else
								syn->firstsynonym = synonym;
							lastsynonym = synonym;
						} //end if
						numsynonyms++;
						if ( !PC_ExpectTokenString( source, "," ) ||
							 !PC_ExpectTokenType( source, TT_NUMBER, 0, &token ) ||
							 !PC_ExpectTokenString( source, ")" ) ) {
							FreeSource( source );
							return NULL;
						} //end if
						if ( pass && ptr ) {
							synonym->weight = token.floatvalue;
							syn->totalweight += synonym->weight;
						} //end if
						if ( PC_CheckTokenString( source, "]" ) )
							break;
						if ( !PC_ExpectTokenString( source, "," ) ) {
							FreeSource( source );
							return NULL;
						} //end if
					} //end while
					if ( numsynonyms < 2 ) {
						SourceError( source, "synonym must have at least two entries" );
						FreeSource( source );
						return NULL;
					} //end if
				} //end else
				else {
					SourceError( source, "unexpected %s", token.string );
					FreeSource( source );
					return NULL;
				} //end if
			} //end else if
		} //end while
		//
		FreeSource( source );
		//
		if ( contextlevel > 0 ) {
			SourceError( source, "missing }" );
			return NULL;
		} //end if
	} //end for
	botimport.Print( PRT_MESSAGE, "loaded %s\n", filename );
	//
	//BotDumpSynonymList(synlist);
	//
	return synlist;
} //end of the function BotLoadSynonyms
//===========================================================================
// replace all the synonyms in the string
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotReplaceSynonyms( char *string, int size, uint64_t context ) {
	const bot_synonymlist_t *syn;
	const bot_synonym_t *synonym;

	for ( syn = synonyms; syn; syn = syn->next ) {
		if ( ( syn->context & context ) == 0 )
			continue;

		for ( synonym = syn->firstsynonym->next; synonym; synonym = synonym->next ) {
			StringReplaceWords( string, size, synonym->string, syn->firstsynonym->string );
		} //end for
	} //end for
} //end of the function BotReplaceSynonyms
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotReplaceWeightedSynonyms( char *string, int size, uint64_t context ) {
	bot_synonymlist_t *syn;
	bot_synonym_t *synonym, *replacement;
	float weight, curweight;

	for ( syn = synonyms; syn; syn = syn->next ) {
		if ( ( syn->context & context ) == 0 )
			continue;

		//choose a weighted random replacement synonym
		weight = random() * syn->totalweight;
		if ( !weight )
			continue;
		curweight = 0;

		for ( replacement = syn->firstsynonym; replacement; replacement = replacement->next ) {
			curweight += replacement->weight;
			if ( weight < curweight )
				break;
		}
		if ( !replacement )
			continue;

		//replace all synonyms with the replacement
		for ( synonym = syn->firstsynonym; synonym; synonym = synonym->next ) {
			if ( synonym == replacement )
				continue;
			StringReplaceWords( string, size, synonym->string, replacement->string );
		} //end for
	} //end for
} //end of the function BotReplaceWeightedSynonyms
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotReplaceReplySynonyms( char *string, int size, uint64_t context ) {
	char *str1, *replacement;
	const char *str2, *endp;
	bot_synonymlist_t *syn;
	bot_synonym_t *synonym;
	int replen;

	endp = string + size;

	for ( str1 = string; *str1 != '\0'; ) {
		//go to the start of the next word
		while ( *str1 && *str1 <= ' ' )
			str1++;
		if ( *str1 == '\0' )
			break;

		for ( syn = synonyms; syn; syn = syn->next ) {
			if ( ( syn->context & context ) == 0 )
				continue;

			for ( synonym = syn->firstsynonym->next; synonym; synonym = synonym->next ) {
				//if the synonym is not at the front of the string continue
				str2 = StringContainsWord( str1, synonym->string );
				if ( !str2 || str2 != str1 )
					continue;

				replacement = syn->firstsynonym->string;
				replen = (int)( strlen( replacement ) );

				//if the replacement IS in front of the string continue
				str2 = StringContainsWord( str1, replacement );
				if ( str2 && str2 == str1 )
					continue;
				if ( str1 + replen >= endp )
					continue;

				memmove( str1 + replen, str1 + strlen( synonym->string ), strlen( str1 + strlen( synonym->string ) ) + 1 );
				//append the synonym replacement
				Com_Memcpy( str1, replacement, replen );
				break;
			}

			//if a synonym has been replaced
			if ( synonym )
				break;
		}

		//skip over this word
		while ( /**str1 &&*/ *str1 > ' ' )
			str1++;
		if ( *str1 == '\0' )
			break;
	} //end while
} //end of the function BotReplaceReplySynonyms
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
static int BotLoadChatMessage( source_t *source, char *chatmessagestring, int size ) {
	char *ptr;
	token_t token;
	int len;

	ptr = chatmessagestring;
	*ptr = '\0';

	while ( 1 ) {
		if ( !PC_ExpectAnyToken( source, &token ) )
			return qfalse;

		//fixed string
		if ( token.type == TT_STRING ) {
			StripDoubleQuotes( token.string );
			len = (int)( strlen( ptr ) );
			if ( len + strlen( token.string ) + 1 > (size_t)size ) {
				SourceError( source, "chat message too long" );
				return qfalse;
			}
			strcpy( &ptr[len], token.string );
		}
		//variable string
		else if ( token.type == TT_NUMBER && ( token.subtype & TT_INTEGER ) ) {
			char intbuf[32];
			int intlen;

			len = (int)( strlen( ptr ) );
			intlen = snprintf( intbuf, sizeof( intbuf ), "%cv%" PRId64 "%c", ESCAPE_CHAR, (int64_t)(scriptSigned_t)token.intvalue, ESCAPE_CHAR );
			if ( len + intlen + 1 > size ) {
				SourceError( source, "chat message too long" );
				return qfalse;
			}
			strcpy( &ptr[len], intbuf );
			//sprintf( &ptr[len], "%cv%" PRId64 "%c", ESCAPE_CHAR, (int64_t)(scriptSigned_t)token.intvalue, ESCAPE_CHAR );
		}
		//random string
		else if ( token.type == TT_NAME ) {
			len = (int)( strlen( ptr ) );
			if ( len + strlen( token.string ) + 4 > (size_t)size ) {
				SourceError( source, "chat message too long" );
				return qfalse;
			}
			snprintf( &ptr[len], size - len, "%cr%s%c", ESCAPE_CHAR, token.string, ESCAPE_CHAR );
		} else {
			SourceError( source, "unknown message component %s", token.string );
			return qfalse;
		}

		if ( PC_CheckTokenString( source, ";" ) )
			break;

		if ( !PC_ExpectTokenString( source, "," ) )
			return qfalse;
	}

	return qtrue;
} //end of the function BotLoadChatMessage
#if 0
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotDumpRandomStringList(bot_randomlist_t *randomlist)
{
	FILE *fp;
	bot_randomlist_t *random;
	bot_randomstring_t *rs;

	fp = Log_FilePointer();
	if (!fp) return;
	for (random = randomlist; random; random = random->next)
	{
		FS_OSPrintf(fp, "%s = {", random->string);
		for (rs = random->firstrandomstring; rs; rs = rs->next)
		{
			FS_OSPrintf(fp, "\"%s\"", rs->string);
			if (rs->next) FS_OSPrintf(fp, ", ");
			else FS_OSPrintf(fp, "}\n");
		} //end for
	} //end for
} //end of the function BotDumpRandomStringList
#endif
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static bot_randomlist_t *BotLoadRandomStrings( const char *filename ) {
	int pass;
	char *ptr = NULL, chatmessagestring[MAX_MESSAGE_SIZE];
	source_t *source;
	token_t token;
	bot_randomlist_t *randomlist, *lastrandom, *random;
	bot_randomstring_t *randomstring;
	size_t size, len;

#ifdef DEBUG
	int starttime = Sys_MilliSeconds();
#endif //DEBUG

	size = 0;
	randomlist = NULL;
	random = NULL;
	//the synonyms are parsed in two phases
	for ( pass = 0; pass < 2; pass++ ) {
		//
		if ( pass && size )
			ptr = (char *)GetClearedHunkMemory(size);
		//
		PC_SetBaseFolder( BOTFILESBASEFOLDER );
		source = LoadSourceFile( filename );
		if ( !source ) {
			botimport.Print( PRT_ERROR, "couldn't load %s\n", filename );
			return NULL;
		} //end if
		//
		randomlist = NULL; //list
		lastrandom = NULL; //last
		//
		while ( PC_ReadToken( source, &token ) ) {
			if ( token.type != TT_NAME ) {
				SourceError( source, "unknown random %s", token.string );
				FreeSource( source );
				return NULL;
			} //end if
			len = strlen( token.string ) + 1;
			len = PAD( len, sizeof( uintptr_t ) );
			size += sizeof( bot_randomlist_t ) + len;
			if ( pass && ptr ) {
				random = (bot_randomlist_t *)ptr;
				ptr += sizeof( bot_randomlist_t );
				random->string = ptr;
				ptr += len;
				strcpy( random->string, token.string );
				random->firstrandomstring = NULL;
				random->numstrings = 0;
				//
				if ( lastrandom )
					lastrandom->next = random;
				else
					randomlist = random;
				lastrandom = random;
			} //end if
			if ( !PC_ExpectTokenString( source, "=" ) ||
				 !PC_ExpectTokenString( source, "{" ) ) {
				FreeSource( source );
				return NULL;
			} //end if
			while ( !PC_CheckTokenString( source, "}" ) ) {
				if ( !BotLoadChatMessage( source, chatmessagestring, sizeof( chatmessagestring ) ) ) {
					FreeSource( source );
					return NULL;
				} //end if
				len = strlen( chatmessagestring ) + 1;
				len = PAD( len, sizeof( uintptr_t ) );
				size += sizeof( bot_randomstring_t ) + len;
				if ( pass && ptr ) {
					randomstring = (bot_randomstring_t *)ptr;
					ptr += sizeof( bot_randomstring_t );
					randomstring->string = ptr;
					ptr += len;
					strcpy( randomstring->string, chatmessagestring );
					//
					random->numstrings++;
					randomstring->next = random->firstrandomstring;
					random->firstrandomstring = randomstring;
				} //end if
			} //end while
		} //end while
		//free the source after one pass
		FreeSource( source );
	} //end for
	botimport.Print( PRT_MESSAGE, "loaded %s\n", filename );
	//
#ifdef DEBUG
	botimport.Print( PRT_MESSAGE, "random strings %d msec\n", Sys_MilliSeconds() - starttime );
	//BotDumpRandomStringList(randomlist);
#endif //DEBUG
	//
	return randomlist;
} //end of the function BotLoadRandomStrings
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static const char *RandomString( const char *name ) {
	const bot_randomlist_t *random;
	const bot_randomstring_t *rs;
	int i;

	for ( random = randomstrings; random; random = random->next ) {
		if ( strcmp( random->string, name ) == 0 ) {
			i = (int)( random() * random->numstrings );
			for ( rs = random->firstrandomstring; rs; rs = rs->next ) {
				if ( --i < 0 )
					break;
			}
			if ( rs ) {
				return rs->string;
			}
		}
	}
	return NULL;
} //end of the function RandomString
#if 0
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotDumpMatchTemplates(bot_matchtemplate_t *matches)
{
	FILE *fp;
	bot_matchtemplate_t *mt;
	bot_matchpiece_t *mp;
	bot_matchstring_t *ms;

	fp = Log_FilePointer();
	if (!fp) return;
	for (mt = matches; mt; mt = mt->next)
	{
	        FS_OSPrintf(fp, "{ " );
		for (mp = mt->first; mp; mp = mp->next)
		{
			if (mp->type == MT_STRING)
			{
				for (ms = mp->firststring; ms; ms = ms->next)
				{
					FS_OSPrintf(fp, "\"%s\"", ms->string);
					if (ms->next) FS_OSPrintf(fp, "|");
				} //end for
			} //end if
			else if (mp->type == MT_VARIABLE)
			{
				FS_OSPrintf(fp, "%d", mp->variable);
			} //end else if
			if (mp->next) FS_OSPrintf(fp, ", ");
		} //end for
		FS_OSPrintf(fp, " = (%d, %d);}\n", mt->type, mt->subtype);
	} //end for
} //end of the function BotDumpMatchTemplates
#endif
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotFreeMatchPieces( bot_matchpiece_t *matchpieces ) {
	bot_matchpiece_t *mp, *nextmp;
	bot_matchstring_t *ms, *nextms;

	for ( mp = matchpieces; mp; mp = nextmp ) {
		nextmp = mp->next;
		if ( mp->type == MT_STRING ) {
			for ( ms = mp->firststring; ms; ms = nextms ) {
				nextms = ms->next;
				FreeMemory( ms );
			} //end for
		} //end if
		FreeMemory( mp );
	} //end for
} //end of the function BotFreeMatchPieces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static bot_matchpiece_t *BotLoadMatchPieces( source_t *source, const char *endtoken ) {
	int lastwasvariable, emptystring;
	token_t token;
	bot_matchpiece_t *matchpiece, *firstpiece, *lastpiece;
	bot_matchstring_t *matchstring, *lastmatchstring;

	firstpiece = NULL;
	lastpiece = NULL;
	//
	lastwasvariable = qfalse;
	//
	while ( PC_ReadToken( source, &token ) ) {
		if ( token.type == TT_NUMBER && ( token.subtype & TT_INTEGER ) ) {
			if ( token.intvalue >= MAX_MATCHVARIABLES ) {
				SourceError( source, "can't have more than %d match variables", MAX_MATCHVARIABLES );
				FreeSource( source );
				BotFreeMatchPieces( firstpiece );
				return NULL;
			} //end if
			if ( lastwasvariable ) {
				SourceError( source, "not allowed to have adjacent variables" );
				FreeSource( source );
				BotFreeMatchPieces( firstpiece );
				return NULL;
			} //end if
			lastwasvariable = qtrue;
			//
			matchpiece = (bot_matchpiece_t *)GetClearedHunkMemory(sizeof(bot_matchpiece_t));
			matchpiece->type = MT_VARIABLE;
			matchpiece->variable = token.intvalue;
			matchpiece->next = NULL;
			if ( lastpiece )
				lastpiece->next = matchpiece;
			else
				firstpiece = matchpiece;
			lastpiece = matchpiece;
		} //end if
		else if ( token.type == TT_STRING ) {
			//
			matchpiece = (bot_matchpiece_t *)GetClearedHunkMemory(sizeof(bot_matchpiece_t));
			matchpiece->firststring = NULL;
			matchpiece->type = MT_STRING;
			matchpiece->variable = 0;
			matchpiece->next = NULL;
			if ( lastpiece )
				lastpiece->next = matchpiece;
			else
				firstpiece = matchpiece;
			lastpiece = matchpiece;
			//
			lastmatchstring = NULL;
			emptystring = qfalse;
			//
			do {
				if ( matchpiece->firststring ) {
					if ( !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ) {
						FreeSource( source );
						BotFreeMatchPieces( firstpiece );
						return NULL;
					} //end if
				} //end if
				StripDoubleQuotes( token.string );
				matchstring = (bot_matchstring_t *)GetClearedHunkMemory(sizeof(bot_matchstring_t) + strlen(token.string) + 1);
				matchstring->string = (char *)matchstring + sizeof( bot_matchstring_t );
				strcpy( matchstring->string, token.string );
				if ( !strlen( token.string ) )
					emptystring = qtrue;
				matchstring->next = NULL;
				if ( lastmatchstring )
					lastmatchstring->next = matchstring;
				else
					matchpiece->firststring = matchstring;
				lastmatchstring = matchstring;
			} while ( PC_CheckTokenString( source, "|" ) );
			//if there was no empty string found
			if ( !emptystring )
				lastwasvariable = qfalse;
		} //end if
		else {
			SourceError( source, "invalid token %s", token.string );
			FreeSource( source );
			BotFreeMatchPieces( firstpiece );
			return NULL;
		} //end else
		if ( PC_CheckTokenString( source, endtoken ) )
			break;
		if ( !PC_ExpectTokenString( source, "," ) ) {
			FreeSource( source );
			BotFreeMatchPieces( firstpiece );
			return NULL;
		} //end if
	} //end while
	return firstpiece;
} //end of the function BotLoadMatchPieces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotFreeMatchTemplates( bot_matchtemplate_t *mt ) {
	bot_matchtemplate_t *nextmt;

	for ( ; mt; mt = nextmt ) {
		nextmt = mt->next;
		BotFreeMatchPieces( mt->first );
		FreeMemory( mt );
	} //end for
} //end of the function BotFreeMatchTemplates
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static bot_matchtemplate_t *BotLoadMatchTemplates( const char *matchfile ) {
	source_t *source;
	token_t token;
	bot_matchtemplate_t *matchtemplate, *matches, *lastmatch;
	uint64_t context;

	PC_SetBaseFolder( BOTFILESBASEFOLDER );
	source = LoadSourceFile( matchfile );
	if ( !source ) {
		botimport.Print( PRT_ERROR, "couldn't load %s\n", matchfile );
		return NULL;
	} //end if
	//
	matches = NULL; //list with matches
	lastmatch = NULL; //last match in the list

	while ( PC_ReadToken( source, &token ) ) {
		if ( token.type != TT_NUMBER || !( token.subtype & TT_INTEGER ) ) {
			SourceError( source, "expected integer, found %s", token.string );
			BotFreeMatchTemplates( matches );
			FreeSource( source );
			return NULL;
		} //end if
		//the context
		context = token.intvalue;
		//
		if ( !PC_ExpectTokenString( source, "{" ) ) {
			BotFreeMatchTemplates( matches );
			FreeSource( source );
			return NULL;
		} //end if
		//
		while ( PC_ReadToken( source, &token ) ) {
			if ( !strcmp( token.string, "}" ) )
				break;
			//
			PC_UnreadLastToken( source );
			//
			matchtemplate = (bot_matchtemplate_t *)GetClearedHunkMemory(sizeof(bot_matchtemplate_t));
			matchtemplate->context = context;
			matchtemplate->next = NULL;
			//add the match template to the list
			if ( lastmatch )
				lastmatch->next = matchtemplate;
			else
				matches = matchtemplate;
			lastmatch = matchtemplate;
			//load the match template
			matchtemplate->first = BotLoadMatchPieces( source, "=" );
			if ( !matchtemplate->first ) {
				BotFreeMatchTemplates( matches );
				return NULL;
			} //end if
			//read the match type
			if ( !PC_ExpectTokenString( source, "(" ) ||
				 !PC_ExpectTokenType( source, TT_NUMBER, TT_INTEGER, &token ) ) {
				BotFreeMatchTemplates( matches );
				FreeSource( source );
				return NULL;
			} //end if
			matchtemplate->type = token.intvalue;
			//read the match subtype
			if ( !PC_ExpectTokenString( source, "," ) ||
				 !PC_ExpectTokenType( source, TT_NUMBER, TT_INTEGER, &token ) ) {
				BotFreeMatchTemplates( matches );
				FreeSource( source );
				return NULL;
			} //end if
			matchtemplate->subtype = token.intvalue;
			//read trailing punctuations
			if ( !PC_ExpectTokenString( source, ")" ) ||
				 !PC_ExpectTokenString( source, ";" ) ) {
				BotFreeMatchTemplates( matches );
				FreeSource( source );
				return NULL;
			} //end if
		} //end while
	} //end while
	//free the source
	FreeSource( source );
	botimport.Print( PRT_MESSAGE, "loaded %s\n", matchfile );
	//
	//BotDumpMatchTemplates(matches);
	//
	return matches;
} //end of the function BotLoadMatchTemplates
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static int StringsMatch( bot_matchpiece_t *pieces, bot_match_t *match ) {
	int lastvariable, index;
	const char *strptr, *newstrptr;
	bot_matchpiece_t *mp;
	bot_matchstring_t *ms;

	//no last variable
	lastvariable = -1;
	//pointer to the string to compare the match string with
	strptr = match->string;
	//Log_Write("match: %s", strptr);
	//compare the string with the current match string
	for ( mp = pieces; mp; mp = mp->next ) {
		//if it is a piece of string
		if ( mp->type == MT_STRING ) {
			newstrptr = NULL;
			for ( ms = mp->firststring; ms; ms = ms->next ) {
				if ( !strlen( ms->string ) ) {
					newstrptr = strptr;
					break;
				} //end if
				//Log_Write("MT_STRING: %s", mp->string);
				index = StringContains( strptr, ms->string, qfalse );
				if ( index >= 0 ) {
					newstrptr = strptr + index;
					if ( lastvariable >= 0 ) {
						match->variables[lastvariable].length =
							(int)( ( newstrptr - match->string ) - match->variables[lastvariable].offset );
						//newstrptr - match->variables[lastvariable].ptr;
						lastvariable = -1;
						break;
					} //end if
					else if ( index == 0 ) {
						break;
					} //end else
					newstrptr = NULL;
				} //end if
			} //end for
			if ( !newstrptr )
				return qfalse;
			strptr = newstrptr + strlen( ms->string );
		} //end if
		//if it is a variable piece of string
		else if ( mp->type == MT_VARIABLE ) {
			//Log_Write("MT_VARIABLE");
			match->variables[mp->variable].offset = (signed char)( strptr - match->string );
			lastvariable = mp->variable;
		} //end else if
	} //end for
	//if a match was found
	if ( !mp && ( lastvariable >= 0 || !strlen( strptr ) ) ) {
		//if the last piece was a variable string
		if ( lastvariable >= 0 ) {
			Q_ASSERT( match->variables[lastvariable].offset >= 0 );
			match->variables[lastvariable].length =
				(int)strlen( &match->string[(int)match->variables[lastvariable].offset] );
		} //end if
		return qtrue;
	} //end if
	return qfalse;
} //end of the function StringsMatch
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotFindMatch( const char *str, bot_match_t *match, uint64_t context ) {
	int i;
	bot_matchtemplate_t *ms;

	Q_strncpyz( match->string, str, sizeof( match->string ) );
	//remove any trailing enters
	while ( strlen( match->string ) &&
			match->string[strlen( match->string ) - 1] == '\n' ) {
		match->string[strlen( match->string ) - 1] = '\0';
	} //end while
	//compare the string with all the match strings
	for ( ms = matchtemplates; ms; ms = ms->next ) {
		if ( !( ms->context & context ) )
			continue;
		//reset the match variable offsets
		for ( i = 0; i < MAX_MATCHVARIABLES; i++ )
			match->variables[i].offset = -1;
		//
		if ( StringsMatch( ms->first, match ) ) {
			match->type = ms->type;
			match->subtype = ms->subtype;
			return qtrue;
		} //end if
	} //end for
	return qfalse;
} //end of the function BotFindMatch
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotMatchVariable( bot_match_t *match, int variable, char *buf, int size ) {
	if ( variable < 0 || variable >= MAX_MATCHVARIABLES ) {
		botimport.Print( PRT_FATAL, "BotMatchVariable: variable out of range\n" );
		buf[0] = '\0';
		return;
	}

	if ( match->variables[variable].offset >= 0 ) {
		if ( match->variables[variable].length < size )
			size = match->variables[variable].length + 1;
		Q_ASSERT( match->variables[variable].offset >= 0 );
		Q_strncpyz( buf, &match->string[(int)match->variables[variable].offset], size );
	} else {
		buf[0] = '\0';
	} //end else
} //end of the function BotMatchVariable
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static bot_stringlist_t *BotFindStringInList( bot_stringlist_t *list, const char *string ) {
	bot_stringlist_t *s;

	for ( s = list; s; s = s->next ) {
		if ( !strcmp( s->string, string ) )
			return s;
	}

	return NULL;
} //end of the function BotFindStringInList
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static bot_stringlist_t *BotCheckChatMessageIntegrety( char *message, bot_stringlist_t *stringlist ) {
	int i;
	char *msgptr;
	char temp[MAX_MESSAGE_SIZE];
	bot_stringlist_t *s;

	msgptr = message;
	//
	while ( *msgptr ) {
		if ( *msgptr == ESCAPE_CHAR ) {
			msgptr++;
			switch ( *msgptr ) {
			case 'v': //variable
			{
				//step over the 'v'
				msgptr++;
				while ( *msgptr && *msgptr != ESCAPE_CHAR )
					msgptr++;
				//step over the trailing escape char
				if ( *msgptr )
					msgptr++;
				break;
			} //end case
			case 'r': //random
			{
				//step over the 'r'
				msgptr++;
				for ( i = 0; ( *msgptr && *msgptr != ESCAPE_CHAR ); i++ ) {
					temp[i] = *msgptr++;
				} //end while
				temp[i] = '\0';
				//step over the trailing escape char
				if ( *msgptr )
					msgptr++;
				//find the random keyword
				if ( !RandomString( temp ) ) {
					if ( !BotFindStringInList( stringlist, temp ) ) {
						Log_Write( "%s = {\"%s\"} //MISSING RANDOM\r\n", temp, temp );
						s = (bot_stringlist_t *)GetClearedMemory(sizeof(bot_stringlist_t) + strlen(temp) + 1);
						s->string = (char *)s + sizeof( bot_stringlist_t );
						strcpy( s->string, temp );
						s->next = stringlist;
						stringlist = s;
					} //end if
				} //end if
				break;
			} //end case
			default: {
				botimport.Print( PRT_FATAL, "BotCheckChatMessageIntegrety: message \"%s\" invalid escape char\n", message );
				break;
			} //end default
			} //end switch
		} //end if
		else {
			msgptr++;
		} //end else
	} //end while
	return stringlist;
} //end of the function BotCheckChatMessageIntegrety
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotCheckInitialChatIntegrety( bot_chat_t *chat ) {
	bot_chattype_t *t;
	bot_chatmessage_t *cm;
	bot_stringlist_t *stringlist, *s, *nexts;

	stringlist = NULL;
	for ( t = chat->types; t; t = t->next ) {
		for ( cm = t->firstchatmessage; cm; cm = cm->next ) {
			stringlist = BotCheckChatMessageIntegrety( cm->chatmessage, stringlist );
		} //end for
	} //end for
	for ( s = stringlist; s; s = nexts ) {
		nexts = s->next;
		FreeMemory( s );
	} //end for
} //end of the function BotCheckInitialChatIntegrety
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotCheckReplyChatIntegrety( bot_replychat_t *replychat ) {
	bot_replychat_t *rp;
	bot_chatmessage_t *cm;
	bot_stringlist_t *stringlist, *s, *nexts;

	stringlist = NULL;
	for ( rp = replychat; rp; rp = rp->next ) {
		for ( cm = rp->firstchatmessage; cm; cm = cm->next ) {
			stringlist = BotCheckChatMessageIntegrety( cm->chatmessage, stringlist );
		} //end for
	} //end for
	for ( s = stringlist; s; s = nexts ) {
		nexts = s->next;
		FreeMemory( s );
	} //end for
} //end of the function BotCheckReplyChatIntegrety
#if 0
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotDumpReplyChat(bot_replychat_t *replychat)
{
	FILE *fp;
	bot_replychat_t *rp;
	bot_replychatkey_t *key;
	bot_chatmessage_t *cm;
	bot_matchpiece_t *mp;

	fp = Log_FilePointer();
	if (!fp) return;
	FS_OSPrintf(fp, "BotDumpReplyChat:\n");
	for (rp = replychat; rp; rp = rp->next)
	{
		FS_OSPrintf(fp, "[");
		for (key = rp->keys; key; key = key->next)
		{
			if (key->flags & RCKFL_AND) FS_OSPrintf(fp, "&");
			else if (key->flags & RCKFL_NOT) FS_OSPrintf(fp, "!");
			//
			if (key->flags & RCKFL_NAME) FS_OSPrintf(fp, "name");
			else if (key->flags & RCKFL_GENDERFEMALE) FS_OSPrintf(fp, "female");
			else if (key->flags & RCKFL_GENDERMALE) FS_OSPrintf(fp, "male");
			else if (key->flags & RCKFL_GENDERLESS) FS_OSPrintf(fp, "it");
			else if (key->flags & RCKFL_VARIABLES)
			{
				FS_OSPrintf(fp, "(");
				for (mp = key->match; mp; mp = mp->next)
				{
					if (mp->type == MT_STRING) FS_OSPrintf(fp, "\"%s\"", mp->firststring->string);
					else FS_OSPrintf(fp, "%d", mp->variable);
					if (mp->next) FS_OSPrintf(fp, ", ");
				} //end for
				FS_OSPrintf(fp, ")");
			} //end if
			else if (key->flags & RCKFL_STRING)
			{
				FS_OSPrintf(fp, "\"%s\"", key->string);
			} //end if
			if (key->next) FS_OSPrintf(fp, ", ");
			else FS_OSPrintf(fp, "] = %1.0f\n", rp->priority);
		} //end for
		FS_OSPrintf(fp, "{\n");
		for (cm = rp->firstchatmessage; cm; cm = cm->next)
		{
			FS_OSPrintf(fp, "\t\"%s\";\n", cm->chatmessage);
		} //end for
		FS_OSPrintf(fp, "}\n");
	} //end for
} //end of the function BotDumpReplyChat
#endif
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotFreeReplyChat( bot_replychat_t *replychat ) {
	bot_replychat_t *rp, *nextrp;
	bot_replychatkey_t *key, *nextkey;
	bot_chatmessage_t *cm, *nextcm;

	for ( rp = replychat; rp; rp = nextrp ) {
		nextrp = rp->next;
		for ( key = rp->keys; key; key = nextkey ) {
			nextkey = key->next;
			if ( key->match )
				BotFreeMatchPieces( key->match );
			if ( key->string )
				FreeMemory( key->string );
			FreeMemory( key );
		} //end for
		for ( cm = rp->firstchatmessage; cm; cm = nextcm ) {
			nextcm = cm->next;
			FreeMemory( cm );
		} //end for
		FreeMemory( rp );
	} //end for
} //end of the function BotFreeReplyChat
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
static void BotCheckValidReplyChatKeySet( source_t *source, bot_replychatkey_t *keys ) {
	int allprefixed, hasvariableskey, hasstringkey;
	bot_matchpiece_t *m;
	bot_matchstring_t *ms;
	bot_replychatkey_t *key, *key2;

	//
	allprefixed = qtrue;
	hasvariableskey = hasstringkey = qfalse;
	for ( key = keys; key; key = key->next ) {
		if ( !( key->flags & ( RCKFL_AND | RCKFL_NOT ) ) ) {
			allprefixed = qfalse;
			if ( key->flags & RCKFL_VARIABLES ) {
				for ( m = key->match; m; m = m->next ) {
					if ( m->type == MT_VARIABLE )
						hasvariableskey = qtrue;
				} //end for
			} //end if
			else if ( key->flags & RCKFL_STRING ) {
				hasstringkey = qtrue;
			} //end else if
		} //end if
		else if ( ( key->flags & RCKFL_AND ) && ( key->flags & RCKFL_STRING ) ) {
			for ( key2 = keys; key2; key2 = key2->next ) {
				if ( key2 == key )
					continue;
				if ( key2->flags & RCKFL_NOT )
					continue;
				if ( key2->flags & RCKFL_VARIABLES ) {
					for ( m = key2->match; m; m = m->next ) {
						if ( m->type == MT_STRING ) {
							for ( ms = m->firststring; ms; ms = ms->next ) {
								if ( StringContains( ms->string, key->string, qfalse ) != -1 ) {
									break;
								} //end if
							} //end for
							if ( ms )
								break;
						} //end if
						else if ( m->type == MT_VARIABLE ) {
							break;
						} //end if
					} //end for
					if ( !m ) {
						SourceWarning( source, "one of the match templates does not "
											   "leave space for the key %s with the & prefix",
							key->string );
					} //end if
				} //end if
			} //end for
		} //end else
		if ( ( key->flags & RCKFL_NOT ) && ( key->flags & RCKFL_STRING ) ) {
			for ( key2 = keys; key2; key2 = key2->next ) {
				if ( key2 == key )
					continue;
				if ( key2->flags & RCKFL_NOT )
					continue;
				if ( key2->flags & RCKFL_STRING ) {
					if ( StringContains( key2->string, key->string, qfalse ) != -1 ) {
						SourceWarning( source, "the key %s with prefix ! is inside the key %s", key->string, key2->string );
					} //end if
				} //end if
				else if ( key2->flags & RCKFL_VARIABLES ) {
					for ( m = key2->match; m; m = m->next ) {
						if ( m->type == MT_STRING ) {
							for ( ms = m->firststring; ms; ms = ms->next ) {
								if ( StringContains( ms->string, key->string, qfalse ) != -1 ) {
									SourceWarning( source, "the key %s with prefix ! is inside "
														   "the match template string %s",
										key->string, ms->string );
								} //end if
							} //end for
						} //end if
					} //end for
				} //end else if
			} //end for
		} //end if
	} //end for
	if ( allprefixed )
		SourceWarning( source, "all keys have a & or ! prefix" );
	if ( hasvariableskey && hasstringkey ) {
		SourceWarning( source, "variables from the match template(s) could be "
							   "invalid when outputting one of the chat messages" );
	} //end if
} //end of the function BotCheckValidReplyChatKeySet
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
static bot_replychat_t *BotLoadReplyChat( const char *filename ) {
	char chatmessagestring[MAX_MESSAGE_SIZE];
	char namebuffer[MAX_MESSAGE_SIZE];
	source_t *source;
	token_t token;
	bot_chatmessage_t *chatmessage = NULL;
	bot_replychat_t *replychat, *replychatlist;
	bot_replychatkey_t *key;

	PC_SetBaseFolder( BOTFILESBASEFOLDER );
	source = LoadSourceFile( filename );
	if ( !source ) {
		botimport.Print( PRT_ERROR, "couldn't load %s\n", filename );
		return NULL;
	} //end if
	//
	replychatlist = NULL;
	//
	while ( PC_ReadToken( source, &token ) ) {
		if ( strcmp( token.string, "[" ) ) {
			SourceError( source, "expected [, found %s", token.string );
			BotFreeReplyChat( replychatlist );
			FreeSource( source );
			return NULL;
		} //end if
		//
		replychat = (bot_replychat_t *)GetClearedHunkMemory(sizeof(bot_replychat_t));
		replychat->keys = NULL;
		replychat->next = replychatlist;
		replychatlist = replychat;
		//read the keys, there must be at least one key
		do {
			//allocate a key
			key = (bot_replychatkey_t *)GetClearedHunkMemory(sizeof(bot_replychatkey_t));
			key->flags = 0;
			key->string = NULL;
			key->match = NULL;
			key->next = replychat->keys;
			replychat->keys = key;
			//check for MUST BE PRESENT and MUST BE ABSENT keys
			if ( PC_CheckTokenString( source, "&" ) )
				key->flags |= RCKFL_AND;
			else if ( PC_CheckTokenString( source, "!" ) )
				key->flags |= RCKFL_NOT;
			//special keys
			if ( PC_CheckTokenString( source, "name" ) )
				key->flags |= RCKFL_NAME;
			else if ( PC_CheckTokenString( source, "female" ) )
				key->flags |= RCKFL_GENDERFEMALE;
			else if ( PC_CheckTokenString( source, "male" ) )
				key->flags |= RCKFL_GENDERMALE;
			else if ( PC_CheckTokenString( source, "it" ) )
				key->flags |= RCKFL_GENDERLESS;
			else if ( PC_CheckTokenString( source, "(" ) ) //match key
			{
				key->flags |= RCKFL_VARIABLES;
				key->match = BotLoadMatchPieces( source, ")" );
				if ( !key->match ) {
					BotFreeReplyChat( replychatlist );
					return NULL;
				} //end if
			} //end else if
			else if ( PC_CheckTokenString( source, "<" ) ) //bot names
			{
				key->flags |= RCKFL_BOTNAMES;
				strcpy( namebuffer, "" );
				do {
					if ( !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ) {
						BotFreeReplyChat( replychatlist );
						FreeSource( source );
						return NULL;
					} //end if
					StripDoubleQuotes( token.string );
					if ( strlen( namebuffer ) )
						strcat( namebuffer, "\\" );
					strcat( namebuffer, token.string );
				} while ( PC_CheckTokenString( source, "," ) );
				if ( !PC_ExpectTokenString( source, ">" ) ) {
					BotFreeReplyChat( replychatlist );
					FreeSource( source );
					return NULL;
				} //end if
				key->string = (char *)GetClearedHunkMemory(strlen(namebuffer) + 1);
				strcpy( key->string, namebuffer );
			} //end else if
			else //normal string key
			{
				key->flags |= RCKFL_STRING;
				if ( !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ) {
					BotFreeReplyChat( replychatlist );
					FreeSource( source );
					return NULL;
				} //end if
				StripDoubleQuotes( token.string );
				key->string = (char *)GetClearedHunkMemory(strlen(token.string) + 1);
				strcpy( key->string, token.string );
			} //end else
			//
			PC_CheckTokenString( source, "," );
		} while ( !PC_CheckTokenString( source, "]" ) );
		//
		BotCheckValidReplyChatKeySet( source, replychat->keys );
		//read the = sign and the priority
		if ( !PC_ExpectTokenString( source, "=" ) ||
			 !PC_ExpectTokenType( source, TT_NUMBER, 0, &token ) ) {
			BotFreeReplyChat( replychatlist );
			FreeSource( source );
			return NULL;
		} //end if
		replychat->priority = token.floatvalue;
		//read the leading {
		if ( !PC_ExpectTokenString( source, "{" ) ) {
			BotFreeReplyChat( replychatlist );
			FreeSource( source );
			return NULL;
		} //end if
		replychat->numchatmessages = 0;
		//while the trailing } is not found
		while ( !PC_CheckTokenString( source, "}" ) ) {
			if ( !BotLoadChatMessage( source, chatmessagestring, sizeof( chatmessagestring ) ) ) {
				BotFreeReplyChat( replychatlist );
				FreeSource( source );
				return NULL;
			} //end if
			chatmessage = (bot_chatmessage_t *)GetClearedHunkMemory(sizeof(bot_chatmessage_t) + strlen(chatmessagestring) + 1);
			chatmessage->chatmessage = (char *)chatmessage + sizeof( bot_chatmessage_t );
			strcpy( chatmessage->chatmessage, chatmessagestring );
			chatmessage->time = -2 * CHATMESSAGE_RECENTTIME;
			chatmessage->next = replychat->firstchatmessage;
			//add the chat message to the reply chat
			replychat->firstchatmessage = chatmessage;
			replychat->numchatmessages++;
		} //end while
	} //end while
	FreeSource( source );
	botimport.Print( PRT_MESSAGE, "loaded %s\n", filename );
	//
	//BotDumpReplyChat(replychatlist);
	if ( botDeveloper ) {
		BotCheckReplyChatIntegrety( replychatlist );
	} //end if
	//
	if ( !replychatlist )
		botimport.Print( PRT_MESSAGE, "no rchats\n" );
	//
	return replychatlist;
} //end of the function BotLoadReplyChat
#if 0
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotDumpInitialChat(bot_chat_t *chat)
{
	bot_chattype_t *t;
	bot_chatmessage_t *m;

	Log_Write("{");
	for (t = chat->types; t; t = t->next)
	{
		Log_Write(" type \"%s\"", t->name);
		Log_Write(" {");
		Log_Write("  numchatmessages = %d", t->numchatmessages);
		for (m = t->firstchatmessage; m; m = m->next)
		{
			Log_Write("  \"%s\"", m->chatmessage);
		} //end for
		Log_Write(" }");
	} //end for
	Log_Write("}");
} //end of the function BotDumpInitialChat
#endif
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static bot_chat_t *BotLoadInitialChat( const char *chatfile, const char *chatname ) {
	int pass, foundchat, indent;
	size_t size;
	char *ptr = NULL;
	char chatmessagestring[MAX_MESSAGE_SIZE];
	source_t *source;
	token_t token;
	bot_chat_t *chat = NULL;
	bot_chattype_t *chattype = NULL;
	bot_chatmessage_t *chatmessage = NULL;
#ifdef DEBUG
	int starttime;

	starttime = Sys_MilliSeconds();
#endif //DEBUG
	//
	size = 0;
	foundchat = qfalse;
	//a bot chat is parsed in two phases
	for ( pass = 0; pass < 2; pass++ ) {
		//allocate memory
		if ( pass && size )
			ptr = (char *)GetClearedMemory(size);
		//load the source file
		PC_SetBaseFolder( BOTFILESBASEFOLDER );
		source = LoadSourceFile( chatfile );
		if ( !source ) {
			botimport.Print( PRT_ERROR, "couldn't load %s\n", chatfile );
			return NULL;
		} //end if
		//chat structure
		if ( pass ) {
			chat = (bot_chat_t *)ptr;
			ptr += sizeof( bot_chat_t );
		} //end if
		size = sizeof( bot_chat_t );
		//
		while ( PC_ReadToken( source, &token ) ) {
			if ( !strcmp( token.string, "chat" ) ) {
				if ( !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ) {
					FreeSource( source );
					return NULL;
				} //end if
				StripDoubleQuotes( token.string );
				//after the chat name we expect an opening brace
				if ( !PC_ExpectTokenString( source, "{" ) ) {
					FreeSource( source );
					return NULL;
				} //end if
				//if the chat name is found
				if ( !Q_stricmp( token.string, chatname ) ) {
					foundchat = qtrue;
					//read the chat types
					while ( 1 ) {
						if ( !PC_ExpectAnyToken( source, &token ) ) {
							FreeSource( source );
							return NULL;
						} //end if
						if ( !strcmp( token.string, "}" ) )
							break;
						if ( strcmp( token.string, "type" ) ) {
							SourceError( source, "expected type found %s", token.string );
							FreeSource( source );
							return NULL;
						} //end if
						//expect the chat type name
						if ( !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ||
							 !PC_ExpectTokenString( source, "{" ) ) {
							FreeSource( source );
							return NULL;
						} //end if
						StripDoubleQuotes( token.string );
						if ( pass && ptr ) {
							chattype = (bot_chattype_t *)ptr;
							Q_strncpyz( chattype->name, token.string, sizeof( chattype->name ) );
							chattype->firstchatmessage = NULL;
							//add the chat type to the chat
							chattype->next = chat->types;
							chat->types = chattype;
							//
							ptr += sizeof( bot_chattype_t );
						} //end if
						size += sizeof( bot_chattype_t );
						//read the chat messages
						while ( !PC_CheckTokenString( source, "}" ) ) {
							size_t len;
							if ( !BotLoadChatMessage( source, chatmessagestring, sizeof( chatmessagestring ) ) ) {
								FreeSource( source );
								return NULL;
							} //end if
							len = strlen( chatmessagestring ) + 1;
							len = PAD( len, sizeof( uintptr_t ) );
							if ( pass && ptr ) {
								chatmessage = (bot_chatmessage_t *)ptr;
								chatmessage->time = -2 * CHATMESSAGE_RECENTTIME;
								//put the chat message in the list
								chatmessage->next = chattype->firstchatmessage;
								chattype->firstchatmessage = chatmessage;
								//store the chat message
								ptr += sizeof( bot_chatmessage_t );
								chatmessage->chatmessage = ptr;
								strcpy( chatmessage->chatmessage, chatmessagestring );
								ptr += len;
								//the number of chat messages increased
								chattype->numchatmessages++;
							} //end if
							size += sizeof( bot_chatmessage_t ) + len;
						} //end if
					} //end while
				} //end if
				else //skip the bot chat
				{
					indent = 1;
					while ( indent ) {
						if ( !PC_ExpectAnyToken( source, &token ) ) {
							FreeSource( source );
							return NULL;
						} //end if
						if ( !strcmp( token.string, "{" ) )
							indent++;
						else if ( !strcmp( token.string, "}" ) )
							indent--;
					} //end while
				} //end else
			} //end if
			else {
				SourceError( source, "unknown definition %s", token.string );
				FreeSource( source );
				return NULL;
			} //end else
		} //end while
		//free the source
		FreeSource( source );
		//if the requested character is not found
		if ( !foundchat ) {
			botimport.Print( PRT_ERROR, "couldn't find chat %s in %s\n", chatname, chatfile );
			return NULL;
		} //end if
	} //end for
	//
	botimport.Print( PRT_MESSAGE, "loaded %s from %s\n", chatname, chatfile );
	//
	//BotDumpInitialChat(chat);
	if ( botDeveloper ) {
		BotCheckInitialChatIntegrety( chat );
	} //end if
#ifdef DEBUG
	botimport.Print( PRT_MESSAGE, "initial chats loaded in %d msec\n", Sys_MilliSeconds() - starttime );
#endif //DEBUG
	// Retain the source identity for privately owned checkpoint content too.
	Q_strncpyz( chat->filename, chatfile, sizeof( chat->filename ) );
	Q_strncpyz( chat->chatname, chatname, sizeof( chat->chatname ) );
	//character was read successfully
	return chat;
} //end of the function BotLoadInitialChat
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
static void BotFreeChatFile( int chatstate ) {
	bot_chatstate_t *cs;

	cs = BotChatStateFromHandle( chatstate );
	if ( !cs )
		return;
	if ( cs->chat )
		FreeMemory( cs->chat );
	cs->chat = NULL;
} //end of the function BotFreeChatFile
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotLoadChatFile( int chatstate, const char *chatfile, const char *chatname ) {
	bot_chatstate_t *cs;
	int n, avail = 0;

	cs = BotChatStateFromHandle( chatstate );
	if ( !cs )
		return BLERR_CANNOTLOADICHAT;
	BotFreeChatFile( chatstate );

	if ( !LibVarGetValue( "bot_reloadcharacters" ) ) {
		avail = -1;
		for ( n = 0; n < MAX_CLIENTS; n++ ) {
			if ( !ichatdata[n] ) {
				if ( avail == -1 ) {
					avail = n;
				}
				continue;
			}
			if ( strcmp( chatfile, ichatdata[n]->filename ) != 0 ) {
				continue;
			}
			if ( strcmp( chatname, ichatdata[n]->chatname ) != 0 ) {
				continue;
			}
			cs->chat = ichatdata[n]->chat;
			//		botimport.Print( PRT_MESSAGE, "retained %s from %s\n", chatname, chatfile );
			return BLERR_NOERROR;
		}

		if ( avail == -1 ) {
			botimport.Print( PRT_FATAL, "ichatdata table full; couldn't load chat %s from %s\n", chatname, chatfile );
			return BLERR_CANNOTLOADICHAT;
		}
	}

	cs->chat = BotLoadInitialChat( chatfile, chatname );
	if ( !cs->chat ) {
		botimport.Print( PRT_FATAL, "couldn't load chat %s from %s\n", chatname, chatfile );
		return BLERR_CANNOTLOADICHAT;
	} //end if
	if ( !LibVarGetValue( "bot_reloadcharacters" ) ) {
		ichatdata[avail] = (bot_ichatdata_t *)GetClearedMemory( sizeof(bot_ichatdata_t) );
		ichatdata[avail]->chat = cs->chat;
		Q_strncpyz( ichatdata[avail]->chatname, chatname, sizeof( ichatdata[avail]->chatname ) );
		Q_strncpyz( ichatdata[avail]->filename, chatfile, sizeof( ichatdata[avail]->filename ) );
	} //end if

	return BLERR_NOERROR;
} //end of the function BotLoadChatFile
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
static int BotExpandChatMessage( char *outmessage, int size, const char *message, uint64_t mcontext,
	bot_match_t *match, uint64_t vcontext, int reply ) {
	int num, len, i, expansion;
	char *outputbuf;
	const char *ptr, *msgptr;
	char temp[MAX_MESSAGE_SIZE];

	expansion = qfalse;
	msgptr = message;
	outputbuf = outmessage;
	len = 0;

	while ( *msgptr != '\0' ) {
		if ( *msgptr == ESCAPE_CHAR ) {
			msgptr++;
			switch ( *msgptr ) {
			case 'v': //variable
			{
				msgptr++;
				num = 0;

				while ( *msgptr != '\0' && *msgptr != ESCAPE_CHAR )
					num = num * 10 + ( *msgptr++ ) - '0';

				//step over the trailing escape char
				if ( *msgptr != '\0' )
					msgptr++;

				if ( (size_t)num >= ARRAY_LEN( match->variables ) ) {
					botimport.Print( PRT_ERROR, "%s(): message \"%s\" variable %d out of range\n", __func__, message, num );
					return qfalse;
				}
				if ( match->variables[num].offset >= 0 ) {
					Q_ASSERT( match->variables[num].offset >= 0 );
					ptr = &match->string[(int)match->variables[num].offset];
					for ( i = 0; i < match->variables[num].length; i++ ) {
						temp[i] = ptr[i];
					}
					temp[i] = 0;
					//if it's a reply message
					if ( reply ) {
						//replace the reply synonyms in the variables
						BotReplaceReplySynonyms( temp, sizeof( temp ), vcontext );
					} else {
						//replace synonyms in the variable context
						BotReplaceSynonyms( temp, sizeof( temp ), vcontext );
					}

					if ( len + strlen( temp ) >= (size_t)size ) {
						botimport.Print( PRT_ERROR, "%s(): message \"%s\" too long\n", __func__, message );
						return qfalse;
					}
					strcpy( &outputbuf[len], temp );
					len = (int)( (size_t)len + ( strlen( temp ) ) );
				} //end if
				break;
			}
			case 'r': //random
			{
				msgptr++;
				for ( i = 0; ( *msgptr && *msgptr != ESCAPE_CHAR ); i++ ) {
					temp[i] = *msgptr++;
				} //end while
				temp[i] = '\0';

				//step over the trailing escape char
				if ( *msgptr != '\0' )
					msgptr++;

				//find the random keyword
				ptr = RandomString( temp );
				if ( !ptr ) {
					botimport.Print( PRT_ERROR, "%s(): unknown random string \"%s\"\n", __func__, temp );
					return qfalse;
				}

				if ( len + strlen( ptr ) >= (size_t)size ) {
					botimport.Print( PRT_ERROR, "%s(): message \"%s\" too long\n", __func__, message );
					return qfalse;
				}
				strcpy( &outputbuf[len], ptr );
				len = (int)( (size_t)len + ( strlen( ptr ) ) );
				expansion = qtrue;
				break;
			}
			default: {
				botimport.Print( PRT_FATAL, "%s(): message \"%s\" invalid escape char\n", __func__, message );
				break;
			}
			} //end switch
		} //end if
		else {
			if ( len >= size ) {
				botimport.Print( PRT_ERROR, "%s(): message \"%s\" too long\n", __func__, message );
				break;
			}
			outputbuf[len++] = *msgptr++;
		}
	}
	outputbuf[len] = '\0';

	//replace synonyms weighted in the message context
	BotReplaceWeightedSynonyms( outputbuf, size, mcontext );

	//return true if a random was expanded
	return expansion;
} //end of the function BotExpandChatMessage
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
static void BotConstructChatMessage( bot_chatstate_t *chatstate, const char *message, uint64_t mcontext,
	bot_match_t *match, uint64_t vcontext, int reply ) {
	int i;
	char srcmessage[MAX_MESSAGE_SIZE];

	Q_strncpyz( srcmessage, message, sizeof( srcmessage ) );

	for ( i = 0; i < 10; i++ ) {
		if ( !BotExpandChatMessage( chatstate->chatmessage, sizeof( chatstate->chatmessage ), srcmessage, mcontext, match, vcontext, reply ) ) {
			break;
		}
		strcpy( srcmessage, chatstate->chatmessage );
	}

	if ( i >= 10 ) {
		botimport.Print( PRT_WARNING, "too many expansions in chat message\n" );
		botimport.Print( PRT_WARNING, "%s\n", chatstate->chatmessage );
	} //end if
} //end of the function BotConstructChatMessage
//===========================================================================
// randomly chooses one of the chat message of the given type
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static const char *BotChooseInitialChatMessage( bot_chatstate_t *cs, const char *type ) {
	int n, numchatmessages;
	float besttime;
	bot_chattype_t *t;
	bot_chatmessage_t *m, *bestchatmessage;
	bot_chat_t *chat;

	chat = cs->chat;
	for ( t = chat->types; t; t = t->next ) {
		if ( !Q_stricmp( t->name, type ) ) {
			numchatmessages = 0;
			for ( m = t->firstchatmessage; m; m = m->next ) {
				if ( m->time > AAS_Time() )
					continue;
				numchatmessages++;
			} //end if
			//if all chat messages have been used recently
			if ( numchatmessages <= 0 ) {
				besttime = 0;
				bestchatmessage = NULL;
				for ( m = t->firstchatmessage; m; m = m->next ) {
					if ( !besttime || m->time < besttime ) {
						bestchatmessage = m;
						besttime = m->time;
					} //end if
				} //end for
				if ( bestchatmessage )
					return bestchatmessage->chatmessage;
			} //end if
			else //choose a chat message randomly
			{
				n = (int)( random() * numchatmessages );
				for ( m = t->firstchatmessage; m; m = m->next ) {
					if ( m->time > AAS_Time() )
						continue;
					if ( --n < 0 ) {
						m->time = AAS_Time() + CHATMESSAGE_RECENTTIME;
						return m->chatmessage;
					} //end if
				} //end for
			} //end else
			return NULL;
		} //end if
	} //end for
	return NULL;
} //end of the function BotChooseInitialChatMessage
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotNumInitialChats( int chatstate, const char *type ) {
	bot_chatstate_t *cs;
	bot_chattype_t *t;

	cs = BotChatStateFromHandle( chatstate );
	if ( !cs )
		return 0;

	for ( t = cs->chat->types; t; t = t->next ) {
		if ( !Q_stricmp( t->name, type ) ) {
			if ( LibVarGetValue( "bot_testichat" ) ) {
				botimport.Print( PRT_MESSAGE, "%s has %d chat lines\n", type, t->numchatmessages );
				botimport.Print( PRT_MESSAGE, "-------------------\n" );
			}
			return t->numchatmessages;
		} //end if
	} //end for
	return 0;
} //end of the function BotNumInitialChats
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotInitialChat( int chatstate, const char *type, int mcontext, const char *var0, const char *var1, const char *var2, const char *var3, const char *var4, const char *var5, const char *var6, const char *var7 ) {
	const char *message;
	int index, len;
	bot_match_t match;
	bot_chatstate_t *cs;

	cs = BotChatStateFromHandle( chatstate );
	if ( !cs )
		return;
	//if no chat file is loaded
	if ( !cs->chat )
		return;
	//choose a chat message randomly of the given type
	message = BotChooseInitialChatMessage( cs, type );
	//if there's no message of the given type
	if ( !message ) {
#ifdef DEBUG
		botimport.Print( PRT_MESSAGE, "no chat messages of type %s\n", type );
#endif //DEBUG
		return;
	} //end if
	//
	Com_Memset( &match, 0, sizeof( match ) );
	index = 0;
	if ( var0 ) {
		len = (int)strlen( var0 );
		match.variables[0].offset = (signed char)( index );
		if ( (size_t)( len + index ) < sizeof( match.string ) ) {
			match.variables[0].length = len;
			strcat( match.string, var0 );
			index = (int)( (size_t)index + ( strlen( var0 ) ) );
		}
	}
	if ( var1 ) {
		len = (int)strlen( var1 );
		match.variables[1].offset = (signed char)( index );
		if ( (size_t)( len + index ) < sizeof( match.string ) ) {
			match.variables[1].length = len;
			strcat( match.string, var1 );
			index += len;
		}
	}
	if ( var2 ) {
		len = (int)strlen( var2 );
		match.variables[2].offset = (signed char)( index );
		if ( (size_t)( len + index ) < sizeof( match.string ) ) {
			match.variables[2].length = len;
			strcat( match.string, var2 );
			index += len;
		}
	}
	if ( var3 ) {
		len = (int)strlen( var3 );
		match.variables[3].offset = (signed char)( index );
		if ( (size_t)( len + index ) < sizeof( match.string ) ) {
			match.variables[3].length = len;
			strcat( match.string, var3 );
			index += len;
		}
	}
	if ( var4 ) {
		len = (int)strlen( var4 );
		match.variables[4].offset = (signed char)( index );
		if ( (size_t)( len + index ) < sizeof( match.string ) ) {
			match.variables[4].length = len;
			strcat( match.string, var4 );
			index += len;
		}
	}
	if ( var5 ) {
		len = (int)strlen( var5 );
		match.variables[5].offset = (signed char)( index );
		if ( (size_t)( len + index ) < sizeof( match.string ) ) {
			match.variables[5].length = len;
			strcat( match.string, var5 );
			index += len;
		}
	}
	if ( var6 ) {
		len = (int)strlen( var6 );
		match.variables[6].offset = (signed char)( index );
		if ( (size_t)( len + index ) < sizeof( match.string ) ) {
			match.variables[6].length = len;
			strcat( match.string, var6 );
			index += len;
		}
	}
	if ( var7 ) {
		len = (int)strlen( var7 );
		match.variables[7].offset = (signed char)( index );
		if ( (size_t)( len + index ) < sizeof( match.string ) ) {
			match.variables[7].length = (int)( strlen( var7 ) );
			strcat( match.string, var7 );
			//index += len;
		}
	}

	BotConstructChatMessage( cs, message, mcontext, &match, 0, qfalse );
}
#if 0
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotPrintReplyChatKeys(bot_replychat_t *replychat)
{
	bot_replychatkey_t *key;
	bot_matchpiece_t *mp;

	botimport.Print(PRT_MESSAGE, "[");
	for (key = replychat->keys; key; key = key->next)
	{
		if (key->flags & RCKFL_AND) botimport.Print(PRT_MESSAGE, "&");
		else if (key->flags & RCKFL_NOT) botimport.Print(PRT_MESSAGE, "!");
		//
		if (key->flags & RCKFL_NAME) botimport.Print(PRT_MESSAGE, "name");
		else if (key->flags & RCKFL_GENDERFEMALE) botimport.Print(PRT_MESSAGE, "female");
		else if (key->flags & RCKFL_GENDERMALE) botimport.Print(PRT_MESSAGE, "male");
		else if (key->flags & RCKFL_GENDERLESS) botimport.Print(PRT_MESSAGE, "it");
		else if (key->flags & RCKFL_VARIABLES)
		{
			botimport.Print(PRT_MESSAGE, "(");
			for (mp = key->match; mp; mp = mp->next)
			{
				if (mp->type == MT_STRING) botimport.Print(PRT_MESSAGE, "\"%s\"", mp->firststring->string);
				else botimport.Print(PRT_MESSAGE, "%d", mp->variable);
				if (mp->next) botimport.Print(PRT_MESSAGE, ", ");
			} //end for
			botimport.Print(PRT_MESSAGE, ")");
		} //end if
		else if (key->flags & RCKFL_STRING)
		{
			botimport.Print(PRT_MESSAGE, "\"%s\"", key->string);
		} //end if
		if (key->next) botimport.Print(PRT_MESSAGE, ", ");
		else botimport.Print(PRT_MESSAGE, "] = %1.0f\n", replychat->priority);
	} //end for
	botimport.Print(PRT_MESSAGE, "{\n");
} //end of the function BotPrintReplyChatKeys
#endif
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotReplyChat( int chatstate, const char *message, int mcontext, int vcontext, const char *var0, const char *var1, const char *var2, const char *var3, const char *var4, const char *var5, const char *var6, const char *var7 ) {
	bot_replychat_t *rchat, *bestrchat;
	bot_replychatkey_t *key;
	bot_chatmessage_t *m, *bestchatmessage;
	bot_match_t match, bestmatch;
	int bestpriority, num, found, res, numchatmessages, index;
	bot_chatstate_t *cs;

	cs = BotChatStateFromHandle( chatstate );
	if ( !cs )
		return qfalse;
	Com_Memset( &match, 0, sizeof( match ) );
	Q_strncpyz( match.string, message, sizeof( match.string ) );
	bestpriority = -1;
	bestchatmessage = NULL;
	bestrchat = NULL;
	//go through all the reply chats
	for ( rchat = replychats; rchat; rchat = rchat->next ) {
		found = qfalse;
		for ( key = rchat->keys; key; key = key->next ) {
			res = qfalse;
			//get the match result
			if ( key->flags & RCKFL_NAME )
				res = ( StringContains( message, cs->name, qfalse ) != -1 );
			else if ( key->flags & RCKFL_BOTNAMES )
				res = ( StringContains( key->string, cs->name, qfalse ) != -1 );
			else if ( key->flags & RCKFL_GENDERFEMALE )
				res = ( cs->gender == CHAT_GENDERFEMALE );
			else if ( key->flags & RCKFL_GENDERMALE )
				res = ( cs->gender == CHAT_GENDERMALE );
			else if ( key->flags & RCKFL_GENDERLESS )
				res = ( cs->gender == CHAT_GENDERLESS );
			else if ( key->flags & RCKFL_VARIABLES )
				res = StringsMatch( key->match, &match );
			else if ( key->flags & RCKFL_STRING )
				res = ( StringContainsWord( message, key->string ) != NULL );
			//if the key must be present
			if ( key->flags & RCKFL_AND ) {
				if ( !res ) {
					found = qfalse;
					break;
				} //end if
			} //end else if
			//if the key must be absent
			else if ( key->flags & RCKFL_NOT ) {
				if ( res ) {
					found = qfalse;
					break;
				} //end if
			} //end if
			else if ( res ) {
				found = qtrue;
			} //end else
		} //end for
		//
		if ( found ) {
			if ( rchat->priority > bestpriority ) {
				numchatmessages = 0;
				for ( m = rchat->firstchatmessage; m; m = m->next ) {
					if ( m->time > AAS_Time() )
						continue;
					numchatmessages++;
				} //end if
				num = (int)( random() * numchatmessages );
				for ( m = rchat->firstchatmessage; m; m = m->next ) {
					if ( --num < 0 )
						break;
					if ( m->time > AAS_Time() )
						continue;
				} //end for
				//if the reply chat has a message
				if ( m ) {
					Com_Memcpy( &bestmatch, &match, sizeof( bot_match_t ) );
					bestchatmessage = m;
					bestrchat = rchat;
					bestpriority = (int)( rchat->priority );
				} //end if
			} //end if
		} //end if
	} //end for
	if ( bestchatmessage ) {
		int len;
		index = (int)( strlen( bestmatch.string ) );
		if ( var0 ) {
			len = (int)strlen( var0 );
			bestmatch.variables[0].offset = (signed char)( index );
			if ( (size_t)( len + index ) < sizeof( bestmatch.string ) ) {
				bestmatch.variables[0].length = len;
				strcat( bestmatch.string, var0 );
				index += len;
			}
		}
		if ( var1 ) {
			len = (int)strlen( var1 );
			bestmatch.variables[1].offset = (signed char)( index );
			if ( (size_t)( len + index ) < sizeof( bestmatch.string ) ) {
				bestmatch.variables[1].length = len;
				strcat( bestmatch.string, var1 );
				index += len;
			}
		}
		if ( var2 ) {
			len = (int)strlen( var2 );
			bestmatch.variables[2].offset = (signed char)( index );
			if ( (size_t)( len + index ) < sizeof( bestmatch.string ) ) {
				bestmatch.variables[2].length = len;
				strcat( bestmatch.string, var2 );
				index += len;
			}
		}
		if ( var3 ) {
			len = (int)strlen( var3 );
			bestmatch.variables[3].offset = (signed char)( index );
			if ( (size_t)( len + index ) < sizeof( bestmatch.string ) ) {
				bestmatch.variables[3].length = len;
				strcat( bestmatch.string, var3 );
				index += len;
			}
		}
		if ( var4 ) {
			len = (int)strlen( var4 );
			bestmatch.variables[4].offset = (signed char)( index );
			if ( (size_t)( len + index ) < sizeof( bestmatch.string ) ) {
				bestmatch.variables[4].length = len;
				strcat( bestmatch.string, var4 );
				index += len;
			}
		}
		if ( var5 ) {
			len = (int)strlen( var5 );
			bestmatch.variables[5].offset = (signed char)( index );
			if ( (size_t)( len + index ) < sizeof( bestmatch.string ) ) {
				bestmatch.variables[5].length = len;
				strcat( bestmatch.string, var5 );
				index += len;
			}
		}
		if ( var6 ) {
			len = (int)strlen( var6 );
			bestmatch.variables[6].offset = (signed char)( index );
			if ( (size_t)( len + index ) < sizeof( bestmatch.string ) ) {
				bestmatch.variables[6].length = len;
				strcat( bestmatch.string, var6 );
				index += len;
			}
		}
		if ( var7 ) {
			len = (int)strlen( var7 );
			bestmatch.variables[7].offset = (signed char)( index );
			if ( (size_t)( len + index ) < sizeof( bestmatch.string ) ) {
				bestmatch.variables[7].length = len;
				strcat( bestmatch.string, var7 );
				//index += len;
			}
		}
		if ( LibVarGetValue( "bot_testrchat" ) ) {
			for ( m = bestrchat->firstchatmessage; m; m = m->next ) {
				BotConstructChatMessage( cs, m->chatmessage, mcontext, &bestmatch, vcontext, qtrue );
				BotRemoveTildes( cs->chatmessage );
				botimport.Print( PRT_MESSAGE, "%s\n", cs->chatmessage );
			}
		} else {
			bestchatmessage->time = AAS_Time() + CHATMESSAGE_RECENTTIME;
			BotConstructChatMessage( cs, bestchatmessage->chatmessage, mcontext, &bestmatch, vcontext, qtrue );
		}
		return qtrue;
	}
	return qfalse;
} //end of the function BotReplyChat
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotChatLength( int chatstate ) {
	bot_chatstate_t *cs;

	cs = BotChatStateFromHandle( chatstate );
	if ( !cs )
		return 0;
	return (int)( strlen( cs->chatmessage ) );
} //end of the function BotChatLength
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotEnterChat( int chatstate, int clientto, int sendto ) {
	bot_chatstate_t *cs;

	cs = BotChatStateFromHandle( chatstate );
	if ( !cs )
		return;

	if ( strlen( cs->chatmessage ) ) {
		BotRemoveTildes( cs->chatmessage );
		if ( LibVarGetValue( "bot_testichat" ) ) {
			botimport.Print( PRT_MESSAGE, "%s\n", cs->chatmessage );
		} else {
			switch ( sendto ) {
			case CHAT_TEAM:
				EA_Command( cs->client, va( "say_team %s", cs->chatmessage ) );
				break;
			case CHAT_TELL:
				EA_Command( cs->client, va( "tell %d %s", clientto, cs->chatmessage ) );
				break;
			default: //CHAT_ALL
				EA_Command( cs->client, va( "say %s", cs->chatmessage ) );
				break;
			}
		}
		//clear the chat message from the state
		strcpy( cs->chatmessage, "" );
	} //end if
} //end of the function BotEnterChat
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotGetChatMessage( int chatstate, char *buf, int size ) {
	bot_chatstate_t *cs;

	cs = BotChatStateFromHandle( chatstate );
	if ( !cs )
		return;

	BotRemoveTildes( cs->chatmessage );
	Q_strncpyz( buf, cs->chatmessage, size );
	//clear the chat message from the state
	cs->chatmessage[0] = '\0';
} //end of the function BotGetChatMessage
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotSetChatGender( int chatstate, int gender ) {
	bot_chatstate_t *cs;

	cs = BotChatStateFromHandle( chatstate );
	if ( !cs )
		return;
	switch ( gender ) {
	case CHAT_GENDERFEMALE:
		cs->gender = CHAT_GENDERFEMALE;
		break;
	case CHAT_GENDERMALE:
		cs->gender = CHAT_GENDERMALE;
		break;
	default:
		cs->gender = CHAT_GENDERLESS;
		break;
	} //end switch
} //end of the function BotSetChatGender
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotSetChatName( int chatstate, const char *name, int client ) {
	bot_chatstate_t *cs;

	cs = BotChatStateFromHandle( chatstate );
	if ( !cs )
		return;
	cs->client = client;
	Q_strncpyz( cs->name, name, sizeof( cs->name ) );
} //end of the function BotSetChatName
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotResetChatAI( void ) {
	bot_replychat_t *rchat;
	bot_chatmessage_t *m;

	for ( rchat = replychats; rchat; rchat = rchat->next ) {
		for ( m = rchat->firstchatmessage; m; m = m->next ) {
			m->time = 0;
		} //end for
	} //end for
} //end of the function BotResetChatAI
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
int BotAllocChatState( void ) {
	int i;

	for ( i = 1; i <= MAX_CLIENTS; i++ ) {
		if ( !botchatstates[i] ) {
			botchatstates[i] = (bot_chatstate_t *)GetClearedMemory(sizeof(bot_chatstate_t));
			return i;
		} //end if
	} //end for
	return 0;
} //end of the function BotAllocChatState
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
void BotFreeChatState( int handle ) {
	int h;

	if ( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "chat state handle %d out of range\n", handle );
		return;
	} //end if
	if ( !botchatstates[handle] ) {
		botimport.Print( PRT_FATAL, "invalid chat state %d\n", handle );
		return;
	} //end if
	if ( LibVarGetValue( "bot_reloadcharacters" ) ) {
		BotFreeChatFile( handle );
	} //end if
	//free all the console messages left in the chat state
	for ( h = BotNextConsoleMessage( handle, NULL ); h; h = BotNextConsoleMessage( handle, NULL ) ) {
		//remove the console message
		BotRemoveConsoleMessage( handle, h );
	} //end for
	FreeMemory( botchatstates[handle] );
	botchatstates[handle] = NULL;
} //end of the function BotFreeChatState
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotSetupChatAI( void ) {
	const char *file;

#ifdef DEBUG
	int starttime = Sys_MilliSeconds();
#endif //DEBUG

	file = LibVarString( "synfile", "syn.c" );
	synonyms = BotLoadSynonyms( file );
	file = LibVarString( "rndfile", "rnd.c" );
	randomstrings = BotLoadRandomStrings( file );
	file = LibVarString( "matchfile", "match.c" );
	matchtemplates = BotLoadMatchTemplates( file );
	//
	if ( !LibVarValue( "nochat", "0" ) ) {
		file = LibVarString( "rchatfile", "rchat.c" );
		replychats = BotLoadReplyChat( file );
	} //end if

	InitConsoleMessageHeap();

#ifdef DEBUG
	botimport.Print( PRT_MESSAGE, "setup chat AI %d msec\n", Sys_MilliSeconds() - starttime );
#endif //DEBUG
	return BLERR_NOERROR;
} //end of the function BotSetupChatAI
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotShutdownChatAI( void ) {
	int i;

	//free all remaining chat states
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( botchatstates[i] ) {
			BotFreeChatState( i );
		} //end if
	} //end for
	//free all cached chats
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( ichatdata[i] ) {
			FreeMemory( ichatdata[i]->chat );
			FreeMemory( ichatdata[i] );
			ichatdata[i] = NULL;
		} //end if
	} //end for
	if ( consolemessageheap )
		FreeMemory( consolemessageheap );
	consolemessageheap = NULL;
	allocatedConsoleMessages = 0;
	if ( matchtemplates )
		BotFreeMatchTemplates( matchtemplates );
	matchtemplates = NULL;
	if ( randomstrings )
		FreeMemory( randomstrings );
	randomstrings = NULL;
	if ( synonyms )
		FreeMemory( synonyms );
	synonyms = NULL;
	if ( replychats )
		BotFreeReplyChat( replychats );
	replychats = NULL;
} //end of the function BotShutdownChatAI

// Command-only queue drafts. Chunks keep the maximum 65,536-message pool below
// the archive record limit; free payload is overwritten by BotQueueConsoleMessage.
static constexpr int MAX_SAVED_CONSOLE_MESSAGES = 65536;
static constexpr int CHAT_SAVE_CHUNK = 64;
struct chatQueueHeader_t {
	int32_t count, free;
	uint32_t present[MAX_CLIENTS + 1];
	int32_t first[MAX_CLIENTS + 1], last[MAX_CLIENTS + 1];
};
static constexpr stateField_t chatQueueHeaderFields[] = {
	{ "count", offsetof( chatQueueHeader_t, count ), 1, stateType_t::Int32 },
	{ "free", offsetof( chatQueueHeader_t, free ), 1, stateType_t::Int32 },
	{ "present", offsetof( chatQueueHeader_t, present ), MAX_CLIENTS + 1, stateType_t::UInt32 },
	{ "first", offsetof( chatQueueHeader_t, first ), MAX_CLIENTS + 1, stateType_t::Int32 },
	{ "last", offsetof( chatQueueHeader_t, last ), MAX_CLIENTS + 1, stateType_t::Int32 }
};
static constexpr stateSchema_t chatQueueHeaderSchema = { "botlib.chatQueuePool", 1, 1, sizeof( chatQueueHeader_t ), chatQueueHeaderFields, 5 };
static constexpr stateField_t chatActorFields[] = {
	{ "gender", offsetof( bot_chatstate_t, gender ), 1, stateType_t::Int32 },
	{ "client", offsetof( bot_chatstate_t, client ), 1, stateType_t::Int32 },
	{ "name", offsetof( bot_chatstate_t, name ), 32, stateType_t::String },
	{ "chatmessage", offsetof( bot_chatstate_t, chatmessage ), MAX_MESSAGE_SIZE, stateType_t::String },
	{ "handle", offsetof( bot_chatstate_t, handle ), 1, stateType_t::Int32 },
	{ "numconsolemessages", offsetof( bot_chatstate_t, numconsolemessages ), 1, stateType_t::Int32 }
};
static constexpr stateSchema_t chatActorSchema = { "botlib.chatActor", 1, 1, sizeof( bot_chatstate_t ), chatActorFields, 6 };
static int32_t savedChatNext[MAX_SAVED_CONSOLE_MESSAGES];
static int32_t savedChatPrevious[MAX_SAVED_CONSOLE_MESSAGES];
static bool savedChatLive[MAX_SAVED_CONSOLE_MESSAGES];
static bot_chatstate_t savedChatActors[MAX_CLIENTS + 1];
struct chatQueueChunk_t {
	int32_t handle[CHAT_SAVE_CHUNK], type[CHAT_SAVE_CHUNK];
	float time[CHAT_SAVE_CHUNK];
	char message[CHAT_SAVE_CHUNK][MAX_MESSAGE_SIZE];
};
static int ChatMessageSlot( const bot_consolemessage_t *message ) {
	if ( !message )
		return -1;
	const uintptr_t address = reinterpret_cast<uintptr_t>( message ), base = reinterpret_cast<uintptr_t>( consolemessageheap );
	if ( !consolemessageheap || address < base || ( address - base ) % sizeof( *message ) ||
		 ( address - base ) / sizeof( *message ) >= uint32_t( allocatedConsoleMessages ) )
		return -2;
	return int( ( address - base ) / sizeof( *message ) );
}
static bool ChatQueueLinks( stateWriter_t *writer, const stateReader_t *reader, uint32_t count ) {
	if ( !count )
		return true;
	const stateField_t field = { "next", 0, count, stateType_t::Int32 };
	const stateSchema_t schema = { "botlib.chatQueueLinks", 1, 1, sizeof( savedChatNext ), &field, 1 };
	uint32_t version;
	return writer ? State_Append( writer, schema, 0, savedChatNext ) : State_Find( *reader, schema, 0, savedChatNext, &version );
}
static bool ValidChatQueues( const chatQueueHeader_t &header, bot_chatstate_t *const *states = botchatstates ) {
	if ( header.count < 0 || header.count > MAX_SAVED_CONSOLE_MESSAGES || header.count != allocatedConsoleMessages ||
		 bool( header.count ) != bool( consolemessageheap ) || header.present[0] || states[0] )
		return false;
	memset( savedChatLive, 0, sizeof( savedChatLive ) );
	// -2 means unvisited; -1 is a valid list head predecessor.
	for ( int i = 0; i < header.count; ++i ) {
		savedChatPrevious[i] = -2;
		if ( savedChatNext[i] < -1 || savedChatNext[i] >= header.count )
			return false;
	}
	for ( int owner = 0; owner <= MAX_CLIENTS; ++owner ) {
		if ( header.present[owner] != uint32_t( states[owner] != nullptr ) )
			return false;
		if ( owner && !header.present[owner] )
			continue;
		const auto &actor = savedChatActors[owner];
		if ( owner && ( actor.gender < CHAT_GENDERLESS || actor.gender > CHAT_GENDERMALE || actor.client < 0 || actor.client >= MAX_CLIENTS ||
						  actor.handle < 0 || actor.handle > 8192 || actor.numconsolemessages < 0 || actor.numconsolemessages > header.count ) )
			return false;
		int previous = -1, count = 0;
		for ( int slot = owner ? header.first[owner] : header.free; slot != -1; slot = savedChatNext[slot] ) {
			if ( slot < 0 || slot >= header.count || savedChatPrevious[slot] != -2 )
				return false;
			savedChatPrevious[slot] = previous;
			savedChatLive[slot] = owner != 0;
			previous = slot;
			++count;
		}
		if ( owner && ( count != actor.numconsolemessages || previous != header.last[owner] ) )
			return false;
	}
	for ( int i = 0; i < header.count; ++i )
		if ( savedChatPrevious[i] == -2 )
			return false;
	return true;
}
static bool ChatQueueChunk( stateWriter_t *writer, const stateReader_t *reader, uint32_t base, uint32_t count, bool apply ) {
	bool live = false;
	for ( uint32_t i = 0; i < count; ++i )
		live |= savedChatLive[base + i];
	if ( !live )
		return true;
	chatQueueChunk_t chunk{};
	stateField_t fields[CHAT_SAVE_CHUNK + 3] = {
		{ "handle", offsetof( chatQueueChunk_t, handle ), count, stateType_t::Int32 },
		{ "type", offsetof( chatQueueChunk_t, type ), count, stateType_t::Int32 },
		{ "time", offsetof( chatQueueChunk_t, time ), count, stateType_t::Float32 }
	};
	char names[CHAT_SAVE_CHUNK][16];
	for ( uint32_t i = 0; i < count; ++i ) {
		snprintf( names[i], sizeof( names[i] ), "message%u", i );
		fields[3 + i] = { names[i], uint32_t( offsetof( chatQueueChunk_t, message ) + i * MAX_MESSAGE_SIZE ), MAX_MESSAGE_SIZE, stateType_t::String };
		if ( writer && savedChatLive[base + i] ) {
			const auto &message = consolemessageheap[base + i];
			chunk.handle[i] = message.handle;
			chunk.type[i] = message.type;
			chunk.time[i] = message.time;
			memcpy( chunk.message[i], message.message, MAX_MESSAGE_SIZE );
		}
	}
	const stateSchema_t schema = { "botlib.chatQueueText", 1, 1, sizeof( chunk ), fields, count + 3 };
	uint32_t version;
	if ( reader && !State_Find( *reader, schema, base / CHAT_SAVE_CHUNK, &chunk, &version ) )
		return false;
	for ( uint32_t i = 0; i < count; ++i )
		if ( savedChatLive[base + i] &&
			 ( chunk.handle[i] < 1 || chunk.handle[i] > 8192 || !std::isfinite( chunk.time[i] ) || !memchr( chunk.message[i], 0, MAX_MESSAGE_SIZE ) ) )
			return false;
	if ( writer )
		return State_Append( writer, schema, base / CHAT_SAVE_CHUNK, &chunk );
	if ( apply )
		for ( uint32_t i = 0; i < count; ++i )
			if ( savedChatLive[base + i] ) {
				auto &message = consolemessageheap[base + i];
				message.handle = chunk.handle[i];
				message.type = chunk.type[i];
				message.time = chunk.time[i];
				memcpy( message.message, chunk.message[i], MAX_MESSAGE_SIZE );
			}
	return true;
}
bool Bot_WriteChatQueueState( stateWriter_t *writer ) {
	if ( !writer )
		return false;
	if ( allocatedConsoleMessages < 0 || allocatedConsoleMessages > MAX_SAVED_CONSOLE_MESSAGES ||
		 bool( allocatedConsoleMessages ) != bool( consolemessageheap ) ) {
		writer->failed = true;
		return false;
	}
	chatQueueHeader_t header{};
	header.count = allocatedConsoleMessages;
	header.free = header.count ? ChatMessageSlot( freeconsolemessages ) : -1;
	for ( int i = 0; i <= MAX_CLIENTS; ++i ) {
		header.present[i] = botchatstates[i] != nullptr;
		if ( !header.present[i] )
			continue;
		savedChatActors[i] = *botchatstates[i];
		header.first[i] = ChatMessageSlot( savedChatActors[i].firstmessage );
		header.last[i] = ChatMessageSlot( savedChatActors[i].lastmessage );
	}
	for ( int i = 0; i < header.count; ++i )
		savedChatNext[i] = ChatMessageSlot( consolemessageheap[i].next );
	if ( !ValidChatQueues( header ) ) {
		writer->failed = true;
		return false;
	}
	for ( int i = 0; i < header.count; ++i )
		if ( ChatMessageSlot( consolemessageheap[i].prev ) != savedChatPrevious[i] ) {
			writer->failed = true;
			return false;
		}
	if ( !State_Append( writer, chatQueueHeaderSchema, 0, &header ) || !ChatQueueLinks( writer, nullptr, uint32_t( header.count ) ) )
		return false;
	for ( uint32_t i = 1; i <= MAX_CLIENTS; ++i )
		if ( header.present[i] && !State_Append( writer, chatActorSchema, i, &savedChatActors[i] ) )
			return false;
	for ( uint32_t base = 0; base < uint32_t( header.count ); base += CHAT_SAVE_CHUNK )
		if ( !ChatQueueChunk( writer, nullptr, base, uint32_t( header.count ) - base < CHAT_SAVE_CHUNK ? uint32_t( header.count ) - base : CHAT_SAVE_CHUNK, false ) ) {
			writer->failed = true;
			return false;
		}
	return true;
}
static bool ReadChatQueueState( const stateReader_t &reader, bool apply, bot_chatstate_t *const *states ) {
	chatQueueHeader_t header;
	uint32_t version;
	if ( !State_Find( reader, chatQueueHeaderSchema, 0, &header, &version ) || header.count < 0 || header.count > MAX_SAVED_CONSOLE_MESSAGES ||
		 !ChatQueueLinks( nullptr, &reader, uint32_t( header.count ) ) )
		return false;
	for ( uint32_t i = 1; i <= MAX_CLIENTS; ++i )
		if ( header.present[i] && !State_Find( reader, chatActorSchema, i, &savedChatActors[i], &version ) )
			return false;
	if ( !ValidChatQueues( header, states ) )
		return false;
	for ( uint32_t base = 0; base < uint32_t( header.count ); base += CHAT_SAVE_CHUNK )
		if ( !ChatQueueChunk( nullptr, &reader, base, uint32_t( header.count ) - base < CHAT_SAVE_CHUNK ? uint32_t( header.count ) - base : CHAT_SAVE_CHUNK, false ) )
			return false;
	if ( apply ) {
		for ( int i = 0; i < header.count; ++i ) {
			auto &message = consolemessageheap[i];
			message = {};
			message.next = savedChatNext[i] < 0 ? nullptr : &consolemessageheap[savedChatNext[i]];
			message.prev = savedChatPrevious[i] < 0 ? nullptr : &consolemessageheap[savedChatPrevious[i]];
		}
		for ( uint32_t base = 0; base < uint32_t( header.count ); base += CHAT_SAVE_CHUNK )
			if ( !ChatQueueChunk( nullptr, &reader, base, uint32_t( header.count ) - base < CHAT_SAVE_CHUNK ? uint32_t( header.count ) - base : CHAT_SAVE_CHUNK, true ) )
				return false;
		for ( int i = 1; i <= MAX_CLIENTS; ++i )
			if ( header.present[i] ) {
				auto *chat = states[i]->chat; // Immutable chat ownership is restored separately.
				*states[i] = savedChatActors[i];
				states[i]->chat = chat;
				states[i]->firstmessage = header.first[i] < 0 ? nullptr : &consolemessageheap[header.first[i]];
				states[i]->lastmessage = header.last[i] < 0 ? nullptr : &consolemessageheap[header.last[i]];
			}
		freeconsolemessages = header.free < 0 ? nullptr : &consolemessageheap[header.free];
	}
	return true;
}

bool Bot_ReadChatQueueState( const stateReader_t &reader, bool apply ) {
	return ReadChatQueueState( reader, apply, botchatstates );
}
bool Bot_PrepareChatQueueState( const stateReader_t &reader ) {
	for ( const auto *state : botchatstates )
		if ( state )
			return false;
	chatQueueHeader_t header;
	uint32_t version;
	if ( !State_Find( reader, chatQueueHeaderSchema, 0, &header, &version ) || header.present[0] )
		return false;
	for ( uint32_t present : header.present )
		if ( present > 1 )
			return false;
	bot_chatstate_t *draft[MAX_CLIENTS + 1] = {};
	for ( int i = 1; i <= MAX_CLIENTS; ++i )
		if ( header.present[i] )
			draft[i] = (bot_chatstate_t *)GetClearedMemory( sizeof( bot_chatstate_t ) );
	if ( !ReadChatQueueState( reader, true, draft ) ) {
		for ( auto *state : draft )
			if ( state )
				FreeMemory( state );
		return false;
	}
	memcpy( botchatstates, draft, sizeof( draft ) );
	return true;
}

// ponytail: at most 8,192 lines and 65,536 syntax nodes per loaded chat owner;
// reject larger checkpoints explicitly, increase only for measured content needs.
static constexpr uint32_t MAX_SAVED_CHAT_LINES = 8192;
static bot_chatmessage_t *savedChatLines[MAX_SAVED_CHAT_LINES];
static float savedChatTimes[MAX_SAVED_CHAT_LINES];
struct chatContentIdentity_t {
	char filename[MAX_QPATH], chatname[MAX_QPATH];
	uint32_t count;
	uint8_t hash[32];
};
static constexpr stateField_t chatContentFields[] = {
	{ "filename", offsetof( chatContentIdentity_t, filename ), MAX_QPATH, stateType_t::String },
	{ "chatname", offsetof( chatContentIdentity_t, chatname ), MAX_QPATH, stateType_t::String },
	{ "count", offsetof( chatContentIdentity_t, count ), 1, stateType_t::UInt32 },
	{ "hash", offsetof( chatContentIdentity_t, hash ), 32, stateType_t::Bytes }
};
static constexpr stateSchema_t chatContentSchema = { "botlib.chatContent", 1, 1, sizeof( chatContentIdentity_t ), chatContentFields, 4 };
struct chatContentPool_t {
	uint32_t cached[MAX_CLIENTS];
	int32_t owner[MAX_CLIENTS + 1]; // absent=-3, private=-2, no chat=-1, otherwise cache slot
};
static constexpr stateField_t chatContentPoolFields[] = {
	{ "cached", offsetof( chatContentPool_t, cached ), MAX_CLIENTS, stateType_t::UInt32 },
	{ "owner", offsetof( chatContentPool_t, owner ), MAX_CLIENTS + 1, stateType_t::Int32 }
};
static constexpr stateSchema_t chatContentPoolSchema = { "botlib.chatContentPool", 1, 1, sizeof( chatContentPool_t ), chatContentPoolFields, 2 };
static bool ChatContentPool( chatContentPool_t *pool ) {
	*pool = {};
	if ( botchatstates[0] )
		return false;
	for ( int i = 0; i < MAX_CLIENTS; ++i )
		if ( ichatdata[i] ) {
			if ( !ichatdata[i]->chat )
				return false;
			for ( int j = 0; j < i; ++j )
				if ( ichatdata[j] && ichatdata[i]->chat == ichatdata[j]->chat )
					return false;
			pool->cached[i] = 1;
		}
	for ( int i = 0; i <= MAX_CLIENTS; ++i ) {
		const auto *actor = botchatstates[i];
		pool->owner[i] = !actor ? -3 : !actor->chat ? -1
													: -2;
		if ( !actor || !actor->chat )
			continue;
		for ( int j = 0; j < MAX_CLIENTS; ++j )
			if ( ichatdata[j] && actor->chat == ichatdata[j]->chat )
				pool->owner[i] = j;
		if ( pool->owner[i] == -2 )
			for ( int j = 1; j < i; ++j )
				if ( botchatstates[j] && actor->chat == botchatstates[j]->chat )
					return false;
	}
	return true;
}
static bool ChatHashString( Sha_256 *hash, const char *text ) {
	const size_t length = text ? strlen( text ) : 0;
	if ( length >= MAX_TOKEN )
		return false;
	const uint32_t size = text ? uint32_t( length ) : UINT32_MAX;
	sha_256_write( hash, &size, sizeof( size ) );
	if ( length )
		sha_256_write( hash, text, length );
	return true;
}
static bool ChatHashNode( Sha_256 *hash, uint32_t kind, uint32_t *nodes ) {
	if ( ++*nodes > 65536 )
		return false;
	sha_256_write( hash, &kind, sizeof( kind ) );
	return true;
}
static void ChatHashEnd( Sha_256 *hash ) {
	const uint32_t end = 0;
	sha_256_write( hash, &end, sizeof( end ) );
}
static bool ChatHashMatch( Sha_256 *hash, const bot_matchpiece_t *pieces, uint32_t *nodes ) {
	for ( auto *piece = pieces; piece; piece = piece->next ) {
		if ( !ChatHashNode( hash, 1, nodes ) )
			return false;
		const int32_t fields[] = { piece->type, piece->variable };
		sha_256_write( hash, fields, sizeof( fields ) );
		for ( auto *string = piece->firststring; string; string = string->next )
			if ( !ChatHashNode( hash, 2, nodes ) || !ChatHashString( hash, string->string ) )
				return false;
		ChatHashEnd( hash );
	}
	ChatHashEnd( hash );
	return true;
}
static bool ChatHashMessages( Sha_256 *hash, bot_chatmessage_t *messages, int expected, uint32_t *count, uint32_t *nodes ) {
	int actual = 0;
	for ( auto *message = messages; message; message = message->next ) {
		if ( !ChatHashNode( hash, 3, nodes ) || !ChatHashString( hash, message->chatmessage ) || !message->chatmessage ||
			 !std::isfinite( message->time ) || *count >= MAX_SAVED_CHAT_LINES )
			return false;
		savedChatLines[*count] = message;
		savedChatTimes[( *count )++] = message->time;
		++actual;
	}
	ChatHashEnd( hash );
	return actual == expected;
}
static bool ChatContentIdentity( bot_chat_t *chat, bool global, chatContentIdentity_t *identity ) {
	Sha_256 hash;
	sha_256_init( &hash, identity->hash );
	uint32_t nodes = 0;
	identity->count = 0;
	for ( auto *type = chat ? chat->types : nullptr; type; type = type->next ) {
		if ( !memchr( type->name, 0, sizeof( type->name ) ) || !ChatHashNode( &hash, 4, &nodes ) || !ChatHashString( &hash, type->name ) ||
			 !ChatHashMessages( &hash, type->firstchatmessage, type->numchatmessages, &identity->count, &nodes ) )
			return false;
	}
	ChatHashEnd( &hash );
	if ( global ) {
		for ( auto *list = synonyms; list; list = list->next ) {
			if ( !ChatHashNode( &hash, 5, &nodes ) || !std::isfinite( list->totalweight ) )
				return false;
			sha_256_write( &hash, &list->context, sizeof( list->context ) );
			sha_256_write( &hash, &list->totalweight, sizeof( list->totalweight ) );
			for ( auto *entry = list->firstsynonym; entry; entry = entry->next ) {
				if ( !ChatHashNode( &hash, 6, &nodes ) || !ChatHashString( &hash, entry->string ) || !std::isfinite( entry->weight ) )
					return false;
				sha_256_write( &hash, &entry->weight, sizeof( entry->weight ) );
			}
			ChatHashEnd( &hash );
		}
		ChatHashEnd( &hash );
		for ( auto *list = randomstrings; list; list = list->next ) {
			if ( !ChatHashNode( &hash, 7, &nodes ) || !ChatHashString( &hash, list->string ) )
				return false;
			int actual = 0;
			for ( auto *entry = list->firstrandomstring; entry; entry = entry->next ) {
				if ( !ChatHashNode( &hash, 8, &nodes ) || !ChatHashString( &hash, entry->string ) )
					return false;
				++actual;
			}
			if ( actual != list->numstrings )
				return false;
			ChatHashEnd( &hash );
		}
		ChatHashEnd( &hash );
		for ( auto *entry = matchtemplates; entry; entry = entry->next ) {
			if ( !ChatHashNode( &hash, 9, &nodes ) )
				return false;
			sha_256_write( &hash, &entry->context, sizeof( entry->context ) );
			const int32_t fields[] = { entry->type, entry->subtype };
			sha_256_write( &hash, fields, sizeof( fields ) );
			if ( !ChatHashMatch( &hash, entry->first, &nodes ) )
				return false;
		}
		ChatHashEnd( &hash );
		for ( auto *reply = replychats; reply; reply = reply->next ) {
			if ( !ChatHashNode( &hash, 10, &nodes ) || !std::isfinite( reply->priority ) )
				return false;
			sha_256_write( &hash, &reply->priority, sizeof( reply->priority ) );
			for ( auto *key = reply->keys; key; key = key->next ) {
				if ( !ChatHashNode( &hash, 11, &nodes ) )
					return false;
				sha_256_write( &hash, &key->flags, sizeof( key->flags ) );
				if ( !ChatHashString( &hash, key->string ) || !ChatHashMatch( &hash, key->match, &nodes ) )
					return false;
			}
			ChatHashEnd( &hash );
			if ( !ChatHashMessages( &hash, reply->firstchatmessage, reply->numchatmessages, &identity->count, &nodes ) )
				return false;
		}
		ChatHashEnd( &hash );
	}
	// Reject aliases as well as bounded cycles; each mutable timer has one owner.
	static bot_chatmessage_t *sorted[MAX_SAVED_CHAT_LINES];
	std::copy_n( savedChatLines, identity->count, sorted );
	std::sort( sorted, sorted + identity->count, std::less<bot_chatmessage_t *>{} );
	if ( std::adjacent_find( sorted, sorted + identity->count ) != sorted + identity->count )
		return false;
	sha_256_close( &hash );
	return true;
}
static bool ChatContentRecord( stateWriter_t *writer, const stateReader_t *reader, uint32_t slot, bot_chat_t *chat, const bot_ichatdata_t *cache, bool global, bool apply ) {
	chatContentIdentity_t loaded{};
	if ( cache || chat ) {
		const char *filename = cache ? cache->filename : chat->filename;
		const char *chatname = cache ? cache->chatname : chat->chatname;
		if ( !memchr( filename, 0, MAX_QPATH ) || !memchr( chatname, 0, MAX_QPATH ) )
			return false;
		strcpy( loaded.filename, filename );
		strcpy( loaded.chatname, chatname );
	}
	if ( !ChatContentIdentity( chat, global, &loaded ) )
		return false;
	uint32_t version;
	if ( writer ) {
		if ( !State_Append( writer, chatContentSchema, slot, &loaded ) )
			return false;
	} else {
		chatContentIdentity_t saved;
		if ( !State_Find( *reader, chatContentSchema, slot, &saved, &version ) || saved.count != loaded.count ||
			 strcmp( saved.filename, loaded.filename ) || strcmp( saved.chatname, loaded.chatname ) || memcmp( saved.hash, loaded.hash, sizeof( saved.hash ) ) )
			return false;
	}
	if ( !loaded.count )
		return true;
	const stateField_t field = { "time", 0, loaded.count, stateType_t::Float32 };
	const stateSchema_t schema = { "botlib.chatTimes", 1, 1, sizeof( savedChatTimes ), &field, 1 };
	if ( writer )
		return State_Append( writer, schema, slot, savedChatTimes );
	if ( !State_Find( *reader, schema, slot, savedChatTimes, &version ) )
		return false;
	for ( uint32_t i = 0; i < loaded.count; ++i )
		if ( !std::isfinite( savedChatTimes[i] ) )
			return false;
	if ( apply )
		for ( uint32_t i = 0; i < loaded.count; ++i )
			savedChatLines[i]->time = savedChatTimes[i];
	return true;
}
static bool ChatContentRecords( stateWriter_t *writer, const stateReader_t *reader, const chatContentPool_t &pool, bool apply ) {
	for ( uint32_t i = 0; i < MAX_CLIENTS; ++i )
		if ( pool.cached[i] && !ChatContentRecord( writer, reader, i, ichatdata[i]->chat, ichatdata[i], false, apply ) )
			return false;
	for ( uint32_t i = 1; i <= MAX_CLIENTS; ++i )
		if ( pool.owner[i] == -2 && !ChatContentRecord( writer, reader, MAX_CLIENTS + i, botchatstates[i]->chat, nullptr, false, apply ) )
			return false;
	return ChatContentRecord( writer, reader, 2 * MAX_CLIENTS + 1, nullptr, nullptr, true, apply );
}
bool Bot_WriteChatContentState( stateWriter_t *writer ) {
	if ( !writer )
		return false;
	chatContentPool_t pool;
	if ( !ChatContentPool( &pool ) || !State_Append( writer, chatContentPoolSchema, 0, &pool ) || !ChatContentRecords( writer, nullptr, pool, false ) ) {
		writer->failed = true;
		return false;
	}
	return true;
}
bool Bot_ReadChatContentState( const stateReader_t &reader, bool apply ) {
	chatContentPool_t saved, loaded;
	uint32_t version;
	if ( !ChatContentPool( &loaded ) || !State_Find( reader, chatContentPoolSchema, 0, &saved, &version ) || memcmp( &saved, &loaded, sizeof( saved ) ) ||
		 !ChatContentRecords( nullptr, &reader, saved, false ) )
		return false;
	return !apply || ChatContentRecords( nullptr, &reader, saved, true );
}

static bot_chat_t *CreateChatContent( const stateReader_t &reader, uint32_t slot ) {
	chatContentIdentity_t saved;
	uint32_t version;
	if ( !State_Find( reader, chatContentSchema, slot, &saved, &version ) || !saved.filename[0] || !saved.chatname[0] || saved.count > MAX_SAVED_CHAT_LINES )
		return nullptr;
	auto *chat = BotLoadInitialChat( saved.filename, saved.chatname );
	if ( chat && !ChatContentRecord( nullptr, &reader, slot, chat, nullptr, false, true ) ) {
		FreeMemory( chat );
		return nullptr;
	}
	return chat;
}
bool Bot_PrepareChatContentState( const stateReader_t &reader ) {
	for ( const auto *cache : ichatdata )
		if ( cache )
			return false;
	for ( const auto *state : botchatstates )
		if ( state && state->chat )
			return false;
	chatContentPool_t pool;
	uint32_t version;
	if ( !State_Find( reader, chatContentPoolSchema, 0, &pool, &version ) || pool.owner[0] != -3 ||
		 !ChatContentRecord( nullptr, &reader, 2 * MAX_CLIENTS + 1, nullptr, nullptr, true, false ) )
		return false;
	for ( uint32_t cached : pool.cached )
		if ( cached > 1 )
			return false;
	for ( int i = 0; i <= MAX_CLIENTS; ++i )
		if ( pool.owner[i] < -3 || pool.owner[i] >= MAX_CLIENTS ||
			 ( pool.owner[i] == -3 ) != ( botchatstates[i] == nullptr ) ||
			 ( pool.owner[i] >= 0 && !pool.cached[pool.owner[i]] ) )
			return false;
	bot_ichatdata_t *caches[MAX_CLIENTS] = {};
	bot_chat_t *privateChats[MAX_CLIENTS + 1] = {};
	bool valid = true;
	for ( uint32_t i = 0; i < MAX_CLIENTS && valid; ++i )
		if ( pool.cached[i] ) {
			auto *chat = CreateChatContent( reader, i );
			valid = chat != nullptr;
			if ( valid ) {
				caches[i] = (bot_ichatdata_t *)GetClearedMemory( sizeof( bot_ichatdata_t ) );
				caches[i]->chat = chat;
				strcpy( caches[i]->filename, chat->filename );
				strcpy( caches[i]->chatname, chat->chatname );
			}
		}
	for ( uint32_t i = 1; i <= MAX_CLIENTS && valid; ++i )
		if ( pool.owner[i] == -2 ) {
			privateChats[i] = CreateChatContent( reader, MAX_CLIENTS + i );
			valid = privateChats[i] != nullptr;
		}
	if ( !valid ) {
		for ( auto *cache : caches )
			if ( cache ) {
				FreeMemory( cache->chat );
				FreeMemory( cache );
			}
		for ( auto *chat : privateChats )
			if ( chat )
				FreeMemory( chat );
		return false;
	}
	memcpy( ichatdata, caches, sizeof( caches ) );
	for ( int i = 1; i <= MAX_CLIENTS; ++i )
		if ( botchatstates[i] )
			botchatstates[i]->chat = pool.owner[i] >= 0 ? caches[pool.owner[i]]->chat : privateChats[i];
	return ChatContentRecord( nullptr, &reader, 2 * MAX_CLIENTS + 1, nullptr, nullptr, true, true );
}

bool Bot_HasChatState( int handle ) {
	return handle > 0 && handle <= MAX_CLIENTS && botchatstates[handle] != nullptr;
}
