#include "common.h"

extern "C" int LLVMFuzzerTestOneInput( const uint8_t *data, size_t size )
{
	static byte storage[MAX_MSGLEN + 4];
	msg_t msg;
	usercmd_t command = { 0 }, decoded;
	entityState_t entity = { 0 }, entityDecoded;
	playerState_t player = { 0 }, playerDecoded;
	int operation;
	if ( size < 1 || size > MAX_MSGLEN ) return 0;
	operation = data[0] % 12;
	memset( storage, 0, sizeof( storage ) );
	memcpy( storage, data + 1, size - 1 );
	if ( setjmp( fuzz_error ) ) return 0;
	MSG_Init( &msg, storage, MAX_MSGLEN );
	msg.cursize = (int)size - 1;
	MSG_BeginReading( &msg );
	switch ( operation ) {
	case 0: MSG_ReadByte( &msg ); break;
	case 1: MSG_ReadShort( &msg ); break;
	case 2: MSG_ReadLong( &msg ); break;
	case 3: MSG_ReadFloat( &msg ); break;
	case 4: MSG_ReadString( &msg ); break;
	case 5: MSG_ReadStringLine( &msg ); break;
	case 6: MSG_ReadBigString( &msg ); break;
	case 7: MSG_ReadChar( &msg ); break;
	case 8: MSG_ReadDeltaUsercmdKey( &msg, 0, &command, &decoded ); break;
	case 9: MSG_ReadDeltaEntity( &msg, &entity, &entityDecoded, 1 ); break;
	case 10: MSG_ReadDeltaPlayerstate( &msg, &player, &playerDecoded ); break;
	case 11: MSG_ReadAngle16( &msg ); break;
	}
	return 0;
}
