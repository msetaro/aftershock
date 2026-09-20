#include "devtools_public.h"
#include "../qcommon/qcommon_public.h"
#include "../qcommon/keys_public.h"
#include "../../third_party/imgui/imgui.h"

static cvar_t *enabled;
static ImGuiContext *context;
static uint32_t fontTexture, lastTime;
static float mouseX = 320, mouseY = 240;
static int screenWidth = 640, screenHeight = 480;
static uint32_t renderedFrames, allocations;
static devUiVertex_t vertices[65536];
static uint32_t indices[196608];
static devUiCommand_t commands[4096];

static void *Allocate( size_t size, void * ) {
	++allocations;
	return Z_TagMalloc( size, TAG_DEVTOOLS );
}

static void Free( void *memory, void * ) {
	if ( memory )
		Z_Free( memory );
}

static void Status( void ) {
	Com_Printf( "Developer tools: enabled=%i frames=%u arena=%" PRIz "u/16777216 allocations=%u\n",
		enabled->integer, renderedFrames, Z_DevMemoryUsed(), allocations );
}

void DevTools_Init( void ) {
	enabled = Cvar_Get( "dev_tools", "0", CVAR_TEMP );
	Cvar_SetDescription( enabled, "Development overlay; Escape closes it. Absent from shipping builds." );
	Cmd_AddCommand( "devtools_status", Status );
}

void DevTools_Reset( void ) {
	if ( context )
		ImGui::DestroyContext( context );
	context = nullptr;
	fontTexture = 0;
	lastTime = 0;
}

static bool Visible( void ) {
	return enabled && enabled->integer && context;
}

bool DevTools_Key( int key, bool down ) {
	if ( !Visible() )
		return false;
	if ( key == K_ESCAPE && down ) {
		Cvar_Set( "dev_tools", "0" );
		Key_ClearStates();
		return true;
	}
	ImGuiIO &io = ImGui::GetIO();
	if ( key >= K_MOUSE1 && key <= K_MOUSE5 ) {
		io.AddMouseButtonEvent( key - K_MOUSE1, down );
		return true;
	}
	if ( key == K_MWHEELUP || key == K_MWHEELDOWN ) {
		if ( down )
			io.AddMouseWheelEvent( 0, key == K_MWHEELUP ? 1.0f : -1.0f );
		return true;
	}
	ImGuiKey translated = ImGuiKey_None;
	if ( key >= 'a' && key <= 'z' )
		translated = (ImGuiKey)( ImGuiKey_A + key - 'a' );
	else if ( key >= '0' && key <= '9' )
		translated = (ImGuiKey)( ImGuiKey_0 + key - '0' );
	else {
		switch ( key ) {
		case K_TAB:
			translated = ImGuiKey_Tab;
			break;
		case K_ENTER:
			translated = ImGuiKey_Enter;
			break;
		case K_BACKSPACE:
			translated = ImGuiKey_Backspace;
			break;
		case K_DEL:
			translated = ImGuiKey_Delete;
			break;
		case K_INS:
			translated = ImGuiKey_Insert;
			break;
		case K_LEFTARROW:
			translated = ImGuiKey_LeftArrow;
			break;
		case K_RIGHTARROW:
			translated = ImGuiKey_RightArrow;
			break;
		case K_UPARROW:
			translated = ImGuiKey_UpArrow;
			break;
		case K_DOWNARROW:
			translated = ImGuiKey_DownArrow;
			break;
		case K_HOME:
			translated = ImGuiKey_Home;
			break;
		case K_END:
			translated = ImGuiKey_End;
			break;
		case K_PGUP:
			translated = ImGuiKey_PageUp;
			break;
		case K_PGDN:
			translated = ImGuiKey_PageDown;
			break;
		case K_SPACE:
			translated = ImGuiKey_Space;
			break;
		case K_CTRL:
			translated = ImGuiMod_Ctrl;
			break;
		case K_SHIFT:
			translated = ImGuiMod_Shift;
			break;
		case K_ALT:
			translated = ImGuiMod_Alt;
			break;
		default:
			break;
		}
	}
	if ( translated != ImGuiKey_None )
		io.AddKeyEvent( translated, down );
	return true;
}

bool DevTools_Char( uint32_t character ) {
	if ( !Visible() )
		return false;
	if ( character >= 32 && character <= 0x10ffff )
		ImGui::GetIO().AddInputCharacter( character );
	return true;
}

bool DevTools_Mouse( int dx, int dy ) {
	if ( !Visible() )
		return false;
	mouseX = Com_Clamp( 0, (float)screenWidth, mouseX + (float)dx );
	mouseY = Com_Clamp( 0, (float)screenHeight, mouseY + (float)dy );
	ImGui::GetIO().AddMousePosEvent( mouseX, mouseY );
	return true;
}

static bool CopyDrawData( const ImDrawData *data, devUiDraw_t *draw ) {
	*draw = { vertices, indices, commands, 0, 0, 0 };
	for ( int listIndex = 0; listIndex < data->CmdListsCount; ++listIndex ) {
		const ImDrawList *list = data->CmdLists[listIndex];
		const uint32_t base = draw->vertexCount;
		if ( (uint32_t)list->VtxBuffer.Size > ARRAY_LEN( vertices ) - base )
			return false;
		for ( int i = 0; i < list->VtxBuffer.Size; ++i ) {
			const ImDrawVert *v = &list->VtxBuffer[i];
			vertices[draw->vertexCount++] = { v->pos.x - data->DisplayPos.x, v->pos.y - data->DisplayPos.y, v->uv.x, v->uv.y, v->col };
		}
		for ( int c = 0; c < list->CmdBuffer.Size; ++c ) {
			const ImDrawCmd *command = &list->CmdBuffer[c];
			if ( command->UserCallback )
				continue; // State is fully set for each draw; no custom callbacks are installed.
			if ( draw->commandCount == ARRAY_LEN( commands ) || command->ElemCount > ARRAY_LEN( indices ) - draw->indexCount ||
				 command->IdxOffset > (uint32_t)list->IdxBuffer.Size || command->ElemCount > (uint32_t)list->IdxBuffer.Size - command->IdxOffset ||
				 command->GetTexID() > UINT32_MAX )
				return false;
			commands[draw->commandCount++] = { (uint32_t)command->GetTexID(), draw->indexCount, command->ElemCount,
				{ command->ClipRect.x - data->DisplayPos.x, command->ClipRect.y - data->DisplayPos.y,
					command->ClipRect.z - data->DisplayPos.x, command->ClipRect.w - data->DisplayPos.y } };
			for ( uint32_t i = 0; i < command->ElemCount; ++i ) {
				const uint32_t index = (uint32_t)list->IdxBuffer[command->IdxOffset + i] + command->VtxOffset;
				if ( index >= (uint32_t)list->VtxBuffer.Size )
					return false;
				indices[draw->indexCount++] = base + index;
			}
		}
	}
	return true;
}

void DevTools_Draw( const refexport_t *renderer, int width, int height, int milliseconds ) {
	if ( !enabled || !enabled->integer || width <= 0 || height <= 0 )
		return;
	screenWidth = width;
	screenHeight = height;
	if ( !context ) {
		Key_ClearStates();
		Z_InitDevMemory();
		ImGui::SetAllocatorFunctions( Allocate, Free );
		context = ImGui::CreateContext();
		ImGuiIO &io = ImGui::GetIO();
		io.IniFilename = nullptr;
		io.LogFilename = nullptr;
		io.MouseDrawCursor = true;
		io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
		io.Fonts->AddFontDefaultBitmap();
		unsigned char *pixels;
		int fontWidth, fontHeight;
		io.Fonts->GetTexDataAsRGBA32( &pixels, &fontWidth, &fontHeight );
		fontTexture = renderer->CreateDeveloperTexture( pixels, fontWidth, fontHeight );
		io.Fonts->SetTexID( (ImTextureID)fontTexture );
		io.AddMousePosEvent( mouseX, mouseY );
	}
	if ( !fontTexture )
		return;
	ImGuiIO &io = ImGui::GetIO();
	io.DisplaySize = ImVec2( (float)width, (float)height );
	const uint32_t elapsed = (uint32_t)milliseconds - lastTime;
	io.DeltaTime = lastTime ? Com_Clamp( 0.001f, 0.25f, (float)elapsed * 0.001f ) : 1.0f / 60.0f;
	lastTime = (uint32_t)milliseconds;
	ImGui::NewFrame();
	static char command[1024], filter[128], selected[MAX_STRING_CHARS], value[MAX_CVAR_VALUE_STRING];
	bool execute = false, apply = false;
	ImGui::SetNextWindowSize( ImVec2( (float)MIN( width - 20, 700 ), (float)MIN( height - 20, 540 ) ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowPos( ImVec2( 10, 10 ), ImGuiCond_FirstUseEver );
	if ( ImGui::Begin( "Aftershock developer tools" ) ) {
		ImGui::Text( "Escape closes | UI arena %zu / 16777216 bytes", Z_DevMemoryUsed() );
		if ( ImGui::BeginTabBar( "Tools" ) ) {
			if ( ImGui::BeginTabItem( "Console" ) ) {
				if ( ImGui::BeginChild( "Output", ImVec2( 0, -38 ), ImGuiChildFlags_Borders ) )
					ImGui::TextUnformatted( DevTools_Console() );
				ImGui::EndChild();
				execute = ImGui::InputText( "Command", command, sizeof( command ), ImGuiInputTextFlags_EnterReturnsTrue );
				ImGui::SameLine();
				execute |= ImGui::Button( "Run" );
				ImGui::EndTabItem();
			}
			if ( ImGui::BeginTabItem( "Cvars" ) ) {
				ImGui::InputText( "Search", filter, sizeof( filter ) );
				if ( ImGui::BeginChild( "Variables", ImVec2( 0, -110 ), ImGuiChildFlags_Borders ) ) {
					for ( const cvar_t *var = Cvar_First(); var; var = var->next ) {
						if ( !var->name || ( *filter && !Q_stristr( var->name, filter ) && ( !var->description || !Q_stristr( var->description, filter ) ) ) )
							continue;
						if ( ImGui::Selectable( var->name, !strcmp( selected, var->name ) ) ) {
							Q_strncpyz( selected, var->name, sizeof( selected ) );
							Q_strncpyz( value, var->string, sizeof( value ) );
						}
						if ( !strcmp( selected, var->name ) && var->description )
							ImGui::TextWrapped( "%s", var->description );
					}
				}
				ImGui::EndChild();
				ImGui::TextUnformatted( selected );
				apply = ImGui::InputText( "Value", value, sizeof( value ), ImGuiInputTextFlags_EnterReturnsTrue );
				ImGui::SameLine();
				apply |= ImGui::Button( "Apply" );
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
	ImGui::Render();
	devUiDraw_t draw;
	if ( CopyDrawData( ImGui::GetDrawData(), &draw ) ) {
		renderer->DrawDeveloperUI( &draw );
		++renderedFrames;
	} else {
		Com_Printf( "Developer UI draw capacity exceeded\n" );
	}
	// Engine mutation/error handling runs after all vendor UI calls return.
	if ( apply && *selected )
		Cvar_Set2( selected, value, qfalse );
	if ( execute && *command ) {
		Cbuf_AddText( command );
		Cbuf_AddText( "\n" );
		command[0] = '\0';
	}
}
