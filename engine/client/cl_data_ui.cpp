#include "client.h"
#include "../ui/ui_public.h"
#include "../public/cg_native_public.h"
#include "../public/ui_native_public.h"
#include <algorithm>
#include <cmath>

static uiDocument_t document;
static uiDocumentState_t state;
static cvar_t *documentPath, *language, *safeArea;
static qhandle_t atlas;
static bool loaded, rebinding;
static uint32_t draws, hudDraws;

static uint32_t Locale() {
	for ( uint32_t i = 0; i < document.header.localeCount; ++i )
		if ( !Q_stricmp( language->string, document.locales[i].name ) )
			return i;
	return 0;
}
static void Reload() {
	if ( !documentPath || !documentPath->string[0] ) {
		loaded = false;
		return;
	}
	fileHandle_t file;
	const int length = FS_FOpenFileRead( documentPath->string, &file, qtrue );
	static uint8_t bytes[65536];
	static uiDocument_t replacement;
	bool valid = file != FS_INVALID_HANDLE && length > 0 && length <= int( sizeof( bytes ) );
	if ( file != FS_INVALID_HANDLE ) {
		valid = valid && FS_Read( bytes, length, file ) == length;
		FS_FCloseFile( file );
	}
	valid = valid && UI_ReadDocument( bytes, size_t( length ), &replacement );
	const qhandle_t replacementAtlas = valid ? re.RegisterShaderNoMip( replacement.header.atlas ) : 0;
	if ( !valid || !replacementAtlas ) {
		Com_Printf( "UI document rejected: %s\n", documentPath->string );
		return;
	}
	char page[32] = "main";
	if ( loaded )
		Q_strncpyz( page, document.pages[state.page].name, sizeof( page ) );
	document = replacement;
	atlas = replacementAtlas;
	loaded = true;
	rebinding = false;
	if ( !UI_OpenDocument( document, &state, page ) )
		UI_OpenDocument( document, &state, "main" );
	Com_Printf( "UI document loaded: %s pages=%u items=%u locales=%u\n", documentPath->string,
		document.header.pageCount, document.header.itemCount, document.header.localeCount );
}
static void Info() {
	Com_Printf( "UI document: loaded=%d page=%s focus=%s locale=%s binding=%d size=%dx%d draws=%u hud=%u\n",
		loaded ? 1 : 0, loaded ? document.pages[state.page].name : "none",
		loaded && state.focus != UI_NO_FOCUS ? document.items[state.focus].name : "none",
		loaded ? document.locales[Locale()].name : "none", rebinding ? 1 : 0,
		cls.glconfig.vidWidth, cls.glconfig.vidHeight, draws, hudDraws );
}
void CL_DataUIInit() {
	documentPath = Cvar_Get( "ui_document", "", CVAR_ARCHIVE );
	language = Cvar_Get( "ui_language", "en", CVAR_ARCHIVE );
	safeArea = Cvar_Get( "ui_safeArea", "0.05", CVAR_ARCHIVE );
	Cvar_CheckRange( safeArea, "0", "0.2", CV_FLOAT );
	Cvar_SetDescription( documentPath, "Cooked menu and HUD document; empty uses the classic UI. Apply edited data with ui_reload." );
	Cvar_SetDescription( safeArea, "Viewport margin reserved for authored UI on each edge." );
	Cmd_AddCommand( "ui_reload", Reload );
	Cmd_AddCommand( "ui_info", Info );
	draws = hudDraws = 0;
	Reload();
}
void CL_DataUIShutdown() {
	loaded = rebinding = false;
	atlas = 0;
	Cmd_RemoveCommand( "ui_reload" );
	Cmd_RemoveCommand( "ui_info" );
}
bool CL_DataUIFullscreen() {
	return loaded && ( Key_GetCatcher() & KEYCATCH_UI ) && ( cls.state == CA_DISCONNECTED || cls.state == CA_ACTIVE );
}
static void Quad( float x, float y, float width, float height, const float color[4] ) {
	re.SetColor( color );
	re.DrawStretchPic( x, y, width, height, 0, 0, 1, 1, cls.whiteShader );
}
static void Run( const uiTextRun_t &run, float x, float y, float scale ) {
	re.DrawStretchPic( x, y, float( run.width ) * .5f * scale, float( run.height ) * .5f * scale,
		float( run.x ) / float( document.header.atlasWidth ), float( run.y ) / float( document.header.atlasHeight ),
		float( run.x + run.width ) / float( document.header.atlasWidth ), float( run.y + run.height ) / float( document.header.atlasHeight ), atlas );
}
static void Label( uint32_t text, const uiRectangle_t &rectangle, bool rtl ) {
	const auto &run = document.runs[Locale() * document.header.textCount + text];
	const float scale = std::min( { rectangle.scale, rectangle.width * 2 / float( run.width ), rectangle.height * 2 / float( run.height ) } );
	const float width = float( run.width ) * .5f * scale, height = float( run.height ) * .5f * scale;
	Run( run, rectangle.x + ( rtl ? rectangle.width - width : 0 ), rectangle.y + ( rectangle.height - height ) * .5f, scale );
}
static void Characters( const char *text, const uiRectangle_t &rectangle ) {
	const auto *glyphs = document.runs + document.header.textCount * document.header.localeCount;
	float advance = 0;
	for ( const auto *p = (const unsigned char *)text; *p; ++p )
		advance += glyphs[( *p >= 32 && *p < 127 ? *p : '?' ) - 32].advance;
	const float scale = std::min( { rectangle.scale, rectangle.height / 40, rectangle.width / std::max( 1.0f, advance ) } );
	float x = rectangle.x + rectangle.width - advance * scale;
	const float baseline = rectangle.y + ( rectangle.height + 24 * scale ) * .5f;
	for ( const auto *p = (const unsigned char *)text; *p; ++p ) {
		const auto &run = glyphs[( *p >= 32 && *p < 127 ? *p : '?' ) - 32];
		Run( run, x + run.left * scale, baseline + run.top * scale, scale );
		x += run.advance * scale;
	}
}
static int BindingKey( const char *command ) {
	for ( int key = 0; key < MAX_KEYS; ++key ) {
		const char *binding = Key_GetBinding( key );
		if ( binding && !Q_stricmp( command, binding ) )
			return key;
	}
	return -1;
}
static void DrawPage( uint32_t pageIndex, const int values[5] ) {
	const auto &page = document.pages[pageIndex];
	const bool rtl = document.locales[Locale()].rtl != 0;
	for ( uint32_t i = page.first; i < page.first + page.count; ++i ) {
		const auto &item = document.items[i];
		uiRectangle_t rectangle;
		if ( !UI_Layout( document, item, cls.glconfig.vidWidth, cls.glconfig.vidHeight, safeArea->value, &rectangle ) )
			continue;
		if ( item.kind == UI_BUTTON || item.kind == UI_SLIDER || item.kind == UI_BINDING ) {
			const float focused[4] = { .16f, .28f, .38f, 1 }, normal[4] = { .055f, .075f, .095f, 1 };
			Quad( rectangle.x, rectangle.y, rectangle.width, rectangle.height, i == state.focus ? focused : normal );
			rectangle.x += 16 * rectangle.scale;
			rectangle.width = std::max( 1.0f, rectangle.width - 32 * rectangle.scale );
		}
		re.SetColor( item.color );
		if ( item.kind == UI_VALUE ) {
			if ( !values )
				continue;
			static const char *names[5] = { "health", "armor", "ammo", "ping", "fps" };
			for ( int field = 0; field < 5; ++field ) {
				if ( strcmp( item.target, names[field] ) )
					continue;
				char number[32];
				Com_sprintf( number, sizeof( number ), "%d", values[field] );
				Characters( number, rectangle );
			}
		} else if ( item.kind == UI_SLIDER || item.kind == UI_BINDING ) {
			auto label = rectangle;
			label.width *= .58f;
			if ( rtl )
				label.x += rectangle.width - label.width;
			Label( item.text, label, rtl );
			auto value = rectangle;
			value.width *= .38f;
			if ( !rtl )
				value.x += rectangle.width - value.width;
			char text[64];
			if ( item.kind == UI_SLIDER )
				Com_sprintf( text, sizeof( text ), "%.2f", double( Cvar_VariableValue( item.target ) ) );
			else {
				const int key = BindingKey( item.target );
				const char *name = rebinding && i == state.focus ? "..." : key < 0 ? "-"
																				   : Key_KeynumToString( key );
				if ( !strncmp( name, "PAD0_", 5 ) )
					name += 5;
				Q_strncpyz( text, name, sizeof( text ) );
			}
			Characters( text, value );
		} else
			Label( item.text, rectangle, rtl );
	}
	re.SetColor( nullptr );
	++draws;
}
bool CL_DataUIDrawMenu() {
	if ( !CL_DataUIFullscreen() )
		return false;
	const float background[4] = { .018f, .026f, .038f, 1 };
	Quad( 0, 0, float( cls.glconfig.vidWidth ), float( cls.glconfig.vidHeight ), background );
	DrawPage( state.page, nullptr );
	return true;
}
int CGameImport_DrawAuthoredHUD( int health, int armor, int ammo, int ping ) {
	if ( !loaded )
		return 0;
	const int values[5] = { health, armor, ammo, ping, cls.frametime > 0 ? 1000 / cls.frametime : 0 };
	DrawPage( uint32_t( UI_FindPage( document, "hud" ) ), values );
	++hudDraws;
	return 1;
}
static void Back() {
	if ( rebinding ) {
		rebinding = false;
		return;
	}
	if ( strcmp( document.pages[state.page].name, "main" ) )
		UI_OpenDocument( document, &state, "main" );
	else if ( cls.state == CA_ACTIVE )
		NativeUI_SetActiveMenu( UIMENU_NONE );
}
bool CL_DataUIKey( int key ) {
	if ( !loaded || ( Key_GetCatcher() & ( KEYCATCH_CONSOLE | KEYCATCH_CGAME | KEYCATCH_MESSAGE ) ) )
		return false;
	if ( !CL_DataUIFullscreen() ) {
		if ( cls.state != CA_ACTIVE || ( key != K_ESCAPE && key != K_PAD0_START ) )
			return false;
		NativeUI_SetActiveMenu( UIMENU_INGAME );
		UI_OpenDocument( document, &state, "options" );
		return true;
	}
	if ( key == K_ESCAPE || key == K_PAD0_B ) {
		Back();
		return true;
	}
	if ( rebinding && state.focus != UI_NO_FOCUS ) {
		const char *command = document.items[state.focus].target;
		for ( int previous = 0; previous < MAX_KEYS; ++previous ) {
			const char *binding = Key_GetBinding( previous );
			if ( binding && !Q_stricmp( binding, command ) )
				Key_SetBinding( previous, "" );
		}
		Key_SetBinding( key, command );
		rebinding = false;
		return true;
	}
	const int direction = key == K_UPARROW || key == K_PAD0_DPAD_UP || key == K_PAD0_LEFTSTICK_UP ? -1 : key == K_DOWNARROW || key == K_PAD0_DPAD_DOWN || key == K_PAD0_LEFTSTICK_DOWN ? 1
																																													   : 0;
	if ( direction ) {
		UI_Navigate( document, &state, direction );
		return true;
	}
	if ( state.focus == UI_NO_FOCUS )
		return true;
	const auto &item = document.items[state.focus];
	const int adjust = key == K_LEFTARROW || key == K_PAD0_DPAD_LEFT || key == K_PAD0_LEFTSTICK_LEFT ? -1 : key == K_RIGHTARROW || key == K_PAD0_DPAD_RIGHT || key == K_PAD0_LEFTSTICK_RIGHT ? 1
																																															 : 0;
	if ( item.kind == UI_SLIDER && adjust )
		Cvar_SetValue( item.target, UI_Nudge( item, Cvar_VariableValue( item.target ), adjust ) );
	if ( key != K_ENTER && key != K_PAD0_A )
		return true;
	if ( item.kind == UI_BINDING ) {
		rebinding = true;
		return true;
	}
	if ( item.kind != UI_BUTTON )
		return true;
	switch ( item.action ) {
	case UI_ACTION_PAGE:
		UI_OpenDocument( document, &state, item.target );
		break;
	case UI_ACTION_RESUME:
		NativeUI_SetActiveMenu( UIMENU_NONE );
		break;
	case UI_ACTION_MAP: {
		char command[96];
		Com_sprintf( command, sizeof( command ), "map %s\n", item.target );
		NativeUI_SetActiveMenu( UIMENU_NONE );
		Cbuf_AddText( command );
		break;
	}
	case UI_ACTION_QUIT:
		Cbuf_AddText( "quit\n" );
		break;
	default:
		break;
	}
	return true;
}
