#ifndef VOICE_PUBLIC_H
#define VOICE_PUBLIC_H

#include "q_shared.h"
#include "qcommon_public.h"

// Serialized field by field; this is an in-memory queue record, not a wire struct.
struct voicePacket_t {
	uint32_t sequence;
	int sender, frames, size, flags;
	byte generation, targets[( MAX_CLIENTS + 7 ) / 8], data[4000];
};
bool MSG_ReadVoice( msg_t *msg, voicePacket_t *packet, bool fromClient );
bool MSG_WriteVoice( msg_t *msg, const voicePacket_t &packet, bool fromClient );

#endif
