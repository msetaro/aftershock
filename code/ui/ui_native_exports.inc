/* Typed exports; included after the module namespace. */

int NativeUI_CallDepth;
bool NativeUI_Running;

void NativeUI_Init( int connected ) {
	++NativeUI_CallDepth;
	ui::UI_Init();
	--NativeUI_CallDepth;
}

void NativeUI_Shutdown( void ) {
	++NativeUI_CallDepth;
	ui::UI_Shutdown();
	--NativeUI_CallDepth;
}

void NativeUI_KeyEvent( int key, int down ) {
	++NativeUI_CallDepth;
	ui::UI_KeyEvent( key, down );
	--NativeUI_CallDepth;
}

void NativeUI_MouseEvent( int dx, int dy ) {
	++NativeUI_CallDepth;
	ui::UI_MouseEvent( dx, dy );
	--NativeUI_CallDepth;
}

void NativeUI_Refresh( int time ) {
	++NativeUI_CallDepth;
	ui::UI_Refresh( time );
	--NativeUI_CallDepth;
}

int NativeUI_IsFullscreen( void ) {
	++NativeUI_CallDepth;
	int result = ui::UI_IsFullscreen();
	--NativeUI_CallDepth;
	return result;
}

void NativeUI_SetActiveMenu( int menu ) {
	++NativeUI_CallDepth;
	ui::UI_SetActiveMenu( (ui::uiMenuCommand_t)menu );
	--NativeUI_CallDepth;
}

int NativeUI_ConsoleCommand( int time ) {
	++NativeUI_CallDepth;
	int result = ui::UI_ConsoleCommand( time );
	--NativeUI_CallDepth;
	return result;
}

void NativeUI_DrawConnectScreen( int overlay ) {
	++NativeUI_CallDepth;
	ui::UI_DrawConnectScreen( (ui::qboolean)overlay );
	--NativeUI_CallDepth;
}
