#include "devtools_public.h"
#include "../animation/animation_public.h"
#include "../weapons/weapons_public.h"
#include "../qcommon/qcommon_public.h"
#include "../qcommon/keys_public.h"
#include "../../third_party/imgui/imgui.h"
#include <inttypes.h>

static cvar_t *enabled;
static ImGuiContext *context;
static bool inputCaptured;
static uint32_t fontTexture, lastTime;
static float mouseX = 320, mouseY = 240;
static int screenWidth = 640, screenHeight = 480;
static uint32_t renderedFrames, allocations, animationFrames, drawnLines, drawnLabels;
static devUiVertex_t vertices[65536];
static uint32_t indices[196608];
static devUiCommand_t commands[4096];

static struct {
	char path[MAX_QPATH], skinPath[MAX_QPATH];
	int model, skin, frame, clip;
	char clipName[64];
	float phase, yaw, fps;
	bool play, load, draw;
	int x, y, width, height;
} animation;

// The graph editor keeps source text and preview state separate from live game
// assets. Cooking stays offline; a map restart selects a new gameplay revision.
static struct {
	char path[MAX_QPATH], source[MAX_QPATH], loadedSource[MAX_QPATH];
	char text[65536], saved[65536], status[256], lastEvent[64];
	animAsset_t asset;
	void *storage;
	animState_t state;
	float parameters[ANIM_MAX_PARAMETERS];
	uint32_t time, remainder, previews;
	int model;
	bool load, read, save, select, play, draw;
} graph;

static struct {
	bool visible, select, loaded, ads;
	int action, attachments, slot;
	char path[MAX_QPATH], loadedPath[MAX_QPATH], status[160];
	weaponDef_t definition;
} weaponRange;

static void WeaponRangeCommand( void ) {
	weaponRange.visible = weaponRange.select = true;
	weaponRange.slot = 1;
	Cvar_Set( "dev_tools", "1" );
	if ( !weaponRange.path[0] ) {
		const char *cursor = Cvar_VariableString( "g_weapons" );
		Q_strncpyz( weaponRange.path, COM_Parse( &cursor ), sizeof( weaponRange.path ) );
	}
	weaponRange.action = 1;
	Com_Printf( "Developer weapon range: opened\n" );
}
static void InspectWeaponRange( void ) {
	if ( !weaponRange.visible )
		return;
	const auto flags = weaponRange.select ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
	weaponRange.select = false;
	if ( !ImGui::BeginTabItem( "Range", &weaponRange.visible, flags ) )
		return;
	ImGui::SetNextItemWidth( 450 );
	ImGui::InputText( "##Weapon asset", weaponRange.path, sizeof( weaponRange.path ) );
	ImGui::SameLine();
	if ( ImGui::Button( "Inspect" ) )
		weaponRange.action = 1;
	if ( weaponRange.loaded ) {
		const auto &weapon = weaponRange.definition;
		ImGui::Text( "%s | damage %.1f | interval %u ms | magazine %u + 1", weapon.name, double( weapon.damage ), weapon.intervalMs, weapon.magazine );
		ImGui::Text( "Spread %.2f | ADS %.1f / %u ms | %u recoil samples", double( weapon.spreadDegrees ), double( weapon.adsFov ), weapon.adsMs, weapon.recoilCount );
	}
	ImGui::TextWrapped( "%s", weaponRange.status );
	ImGui::SetCursorPosY( 160 );
	const bool local = DevTools_Game() && Cvar_VariableIntegerValue( "sv_cheats" ) && DevTools_ViewClient() >= 0;
	ImGui::BeginDisabled( !local );
	if ( ImGui::Button( "Spawn moving target", ImVec2( 162, 20 ) ) )
		weaponRange.action = 2;
	ImGui::SameLine();
	ImGui::SetNextItemWidth( 90 );
	ImGui::InputInt( "##Weapon slot", &weaponRange.slot );
	weaponRange.slot = MAX( 1, MIN( weaponRange.slot, int( WEAPON_MAX_DEFINITIONS ) ) );
	ImGui::SameLine();
	if ( ImGui::Button( "Select slot", ImVec2( 90, 20 ) ) )
		weaponRange.action = 3;
	ImGui::SetCursorPosY( 184 );
	if ( ImGui::Button( "Fire", ImVec2( 42, 20 ) ) )
		weaponRange.action = 5;
	ImGui::SameLine();
	if ( ImGui::Button( "Reload", ImVec2( 60, 20 ) ) )
		weaponRange.action = 6;
	ImGui::SameLine();
	if ( ImGui::Button( "Melee", ImVec2( 55, 20 ) ) )
		weaponRange.action = 7;
	ImGui::SameLine();
	if ( ImGui::Button( "Offhand", ImVec2( 70, 20 ) ) )
		weaponRange.action = 8;
	if ( ImGui::Checkbox( "ADS", &weaponRange.ads ) )
		weaponRange.action = 9;
	if ( weaponRange.loaded && weaponRange.definition.attachmentCount ) {
		ImGui::SliderInt( "Attachment mask", &weaponRange.attachments, 0, ( 1 << weaponRange.definition.attachmentCount ) - 1 );
		if ( ImGui::Button( "Apply attachments" ) )
			weaponRange.action = 10;
	}
	ImGui::SetCursorPosY( 276 );
	if ( ImGui::Button( "Restart with inspected weapon", ImVec2( 260, 20 ) ) )
		weaponRange.action = 11;
	ImGui::EndDisabled();
	ImGui::SameLine();
	if ( ImGui::Button( "Capture panel", ImVec2( 120, 20 ) ) )
		weaponRange.action = 12;
	if ( !local )
		ImGui::TextWrapped( "Open a local developer map to use range controls." );
	ImGui::TextWrapped( "Inspect a cooked weapon, tune its source and recook, then restart. The active game keeps its map-start revision. Start the map with g_rewind 1 for moving targets." );
	const auto *net = DevTools_Network();
	ImGui::Text( "Rewind reports %" PRIu64 " | hits %" PRIu64 " | view age %u ms", net->rewindReports, net->rewindHits, net->rewindAge );
	ImGui::EndTabItem();
}
static void EditWeaponRange( void ) {
	const int action = weaponRange.action;
	weaponRange.action = 0;
	if ( action == 1 ) {
		uint8_t hash[32];
		weaponRange.loaded = Weapon_LoadFile( weaponRange.path, &weaponRange.definition, hash );
		if ( weaponRange.loaded )
			Q_strncpyz( weaponRange.loadedPath, weaponRange.path, sizeof( weaponRange.loadedPath ) );
		Q_strncpyz( weaponRange.status, weaponRange.loaded ? "Cooked definition loaded." : "Weapon load failed; choose a cooked .asweapon file.", sizeof( weaponRange.status ) );
		return;
	}
	if ( action == 12 ) {
		Cbuf_AddText( "screenshot range-panel\n" );
		return;
	}
	if ( !action || !DevTools_Game() || !Cvar_VariableIntegerValue( "sv_cheats" ) || DevTools_ViewClient() < 0 )
		return;
	switch ( action ) {
	case 2:
		Cbuf_AddText( va( "rewind_target %d\n", DevTools_ViewClient() ) );
		Com_Printf( "Developer weapon range: target\n" );
		break;
	case 3:
		Cbuf_AddText( va( "weapon %d\n", weaponRange.slot ) );
		break;
	case 5:
		Cbuf_AddText( "+attack\nwait 2\n-attack\n" );
		Com_Printf( "Developer weapon range: fire\n" );
		break;
	case 6:
		Cbuf_AddText( "+button13\nwait 2\n-button13\n" );
		break;
	case 7:
		Cbuf_AddText( "+button14\nwait 2\n-button14\n" );
		break;
	case 8:
		Cbuf_AddText( "+button15\nwait 2\n-button15\n" );
		break;
	case 9:
		Cbuf_AddText( weaponRange.ads ? "+button12\n" : "-button12\n" );
		break;
	case 10:
		Cbuf_AddText( va( "cmd weapon_attachment 0 %d\n", weaponRange.attachments ) );
		break;
	case 11:
		if ( weaponRange.loaded ) {
			Cvar_Set( "g_weapons", weaponRange.loadedPath );
			Cvar_Set( "g_rewind", "1" );
			Cbuf_AddText( "map_restart 0\n" );
		}
		break;
	}
}

template <typename T>
static T GraphRecord( animSectionIndex_t section, uint32_t index ) {
	T record;
	memcpy( &record, graph.asset.data + graph.asset.header.sections[section].offset + index * sizeof( T ), sizeof( record ) );
	return record;
}
static void GraphCommand( void ) {
	const char *operation = Cmd_Argv( 1 );
	if ( !strcmp( operation, "load" ) ) {
		Q_strncpyz( graph.path, Cmd_Argv( 2 ), sizeof( graph.path ) );
		graph.load = graph.select = true;
	} else if ( !strcmp( operation, "source" ) ) {
		Q_strncpyz( graph.source, Cmd_Argv( 2 ), sizeof( graph.source ) );
		graph.read = graph.select = true;
	} else {
		Com_Printf( "dev_animation load <cooked.asanim> | source <animation_source/file.json>\n" );
	}
}
static bool GraphReadSource( const char *path, char *text, size_t capacity ) {
	fileHandle_t file;
	const int length = FS_FOpenFileRead( path, &file, qfalse );
	if ( !file )
		return false;
	const bool success = length >= 0 && size_t( length ) < capacity && FS_Read( text, length, file ) == length;
	FS_FCloseFile( file );
	if ( success && memchr( text, 0, size_t( length ) ) )
		return false;
	if ( success )
		text[length] = '\0';
	return success;
}
static bool GraphWriteSource( const char *path, const char *text ) {
	const fileHandle_t file = FS_FOpenFileWrite( path );
	if ( !file )
		return false;
	const int length = int( strlen( text ) );
	const bool success = FS_Write( text, length, file ) == length;
	FS_FCloseFile( file );
	return success;
}
static void EditGraph( const refexport_t *renderer ) {
	if ( graph.load ) {
		graph.load = false;
		animAsset_t asset;
		void *storage = Anim_LoadFile( graph.path, &asset );
		if ( storage ) {
			Anim_FreeFile( graph.storage );
			graph.storage = storage;
			graph.asset = asset;
			graph.model = renderer->RegisterModel( asset.header.model );
			graph.time = graph.remainder = graph.previews = 0;
			graph.play = false;
			graph.lastEvent[0] = '\0';
			Anim_Reset( &asset, 0, &graph.state );
			Anim_DefaultParameters( &asset, graph.parameters );
			Q_strncpyz( graph.status, "Graph loaded. Gameplay keeps its map-start revision.", sizeof( graph.status ) );
			Com_Printf( "Animation graph loaded: state=%s\n", Anim_StateName( &asset, graph.state.current ) );
		} else {
			Q_strncpyz( graph.status, "Graph load failed; the previous preview remains available.", sizeof( graph.status ) );
		}
	}
	if ( graph.read || graph.save ) {
		const bool read = graph.read;
		graph.read = graph.save = false;
		// Restrict source edits to a dedicated loose-file project in fs_homepath.
		bool valid = !strncmp( graph.source, "animation_source/", 17 ) && !strstr( graph.source, ".." ) && COM_CompareExtension( graph.source, ".json" );
		for ( const char *p = graph.source; *p; ++p )
			valid &= ( *p >= 'a' && *p <= 'z' ) || ( *p >= '0' && *p <= '9' ) || *p == '/' || *p == '_' || *p == '-' || *p == '.';
		if ( !valid ) {
			Q_strncpyz( graph.status, "Use a lowercase animation_source/*.json path.", sizeof( graph.status ) );
			return;
		}
		char current[sizeof( graph.text )];
		if ( !GraphReadSource( graph.source, current, sizeof( current ) ) ) {
			Q_strncpyz( graph.status, "Source read failed or exceeds 65535 bytes; edits retained.", sizeof( graph.status ) );
			return;
		}
		if ( read ) {
			if ( strcmp( graph.text, graph.saved ) ) {
				Q_strncpyz( graph.status, "Unsaved edits: save or undo them before loading another source.", sizeof( graph.status ) );
				return;
			}
			Q_strncpyz( graph.text, current, sizeof( graph.text ) );
			Q_strncpyz( graph.saved, current, sizeof( graph.saved ) );
			Q_strncpyz( graph.loadedSource, graph.source, sizeof( graph.loadedSource ) );
			Q_strncpyz( graph.status, "Source loaded. Save keeps a numbered backup; the cooker validates JSON.", sizeof( graph.status ) );
		} else {
			if ( strcmp( graph.source, graph.loadedSource ) || strcmp( current, graph.saved ) ) {
				Q_strncpyz( graph.status, "Source changed outside the editor; save refused, edits retained.", sizeof( graph.status ) );
				return;
			}
			char backup[MAX_QPATH + 16];
			int revision;
			for ( revision = 0; revision < 1000; ++revision ) {
				Com_sprintf( backup, sizeof( backup ), "%s.bak.%03d", graph.source, revision );
				if ( !FS_FileExists( backup ) )
					break;
			}
			if ( revision == 1000 || !GraphWriteSource( backup, current ) ) {
				Q_strncpyz( graph.status, "Backup failed or 1000 revisions reached; source unchanged.", sizeof( graph.status ) );
				return;
			}
			if ( !GraphWriteSource( graph.source, graph.text ) || !GraphReadSource( graph.source, current, sizeof( current ) ) || strcmp( current, graph.text ) ) {
				Com_sprintf( graph.status, sizeof( graph.status ), "Save failed; edits retained, previous source in %s.", backup );
				return;
			}
			Q_strncpyz( graph.saved, graph.text, sizeof( graph.saved ) );
			Q_strncpyz( graph.status, "Source saved. Check cooker output, then load the cooked graph.", sizeof( graph.status ) );
			Com_Printf( "Animation source saved: %s (backup %s)\n", graph.source, backup );
		}
	}
}
static void InspectGraph( uint32_t elapsed ) {
	const ImGuiTabItemFlags flags = graph.select ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
	graph.select = false;
	if ( !ImGui::BeginTabItem( "Graph", nullptr, flags ) )
		return;
	ImGui::SetNextItemWidth( 370 );
	ImGui::InputText( "##Cooked graph", graph.path, sizeof( graph.path ) );
	ImGui::SameLine();
	graph.load |= ImGui::Button( "Load graph" );
	if ( ImGui::BeginTabBar( "Graph views" ) ) {
		if ( ImGui::BeginTabItem( "Preview" ) ) {
			if ( graph.storage ) {
				ImGui::Text( "State: %s | last event: %s", Anim_StateName( &graph.asset, graph.state.current ), graph.lastEvent );
				ImGui::Checkbox( "Play fixed steps", &graph.play );
				ImGui::SameLine();
				if ( ImGui::Button( "Reset" ) ) {
					graph.time = graph.remainder = 0;
					Anim_Reset( &graph.asset, 0, &graph.state );
				}
				if ( ImGui::BeginChild( "Inputs", ImVec2( 0, 110 ), ImGuiChildFlags_Borders ) ) {
					for ( uint32_t i = 0; i < graph.asset.header.sections[ANIM_PARAMETERS].count; ++i ) {
						const auto parameter = GraphRecord<animFileParameter_t>( ANIM_PARAMETERS, i );
						ImGui::SliderFloat( parameter.name, &graph.parameters[i], parameter.minimum, parameter.maximum );
					}
				}
				ImGui::EndChild();
				ImGui::SliderFloat( "Yaw", &animation.yaw, -180, 180 );
				if ( graph.play ) {
					graph.remainder += MIN( elapsed, 250U );
					while ( graph.remainder >= 20 ) {
						graph.time += 20;
						graph.remainder -= 20;
						animEvents_t events;
						if ( !Anim_Tick( &graph.asset, graph.parameters, graph.time, &graph.state, &events ) ) {
							graph.play = false;
							Q_strncpyz( graph.status, "Graph tick failed; preview paused.", sizeof( graph.status ) );
							break;
						}
						if ( events.count )
							Q_strncpyz( graph.lastEvent, Anim_EventName( &graph.asset, events.items[events.count - 1].id ), sizeof( graph.lastEvent ) );
					}
				}
				const ImVec2 position = ImGui::GetCursorScreenPos(), size = ImGui::GetContentRegionAvail();
				animation.x = int( position.x );
				animation.y = int( position.y );
				animation.width = int( size.x );
				animation.height = int( size.y ) - 35;
				graph.draw = animation.width > 32 && animation.height > 32;
				if ( graph.draw )
					ImGui::InvisibleButton( "Graph pose", ImVec2( size.x, float( animation.height ) ) );
			}
			ImGui::EndTabItem();
		}
		if ( ImGui::BeginTabItem( "Source" ) ) {
			ImGui::SetNextItemWidth( 460 );
			ImGui::InputText( "##Source path", graph.source, sizeof( graph.source ) );
			graph.read |= ImGui::Button( "Load source" );
			ImGui::SameLine();
			graph.save |= ImGui::Button( "Save + backup" );
			ImGui::SameLine();
			if ( ImGui::Button( "Undo edits" ) )
				Q_strncpyz( graph.text, graph.saved, sizeof( graph.text ) );
			ImGui::InputTextMultiline( "##Graph JSON", graph.text, sizeof( graph.text ), ImVec2( -1, 210 ), ImGuiInputTextFlags_AllowTabInput );
			ImGui::EndTabItem();
		}
		if ( ImGui::BeginTabItem( "Tables" ) ) {
			if ( graph.storage ) {
				if ( ImGui::TreeNode( "States / events" ) ) {
					for ( uint32_t i = 0; i < graph.asset.header.sections[ANIM_STATES].count; ++i ) {
						const auto state = GraphRecord<animFileState_t>( ANIM_STATES, i );
						ImGui::Text( "%s: node %u, speed %.3f, %s", state.name, state.node, double( state.speedQ16 ) / 65536, state.flags & ANIM_LOOP ? "loop" : "once" );
						for ( uint32_t e = 0; e < state.eventCount; ++e ) {
							const auto event = GraphRecord<animFileEvent_t>( ANIM_EVENTS, state.firstEvent + e );
							ImGui::Text( "  %u ms: %s (bone %d)", event.timeMs, event.name, event.bone );
						}
					}
					ImGui::TreePop();
				}
				if ( ImGui::TreeNode( "Transitions" ) ) {
					const char *operations[] = { "==", "!=", "<", "<=", ">", ">=" };
					for ( uint32_t i = 0; i < graph.asset.header.sections[ANIM_TRANSITIONS].count; ++i ) {
						const auto transition = GraphRecord<animFileTransition_t>( ANIM_TRANSITIONS, i );
						ImGui::Text( "%s -> %s: %u ms%s", transition.from == ANIM_ANY_STATE ? "*" : Anim_StateName( &graph.asset, transition.from ), Anim_StateName( &graph.asset, transition.to ), transition.blendMs, transition.flags & ANIM_ON_END ? " on end" : "" );
						for ( uint32_t c = 0; c < transition.conditionCount; ++c ) {
							const auto condition = GraphRecord<animFileCondition_t>( ANIM_CONDITIONS, transition.firstCondition + c );
							const auto parameter = GraphRecord<animFileParameter_t>( ANIM_PARAMETERS, condition.parameter );
							ImGui::Text( "  %s %s %.3f", parameter.name, operations[condition.operation], double( condition.value ) );
						}
					}
					ImGui::TreePop();
				}
				if ( ImGui::TreeNode( "Blend nodes / masks" ) ) {
					for ( uint32_t i = 0; i < graph.asset.header.sections[ANIM_NODES].count; ++i ) {
						const auto node = GraphRecord<animFileNode_t>( ANIM_NODES, i );
						ImGui::Text( "%u %s: %s a=%u b=%u reference=%u", i, node.name, node.kind == ANIM_CLIP ? "clip" : node.kind == ANIM_BLEND ? "blend"
																																				 : "additive",
							node.a, node.b, node.reference );
						if ( node.kind != ANIM_CLIP )
							ImGui::Text( "  parameter=%u mask=%u weight=%.3f", node.parameter, node.mask, double( node.weight ) );
					}
					for ( uint32_t i = 0; i < graph.asset.header.sections[ANIM_MASKS].count; ++i ) {
						const auto mask = GraphRecord<animFileMask_t>( ANIM_MASKS, i );
						ImGui::Text( "Mask %u: %s", i, mask.name );
						for ( uint32_t j = 0; j < graph.asset.header.sections[ANIM_JOINTS].count; ++j ) {
							const auto joint = GraphRecord<animFileJoint_t>( ANIM_JOINTS, j );
							if ( mask.weights[j] )
								ImGui::Text( "  %s %.3f", joint.name, double( mask.weights[j] ) );
						}
					}
					ImGui::TreePop();
				}
			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::TextWrapped( "%s", graph.status );
	ImGui::EndTabItem();
}

static struct {
	int selected, action;
	char key[64], value[1024], classname[64], status[128];
	float origin[3];
} entities;

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
	Com_Printf( "Developer animation: model=%d frame=%d previews=%u clip=%s\n", animation.model, animation.frame, animationFrames, animation.clipName );
	if ( graph.storage )
		Com_Printf( "Developer graph: state=%s previews=%u\n", Anim_StateName( &graph.asset, graph.state.current ), graph.previews );
	Com_Printf( "Developer drawing: lines=%u labels=%u\n", drawnLines, drawnLabels );
	devMemory_t memory;
	Com_DeveloperMemory( &memory );
	Com_Printf( "Developer asset memory: renderer=%" PRIu64 " blocks=%" PRIu64 " hunk=%d\n",
		memory.bytes[TAG_RENDERER], memory.blocks[TAG_RENDERER], memory.hunkPermanent );
}

void DevTools_Init( void ) {
	DevTools_InitEntities();
	DevTools_InitWorld();
	enabled = Cvar_Get( "dev_tools", "0", CVAR_TEMP );
	Cvar_SetDescription( enabled, "Development overlay; Escape closes it. Absent from shipping builds." );
	Cmd_AddCommand( "devtools_status", Status );
	Cmd_AddCommand( "dev_animation", GraphCommand );
	Cmd_AddCommand( "dev_weapon_range", WeaponRangeCommand );
}

void DevTools_Reset( void ) {
	inputCaptured = false;
	DevTools_ClearWorld();
	DevTools_SetView( nullptr );
	if ( context )
		ImGui::DestroyContext( context );
	context = nullptr;
	fontTexture = 0;
	lastTime = 0;
	animation.model = animation.skin = 0;
	Anim_FreeFile( graph.storage );
	graph.storage = nullptr;
	graph.model = 0;
	graph.play = graph.draw = false;
	animation.phase = 0;
	entities.selected = -1;
	entities.key[0] = entities.value[0] = entities.status[0] = '\0';
}

static bool Visible( void ) {
	return inputCaptured && enabled && enabled->integer && context;
}

bool DevTools_Key( int key, bool down ) {
	if ( !Visible() )
		return false;
	if ( key == K_ESCAPE && down ) {
		Cvar_Set( "dev_tools", "0" );
		inputCaptured = false;
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
	if ( image.reloads ) {
		ImGui::SameLine();
		ImGui::Text( "reloads %u", image.reloads );
	}
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
			if ( material.reloads ) {
				ImGui::SameLine();
				ImGui::Text( "reloads %u", material.reloads );
			}
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

static struct {
	bool collision, navigation, entities, refresh;
	float radius = 512;
} worldDebug;

static void InspectWorld( void ) {
	if ( !ImGui::BeginTabItem( "World" ) )
		return;
	worldDebug.refresh |= ImGui::Checkbox( "Collision surfaces", &worldDebug.collision );
	worldDebug.refresh |= ImGui::Checkbox( "Navigation areas and routes", &worldDebug.navigation );
	ImGui::Checkbox( "Live entity bounds", &worldDebug.entities );
	ImGui::SliderFloat( "Radius", &worldDebug.radius, 64, 2048 );
	worldDebug.refresh |= ImGui::Button( "Refresh around camera" );
	const devLine_t *lines;
	ImGui::Text( "World cache %u / 4096 lines; omitted %u", DevTools_Lines( &lines, true ), DevTools_DebugDropped() );
	ImGui::TextWrapped( "X-ray wireframes: cyan world brushes, pink patch facets, green navigation, orange reachability. Stripped AAS files show area bounds in place of missing ground faces. Cached surfaces refresh on request, up to 1024 nearby brushes. Navigation requires a local server with loaded AAS. Refresh after moving." );
	ImGui::TextWrapped( "Game debug lines, boxes and text use engine/public/dev_public.h. Left click outside the tools to pick a local entity, or use Pick crosshair in Entities." );
	ImGui::EndTabItem();
}

static void DrawDebugLine( ImDrawList *draw, const refdef_t *view, const devLine_t *line ) {
	vec3_t start, end, delta;
	VectorCopy( line->start, start );
	VectorCopy( line->end, end );
	VectorSubtract( start, view->vieworg, delta );
	const float a = DotProduct( delta, view->viewaxis[0] );
	VectorSubtract( end, view->vieworg, delta );
	const float b = DotProduct( delta, view->viewaxis[0] );
	if ( a < 0.1f && b < 0.1f )
		return;
	if ( a < 0.1f || b < 0.1f ) {
		const float fraction = ( 0.11f - a ) / ( b - a );
		vec3_t clipped;
		for ( int axis = 0; axis < 3; ++axis )
			clipped[axis] = start[axis] + ( end[axis] - start[axis] ) * fraction;
		if ( a < 0.1f )
			VectorCopy( clipped, start );
		else
			VectorCopy( clipped, end );
	}
	float screenStart[2], screenEnd[2];
	if ( DevTools_Project( view, start, screenStart ) && DevTools_Project( view, end, screenEnd ) ) {
		draw->AddLine( ImVec2( screenStart[0], screenStart[1] ), ImVec2( screenEnd[0], screenEnd[1] ), line->color );
		++drawnLines;
	}
}

static void DrawWorldDebug( void ) {
	drawnLines = drawnLabels = 0;
	const refdef_t *view = DevTools_View();
	if ( !view )
		return;
	if ( const devGameTools_t *game = DevTools_Game() ) {
		for ( int i = 0; i < MAX_GENTITIES; ++i ) {
			if ( i == DevTools_ViewClient() && i != entities.selected )
				continue;
			if ( !worldDebug.entities && i != entities.selected )
				continue;
			devEntity_t entity;
			if ( !game->ReadEntity( i, &entity ) )
				continue;
			if ( !entity.linked ) {
				for ( int axis = 0; axis < 3; ++axis ) {
					entity.mins[axis] = entity.origin[axis] - 8;
					entity.maxs[axis] = entity.origin[axis] + 8;
				}
			}
			const uint32_t color = i == entities.selected ? 0xff00ffffU : 0xffaaaaaaU;
			Dev_DrawBox( entity.mins, entity.maxs, color, 0 );
			if ( i == entities.selected )
				Dev_DrawText( entity.origin, entity.classname, color, 0 );
		}
	}
	ImDrawList *draw = ImGui::GetBackgroundDrawList();
	draw->PushClipRect( ImVec2( (float)view->x, (float)view->y ),
		ImVec2( (float)( view->x + view->width ), (float)( view->y + view->height ) ) );
	for ( int world = 0; world < 2; ++world ) {
		const devLine_t *lines;
		const uint32_t count = DevTools_Lines( &lines, world != 0 );
		for ( uint32_t i = 0; i < count; ++i )
			DrawDebugLine( draw, view, &lines[i] );
	}
	const devText_t *text;
	const uint32_t count = DevTools_Text( &text );
	for ( uint32_t i = 0; i < count; ++i ) {
		float screen[2];
		if ( DevTools_Project( view, text[i].origin, screen ) ) {
			draw->AddText( ImVec2( screen[0], screen[1] ), text[i].color, text[i].text );
			++drawnLabels;
		}
	}
	draw->PopClipRect();
}

static void InspectEntities( void ) {
	if ( !ImGui::BeginTabItem( "Entities" ) )
		return;
	const devGameTools_t *game = DevTools_Game();
	if ( !game ) {
		ImGui::TextWrapped( "Start a local devmap using the native game to inspect and edit entities." );
		ImGui::EndTabItem();
		return;
	}
	if ( ImGui::Button( "Pick crosshair" ) )
		entities.action = 6;
	const bool editable = Cvar_VariableIntegerValue( "sv_cheats" ) != 0;
	if ( !editable )
		ImGui::TextUnformatted( "Read only: editing requires devmap (sv_cheats 1)." );
	if ( ImGui::BeginChild( "Entity list", ImVec2( 0, 95 ), ImGuiChildFlags_Borders ) ) {
		for ( int i = 0; i < MAX_GENTITIES; ++i ) {
			devEntity_t info;
			if ( !game->ReadEntity( i, &info ) )
				continue;
			char label[96];
			Com_sprintf( label, sizeof( label ), "%d: %s", i, info.classname );
			if ( ImGui::Selectable( label, i == entities.selected ) )
				entities.selected = i;
		}
	}
	ImGui::EndChild();
	devEntity_t selected;
	if ( game->ReadEntity( entities.selected, &selected ) ) {
		ImGui::Text( "%d: %s, map record %d", entities.selected, selected.classname, selected.source );
		if ( ImGui::BeginCombo( "Field", entities.key ) ) {
			for ( int i = 0; const char *name = game->FieldName( i ); ++i ) {
				if ( ImGui::Selectable( name, !strcmp( name, entities.key ) ) ) {
					Q_strncpyz( entities.key, name, sizeof( entities.key ) );
					game->ReadField( entities.selected, name, entities.value, sizeof( entities.value ) );
				}
			}
			ImGui::EndCombo();
		}
		ImGui::InputText( "Field value", entities.value, sizeof( entities.value ) );
		ImGui::BeginDisabled( !editable || selected.source < 0 );
		if ( ImGui::Button( "Apply field" ) )
			entities.action = 1;
		ImGui::SameLine();
		if ( ImGui::Button( "Delete entity" ) )
			entities.action = 2;
		ImGui::EndDisabled();
	}
	ImGui::Separator();
	ImGui::InputText( "Spawn class", entities.classname, sizeof( entities.classname ) );
	ImGui::InputFloat3( "Spawn origin", entities.origin );
	if ( ImGui::Button( "At camera" ) ) {
		if ( const refdef_t *view = DevTools_View() )
			VectorMA( view->vieworg, 64, view->viewaxis[0], entities.origin );
	}
	ImGui::SameLine();
	ImGui::BeginDisabled( !editable );
	if ( ImGui::Button( "Spawn" ) )
		entities.action = 3;
	ImGui::SameLine();
	if ( ImGui::Button( "Save entity file" ) )
		entities.action = 4;
	ImGui::SameLine();
	if ( ImGui::Button( "Reload saved" ) )
		entities.action = 5;
	ImGui::EndDisabled();
	ImGui::TextWrapped( "%s", entities.status );
	ImGui::TextWrapped( "Spawn pickups or point markers (target_position, info_notnull, info_player_deathmatch, misc_teleporter_dest). Structural fields require a new entity. Saves keep numbered revisions." );
	ImGui::EndTabItem();
}

static void EditEntities( void ) {
	const devGameTools_t *game = DevTools_Game();
	if ( !entities.action || !game )
		return;
	bool success = false;
	switch ( entities.action ) {
	case 1:
		success = game->WriteField( entities.selected, entities.key, entities.value );
		break;
	case 2:
		success = game->Delete( entities.selected );
		break;
	case 3:
		entities.selected = game->Spawn( entities.classname, entities.origin );
		success = entities.selected >= 0;
		break;
	case 4:
		success = DevTools_SaveEntities();
		break;
	case 6:
		if ( const refdef_t *view = DevTools_View() ) {
			entities.selected = DevTools_PickEntity( (float)view->x + (float)view->width * 0.5f, (float)view->y + (float)view->height * 0.5f );
			success = entities.selected >= 0;
		}
		break;
	case 5:
		if ( *Cvar_VariableString( "dev_entityFile" ) ) {
			Cvar_Set( "dev_loadEntities", "1" );
			Cbuf_AddText( "map_restart 0\n" );
			success = true;
		}
		break;
	default:
		break;
	}
	Q_strncpyz( entities.status, success ? "Entity action completed." : "Action rejected: check field/class, value, map source, cheats and capacity.", sizeof( entities.status ) );
	Com_Printf( "Developer entity action %d: %s (entity %d)\n", entities.action, success ? "completed" : "rejected", entities.selected );
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
				animation.clip = 0;
				Q_strncpyz( animation.path, model.name, sizeof( animation.path ) );
			}
			ImGui::PopID();
		}
	}
	ImGui::EndChild();
	devModel_t model;
	if ( renderer->GetDeveloperModel( animation.model, &model ) && model.frames > 0 ) {
		modelAnimation_t clip;
		if ( !renderer->GetModelAnimation( animation.model, animation.clip, &clip ) ) {
			clip.firstFrame = 0;
			clip.frameCount = model.frames;
			clip.flags = 1;
			Q_strncpyz( clip.name, "All frames", sizeof( clip.name ) );
		}
		modelAnimation_t firstClip;
		if ( renderer->GetModelAnimation( animation.model, 0, &firstClip ) ) {
			if ( ImGui::BeginCombo( "Clip", clip.name ) ) {
				modelAnimation_t choice;
				for ( int i = 0; renderer->GetModelAnimation( animation.model, i, &choice ); i++ ) {
					if ( ImGui::Selectable( choice.name, animation.clip == i ) ) {
						animation.clip = i;
						clip = choice;
						animation.frame = (int)clip.firstFrame;
						animation.phase = (float)clip.firstFrame;
						animation.fps = clip.framesPerSecond;
					}
				}
				ImGui::EndCombo();
			}
		}
		Q_strncpyz( animation.clipName, clip.name, sizeof( animation.clipName ) );
		const int firstFrame = (int)clip.firstFrame, lastFrame = firstFrame + (int)clip.frameCount - 1;
		ImGui::Text( "%d frames, %d model bytes", model.frames, model.bytes );
		if ( model.reloads ) {
			ImGui::SameLine();
			ImGui::Text( "reloads %u", model.reloads );
		}
		animation.frame = MAX( firstFrame, MIN( animation.frame, lastFrame ) );
		if ( ImGui::SliderInt( "Frame", &animation.frame, firstFrame, lastFrame ) ) {
			animation.play = false;
			animation.phase = (float)animation.frame;
		}
		ImGui::Checkbox( "Play", &animation.play );
		ImGui::SameLine();
		ImGui::SliderFloat( "FPS", &animation.fps, 1, 60 );
		ImGui::SliderFloat( "Yaw", &animation.yaw, -180, 180 );
		if ( animation.play ) {
			animation.phase = MAX( (float)firstFrame, animation.phase ) + (float)MIN( elapsed, 250U ) * animation.fps * 0.001f;
			if ( clip.flags & 1 )
				animation.phase = (float)firstFrame + fmodf( animation.phase - (float)firstFrame, (float)clip.frameCount );
			else if ( animation.phase >= (float)lastFrame ) {
				animation.phase = (float)lastFrame;
				animation.play = false;
			}
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
		animation.clip = 0;
		modelAnimation_t clip;
		if ( renderer->GetModelAnimation( animation.model, 0, &clip ) ) {
			animation.frame = (int)clip.firstFrame;
			animation.phase = (float)clip.firstFrame;
			animation.fps = clip.framesPerSecond;
		}
	}
	devModel_t model;
	const int modelHandle = graph.draw ? graph.model : animation.model;
	if ( !( animation.draw || graph.draw ) || !renderer->GetDeveloperModel( modelHandle, &model ) || model.frames < 1 )
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
	entity.hModel = modelHandle;
	entity.customSkin = animation.skin;
	entity.renderfx = RF_NOSHADOW | RF_MINLIGHT;
	entity.oldframe = animation.frame;
	modelAnimation_t clip;
	if ( !renderer->GetModelAnimation( animation.model, animation.clip, &clip ) ) {
		clip.frameCount = model.frames;
		clip.flags = 1;
	}
	const int firstFrame = (int)clip.firstFrame, lastFrame = firstFrame + (int)clip.frameCount - 1;
	entity.oldframe = MAX( firstFrame, MIN( entity.oldframe, lastFrame ) );
	entity.frame = entity.oldframe;
	if ( animation.play )
		entity.frame = entity.oldframe < lastFrame ? entity.oldframe + 1 : ( clip.flags & 1 ) ? firstFrame
																							  : lastFrame;
	entity.backlerp = animation.play ? 1.0f - ( animation.phase - (float)animation.frame ) : 0;
	vec3_t mins, maxs, angles = { 0, animation.yaw, 0 };
	renderer->ModelBounds( modelHandle, mins, maxs );
	const float extent = MAX( maxs[2] - mins[2], MAX( maxs[0] - mins[0], maxs[1] - mins[1] ) );
	entity.origin[0] = MAX( extent, 8.0f ) * 2.0f;
	entity.origin[2] = -( mins[2] + maxs[2] ) * 0.5f;
	VectorCopy( entity.origin, entity.oldorigin );
	AnglesToAxis( angles, entity.axis );
	renderer->ClearScene();
	if ( graph.draw ) {
		animPose_t pose;
		if ( !Anim_Evaluate( &graph.asset, &graph.state, graph.parameters, graph.time, &pose ) || !renderer->AddSkeletalEntityToScene( &entity, &pose, graph.asset.header.modelHash, qfalse ) ) {
			Q_strncpyz( graph.status, "Graph/model revision mismatch or pose rejected; reload both together.", sizeof( graph.status ) );
			return;
		}
		++graph.previews;
	} else {
		renderer->AddRefEntityToScene( &entity, qfalse );
	}
	renderer->RenderScene( &view );
	++animationFrames;
}

static void InspectProfile( const refexport_t *renderer, uint32_t elapsed, uint32_t milliseconds ) {
	static float history[240];
	static uint32_t cursor;
	history[cursor++ % ARRAY_LEN( history )] = (float)elapsed;
	const devNetwork_t *net = DevTools_Network();
	static uint64_t previous[2];
	static uint32_t previousTime;
	static double rate[2];
	const uint32_t interval = milliseconds - previousTime;
	if ( interval >= 1000 ) {
		for ( int i = 0; i < 2; ++i ) {
			rate[i] = (double)( net->bytes[i] - previous[i] ) * 1000.0 / (double)interval;
			previous[i] = net->bytes[i];
		}
		previousTime = milliseconds;
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
	ImGui::Text( "Connected server RX / client TX %.0f / %.0f bytes/s", rate[0], rate[1] );
	ImGui::Text( "Packets %" PRIu64 " / %" PRIu64 "; last datagram %u / %u bytes",
		net->packets[0], net->packets[1], net->lastPacket[0], net->lastPacket[1] );
	ImGui::Text( "Snapshot: %u bits, %s (%" PRIu64 " observed)", net->snapshotBits,
		net->delta ? "delta" : "full", net->snapshots );
	if ( net->predictions ) {
		ImGui::Text( "Prediction error: %.4f last / %.4f peak units", double( net->predictionError ), double( net->predictionPeak ) );
		ImGui::Text( "Mean %.4f units (%" PRIu64 " samples)", net->predictionSum / double( net->predictions ), net->predictions );
	} else
		ImGui::TextUnformatted( "Prediction error unavailable (demo or game without instrumentation)" );
	if ( net->rewindReports ) {
		ImGui::Text( "Server rewind: %u ms / %u ms budget", net->rewindAge, net->rewindLimit );
		ImGui::Text( "Rewind reports: %" PRIu64 ", hits %" PRIu64 ", clamped %" PRIu64,
			net->rewindReports, net->rewindHits, net->rewindClamped );
		ImGui::TextUnformatted( "Server samples the last shot at most four times per second." );
	}
	ImGui::TextWrapped( "Datagram payload sizes exclude transport headers; replay snapshots are not network traffic." );
	ImGui::EndTabItem();
}

void DevTools_Draw( const refexport_t *renderer, int width, int height, int milliseconds ) {
	if ( !enabled || !enabled->integer || width <= 0 || height <= 0 ) {
		inputCaptured = false;
		return;
	}
	if ( !inputCaptured ) {
		// Release game bindings before interception, including retained-context reopen.
		Key_ClearStates();
		if ( context ) {
			ImGuiIO &io = ImGui::GetIO();
			io.ClearEventsQueue();
			io.ClearInputKeys();
			io.ClearInputMouse();
			io.AddMousePosEvent( mouseX, mouseY );
		}
		lastTime = 0;
		inputCaptured = true;
	}
	screenWidth = width;
	screenHeight = height;
	if ( !context ) {
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
	graph.draw = false;
	entities.action = 0;
	worldDebug.refresh = false;
	if ( !*entities.classname )
		Q_strncpyz( entities.classname, "target_position", sizeof( entities.classname ) );
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
			InspectProfile( renderer, elapsed, (uint32_t)milliseconds );
			InspectMemory();
			InspectAnimation( renderer, elapsed );
			InspectEntities();
			InspectWorld();
			InspectGraph( elapsed );
			InspectWeaponRange();
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
	DrawWorldDebug();
	ImGui::Render();
	devUiDraw_t draw;
	if ( CopyDrawData( ImGui::GetDrawData(), &draw ) ) {
		renderer->DrawDeveloperUI( &draw );
		++renderedFrames;
	} else {
		Com_Printf( "Developer UI draw capacity exceeded\n" );
	}
	// Engine mutation/error handling runs after all vendor UI calls return.
	EditWeaponRange();
	EditGraph( renderer );
	DrawAnimation( renderer, milliseconds );
	EditEntities();
	if ( worldDebug.refresh )
		DevTools_RebuildWorld( worldDebug.collision, worldDebug.navigation, worldDebug.radius );
	if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) && !io.WantCaptureMouse )
		entities.selected = DevTools_PickEntity( io.MousePos.x, io.MousePos.y );
	if ( apply && *selected )
		Cvar_Set2( selected, value, qfalse );
	if ( execute && *command ) {
		Cbuf_AddText( command );
		Cbuf_AddText( "\n" );
		command[0] = '\0';
	}
}
