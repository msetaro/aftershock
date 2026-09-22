#ifndef SND_VOICE_H
#define SND_VOICE_H

#include <stdint.h>

struct sVoiceStats_t {
	uint32_t encoded, decoded, rejected, concealed, overruns, queued;
};
bool S_VoiceInit();
void S_VoiceShutdown();
void S_VoiceMix( float ( *output )[2], uint32_t frames, int rate );
void S_VoiceStats( sVoiceStats_t *stats );
bool S_VoiceActive();

#endif
