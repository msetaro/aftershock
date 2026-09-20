#pragma once

#ifdef AFTERSHOCK_DEVTOOLS
#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_public.h"

void DevTools_Init( void );
void DevTools_Reset( void );
void DevTools_Draw( const refexport_t *renderer, int width, int height, int milliseconds );
bool DevTools_Key( int key, bool down );
bool DevTools_Char( uint32_t character );
bool DevTools_Mouse( int dx, int dy );
void DevTools_Log( const char *text );
const char *DevTools_Console( void );
#endif
