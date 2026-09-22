#include "devtools_public.h"
#include "../physics/physics_public.h"
#include "../animation/animation_public.h"
#include "../weapons/weapons_public.h"
#include "../qcommon/qcommon_public.h"
#include "../qcommon/keys_public.h"
#include "../../third_party/imgui/imgui.h"
#include <inttypes.h>
#include <cmath>

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

static const refexport_t *editorRenderer;
static int selectedImage, selectedMaterial;
static char requestedPanel[32], activePanel[32];
static char filters[3][128], selectedCvar[MAX_STRING_CHARS], cvarValue[MAX_CVAR_VALUE_STRING];

bool DevTools_Filter( const char *kind, const char *value ) {
	const int index = !strcmp( kind, "cvars" ) ? 0 : !strcmp( kind, "images" )	? 1
												 : !strcmp( kind, "materials" ) ? 2
																				: -1;
	if ( index < 0 || strlen( value ) >= sizeof( filters[0] ) )
		return false;
	if ( value != filters[index] )
		Q_strncpyz( filters[index], value, sizeof( filters[0] ) );
	return true;
}
bool DevTools_SelectCvar( const char *name ) {
	for ( const cvar_t *var = Cvar_First(); var; var = var->next ) {
		if ( var->name && !strcmp( name, var->name ) ) {
			Q_strncpyz( selectedCvar, name, sizeof( selectedCvar ) );
			Q_strncpyz( cvarValue, var->string, sizeof( cvarValue ) );
			return true;
		}
	}
	return false;
}

static bool BeginPanel( const char *name, bool *open = nullptr, ImGuiTabItemFlags flags = ImGuiTabItemFlags_None ) {
	if ( !strcmp( name, requestedPanel ) )
		flags |= ImGuiTabItemFlags_SetSelected;
	const bool shown = ImGui::BeginTabItem( name, open, flags );
	if ( shown ) {
		Q_strncpyz( activePanel, name, sizeof( activePanel ) );
		if ( !strcmp( name, requestedPanel ) )
			requestedPanel[0] = 0;
	}
	return shown;
}

static struct {
	char path[MAX_QPATH], skinPath[MAX_QPATH];
	int model, skin, frame, clip;
	char clipName[64];
	float phase, yaw, fps;
	bool play, load, draw;
	int x, y, width, height;
} animation;

static struct {
	bool enabled;
	materialOverride_t instance;
} materialPreview;

struct sourceEditor_t {
	char path[MAX_QPATH], loadedPath[MAX_QPATH];
	char text[65536], saved[65536], status[256], result[32];
	bool read, save;
};
static sourceEditor_t effectSource;
static struct {
	char path[MAX_QPATH];
	qhandle_t asset;
	uint32_t instance;
	bool load, start, stop, decal;
} effectPreview;

// The graph editor keeps source text and preview state separate from live game
// assets. Cooking stays offline; a map restart selects a new gameplay revision.
static struct {
	char path[MAX_QPATH], lastEvent[64];
	sourceEditor_t source;
	char requestedTab[16], activeTab[16];
	animAsset_t asset;
	void *storage;
	animState_t state;
	float parameters[ANIM_MAX_PARAMETERS];
	uint32_t time, remainder, previews;
	int model;
	bool load, select, play, draw;
} graph;

static struct {
	bool visible, select, loaded, ads;
	int action, attachments, slot;
	char path[MAX_QPATH], loadedPath[MAX_QPATH], status[160];
	weaponDef_t definition;
} weaponRange;

bool DevTools_Range( const char *action, const char *path, int value ) {
	if ( weaponRange.action )
		return false;
	int command = 0;
	if ( !strcmp( action, "inspect" ) ) {
		if ( !path[0] || strlen( path ) >= sizeof( weaponRange.path ) )
			return false;
		if ( path != weaponRange.path )
			Q_strncpyz( weaponRange.path, path, sizeof( weaponRange.path ) );
		command = 1;
	} else if ( !strcmp( action, "capture" ) )
		command = 12;
	else if ( !strcmp( action, "close" ) ) {
		weaponRange.visible = false;
		return true;
	} else {
		if ( !DevTools_Game() || !Cvar_VariableIntegerValue( "sv_cheats" ) || DevTools_ViewClient() < 0 )
			return false;
		if ( !strcmp( action, "target" ) )
			command = 2;
		else if ( !strcmp( action, "select" ) && value >= 1 && value <= int( WEAPON_MAX_DEFINITIONS ) ) {
			weaponRange.slot = value;
			command = 3;
		} else if ( !strcmp( action, "fire" ) )
			command = 5;
		else if ( !strcmp( action, "reload" ) )
			command = 6;
		else if ( !strcmp( action, "melee" ) )
			command = 7;
		else if ( !strcmp( action, "offhand" ) )
			command = 8;
		else if ( !strcmp( action, "ads" ) && ( value == 0 || value == 1 ) ) {
			weaponRange.ads = value != 0;
			command = 9;
		} else if ( !strcmp( action, "attachments" ) && weaponRange.loaded && value >= 0 && value < ( 1 << weaponRange.definition.attachmentCount ) ) {
			weaponRange.attachments = value;
			command = 10;
		} else if ( !strcmp( action, "restart" ) && weaponRange.loaded )
			command = 11;
		else
			return false;
	}
	weaponRange.action = command;
	return true;
}
static void WeaponRangeCommand( void ) {
	weaponRange.visible = weaponRange.select = true;
	weaponRange.slot = 1;
	Cvar_Set( "dev_tools", "1" );
	if ( !weaponRange.path[0] ) {
		const char *cursor = Cvar_VariableString( "g_weapons" );
		Q_strncpyz( weaponRange.path, COM_Parse( &cursor ), sizeof( weaponRange.path ) );
	}
	DevTools_Range( "inspect", weaponRange.path, 0 );
	Com_Printf( "Developer weapon range: opened\n" );
}
static void InspectWeaponRange( void ) {
	if ( !weaponRange.visible )
		return;
	const auto flags = weaponRange.select ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
	weaponRange.select = false;
	if ( !BeginPanel( "Range", &weaponRange.visible, flags ) )
		return;
	ImGui::SetNextItemWidth( 450 );
	ImGui::InputText( "##Weapon asset", weaponRange.path, sizeof( weaponRange.path ) );
	ImGui::SameLine();
	if ( ImGui::Button( "Inspect" ) )
		DevTools_Range( "inspect", weaponRange.path, 0 );
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
		DevTools_Range( "target", "", 0 );
	ImGui::SameLine();
	ImGui::SetNextItemWidth( 90 );
	ImGui::InputInt( "##Weapon slot", &weaponRange.slot );
	weaponRange.slot = MAX( 1, MIN( weaponRange.slot, int( WEAPON_MAX_DEFINITIONS ) ) );
	ImGui::SameLine();
	if ( ImGui::Button( "Select slot", ImVec2( 90, 20 ) ) )
		DevTools_Range( "select", "", weaponRange.slot );
	ImGui::SetCursorPosY( 184 );
	if ( ImGui::Button( "Fire", ImVec2( 42, 20 ) ) )
		DevTools_Range( "fire", "", 0 );
	ImGui::SameLine();
	if ( ImGui::Button( "Reload", ImVec2( 60, 20 ) ) )
		DevTools_Range( "reload", "", 0 );
	ImGui::SameLine();
	if ( ImGui::Button( "Melee", ImVec2( 55, 20 ) ) )
		DevTools_Range( "melee", "", 0 );
	ImGui::SameLine();
	if ( ImGui::Button( "Offhand", ImVec2( 70, 20 ) ) )
		DevTools_Range( "offhand", "", 0 );
	if ( ImGui::Checkbox( "ADS", &weaponRange.ads ) )
		DevTools_Range( "ads", "", weaponRange.ads ? 1 : 0 );
	if ( weaponRange.loaded && weaponRange.definition.attachmentCount ) {
		ImGui::SliderInt( "Attachment mask", &weaponRange.attachments, 0, ( 1 << weaponRange.definition.attachmentCount ) - 1 );
		if ( ImGui::Button( "Apply attachments" ) )
			DevTools_Range( "attachments", "", weaponRange.attachments );
	}
	ImGui::SetCursorPosY( 276 );
	if ( ImGui::Button( "Restart with inspected weapon", ImVec2( 260, 20 ) ) )
		DevTools_Range( "restart", "", 0 );
	ImGui::EndDisabled();
	ImGui::SameLine();
	if ( ImGui::Button( "Capture panel", ImVec2( 120, 20 ) ) )
		DevTools_Range( "capture", "", 0 );
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
	return DevTools_GraphRecord<T>( &graph.asset, section, index );
}
const animAsset_t *DevTools_GraphAsset( const float **parameters ) {
	*parameters = graph.parameters;
	return graph.storage ? &graph.asset : nullptr;
}
static bool BeginGraphTab( const char *name ) {
	const bool selected = !strcmp( name, graph.requestedTab );
	if ( !ImGui::BeginTabItem( name, nullptr, selected ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None ) )
		return false;
	Q_strncpyz( graph.activeTab, name, sizeof( graph.activeTab ) );
	if ( selected )
		graph.requestedTab[0] = 0;
	return true;
}
bool DevTools_Graph( const char *action, const char *text, float value ) {
	if ( !strcmp( action, "tab" ) ) {
		if ( strcmp( text, "Preview" ) && strcmp( text, "Source" ) && strcmp( text, "Tables" ) )
			return false;
		Q_strncpyz( graph.requestedTab, text, sizeof( graph.requestedTab ) );
		graph.select = true;
	} else if ( !strcmp( action, "load" ) || !strcmp( action, "source" ) ) {
		if ( !text[0] || strlen( text ) >= MAX_QPATH )
			return false;
		const bool load = !strcmp( action, "load" );
		char *path = load ? graph.path : graph.source.path;
		if ( text != path )
			Q_strncpyz( path, text, MAX_QPATH );
		if ( load )
			graph.load = true;
		else
			graph.source.read = true;
		graph.select = true;
	} else if ( !strcmp( action, "text" ) ) {
		if ( strlen( text ) >= sizeof( graph.source.text ) )
			return false;
		if ( text != graph.source.text )
			Q_strncpyz( graph.source.text, text, sizeof( graph.source.text ) );
	} else if ( !strcmp( action, "save" ) ) {
		if ( !graph.source.loadedPath[0] || graph.source.read )
			return false;
		graph.source.save = true;
	} else if ( !strcmp( action, "undo" ) )
		Q_strncpyz( graph.source.text, graph.source.saved, sizeof( graph.source.text ) );
	else if ( !strcmp( action, "play" ) && ( value == 0 || value == 1 ) )
		graph.play = value != 0;
	else if ( !strcmp( action, "reset" ) && graph.storage ) {
		graph.time = graph.remainder = 0;
		Anim_Reset( &graph.asset, 0, &graph.state );
	} else if ( !strcmp( action, "parameter" ) && graph.storage && std::isfinite( value ) ) {
		for ( uint32_t i = 0; i < graph.asset.header.sections[ANIM_PARAMETERS].count; ++i ) {
			const auto parameter = GraphRecord<animFileParameter_t>( ANIM_PARAMETERS, i );
			if ( !strcmp( parameter.name, text ) ) {
				if ( value < parameter.minimum || value > parameter.maximum )
					return false;
				graph.parameters[i] = value;
				return true;
			}
		}
		return false;
	} else
		return false;
	return true;
}
static void GraphCommand( void ) {
	if ( !DevTools_Graph( Cmd_Argv( 1 ), Cmd_Argv( 2 ), 0 ) )
		Com_Printf( "dev_animation load <cooked.asanim> | source <animation_source/file.json>\n" );
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
static void EditSource( sourceEditor_t &editor, const char *prefix ) {
	if ( editor.read || editor.save ) {
		const bool read = editor.read;
		editor.read = editor.save = false;
		// Restrict source edits to a dedicated loose-file project in fs_homepath.
		bool valid = !strncmp( editor.path, prefix, strlen( prefix ) ) && !strstr( editor.path, ".." ) && COM_CompareExtension( editor.path, ".json" );
		for ( const char *p = editor.path; *p; ++p )
			valid &= ( *p >= 'a' && *p <= 'z' ) || ( *p >= '0' && *p <= '9' ) || *p == '/' || *p == '_' || *p == '-' || *p == '.';
		if ( !valid ) {
			Q_strncpyz( editor.result, "invalid_path", sizeof( editor.result ) );
			Com_sprintf( editor.status, sizeof( editor.status ), "Use a lowercase %s*.json path.", prefix );
			return;
		}
		char current[sizeof( editor.text )];
		if ( !GraphReadSource( editor.path, current, sizeof( current ) ) ) {
			Q_strncpyz( editor.result, "read_failed", sizeof( editor.result ) );
			Q_strncpyz( editor.status, "Source read failed or exceeds 65535 bytes; edits retained.", sizeof( editor.status ) );
			return;
		}
		if ( read ) {
			if ( strcmp( editor.text, editor.saved ) ) {
				Q_strncpyz( editor.result, "unsaved_edits", sizeof( editor.result ) );
				Q_strncpyz( editor.status, "Unsaved edits: save or undo them before loading another source.", sizeof( editor.status ) );
				return;
			}
			Q_strncpyz( editor.text, current, sizeof( editor.text ) );
			Q_strncpyz( editor.saved, current, sizeof( editor.saved ) );
			Q_strncpyz( editor.loadedPath, editor.path, sizeof( editor.loadedPath ) );
			Q_strncpyz( editor.result, "source_loaded", sizeof( editor.result ) );
			Q_strncpyz( editor.status, "Source loaded. Save keeps a numbered backup; the cooker validates JSON.", sizeof( editor.status ) );
		} else {
			if ( strcmp( editor.path, editor.loadedPath ) || strcmp( current, editor.saved ) ) {
				Q_strncpyz( editor.result, "source_changed", sizeof( editor.result ) );
				Q_strncpyz( editor.status, "Source changed outside the editor; save refused, edits retained.", sizeof( editor.status ) );
				return;
			}
			char backup[MAX_QPATH + 16];
			int revision;
			for ( revision = 0; revision < 1000; ++revision ) {
				Com_sprintf( backup, sizeof( backup ), "%s.bak.%03d", editor.path, revision );
				if ( !FS_FileExists( backup ) )
					break;
			}
			if ( revision == 1000 || !GraphWriteSource( backup, current ) ) {
				Q_strncpyz( editor.result, "backup_failed", sizeof( editor.result ) );
				Q_strncpyz( editor.status, "Backup failed or 1000 revisions reached; source unchanged.", sizeof( editor.status ) );
				return;
			}
			if ( !GraphWriteSource( editor.path, editor.text ) || !GraphReadSource( editor.path, current, sizeof( current ) ) || strcmp( current, editor.text ) ) {
				Q_strncpyz( editor.result, "save_failed", sizeof( editor.result ) );
				Com_sprintf( editor.status, sizeof( editor.status ), "Save failed; edits retained, previous source in %s.", backup );
				return;
			}
			Q_strncpyz( editor.saved, editor.text, sizeof( editor.saved ) );
			Q_strncpyz( editor.result, "saved", sizeof( editor.result ) );
			Q_strncpyz( editor.status, "Source saved. Check cooker output for validation and reload status.", sizeof( editor.status ) );
			Com_Printf( "Source saved: %s (backup %s)\n", editor.path, backup );
		}
	}
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
			Q_strncpyz( graph.source.result, "loaded", sizeof( graph.source.result ) );
			Q_strncpyz( graph.source.status, "Graph loaded. Gameplay keeps its map-start revision.", sizeof( graph.source.status ) );
			Com_Printf( "Animation graph loaded: state=%s\n", Anim_StateName( &asset, graph.state.current ) );
		} else {
			Q_strncpyz( graph.source.result, "load_failed", sizeof( graph.source.result ) );
			Q_strncpyz( graph.source.status, "Graph load failed; the previous preview remains available.", sizeof( graph.source.status ) );
		}
	}
	EditSource( graph.source, "animation_source/" );
}
bool DevTools_EffectEditor( const char *action, const char *text ) {
	if ( !strcmp( action, "source" ) || !strcmp( action, "load" ) ) {
		if ( !text[0] || strlen( text ) >= MAX_QPATH )
			return false;
		const bool source = !strcmp( action, "source" );
		char *path = source ? effectSource.path : effectPreview.path;
		if ( path != text )
			Q_strncpyz( path, text, MAX_QPATH );
		if ( source )
			effectSource.read = true;
		else
			effectPreview.load = true;
	} else if ( !strcmp( action, "text" ) ) {
		if ( strlen( text ) >= sizeof( effectSource.text ) )
			return false;
		if ( text != effectSource.text )
			Q_strncpyz( effectSource.text, text, sizeof( effectSource.text ) );
	} else if ( !strcmp( action, "save" ) ) {
		if ( !effectSource.loadedPath[0] || effectSource.read )
			return false;
		effectSource.save = true;
	} else if ( !strcmp( action, "undo" ) )
		Q_strncpyz( effectSource.text, effectSource.saved, sizeof( effectSource.text ) );
	else if ( !strcmp( action, "start" ) )
		effectPreview.start = true;
	else if ( !strcmp( action, "stop" ) )
		effectPreview.stop = true;
	else
		return false;
	return true;
}
static void EditEffects( const refexport_t *renderer ) {
	EditSource( effectSource, "effects_source/" );
	if ( effectPreview.load ) {
		effectPreview.load = false;
		effectPreview.decal = COM_CompareExtension( effectPreview.path, ".asdc" );
		effectPreview.asset = effectPreview.decal ? renderer->RegisterDecal( effectPreview.path ) : renderer->RegisterEffect( effectPreview.path );
		Q_strncpyz( effectSource.result, effectPreview.asset ? "loaded" : "load_failed", sizeof( effectSource.result ) );
	}
	if ( effectPreview.start ) {
		effectPreview.start = false;
		const auto *view = DevTools_View();
		vec3_t origin;
		if ( view && effectPreview.decal ) {
			VectorMA( view->vieworg, 8192, view->viewaxis[0], origin );
			trace_t hit;
			CM_BoxTrace( &hit, view->vieworg, origin, vec3_origin, vec3_origin, 0, CONTENTS_SOLID, qfalse );
			if ( !hit.startsolid && !hit.allsolid && hit.fraction < 1 ) {
				vec3_t axis[3];
				VectorCopy( hit.plane.normal, axis[2] );
				PerpendicularVector( axis[0], axis[2] );
				CrossProduct( axis[2], axis[0], axis[1] );
				effectPreview.instance = renderer->ProjectDecal( effectPreview.asset, hit.endpos, axis );
			}
		} else if ( view && DevTools_EntityAtCamera( origin ) )
			effectPreview.instance = renderer->StartEffect( effectPreview.asset, origin, view->viewaxis, 161 );
	}
	if ( effectPreview.stop ) {
		effectPreview.stop = false;
		if ( effectPreview.decal )
			renderer->ClearDecals();
		else
			renderer->StopEffect( effectPreview.instance );
	}
}
static uint32_t physicsAction;
static bool physicsDebug;
static void InspectPhysics() {
	if ( !BeginPanel( "Physics" ) )
		return;
	const auto stats = Phys_Stats();
	ImGui::Text( "Cosmetic storage: %.2f MiB | allocations %u | live blocks %u", double( stats.used ) / ( 1024 * 1024 ), stats.allocations, stats.liveBlocks );
	ImGui::TextUnformatted( "Props and inert grenades; movement and damage use native collision." );
	if ( ImGui::Button( "Throw box" ) )
		physicsAction = 1;
	ImGui::SameLine();
	if ( ImGui::Button( "Drop grenade" ) )
		physicsAction = 2;
	if ( ImGui::Checkbox( "Show collision bounds", &physicsDebug ) )
		physicsAction = 3;
	ImGui::EndTabItem();
}
static void EditPhysics() {
	if ( physicsAction == 1 )
		Cbuf_AddText( "physics_prop box\n" );
	if ( physicsAction == 2 )
		Cbuf_AddText( "physics_prop grenade drop\n" );
	if ( physicsAction == 3 )
		Cvar_Set( "cg_physicsDebug", physicsDebug ? "1" : "0" );
	physicsAction = 0;
}
static void InspectEffects( const refexport_t *renderer ) {
	if ( !BeginPanel( "Effects" ) )
		return;
	ImGui::SetNextItemWidth( 330 );
	ImGui::InputText( "Effect or decal", effectPreview.path, sizeof( effectPreview.path ) );
	if ( ImGui::Button( "Load asset" ) )
		DevTools_EffectEditor( "load", effectPreview.path );
	ImGui::SameLine();
	if ( ImGui::Button( effectPreview.decal ? "Project at aim" : "Play at camera" ) )
		DevTools_EffectEditor( "start", "" );
	ImGui::SameLine();
	if ( ImGui::Button( effectPreview.decal ? "Clear decals" : "Stop emission" ) )
		DevTools_EffectEditor( "stop", "" );
	fxRenderStats_t stats;
	renderer->EffectStats( &stats );
	ImGui::Text( "Particles %u / %u | instances %u / %u | dropped %" PRIu64,
		stats.pool.particles, FX_MAX_PARTICLES, stats.pool.instances, FX_MAX_INSTANCES, stats.pool.dropped );
	ImGui::Text( "Draws %u | lights %u | dropped lights %u | reloads %u", stats.draws, stats.lightDraws, stats.lightDrops, stats.reloads );
	ImGui::Text( "Soft particle draws %u | upload drops %u", stats.softDraws, stats.softDrops );
	decalRenderStats_t decals;
	renderer->DecalStats( &decals );
	ImGui::Text( "Decals %u / %u | draws %u | drops %u | replaced %" PRIu64, decals.active, DCL_MAX_DECALS, decals.draws, decals.dropped, decals.replaced );
	ImGui::SetNextItemWidth( 330 );
	ImGui::InputText( "Source JSON", effectSource.path, sizeof( effectSource.path ) );
	if ( ImGui::Button( "Load source" ) )
		DevTools_EffectEditor( "source", effectSource.path );
	ImGui::SameLine();
	if ( ImGui::Button( "Save + backup" ) )
		DevTools_EffectEditor( "save", "" );
	ImGui::SameLine();
	if ( ImGui::Button( "Undo edits" ) )
		DevTools_EffectEditor( "undo", "" );
	ImGui::InputTextMultiline( "##Effect JSON", effectSource.text, sizeof( effectSource.text ), ImVec2( -1, 175 ), ImGuiInputTextFlags_AllowTabInput );
	ImGui::TextWrapped( "%s", effectSource.status );
	ImGui::TextWrapped( "Run the cooker watcher for effects_source. Saved definitions reload for new bursts; active particles and decals finish with their original definition." );
	ImGui::EndTabItem();
}
static void InspectGraph( uint32_t elapsed ) {
	const ImGuiTabItemFlags flags = graph.select ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
	graph.select = false;
	if ( !BeginPanel( "Graph", nullptr, flags ) )
		return;
	ImGui::SetNextItemWidth( 370 );
	ImGui::InputText( "##Cooked graph", graph.path, sizeof( graph.path ) );
	ImGui::SameLine();
	if ( ImGui::Button( "Load graph" ) )
		DevTools_Graph( "load", graph.path, 0 );
	if ( ImGui::BeginTabBar( "Graph views" ) ) {
		if ( BeginGraphTab( "Preview" ) ) {
			if ( graph.storage ) {
				ImGui::Text( "State: %s | last event: %s", Anim_StateName( &graph.asset, graph.state.current ), graph.lastEvent );
				if ( ImGui::Checkbox( "Play fixed steps", &graph.play ) )
					DevTools_Graph( "play", "", graph.play ? 1.0f : 0.0f );
				ImGui::SameLine();
				if ( ImGui::Button( "Reset" ) )
					DevTools_Graph( "reset", "", 0 );
				if ( ImGui::BeginChild( "Inputs", ImVec2( 0, 110 ), ImGuiChildFlags_Borders ) ) {
					for ( uint32_t i = 0; i < graph.asset.header.sections[ANIM_PARAMETERS].count; ++i ) {
						const auto parameter = GraphRecord<animFileParameter_t>( ANIM_PARAMETERS, i );
						if ( ImGui::SliderFloat( parameter.name, &graph.parameters[i], parameter.minimum, parameter.maximum ) )
							DevTools_Graph( "parameter", parameter.name, graph.parameters[i] );
					}
				}
				ImGui::EndChild();
				if ( ImGui::SliderFloat( "Yaw", &animation.yaw, -180, 180 ) )
					DevTools_SetAnimation( "yaw", animation.yaw );
				if ( graph.play ) {
					graph.remainder += MIN( elapsed, 250U );
					while ( graph.remainder >= 20 ) {
						graph.time += 20;
						graph.remainder -= 20;
						animEvents_t events;
						if ( !Anim_Tick( &graph.asset, graph.parameters, graph.time, &graph.state, &events ) ) {
							graph.play = false;
							Q_strncpyz( graph.source.result, "tick_failed", sizeof( graph.source.result ) );
							Q_strncpyz( graph.source.status, "Graph tick failed; preview paused.", sizeof( graph.source.status ) );
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
		if ( BeginGraphTab( "Source" ) ) {
			ImGui::SetNextItemWidth( 460 );
			ImGui::InputText( "##Source path", graph.source.path, sizeof( graph.source.path ) );
			if ( ImGui::Button( "Load source" ) )
				DevTools_Graph( "source", graph.source.path, 0 );
			ImGui::SameLine();
			if ( ImGui::Button( "Save + backup" ) )
				DevTools_Graph( "save", "", 0 );
			ImGui::SameLine();
			if ( ImGui::Button( "Undo edits" ) )
				DevTools_Graph( "undo", "", 0 );
			if ( ImGui::InputTextMultiline( "##Graph JSON", graph.source.text, sizeof( graph.source.text ), ImVec2( -1, 210 ), ImGuiInputTextFlags_AllowTabInput ) )
				DevTools_Graph( "text", graph.source.text, 0 );
			ImGui::EndTabItem();
		}
		if ( BeginGraphTab( "Tables" ) ) {
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
	ImGui::TextWrapped( "%s", graph.source.status );
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
	editorRenderer = nullptr;
	DevTools_ClearWorld();
	DevTools_SetView( nullptr );
	if ( context )
		ImGui::DestroyContext( context );
	context = nullptr;
	fontTexture = 0;
	lastTime = 0;
	animation.model = animation.skin = 0;
	effectPreview.asset = 0;
	effectPreview.instance = 0;
	effectPreview.load = effectPreview.start = effectPreview.stop = false;
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

const refexport_t *DevTools_Renderer( void ) {
	return editorRenderer;
}
bool DevTools_SelectAsset( const char *kind, int index ) {
	if ( !editorRenderer )
		return false;
	if ( !strcmp( kind, "images" ) ) {
		devImage_t image;
		if ( !editorRenderer->GetDeveloperImage( index, &image ) )
			return false;
		selectedImage = index;
	} else if ( !strcmp( kind, "materials" ) ) {
		devMaterial_t material;
		if ( !editorRenderer->GetDeveloperMaterial( index, &material ) )
			return false;
		selectedMaterial = index;
	} else if ( !strcmp( kind, "models" ) )
		return DevTools_SetAnimation( "model", (float)index );
	else
		return false;
	return true;
}
bool DevTools_MaterialPreview( int index, bool previewEnabled ) {
	devMaterial_t material;
	if ( !editorRenderer || !editorRenderer->GetDeveloperMaterial( index, &material ) || !material.metallicRoughness )
		return false;
	materialPreview.enabled = previewEnabled;
	if ( previewEnabled ) {
		materialPreview.instance.mask = 63;
		materialPreview.instance.values = material.params;
	}
	return true;
}
bool DevTools_SetMaterial( int index, const materialParams_t *params ) {
	devMaterial_t material;
	if ( !params || !editorRenderer || !editorRenderer->GetDeveloperMaterial( index, &material ) || !material.metallicRoughness || params->flags != material.params.flags )
		return false;
	for ( float value : params->color )
		if ( !( value >= 0 && value <= 1 ) )
			return false;
	for ( float value : params->emissive )
		if ( !( value >= 0 && value <= 1 ) )
			return false;
	if ( !( params->metallic >= 0 && params->metallic <= 1 && params->roughness >= 0 && params->roughness <= 1 &&
			 params->alphaCutoff >= 0 && params->alphaCutoff <= 1 && std::isfinite( params->normalScale ) ) )
		return false;
	if ( materialPreview.enabled ) {
		materialPreview.instance.values = *params;
		return true;
	}
	return editorRenderer->SetDeveloperMaterial( index, params );
}

static void InspectAssets( const refexport_t *renderer ) {
	if ( BeginPanel( "Textures" ) ) {
		auto &filter = filters[1];
		if ( ImGui::InputText( "Filter textures", filter, sizeof( filter ) ) )
			DevTools_Filter( "images", filter );
		if ( ImGui::BeginChild( "Images", ImVec2( 0, 120 ), ImGuiChildFlags_Borders ) ) {
			devImage_t image;
			for ( int i = 0; renderer->GetDeveloperImage( i, &image ); ++i ) {
				if ( *filter && !Q_stristr( image.name, filter ) )
					continue;
				ImGui::PushID( i );
				if ( ImGui::Selectable( image.name, selectedImage == i ) )
					DevTools_SelectAsset( "images", i );
				ImGui::PopID();
			}
		}
		ImGui::EndChild();
		ImagePreview( renderer, selectedImage, 200 );
		ImGui::EndTabItem();
	}
	if ( BeginPanel( "Materials" ) ) {
		auto &filter = filters[2];
		if ( ImGui::InputText( "Filter materials", filter, sizeof( filter ) ) )
			DevTools_Filter( "materials", filter );
		if ( ImGui::BeginChild( "Shaders", ImVec2( 0, 120 ), ImGuiChildFlags_Borders ) ) {
			devMaterial_t material;
			for ( int i = 0; renderer->GetDeveloperMaterial( i, &material ); ++i ) {
				if ( *filter && !Q_stristr( material.name, filter ) )
					continue;
				ImGui::PushID( i );
				if ( ImGui::Selectable( material.name, selectedMaterial == i ) )
					DevTools_SelectAsset( "materials", i );
				ImGui::PopID();
			}
		}
		ImGui::EndChild();
		devMaterial_t material;
		if ( renderer->GetDeveloperMaterial( selectedMaterial, &material ) ) {
			ImGui::TextWrapped( "%s", material.name );
			ImGui::Text( "sort %.2f cull %d surface 0x%x content 0x%x", material.sort,
				material.cull, (uint32_t)material.surfaceFlags, (uint32_t)material.contentFlags );
			ImGui::Text( "%s, %s", material.explicitDefinition ? "script defined" : "implicit",
				material.fallback ? "fallback shader" : "loaded" );
			if ( material.reloads ) {
				ImGui::SameLine();
				ImGui::Text( "reloads %u", material.reloads );
			}
			if ( material.metallicRoughness ) {
				ImGui::TextUnformatted( "Metallic / roughness; edits last until source reload" );
				if ( ImGui::Checkbox( "Override preview instance", &materialPreview.enabled ) )
					DevTools_MaterialPreview( selectedMaterial, materialPreview.enabled );
				if ( materialPreview.enabled )
					material.params = materialPreview.instance.values;
				bool changed = ImGui::ColorEdit4( "Base color / opacity", material.params.color );
				changed |= ImGui::ColorEdit3( "Emissive", material.params.emissive );
				changed |= ImGui::SliderFloat( "Metallic", &material.params.metallic, 0, 1 );
				changed |= ImGui::SliderFloat( "Roughness", &material.params.roughness, 0, 1 );
				changed |= ImGui::SliderFloat( "Normal scale", &material.params.normalScale, 0, 2 );
				changed |= ImGui::SliderFloat( "Mask cutoff", &material.params.alphaCutoff, 0, 1 );
				if ( changed )
					DevTools_SetMaterial( selectedMaterial, &material.params );
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
	if ( !BeginPanel( "Memory" ) )
		return;
	devMemory_t memory;
	Com_DeveloperMemory( &memory );
	ImGui::Text( "Hunk: %d total, %d permanent, %d temporary, %d free", memory.hunkTotal,
		memory.hunkPermanent, memory.hunkTemporary, memory.hunkFree );
	ImGui::TextUnformatted( "Zone tags include block headers; hunk tags identify bank/lifetime regions." );
	ImGui::Text( "Hunk low/high permanent %d / %d; low/high temporary %d / %d", memory.hunkBytes[0], memory.hunkBytes[1], memory.hunkBytes[2], memory.hunkBytes[3] );
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

bool DevTools_SelectPanel( const char *name ) {
	static constexpr const char *panels[] = { "Console", "Cvars", "Textures", "Materials", "Profile", "Memory", "Animation", "Entities", "World", "Graph", "Range", "Effects", "Physics", "Definitions" };
	for ( const char *panel : panels ) {
		if ( !strcmp( name, panel ) ) {
			Q_strncpyz( requestedPanel, panel, sizeof( requestedPanel ) );
			if ( !strcmp( name, "Range" ) )
				weaponRange.visible = true;
			Cvar_Set( "dev_tools", "1" );
			return true;
		}
	}
	return false;
}

bool DevTools_SetWorld( bool collision, bool navigation, bool entitiesVisible, float radius ) {
	if ( !std::isfinite( radius ) || radius < 64 || radius > 2048 )
		return false;
	worldDebug.collision = collision;
	worldDebug.navigation = navigation;
	worldDebug.entities = entitiesVisible;
	worldDebug.radius = radius;
	worldDebug.refresh = true;
	return true;
}

bool DevTools_CaptureWorkspace( devWorkspace_t *workspace ) {
	Q_strncpyz( workspace->panel, activePanel[0] ? activePanel : "Console", sizeof( workspace->panel ) );
	Q_strncpyz( workspace->cvar, selectedCvar, sizeof( workspace->cvar ) );
	memcpy( workspace->filters, filters, sizeof( filters ) );
	workspace->enabled = enabled && enabled->integer ? 1U : 0U;
	workspace->collision = worldDebug.collision ? 1U : 0U;
	workspace->navigation = worldDebug.navigation ? 1U : 0U;
	workspace->entities = worldDebug.entities ? 1U : 0U;
	workspace->radius = worldDebug.radius;
	if ( context ) {
		ImGui::SetCurrentContext( context );
		size_t size;
		const char *layout = ImGui::SaveIniSettingsToMemory( &size );
		if ( size >= sizeof( workspace->layout ) )
			return false;
		memcpy( workspace->layout, layout, size + 1 );
	}
	return true;
}
void DevTools_RestoreWorkspace( const devWorkspace_t &workspace ) {
	if ( !DevTools_SelectPanel( workspace.panel ) )
		DevTools_SelectPanel( "Console" );
	selectedCvar[0] = cvarValue[0] = 0;
	if ( workspace.cvar[0] )
		DevTools_SelectCvar( workspace.cvar );
	memcpy( filters, workspace.filters, sizeof( filters ) );
	DevTools_SetWorld( workspace.collision != 0, workspace.navigation != 0, workspace.entities != 0, workspace.radius );
	Cvar_Set( "dev_tools", workspace.enabled ? "1" : "0" );
	if ( context && workspace.layout[0] ) {
		ImGui::SetCurrentContext( context );
		ImGui::LoadIniSettingsFromMemory( workspace.layout );
	}
}

void DevTools_EditorState( devEditorState_t *state ) {
	*state = {};
	Q_strncpyz( state->panel, activePanel, sizeof( state->panel ) );
	Q_strncpyz( state->cvar, selectedCvar, sizeof( state->cvar ) );
	memcpy( state->filters, filters, sizeof( filters ) );
	Q_strncpyz( state->clip, animation.clipName, sizeof( state->clip ) );
	state->frames = renderedFrames;
	state->allocations = allocations;
	state->arena = Z_DevMemoryUsed();
	state->enabled = enabled && enabled->integer;
	state->inputCaptured = inputCaptured;
	state->lines = drawnLines;
	state->labels = drawnLabels;
	const devLine_t *lines;
	state->worldLines = DevTools_Lines( &lines, true );
	state->collision = worldDebug.collision;
	state->navigation = worldDebug.navigation;
	state->entities = worldDebug.entities;
	state->selectedEntity = entities.selected;
	state->model = animation.model;
	state->animationFrame = animation.frame;
	state->viewport[0] = animation.x;
	state->viewport[1] = animation.y;
	state->viewport[2] = animation.width;
	state->viewport[3] = animation.height;
	state->animationPreviews = animationFrames;
	state->animationPlay = animation.play;
	Q_strncpyz( state->graphState, graph.storage ? Anim_StateName( &graph.asset, graph.state.current ) : "", sizeof( state->graphState ) );
	Q_strncpyz( state->graphEvent, graph.lastEvent, sizeof( state->graphEvent ) );
	Q_strncpyz( state->graphResult, graph.source.result, sizeof( state->graphResult ) );
	Q_strncpyz( state->graphTab, graph.activeTab, sizeof( state->graphTab ) );
	state->graphPreviews = graph.previews;
	state->graphTime = graph.time;
	Q_strncpyz( state->effectResult, effectSource.result, sizeof( state->effectResult ) );
	state->effectDirty = strcmp( effectSource.text, effectSource.saved ) != 0;
	state->graphDirty = strcmp( graph.source.text, graph.source.saved ) != 0;
	state->graphPlay = graph.play;
	state->rangeLoaded = weaponRange.loaded;
	state->rangeAds = weaponRange.ads;
	state->rangeSlot = weaponRange.slot;
	state->rangeAttachments = weaponRange.attachments;
	Q_strncpyz( state->rangeName, weaponRange.loaded ? weaponRange.definition.name : "", sizeof( state->rangeName ) );
}

static void InspectWorld( void ) {
	if ( !BeginPanel( "World" ) )
		return;
	bool refresh = ImGui::Checkbox( "Collision surfaces", &worldDebug.collision );
	refresh |= ImGui::Checkbox( "Navigation areas and routes", &worldDebug.navigation );
	refresh |= ImGui::Checkbox( "Live entity bounds", &worldDebug.entities );
	ImGui::SliderFloat( "Radius", &worldDebug.radius, 64, 2048 );
	refresh |= ImGui::Button( "Refresh around camera" );
	if ( refresh )
		DevTools_SetWorld( worldDebug.collision, worldDebug.navigation, worldDebug.entities, worldDebug.radius );
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

bool DevTools_SelectEntity( int entity ) {
	const auto *game = DevTools_Game();
	devEntity_t info;
	if ( entity != -1 && ( !game || !game->ReadEntity( entity, &info ) ) )
		return false;
	entities.selected = entity;
	return true;
}
bool DevTools_EntityAtCamera( float *origin ) {
	const auto *view = DevTools_View();
	if ( !view )
		return false;
	VectorMA( view->vieworg, 64, view->viewaxis[0], origin );
	return true;
}
int DevTools_PickCrosshair( void ) {
	const auto *view = DevTools_View();
	const int picked = view ? DevTools_PickEntity( (float)view->x + (float)view->width * 0.5f, (float)view->y + (float)view->height * 0.5f ) : -1;
	DevTools_SelectEntity( picked );
	return picked;
}
bool DevTools_ReloadEntities( void ) {
	if ( !DevTools_Game() || !Cvar_VariableIntegerValue( "sv_cheats" ) || !*Cvar_VariableString( "dev_entityFile" ) )
		return false;
	Cvar_Set( "dev_loadEntities", "1" );
	Cbuf_AddText( "map_restart 0\n" );
	return true;
}

static struct {
	char name[64], key[32], value[128], original[128], status[128];
	int action;
} definitionEditor;

static void InspectDefinitions() {
	if ( !BeginPanel( "Definitions" ) )
		return;
	const auto *game = DevTools_Game();
	const auto *data = game && game->Definitions ? game->Definitions() : nullptr;
	if ( !data || !data->header.count ) {
		ImGui::TextWrapped( "Load a cooked entity definition file with g_entityDefinitions on a local devmap." );
		ImGui::EndTabItem();
		return;
	}
	const auto *definition = Entity_FindDefinition( *data, Cvar_VariableString( "dev_definitionName" ) );
	if ( !definition )
		definition = &data->definitions[0];
	if ( ImGui::BeginCombo( "Definition", definition->name ) ) {
		for ( uint32_t i = 0; i < data->header.count; ++i ) {
			if ( ImGui::Selectable( data->definitions[i].name, definition == &data->definitions[i] ) ) {
				definition = &data->definitions[i];
				Cvar_Set( "dev_definitionName", definition->name );
			}
		}
		ImGui::EndCombo();
	}
	ImGui::Text( "Native behavior: %s", definition->native );
	const auto *field = Entity_Field( *data, *definition, Cvar_VariableString( "dev_definitionKey" ) );
	if ( !field && definition->fieldCount )
		field = &data->fields[definition->firstField];
	if ( field && ImGui::BeginCombo( "Component field", field->key ) ) {
		for ( uint32_t i = 0; i < definition->fieldCount; ++i ) {
			const auto *candidate = &data->fields[definition->firstField + i];
			char label[64];
			Com_sprintf( label, sizeof( label ), "%s.%s", candidate->component, candidate->key );
			if ( ImGui::Selectable( label, field == candidate ) ) {
				field = candidate;
				Cvar_Set( "dev_definitionKey", field->key );
			}
		}
		ImGui::EndCombo();
	}
	if ( field ) {
		if ( strcmp( definitionEditor.name, definition->name ) || strcmp( definitionEditor.key, field->key ) || strcmp( definitionEditor.original, field->value ) ) {
			Q_strncpyz( definitionEditor.name, definition->name, sizeof( definitionEditor.name ) );
			Q_strncpyz( definitionEditor.key, field->key, sizeof( definitionEditor.key ) );
			Q_strncpyz( definitionEditor.value, field->value, sizeof( definitionEditor.value ) );
			Q_strncpyz( definitionEditor.original, field->value, sizeof( definitionEditor.original ) );
		}
		ImGui::InputText( "Default value", definitionEditor.value, sizeof( definitionEditor.value ) );
		ImGui::BeginDisabled( !Cvar_VariableIntegerValue( "sv_cheats" ) );
		if ( ImGui::Button( "Apply default" ) )
			definitionEditor.action = 1;
		ImGui::SameLine();
		if ( ImGui::Button( "Save definitions" ) )
			definitionEditor.action = 2;
		ImGui::EndDisabled();
	}
	ImGui::TextWrapped( "%s", definitionEditor.status );
	ImGui::TextWrapped( "Defaults affect new spawns. Save writes a numbered cooked revision; select it with g_entityDefinitions and reload the map. JSON remains the source for prefab inheritance and component structure." );
	ImGui::EndTabItem();
}

static void EditDefinitions() {
	const auto *game = DevTools_Game();
	if ( !definitionEditor.action )
		return;
	const bool success = definitionEditor.action == 2 ? DevTools_SaveDefinitions() : game && game->WriteDefinition && game->WriteDefinition( definitionEditor.name, definitionEditor.key, definitionEditor.value );
	Q_strncpyz( definitionEditor.status, success ? "Definition action completed." : "Action rejected: check value, local cheats and storage.", sizeof( definitionEditor.status ) );
	definitionEditor.action = 0;
}

static void InspectEntities( void ) {
	if ( !BeginPanel( "Entities" ) )
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
				DevTools_SelectEntity( i );
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
	if ( ImGui::Button( "At camera" ) )
		DevTools_EntityAtCamera( entities.origin );
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
		success = DevTools_SelectEntity( game->Spawn( entities.classname, entities.origin ) ) && entities.selected >= 0;
		break;
	case 4:
		success = DevTools_SaveEntities();
		break;
	case 6:
		success = DevTools_PickCrosshair() >= 0;
		break;
	case 5:
		success = DevTools_ReloadEntities();
		break;
	default:
		break;
	}
	Q_strncpyz( entities.status, success ? "Entity action completed." : "Action rejected: check field/class, value, map source, cheats and capacity.", sizeof( entities.status ) );
	Com_Printf( "Developer entity action %d: %s (entity %d)\n", entities.action, success ? "completed" : "rejected", entities.selected );
}

bool DevTools_LoadAnimation( const char *path, const char *skin ) {
	if ( !path[0] || strlen( path ) >= sizeof( animation.path ) || strlen( skin ) >= sizeof( animation.skinPath ) )
		return false;
	if ( path != animation.path )
		Q_strncpyz( animation.path, path, sizeof( animation.path ) );
	if ( skin != animation.skinPath )
		Q_strncpyz( animation.skinPath, skin, sizeof( animation.skinPath ) );
	animation.load = true;
	return true;
}

bool DevTools_SetAnimation( const char *field, float value ) {
	if ( !std::isfinite( value ) || !editorRenderer )
		return false;
	if ( !strcmp( field, "play" ) && ( value == 0 || value == 1 ) )
		animation.play = value != 0;
	else if ( !strcmp( field, "fps" ) && value >= 1 && value <= 60 )
		animation.fps = value;
	else if ( !strcmp( field, "yaw" ) && value >= -180 && value <= 180 )
		animation.yaw = value;
	else if ( !strcmp( field, "model" ) && value >= 1 && value <= 4096 && std::trunc( value ) == value ) {
		devModel_t model;
		if ( !editorRenderer->GetDeveloperModel( (int)value, &model ) || model.frames < 1 )
			return false;
		animation.model = (int)value;
		animation.frame = animation.clip = 0;
		animation.phase = 0;
		Q_strncpyz( animation.path, model.name, sizeof( animation.path ) );
	} else if ( !strcmp( field, "clip" ) && value >= 0 && value <= 4096 && std::trunc( value ) == value ) {
		modelAnimation_t clip;
		if ( !editorRenderer->GetModelAnimation( animation.model, (int)value, &clip ) )
			return false;
		animation.clip = (int)value;
		animation.frame = (int)clip.firstFrame;
		animation.phase = (float)clip.firstFrame;
		animation.fps = clip.framesPerSecond;
	} else if ( !strcmp( field, "frame" ) && value >= 0 && value < 2147483648.0f && std::trunc( value ) == value ) {
		devModel_t model;
		if ( !editorRenderer->GetDeveloperModel( animation.model, &model ) || value >= (float)model.frames )
			return false;
		animation.frame = (int)value;
		animation.play = false;
		animation.phase = value;
	} else
		return false;
	return true;
}

static void InspectAnimation( const refexport_t *renderer, uint32_t elapsed ) {
	animation.draw = false;
	if ( !BeginPanel( "Animation" ) )
		return;
	ImGui::InputText( "Model", animation.path, sizeof( animation.path ) );
	ImGui::InputText( "Skin (optional)", animation.skinPath, sizeof( animation.skinPath ) );
	if ( ImGui::Button( "Load model / skin" ) )
		DevTools_LoadAnimation( animation.path, animation.skinPath );
	if ( ImGui::BeginChild( "Loaded models", ImVec2( 0, 70 ), ImGuiChildFlags_Borders ) ) {
		devModel_t model;
		for ( int i = 1; renderer->GetDeveloperModel( i, &model ); ++i ) {
			if ( model.frames < 1 )
				continue;
			ImGui::PushID( i );
			if ( ImGui::Selectable( model.name, i == animation.model ) ) {
				DevTools_SetAnimation( "model", (float)i );
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
						DevTools_SetAnimation( "clip", (float)i );
						clip = choice;
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
			DevTools_SetAnimation( "frame", (float)animation.frame );
		}
		if ( ImGui::Checkbox( "Play", &animation.play ) )
			DevTools_SetAnimation( "play", animation.play ? 1.0f : 0.0f );
		ImGui::SameLine();
		if ( ImGui::SliderFloat( "FPS", &animation.fps, 1, 60 ) )
			DevTools_SetAnimation( "fps", animation.fps );
		if ( ImGui::SliderFloat( "Yaw", &animation.yaw, -180, 180 ) )
			DevTools_SetAnimation( "yaw", animation.yaw );
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

static void LoadAnimation( const refexport_t *renderer ) {
	if ( animation.load ) {
		animation.load = false;
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
}

static void DrawAnimation( const refexport_t *renderer, int milliseconds ) {
	LoadAnimation( renderer );
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
		if ( !Anim_Evaluate( &graph.asset, &graph.state, graph.parameters, graph.time, &pose ) ||
			 !( materialPreview.enabled ? renderer->AddMaterialEntityToScene( &entity, &materialPreview.instance, &pose, graph.asset.header.modelHash, qfalse ) : renderer->AddSkeletalEntityToScene( &entity, &pose, graph.asset.header.modelHash, qfalse ) ) ) {
			Q_strncpyz( graph.source.status, "Graph/model revision mismatch or pose rejected; reload both together.", sizeof( graph.source.status ) );
			return;
		}
		++graph.previews;
	} else if ( materialPreview.enabled ) {
		renderer->AddMaterialEntityToScene( &entity, &materialPreview.instance, nullptr, nullptr, qfalse );
	} else {
		renderer->AddRefEntityToScene( &entity, qfalse );
	}
	renderer->RenderScene( &view );
	++animationFrames;
}

static void InspectProfile( const refexport_t *renderer, uint32_t milliseconds ) {
	const devNetwork_t *net = DevTools_Network();
	static uint64_t previous[2];
	static uint32_t previousTime;
	static double rate[2];
	const uint32_t interval = milliseconds - previousTime;
	if ( interval >= 1000 ) {
		for ( int i = 0; i < 2; ++i ) {
			rate[i] = (double)( net->bytes[i] >= previous[i] ? net->bytes[i] - previous[i] : net->bytes[i] ) * 1000.0 / (double)interval;
			previous[i] = net->bytes[i];
		}
		previousTime = milliseconds;
	}
	if ( !BeginPanel( "Profile" ) )
		return;
	float history[240];
	uint32_t frames = 0;
	while ( const auto *frame = DevTools_CpuFrame( frames ) ) {
		history[frames++] = float( frame->microseconds ) / 1000.0f;
	}
	ImGui::TextUnformatted( "CPU frame ms (newest first)" );
	ImGui::PlotLines( "##cpu-history", history, (int)frames, 0, nullptr, 0, FLT_MAX, ImVec2( ImGui::GetContentRegionAvail().x, 80 ) );
	if ( frames && ImGui::IsItemHovered() && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
		const float padding = ImGui::GetStyle().FramePadding.x;
		const float fraction = ( ImGui::GetMousePos().x - ImGui::GetItemRectMin().x - padding ) / MAX( 1.0f, ImGui::GetItemRectMax().x - ImGui::GetItemRectMin().x - 2.0f * padding );
		const uint32_t age = MIN( frames - 1, (uint32_t)( MAX( 0.0f, fraction ) * float( frames ) ) );
		DevTools_SelectCpuFrame( DevTools_CpuFrame( age ) );
	}
	if ( ImGui::Button( "Inspect peak" ) )
		DevTools_SelectCpuFrame( DevTools_CpuPeak() );
	ImGui::SameLine();
	if ( ImGui::Button( "Live" ) )
		DevTools_SelectCpuFrame( nullptr );
	ImGui::SameLine();
	if ( ImGui::Button( "Clear history" ) ) {
		DevTools_ClearCpuHistory();
		DevTools_SelectCpuFrame( nullptr );
	}
	const auto *selected = DevTools_CpuSelection();
	const auto *frame = selected ? selected : DevTools_CpuFrame( 0 );
	if ( frame ) {
		ImGui::Text( "%s frame %u: %.3f ms; %u scope drops", selected ? "Selected" : "Latest",
			frame->serial, double( frame->microseconds ) / 1000, frame->dropped );
		ImGui::TextUnformatted( "Scope: inclusive / self ms (click history to retain a frame)" );
		for ( uint32_t i = 0; i < frame->count; ++i ) {
			const auto &scope = frame->scopes[i];
			uint32_t depth = 0;
			for ( uint32_t ancestor = scope.parent; ancestor != UINT32_MAX; ancestor = frame->scopes[ancestor].parent )
				++depth;
			const float indent = float( depth ) * 12.0f;
			if ( depth )
				ImGui::Indent( indent );
			ImGui::Text( "%s: %.3f / %.3f ms", scope.name, double( scope.microseconds ) / 1000, double( scope.selfMicroseconds ) / 1000 );
			if ( depth )
				ImGui::Unindent( indent );
		}
	}
	const auto render = renderer->GetDeveloperStats();
	ImGui::Text( "Draw calls %u; scene triangles %u / surfaces %u / submitted entities %u", render.drawCalls, render.triangles, render.surfaces, render.entities );
	ImGui::Text( "GPU geometry %.1f MiB / staging %.1f MiB", double( render.geometryBytes ) / ( 1024 * 1024 ), double( render.stagingBytes ) / ( 1024 * 1024 ) );
	devGpuTiming_t timings[32];
	const uint32_t count = renderer->GetDeveloperTimings( timings, ARRAY_LEN( timings ) );
	postRenderStats_t post;
	renderer->PostStats( &post );
	ImGui::Text( "Post draws %u / drops %u / profile loads %u", post.draws, post.dropped, post.loads );
	ImGui::Text( "Temporal frames %u / drops %u; motion draws %u / reactive %u", post.temporalFrames, post.temporalDropped, post.motionDraws, post.reactiveDraws );
	ImGui::Text( "History entities %u / matched %u / rejected %u / overflow %u", post.historyStored, post.historyMatched, post.historyRejected, post.historyOverflow );
	textureStreamingStats_t textures;
	renderer->TextureStats( &textures );
	ImGui::Text( "Texture residency %.1f / %.1f MiB; source %.1f / %.1f MiB",
		(double)textures.usedBytes / ( 1024 * 1024 ), (double)textures.budgetBytes / ( 1024 * 1024 ),
		(double)textures.sourceBytes / ( 1024 * 1024 ), (double)textures.sourceBudgetBytes / ( 1024 * 1024 ) );
	ImGui::Text( "Textures %u / full %u; promotions %u / demotions %u / deferred %u / failed %u",
		textures.images, textures.fullResolution, textures.promotions, textures.demotions, textures.deferred, textures.failures );
	ImGui::Text( "Streaming CPU %.3f ms; pending %u; retiring %.1f MiB; reloads %u", (double)textures.cpuUsec / 1000,
		textures.pending, (double)textures.retiredBytes / ( 1024 * 1024 ), textures.reloads );
	ImGui::Text( "Texture upload GPU %.3f ms (%" PRIu64 " samples); %" PRIu64 " submissions / %.1f MiB",
		textures.gpuUsec / 1000, textures.gpuSamples, textures.uploadSubmissions, (double)textures.uploadBytes / ( 1024 * 1024 ) );
	postRenderStats_t presentation;
	renderer->PostStats( &presentation );
	ImGui::Text( "Cumulative presentation CPU ms: effects %.3f / decals %.3f / soft-decal backend %.3f / LOD %.3f",
		(double)presentation.effectsCpuUsec / 1000, (double)presentation.decalsCpuUsec / 1000,
		(double)presentation.effectsDrawCpuUsec / 1000, (double)presentation.lodCpuUsec / 1000 );
	ImGui::TextUnformatted( "Completed GPU frame (no additional wait)" );
	for ( uint32_t i = 0; i < count; ++i )
		ImGui::Text( "%s: %.3f ms", timings[i].name, timings[i].microseconds / 1000.0 );
	if ( !count )
		ImGui::TextUnformatted( "GPU timestamp results unavailable" );
	fxRenderStats_t effects;
	renderer->EffectStats( &effects );
	ImGui::Text( "Effects: %u particles / %u instances; %" PRIu64 " dropped", effects.pool.particles, effects.pool.instances, effects.pool.dropped );
	ImGui::Text( "Effect draws %u / lights %u; light drops %u", effects.draws, effects.lightDraws, effects.lightDrops );
	ImGui::Text( "Soft particle draws %u / upload drops %u", effects.softDraws, effects.softDrops );
	decalRenderStats_t decals;
	renderer->DecalStats( &decals );
	ImGui::Text( "Decals %u / %u | draws %u | drops %u | replaced %" PRIu64, decals.active, DCL_MAX_DECALS, decals.draws, decals.dropped, decals.replaced );
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
	if ( ImGui::CollapsingHeader( "Recent datagrams (newest first)" ) ) {
		for ( uint32_t age = 0; const auto *packet = DevTools_NetworkPacket( age ); ++age )
			ImGui::Text( "%u ms: %s %u bytes", packet->milliseconds, packet->outgoing ? "TX" : "RX", packet->bytes );
	}
	if ( ImGui::CollapsingHeader( "Delta field bandwidth" ) ) {
		const devNetworkField_t *fields;
		const uint32_t fieldCount = DevTools_NetworkFields( &fields );
		for ( uint32_t i = 0; i < fieldCount; ++i ) {
			const auto &field = fields[i];
			if ( field.samples[0] || field.samples[1] )
				ImGui::Text( "%s: read %" PRIu64 " / written %" PRIu64 " bits", field.name, field.bits[0], field.bits[1] );
		}
	}
	if ( ImGui::Button( "Clear network counters" ) )
		DevTools_ClearNetwork();
	ImGui::TextWrapped( "Field totals count compressed delta field/control bits, excluding message headers. Reads include replay; writes include local snapshot serialization." );
	ImGui::TextWrapped( "Datagram payload sizes exclude transport headers; replay snapshots are not network traffic." );
	ImGui::EndTabItem();
}

void DevTools_Draw( const refexport_t *renderer, int width, int height, int milliseconds ) {
	physicsDebug = Cvar_VariableIntegerValue( "cg_physicsDebug" ) != 0;
	editorRenderer = renderer;
	EditWeaponRange();
	EditGraph( renderer );
	EditEffects( renderer );
	LoadAnimation( renderer );
	if ( worldDebug.refresh ) {
		DevTools_RebuildWorld( worldDebug.collision, worldDebug.navigation, worldDebug.radius );
		worldDebug.refresh = false;
	}
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
	animation.draw = false;
	graph.draw = false;
	entities.action = 0;
	if ( !*entities.classname )
		Q_strncpyz( entities.classname, "target_position", sizeof( entities.classname ) );
	if ( animation.fps < 1 )
		animation.fps = 15;
	ImGui::NewFrame();
	static char command[1024];
	auto &filter = filters[0];
	auto &selected = selectedCvar;
	auto &value = cvarValue;
	bool execute = false, apply = false;
	ImGui::SetNextWindowSize( ImVec2( (float)MIN( width - 20, 700 ), (float)MIN( height - 20, 540 ) ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowPos( ImVec2( 10, 10 ), ImGuiCond_FirstUseEver );
	if ( ImGui::Begin( "Aftershock developer tools" ) ) {
		ImGui::Text( "Escape closes | UI arena %zu / 16777216 bytes", Z_DevMemoryUsed() );
		if ( ImGui::BeginTabBar( "Tools" ) ) {
			if ( BeginPanel( "Console" ) ) {
				if ( ImGui::BeginChild( "Output", ImVec2( 0, -38 ), ImGuiChildFlags_Borders ) )
					ImGui::TextUnformatted( DevTools_Console() );
				ImGui::EndChild();
				execute = ImGui::InputText( "Command", command, sizeof( command ), ImGuiInputTextFlags_EnterReturnsTrue );
				ImGui::SameLine();
				execute |= ImGui::Button( "Run" );
				ImGui::EndTabItem();
			}
			if ( BeginPanel( "Cvars" ) ) {
				if ( ImGui::InputText( "Search", filter, sizeof( filter ) ) )
					DevTools_Filter( "cvars", filter );
				if ( ImGui::BeginChild( "Variables", ImVec2( 0, -110 ), ImGuiChildFlags_Borders ) ) {
					for ( const cvar_t *var = Cvar_First(); var; var = var->next ) {
						if ( !var->name || ( *filter && !Q_stristr( var->name, filter ) && ( !var->description || !Q_stristr( var->description, filter ) ) ) )
							continue;
						if ( ImGui::Selectable( var->name, !strcmp( selected, var->name ) ) )
							DevTools_SelectCvar( var->name );
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
			InspectProfile( renderer, (uint32_t)milliseconds );
			InspectMemory();
			InspectAnimation( renderer, elapsed );
			InspectEntities();
			InspectDefinitions();
			InspectWorld();
			InspectGraph( elapsed );
			InspectWeaponRange();
			InspectEffects( renderer );
			InspectPhysics();
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
	EditPhysics();
	EditWeaponRange();
	EditGraph( renderer );
	EditEffects( renderer );
	DrawAnimation( renderer, milliseconds );
	EditEntities();
	EditDefinitions();
	if ( worldDebug.refresh ) {
		DevTools_RebuildWorld( worldDebug.collision, worldDebug.navigation, worldDebug.radius );
		worldDebug.refresh = false;
	}
	if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) && !io.WantCaptureMouse )
		DevTools_SelectEntity( DevTools_PickEntity( io.MousePos.x, io.MousePos.y ) );
	if ( apply && *selected )
		Cvar_Set2( selected, value, qfalse );
	if ( execute && *command ) {
		Cbuf_AddText( command );
		Cbuf_AddText( "\n" );
		command[0] = '\0';
	}
}
