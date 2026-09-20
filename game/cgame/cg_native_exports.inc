/* Typed exports; included after the module namespace. */

int NativeCGame_CallDepth;
bool NativeCGame_Running;

void NativeCGame_Init( int message, int command, int client ) {
	++NativeCGame_CallDepth;
	cgame::CG_Init( message, command, client );
	--NativeCGame_CallDepth;
}

void NativeCGame_Shutdown( void ) {
	++NativeCGame_CallDepth;
	cgame::CG_Shutdown();
	--NativeCGame_CallDepth;
}

int NativeCGame_ConsoleCommand( void ) {
	++NativeCGame_CallDepth;
	int result = cgame::CG_ConsoleCommand();
	--NativeCGame_CallDepth;
	return result;
}

void NativeCGame_DrawActiveFrame( int time, int stereo, int demo ) {
	++NativeCGame_CallDepth;
	cgame::CG_DrawActiveFrame( time, (cgame::stereoFrame_t)stereo, (cgame::qboolean)demo );
	--NativeCGame_CallDepth;
}

int NativeCGame_CrosshairPlayer( void ) {
	++NativeCGame_CallDepth;
	int result = cgame::CG_CrosshairPlayer();
	--NativeCGame_CallDepth;
	return result;
}

int NativeCGame_LastAttacker( void ) {
	++NativeCGame_CallDepth;
	int result = cgame::CG_LastAttacker();
	--NativeCGame_CallDepth;
	return result;
}

void NativeCGame_KeyEvent( int key, int down ) {
	++NativeCGame_CallDepth;
	cgame::CG_KeyEvent( key, (cgame::qboolean)down );
	--NativeCGame_CallDepth;
}

void NativeCGame_MouseEvent( int dx, int dy ) {
	++NativeCGame_CallDepth;
	cgame::CG_MouseEvent( dx, dy );
	--NativeCGame_CallDepth;
}

void NativeCGame_EventHandling( int type ) {
	++NativeCGame_CallDepth;
	cgame::CG_EventHandling( type );
	--NativeCGame_CallDepth;
}
