#include "client.h"
#include "../qcommon/voice_public.h"
#include <cmath>

static cvar_t *voiceEnabled, *voiceSend, *voiceTarget, *voiceMeter;
static float voicePower;
static bool capturing, muted[MAX_CLIENTS];
static int16_t capture[2880];
static int captured;
static uint32_t sequence, lastSend;
static byte generation;
#ifdef AFTERSHOCK_DEVTOOLS
static uint32_t testFrames, testPhase;
static void TestVoice() {
	if ( !cl_connectedToCheatServer || cls.state != CA_ACTIVE || Cmd_Argc() != 2 )
		return;
	char *end;
	const char *text = Cmd_Argv( 1 );
	const int64_t count = strtoll( text, &end, 10 );
	if ( end == text || *end || count < 1 || count > 500 )
		return;
	testFrames = uint32_t( count );
	testPhase = 0;
	++generation;
	sequence = 0;
}
#endif

float CL_VoiceLevel() {
	return capturing && voiceMeter && voiceMeter->integer ? voicePower : -1.0f;
}

static void StartVoice() {
	Cvar_Set( "cl_voipSend", "1" );
}
static void StopVoice() {
	Cvar_Set( "cl_voipSend", "0" );
}
static void MuteVoice() {
	if ( Cmd_Argc() != 3 ) {
		Com_Printf( "Usage: voip_mute <client 0..63> <0|1>\n" );
		return;
	}
	char *end;
	const char *text = Cmd_Argv( 1 );
	const int64_t sender = strtoll( text, &end, 10 );
	const char *value = Cmd_Argv( 2 );
	if ( end == text || *end || sender < 0 || sender >= MAX_CLIENTS || ( strcmp( value, "0" ) && strcmp( value, "1" ) ) )
		return;
	muted[sender] = value[0] == '1';
}

void CL_VoiceInit() {
	voiceEnabled = Cvar_Get( "cl_voip", "1", CVAR_ARCHIVE | CVAR_USERINFO );
	voiceMeter = Cvar_Get( "cl_voipShowMeter", "1", CVAR_ARCHIVE );
	voiceSend = Cvar_Get( "cl_voipSend", "0", CVAR_TEMP );
	voiceTarget = Cvar_Get( "cl_voipTarget", "-1", CVAR_TEMP );
	Cvar_CheckRange( voiceEnabled, "0", "1", CV_INTEGER );
	Cvar_CheckRange( voiceSend, "0", "1", CV_INTEGER );
	Cvar_CheckRange( voiceTarget, "-1", "63", CV_INTEGER );
	Cvar_SetDescription( voiceSend, "Explicit microphone capture/transmit; use +voiprecord/-voiprecord." );
	Cvar_SetDescription( voiceTarget, "Direct voice recipient (-1 sends to all voice-enabled clients)." );
	Cmd_AddCommand( "+voiprecord", StartVoice );
	Cmd_AddCommand( "-voiprecord", StopVoice );
	Cmd_AddCommand( "voip_mute", MuteVoice );
#ifdef AFTERSHOCK_DEVTOOLS
	Cmd_AddCommand( "voip_test", TestVoice );
#endif
}

void CL_VoiceReset() {
	SNDDMA_StopVoiceCapture();
	capturing = false;
	captured = 0;
	voicePower = 0;
	sequence = lastSend = 0;
	++generation;
	memset( muted, 0, sizeof( muted ) );
#ifdef AFTERSHOCK_DEVTOOLS
	testFrames = testPhase = 0;
#endif
	if ( voiceSend )
		Cvar_Set( "cl_voipSend", "0" );
	S_VoiceReset();
}

void CL_VoiceShutdown() {
	CL_VoiceReset();
	Cmd_RemoveCommand( "+voiprecord" );
	Cmd_RemoveCommand( "-voiprecord" );
	Cmd_RemoveCommand( "voip_mute" );
#ifdef AFTERSHOCK_DEVTOOLS
	Cmd_RemoveCommand( "voip_test" );
#endif
	voiceEnabled = voiceSend = voiceTarget = voiceMeter = nullptr;
}

void CL_WriteVoice( msg_t *msg ) {
	const char *info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SYSTEMINFO];
	const bool enabled = voiceEnabled && voiceEnabled->integer && cls.state == CA_ACTIVE && !clc.demoplaying &&
						 !strcmp( Info_ValueForKey( info, "sv_voip" ), "1" );
	const bool wantCapture = enabled && voiceSend->integer;
	if ( wantCapture != capturing ) {
		if ( wantCapture ) {
			capturing = SNDDMA_StartVoiceCapture();
			if ( !capturing )
				Cvar_Set( "cl_voipSend", "0" );
			++generation;
			sequence = 0;
		} else {
			SNDDMA_StopVoiceCapture();
			capturing = false;
		}
		captured = 0;
	}
	if ( !enabled )
		return;
	const uint32_t now = uint32_t( cls.realtime );
	if ( uint32_t( now - lastSend ) < 20 )
		return;
	voicePacket_t packet = {};
#ifdef AFTERSHOCK_DEVTOOLS
	if ( testFrames ) {
		for ( int i = 0; i < 960; ++i )
			capture[i] = int16_t( 8000 * std::sin( testPhase++ * ( 6.283185307 * 440 / 48000 ) ) );
		captured = 960;
		--testFrames;
	} else
#endif
		if ( capturing )
		captured += SNDDMA_ReadVoiceCapture( capture + captured, 2880 - captured );
	packet.frames = captured / 960;
	if ( !packet.frames )
		return;
	double power = 0;
	for ( int i = 0; i < packet.frames * 960; ++i )
		power += double( capture[i] ) * capture[i];
	voicePower = float( std::sqrt( power / ( packet.frames * 960 ) ) / 32768.0 );
	packet.size = S_VoiceEncode( capture, packet.data, 256, packet.frames );
	captured -= packet.frames * 960;
	memmove( capture, capture + packet.frames * 960, size_t( captured ) * sizeof( capture[0] ) );
	packet.generation = generation;
	packet.sequence = sequence;
	packet.flags = 2;
	if ( voiceTarget->integer < 0 )
		memset( packet.targets, 255, sizeof( packet.targets ) );
	else
		packet.targets[voiceTarget->integer / 8] = byte( 1u << ( voiceTarget->integer % 8 ) );
	if ( packet.size > 0 )
		MSG_WriteVoice( msg, packet, true );
	sequence += uint32_t( packet.frames );
	lastSend = now;
}

void CL_ParseVoice( msg_t *msg, bool ignore ) {
	voicePacket_t packet;
	if ( !MSG_ReadVoice( msg, &packet, false ) ) {
		Com_Error( ERR_DROP, "Invalid voice packet" );
		return;
	}
	if ( !ignore && voiceEnabled && voiceEnabled->integer && !muted[packet.sender] && ( packet.flags & 2 ) )
		S_VoiceReceive( packet.sender, packet.generation, packet.sequence, packet.frames, packet.data, packet.size );
}
