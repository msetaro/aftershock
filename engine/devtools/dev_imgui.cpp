#include "devtools_public.h"
#include "../qcommon/qcommon_public.h"
#include "../qcommon/keys_public.h"
#include "../../third_party/imgui/imgui.h"
#include <inttypes.h>

static cvar_t *enabled;
static ImGuiContext *context;
static uint32_t fontTexture, lastTime;
static float mouseX = 320, mouseY = 240;
static int screenWidth = 640, screenHeight = 480;
static uint32_t renderedFrames, allocations, animationFrames;
static devUiVertex_t vertices[65536];
static uint32_t indices[196608];
static devUiCommand_t commands[4096];

static struct {
	char path[MAX_QPATH], skinPath[MAX_QPATH];
	int model, skin, frame;
	float phase, yaw, fps;
	bool play, load, draw;
	int x, y, width, height;
} animation;

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
	const devCpuTiming_t *timings;
	const devNetwork_t *net = DevTools_Network();
	Com_Printf( "Developer profile: cpu=%u snapshots=%" PRIu64 " bits=%u\n",
		DevTools_CpuTimings( &timings ), net->snapshots, net->snapshotBits );
	Com_Printf( "Developer animation: model=%d frame=%d previews=%u\n", animation.model, animation.frame, animationFrames );
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
	animation.model = animation.skin = 0;
	animation.phase = 0;
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

static void ImagePreview( const refexport_t *renderer, int index, float extent ) {
	devImage_t image;
	if ( !renderer->GetDeveloperImage( index, &image ) )
		return;
	ImGui::TextWrapped( "%s", image.name );
	ImGui::Text( "%dx%d -> %dx%d, flags 0x%x, format %u", image.width, image.height,
		image.uploadWidth, image.uploadHeight, image.flags, image.format );
	if ( image.uploadWidth > 0 && image.uploadHeight > 0 ) {
		const float scale = extent / (float)MAX( image.uploadWidth, image.uploadHeight );
		ImGui::Image( (ImTextureID)image.texture, ImVec2( (float)image.uploadWidth * scale, (float)image.uploadHeight * scale ) );
	}
}

static void InspectAssets( const refexport_t *renderer ) {
	if ( ImGui::BeginTabItem( "Textures" ) ) {
		static char filter[128];
		static int selected;
		ImGui::InputText( "Filter textures", filter, sizeof( filter ) );
		if ( ImGui::BeginChild( "Images", ImVec2( 0, 120 ), ImGuiChildFlags_Borders ) ) {
			devImage_t image;
			for ( int i = 0; renderer->GetDeveloperImage( i, &image ); ++i ) {
				if ( *filter && !Q_stristr( image.name, filter ) )
					continue;
				ImGui::PushID( i );
				if ( ImGui::Selectable( image.name, selected == i ) )
					selected = i;
				ImGui::PopID();
			}
		}
		ImGui::EndChild();
		ImagePreview( renderer, selected, 200 );
		ImGui::EndTabItem();
	}
	if ( ImGui::BeginTabItem( "Materials" ) ) {
		static char filter[128];
		static int selected;
		ImGui::InputText( "Filter materials", filter, sizeof( filter ) );
		if ( ImGui::BeginChild( "Shaders", ImVec2( 0, 120 ), ImGuiChildFlags_Borders ) ) {
			devMaterial_t material;
			for ( int i = 0; renderer->GetDeveloperMaterial( i, &material ); ++i ) {
				if ( *filter && !Q_stristr( material.name, filter ) )
					continue;
				ImGui::PushID( i );
				if ( ImGui::Selectable( material.name, selected == i ) )
					selected = i;
				ImGui::PopID();
			}
		}
		ImGui::EndChild();
		devMaterial_t material;
		if ( renderer->GetDeveloperMaterial( selected, &material ) ) {
			ImGui::TextWrapped( "%s", material.name );
			ImGui::Text( "sort %.2f cull %d surface 0x%x content 0x%x", material.sort,
				material.cull, (uint32_t)material.surfaceFlags, (uint32_t)material.contentFlags );
			ImGui::Text( "%s, %s", material.explicitDefinition ? "script defined" : "implicit",
				material.fallback ? "fallback shader" : "loaded" );
			for ( int stage = 0; stage < material.stages; ++stage ) {
				if ( !material.present[stage] ) {
					ImGui::Text( "Stage %d: inactive / missing image", stage );
					continue;
				}
				ImGui::Text( "Stage %d state 0x%x", stage, material.stateBits[stage] );
				for ( uint32_t texture : material.textures[stage] ) {
					if ( texture )
						ImagePreview( renderer, (int)texture - 1, 100 );
				}
			}
		}
		ImGui::EndTabItem();
	}
}

static void InspectMemory( void ) {
	if ( !ImGui::BeginTabItem( "Memory" ) )
		return;
	devMemory_t memory;
	Com_DeveloperMemory( &memory );
	ImGui::Text( "Hunk: %d total, %d permanent, %d temporary, %d free", memory.hunkTotal,
		memory.hunkPermanent, memory.hunkTemporary, memory.hunkFree );
	ImGui::TextUnformatted( "Zone bytes include block headers; hunk has lifetime regions, no tags." );
	if ( ImGui::BeginTable( "Tags", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg ) ) {
		ImGui::TableSetupColumn( "Tag" );
		ImGui::TableSetupColumn( "Bytes" );
		ImGui::TableSetupColumn( "Blocks" );
		ImGui::TableHeadersRow();
		for ( int tag = 0; tag < TAG_COUNT; ++tag ) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted( memory.names[tag] );
			ImGui::TableNextColumn();
			ImGui::Text( "%" PRIu64, memory.bytes[tag] );
			ImGui::TableNextColumn();
			ImGui::Text( "%" PRIu64, memory.blocks[tag] );
		}
		ImGui::EndTable();
	}
	ImGui::EndTabItem();
}

static void InspectAnimation( const refexport_t *renderer, uint32_t elapsed ) {
	animation.draw = animation.load = false;
	if ( !ImGui::BeginTabItem( "Animation" ) )
		return;
	ImGui::InputText( "Model", animation.path, sizeof( animation.path ) );
	ImGui::InputText( "Skin (optional)", animation.skinPath, sizeof( animation.skinPath ) );
	animation.load = ImGui::Button( "Load model / skin" );
	if ( ImGui::BeginChild( "Loaded models", ImVec2( 0, 70 ), ImGuiChildFlags_Borders ) ) {
		devModel_t model;
		for ( int i = 1; renderer->GetDeveloperModel( i, &model ); ++i ) {
			if ( model.frames < 1 )
				continue;
			ImGui::PushID( i );
			if ( ImGui::Selectable( model.name, i == animation.model ) ) {
				animation.model = i;
				animation.frame = 0;
				animation.phase = 0;
				Q_strncpyz( animation.path, model.name, sizeof( animation.path ) );
			}
			ImGui::PopID();
		}
	}
	ImGui::EndChild();
	devModel_t model;
	if ( renderer->GetDeveloperModel( animation.model, &model ) && model.frames > 0 ) {
		ImGui::Text( "%d frames, %d model bytes", model.frames, model.bytes );
		animation.frame = MIN( animation.frame, model.frames - 1 );
		if ( ImGui::SliderInt( "Frame", &animation.frame, 0, model.frames - 1 ) ) {
			animation.play = false;
			animation.phase = (float)animation.frame;
		}
		ImGui::Checkbox( "Play", &animation.play );
		ImGui::SameLine();
		ImGui::SliderFloat( "FPS", &animation.fps, 1, 60 );
		ImGui::SliderFloat( "Yaw", &animation.yaw, -180, 180 );
		if ( animation.play ) {
			animation.phase = fmodf( animation.phase + (float)MIN( elapsed, 250U ) * animation.fps * 0.001f, (float)model.frames );
			animation.frame = (int)animation.phase;
		}
		const ImVec2 size = ImGui::GetContentRegionAvail();
		if ( size.x > 32 && size.y > 32 ) {
			const ImVec2 position = ImGui::GetCursorScreenPos();
			ImGui::InvisibleButton( "Preview", size );
			const ImVec2 clipMin = ImGui::GetWindowDrawList()->GetClipRectMin();
			const ImVec2 clipMax = ImGui::GetWindowDrawList()->GetClipRectMax();
			animation.x = (int)MAX( position.x, clipMin.x );
			animation.y = (int)MAX( position.y, clipMin.y );
			animation.width = (int)MIN( position.x + size.x, clipMax.x ) - animation.x;
			animation.height = (int)MIN( position.y + size.y, clipMax.y ) - animation.y;
			animation.draw = animation.width > 32 && animation.height > 32;
		}
	} else {
		ImGui::TextUnformatted( "Select a loaded model or enter an MD3, MDR or IQM path." );
	}
	ImGui::EndTabItem();
}

static void DrawAnimation( const refexport_t *renderer, int milliseconds ) {
	if ( animation.load ) {
		animation.model = renderer->RegisterModel( animation.path );
		animation.skin = *animation.skinPath ? renderer->RegisterSkin( animation.skinPath ) : 0;
		animation.phase = 0;
		animation.frame = 0;
		animation.fps = 15;
	}
	devModel_t model;
	if ( !animation.draw || !renderer->GetDeveloperModel( animation.model, &model ) || model.frames < 1 )
		return;
	refdef_t view = {};
	view.x = animation.x;
	view.y = animation.y;
	view.width = animation.width;
	view.height = animation.height;
	view.fov_y = 40;
	view.fov_x = (float)( atan( tan( DEG2RAD( view.fov_y * 0.5f ) ) * (float)view.width / (float)view.height ) * 360.0 / M_PI );
	view.rdflags = RDF_NOWORLDMODEL;
	view.time = milliseconds;
	AxisClear( view.viewaxis );
	refEntity_t entity = {};
	entity.reType = RT_MODEL;
	entity.hModel = animation.model;
	entity.customSkin = animation.skin;
	entity.renderfx = RF_NOSHADOW | RF_MINLIGHT;
	entity.oldframe = animation.frame;
	entity.frame = animation.play ? ( animation.frame + 1 ) % model.frames : animation.frame;
	entity.backlerp = animation.play ? 1.0f - ( animation.phase - (float)animation.frame ) : 0;
	vec3_t mins, maxs, angles = { 0, animation.yaw, 0 };
	renderer->ModelBounds( animation.model, mins, maxs );
	const float extent = MAX( maxs[2] - mins[2], MAX( maxs[0] - mins[0], maxs[1] - mins[1] ) );
	entity.origin[0] = MAX( extent, 8.0f ) * 2.0f;
	entity.origin[2] = -( mins[2] + maxs[2] ) * 0.5f;
	VectorCopy( entity.origin, entity.oldorigin );
	AnglesToAxis( angles, entity.axis );
	renderer->ClearScene();
	renderer->AddRefEntityToScene( &entity, qfalse );
	renderer->RenderScene( &view );
	++animationFrames;
}

static void InspectProfile( const refexport_t *renderer, uint32_t elapsed ) {
	static float history[240];
	static uint32_t cursor;
	history[cursor++ % ARRAY_LEN( history )] = (float)elapsed;
	const devNetwork_t *net = DevTools_Network();
	static uint64_t previous[2], interval;
	static double rate[2];
	interval += elapsed;
	if ( interval >= 1000 ) {
		for ( int i = 0; i < 2; ++i ) {
			rate[i] = (double)( net->bytes[i] - previous[i] ) * 1000.0 / (double)interval;
			previous[i] = net->bytes[i];
		}
		interval = 0;
	}
	if ( !ImGui::BeginTabItem( "Profile" ) )
		return;
	ImGui::PlotLines( "Frame ms", history, ARRAY_LEN( history ), (int)( cursor % ARRAY_LEN( history ) ), nullptr, 0, 100, ImVec2( 0, 80 ) );
	devGpuTiming_t timings[32];
	const uint32_t count = renderer->GetDeveloperTimings( timings, ARRAY_LEN( timings ) );
	const devCpuTiming_t *cpu;
	const uint32_t cpuCount = DevTools_CpuTimings( &cpu );
	ImGui::TextUnformatted( "Previous CPU frame (inclusive scopes)" );
	for ( uint32_t i = 0; i < cpuCount; ++i )
		ImGui::Text( "%s: %.3f ms", cpu[i].name, (double)cpu[i].microseconds / 1000.0 );
	ImGui::TextUnformatted( "Completed GPU frame (no additional wait)" );
	for ( uint32_t i = 0; i < count; ++i )
		ImGui::Text( "%s: %.3f ms", timings[i].name, timings[i].microseconds / 1000.0 );
	if ( !count )
		ImGui::TextUnformatted( "GPU timestamp results unavailable" );
	ImGui::Separator();
	ImGui::Text( "Client RX/TX %.0f / %.0f bytes/s", rate[0], rate[1] );
	ImGui::Text( "Packets %" PRIu64 " / %" PRIu64 "; last datagram %u / %u bytes",
		net->packets[0], net->packets[1], net->lastPacket[0], net->lastPacket[1] );
	ImGui::Text( "Snapshot: %u bits, %s (%" PRIu64 " observed)", net->snapshotBits,
		net->delta ? "delta" : "full", net->snapshots );
	if ( net->predictions )
		ImGui::Text( "Prediction error: %.4f units (%" PRIu64 " samples)", net->predictionError, net->predictions );
	else
		ImGui::TextUnformatted( "Prediction error unavailable (demo or game without instrumentation)" );
	ImGui::TextWrapped( "Datagram payload sizes exclude transport headers; replay snapshots are not network traffic." );
	ImGui::EndTabItem();
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
	const uint32_t elapsed = lastTime ? (uint32_t)milliseconds - lastTime : 0;
	io.DeltaTime = lastTime ? Com_Clamp( 0.001f, 0.25f, (float)elapsed * 0.001f ) : 1.0f / 60.0f;
	lastTime = (uint32_t)milliseconds;
	animation.draw = animation.load = false;
	if ( animation.fps < 1 )
		animation.fps = 15;
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
			InspectAssets( renderer );
			InspectProfile( renderer, elapsed );
			InspectMemory();
			InspectAnimation( renderer, elapsed );
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
	DrawAnimation( renderer, milliseconds );
	if ( apply && *selected )
		Cvar_Set2( selected, value, qfalse );
	if ( execute && *command ) {
		Cbuf_AddText( command );
		Cbuf_AddText( "\n" );
		command[0] = '\0';
	}
}
