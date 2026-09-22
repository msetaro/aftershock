#include "devtools_public.h"
#include "../qcommon/qcommon_public.h"
#ifndef DEDICATED
#include "../qcommon/keys_public.h"
#endif
#include "../qcommon/json.h"
#include <charconv>
#include <algorithm>
#include <cmath>
#include <inttypes.h>

// Requests and replies are bounded POD data; the inactive channel allocates nothing.
static constexpr uint32_t agentLimit = 524288;
static bool agentActive;
static uint32_t agentFrame, agentSteps, agentStepId;
static int agentTime = 1000, agentDt = 8, agentSeed = 1;

static devAgentInput_t agentInput;
static bool agentCamera;
static vec3_t agentCameraOrigin, agentCameraAngles;
static int64_t agentFrameStart, agentFrameTimes[4096];
static uint32_t agentSamples;
static bool agentSubscribed;
static char agentError[256];
struct agentEvent_t {
	char type[16], detail[256];
	int32_t actor, target, value, time;
	uint32_t frame;
};
static agentEvent_t agentEvents[256];
static uint32_t agentEventCount, agentEventsDropped;
static uint64_t agentHits, agentKills, agentErrors, agentWarnings;

void Dev_AgentEvent( const char *type, int actor, int target, int value, const char *detail ) {
	if ( !agentActive )
		return;
	if ( !strcmp( type, "hit" ) )
		++agentHits;
	if ( !strcmp( type, "kill" ) )
		++agentKills;
	if ( !strcmp( type, "warning" ) )
		++agentWarnings;
	if ( !strcmp( type, "error" ) || !strcmp( type, "assert" ) ) {
		++agentErrors;
		Q_strncpyz( agentError, detail, sizeof( agentError ) );
		if ( agentEventCount == ARRAY_LEN( agentEvents ) )
			DevTools_AgentFlushEvents();
	}
	if ( !agentSubscribed )
		return;
	if ( agentEventCount == ARRAY_LEN( agentEvents ) ) {
		++agentEventsDropped;
		return;
	}
	auto &event = agentEvents[agentEventCount++];
	Q_strncpyz( event.type, type, sizeof( event.type ) );
	Q_strncpyz( event.detail, detail, sizeof( event.detail ) );
	event.actor = actor;
	event.target = target;
	event.value = value;
	event.frame = agentFrame;
	event.time = agentTime;
}


void Dev_AgentAssert( const char *expression, const char *file, int line ) {
	if ( !agentActive )
		return;
	char detail[256];
	snprintf( detail, sizeof( detail ), "%s:%d: %s", file, line, expression );
	Dev_AgentEvent( "assert", -1, -1, line, detail );
	DevTools_AgentFlushEvents();
}

void DevTools_AgentEnable( void ) {
#ifndef NDEBUG
	q_assertReporter = Dev_AgentAssert;
#endif
	agentActive = true;
}
bool DevTools_AgentActive( void ) {
	return agentActive;
}
int DevTools_AgentTime( void ) {
	return agentTime;
}
int DevTools_AgentSeed( void ) {
	return agentSeed;
}


const float *DevTools_AgentCamera( void ) {
	return agentActive && agentCamera ? agentCameraOrigin : nullptr;
}

int Dev_AgentCameraPose( float *origin, float *angles ) {
	if ( !DevTools_AgentCamera() )
		return 0;
	VectorCopy( agentCameraOrigin, origin );
	VectorCopy( agentCameraAngles, angles );
	return 1;
}

void DevTools_AgentView( refdef_t *view ) {
	if ( !DevTools_AgentCamera() || ( view->rdflags & RDF_NOWORLDMODEL ) )
		return;
	VectorCopy( agentCameraOrigin, view->vieworg );
	AnglesToAxis( agentCameraAngles, view->viewaxis );
	const int area = CM_LeafArea( CM_PointLeafnum( agentCameraOrigin ) );
	memset( view->areamask, 0, sizeof( view->areamask ) );
	CM_WriteAreaBits( view->areamask, area );
	for ( auto &bits : view->areamask )
		bits = (byte)~bits;
}

void DevTools_AgentInput( usercmd_t *command, float *viewangles, const int32_t *deltaAngles ) {
	if ( !agentActive || !agentInput.active )
		return;
	command->forwardmove = (int8_t)agentInput.forward;
	command->rightmove = (int8_t)agentInput.right;
	command->upmove = (int8_t)agentInput.up;
	command->buttons = agentInput.buttons;
	if ( agentInput.weapon >= 0 )
		command->weapon = (uint8_t)agentInput.weapon;
	const float angles[3] = { agentInput.pitch, agentInput.yaw, 0 };
	for ( int i = 0; i < 3; ++i ) {
		command->angles[i] = agentInput.raw ? agentInput.angles[i] : ( ANGLE2SHORT( angles[i] ) - deltaAngles[i] ) & 65535;
		viewangles[i] = (float)SHORT2ANGLE( command->angles[i] );
	}
}

struct agentReply_t {
	char *data;
	uint32_t capacity, length;
	bool valid;

	void Text( const char *text ) {
		const size_t size = strlen( text );
		if ( !valid || size >= capacity - length ) {
			valid = false;
			return;
		}
		memcpy( data + length, text, size + 1 );
		length += (uint32_t)size;
	}
	template <typename T>
		requires std::is_integral_v<T>
	void Number( T value ) {
		char text[32];
		const auto result = std::to_chars( text, text + sizeof( text ) - 1, value );
		*result.ptr = 0;
		Text( text );
	}
	void Number( double value ) {
		char text[64];
		if ( std::isfinite( value ) )
			snprintf( text, sizeof( text ), "%.9g", value );
		else
			strcpy( text, "null" );
		Text( text );
	}
	void Vector( const float *value ) {
		Text( "[" );
		for ( int i = 0; i < 3; ++i ) {
			if ( i )
				Text( "," );
			Number( value[i] );
		}
		Text( "]" );
	}
	void String( const char *text ) {
		Text( "\"" );
		for ( const unsigned char *p = (const unsigned char *)text; *p; ++p ) {
			char escaped[7];
			if ( *p < 32 )
				snprintf( escaped, sizeof( escaped ), "\\u%04x", (unsigned)*p );
			else if ( *p == '"' || *p == '\\' ) {
				escaped[0] = '\\';
				escaped[1] = (char)*p;
				escaped[2] = 0;
			} else {
				escaped[0] = (char)*p;
				escaped[1] = 0;
			}
			Text( escaped );
		}
		Text( "\"" );
	}
	bool Error( const char *code, const char *path, const char *hint ) {
		Text( ",\"ok\":false,\"error\":{\"code\":" );
		String( code );
		Text( ",\"path\":" );
		String( path );
		Text( ",\"hint\":" );
		String( hint );
		Text( "}}" );
		return valid;
	}
};

template <typename T>
static bool Agent_Integer( const char *request, const char *end, const char *name, T &value ) {
	const char *p = JSON_ObjectGetNamedValue( request, end, name );
	if ( !p )
		return false;
	const char *stop = JSON_SkipValue( p, end );
	const auto result = std::from_chars( p, stop, value );
	return result.ec == std::errc{} && result.ptr == stop;
}

static bool Agent_Number( const char *request, const char *end, const char *name, float &value, float minimum, float maximum ) {
	const char *p = JSON_ObjectGetNamedValue( request, end, name );
	if ( !p || !( *p == '-' || ( *p >= '0' && *p <= '9' ) ) || JSON_SkipValue( p, end ) - p >= 128 )
		return false;
	const double number = JSON_ValueGetDouble( p, end );
	if ( !std::isfinite( number ) || number < minimum || number > maximum )
		return false;
	value = (float)number;
	return true;
}

static bool Agent_Vector( const char *request, const char *end, const char *name, float *value, float minimum, float maximum, int count = 3 ) {
	const char *p = JSON_ObjectGetNamedValue( request, end, name );
	if ( JSON_ValueGetType( p, end ) != JSONTYPE_ARRAY || JSON_ArrayGetIndex( p, end, nullptr, 0 ) != (uint32_t)count )
		return false;
	for ( int i = 0; i < count; ++i ) {
		const char *element = JSON_ArrayGetValue( p, end, (uint32_t)i );
		if ( !element || !( *element == '-' || ( *element >= '0' && *element <= '9' ) ) || JSON_SkipValue( element, end ) - element >= 128 )
			return false;
		const double number = JSON_ValueGetDouble( element, end );
		if ( !std::isfinite( number ) || number < minimum || number > maximum )
			return false;
		value[i] = (float)number;
	}
	return true;
}

static bool Agent_Bool( const char *request, const char *end, const char *name, bool &value ) {
	const char *p = JSON_ObjectGetNamedValue( request, end, name );
	if ( !p ) {
		value = false;
		return true;
	}
	if ( end - p >= 4 && !memcmp( p, "true", 4 ) ) {
		value = true;
		return true;
	}
	if ( end - p >= 5 && !memcmp( p, "false", 5 ) ) {
		value = false;
		return true;
	}
	return false;
}


static void Agent_State( agentReply_t &reply ) {
	reply.Text( ",\"ok\":true,\"result\":{\"frame\":" );
	reply.Number( agentFrame );
	reply.Text( ",\"time\":" );
	reply.Number( agentTime );
	reply.Text( ",\"player\":" );
#ifndef DEDICATED
	playerState_t player;
	if ( CL_AgentPlayer( &player ) ) {
		reply.Text( "{\"entity\":" );
		reply.Number( player.clientNum );
		reply.Text( ",\"commandTime\":" );
		reply.Number( player.commandTime );
		reply.Text( ",\"origin\":" );
		reply.Vector( player.origin );
		reply.Text( ",\"velocity\":" );
		reply.Vector( player.velocity );
		reply.Text( ",\"angles\":" );
		reply.Vector( player.viewangles );
		reply.Text( ",\"health\":" );
		reply.Number( player.stats[0] );
		reply.Text( ",\"weapon\":" );
		reply.Number( player.weapon );
		reply.Text( ",\"weaponState\":" );
		reply.Number( player.weaponstate );
		reply.Text( ",\"legsAnimation\":" );
		reply.Number( player.legsAnim );
		reply.Text( ",\"torsoAnimation\":" );
		reply.Number( player.torsoAnim );
		reply.Text( "}" );
	} else
#endif
		reply.Text( "null" );
	reply.Text( ",\"camera\":" );
	if ( const auto *view = DevTools_View() ) {
		reply.Text( "{\"origin\":" );
		reply.Vector( view->vieworg );
		reply.Text( ",\"forward\":" );
		reply.Vector( view->viewaxis[0] );
		reply.Text( ",\"fovX\":" );
		reply.Number( view->fov_x );
		reply.Text( ",\"fovY\":" );
		reply.Number( view->fov_y );
		reply.Text( "}" );
	} else
		reply.Text( "null" );
	reply.Text( "}}" );
}

static bool Agent_Cvars( const char *request, const char *end, agentReply_t &reply ) {
	char filter[128] = {};
	const char *p = JSON_ObjectGetNamedValue( request, end, "filter" );
	if ( p && !JSON_ReadString( p, end, filter, sizeof( filter ) ) )
		return reply.Error( "invalid_argument", "$.filter", "Use a name or description substring shorter than 128 bytes." );
	uint32_t offset = 0, limit = 16;
	if ( ( JSON_ObjectGetNamedValue( request, end, "offset" ) && !Agent_Integer( request, end, "offset", offset ) ) || offset > 65535 ||
		 ( JSON_ObjectGetNamedValue( request, end, "limit" ) && !Agent_Integer( request, end, "limit", limit ) ) || limit < 1 || limit > 16 )
		return reply.Error( "invalid_argument", "$", "Use offset 0..65535 and limit 1..16; restart pagination after registering new cvars." );
	reply.Text( ",\"ok\":true,\"result\":{\"items\":[" );
	uint32_t next = 0, count = 0;
	const cvar_t *var = Cvar_First();
	for ( ; var && count < limit; var = var->next, ++next ) {
		if ( next < offset || !var->name || ( *filter && !Q_stristr( var->name, filter ) && ( !var->description || !Q_stristr( var->description, filter ) ) ) )
			continue;
		if ( count++ )
			reply.Text( "," );
		reply.Text( "{\"name\":" );
		reply.String( var->name );
		reply.Text( ",\"value\":" );
		reply.String( var->string );
		reply.Text( ",\"default\":" );
		reply.String( var->resetString ? var->resetString : "" );
		reply.Text( ",\"description\":" );
		reply.String( var->description ? var->description : "" );
		reply.Text( ",\"flags\":" );
		reply.Number( var->flags );
		reply.Text( "}" );
	}
	reply.Text( "],\"next\":" );
	if ( var )
		reply.Number( next );
	else
		reply.Text( "null" );
	reply.Text( "}}" );
	return reply.valid;
}

#ifndef DEDICATED
static bool Agent_GraphTable( const char *request, const char *end, agentReply_t &reply ) {
	char name[32];
	const char *p = JSON_ObjectGetNamedValue( request, end, "section" );
	if ( !JSON_ReadString( p, end, name, sizeof( name ) ) )
		return reply.Error( "invalid_argument", "$.section", "Use parameters/states/transitions/conditions/events/nodes/masks/joints." );
	const char *names[] = { "parameters", "states", "transitions", "conditions", "events", "nodes", "masks", "joints" };
	const animSectionIndex_t sections[] = { ANIM_PARAMETERS, ANIM_STATES, ANIM_TRANSITIONS, ANIM_CONDITIONS, ANIM_EVENTS, ANIM_NODES, ANIM_MASKS, ANIM_JOINTS };
	uint32_t selected = 0;
	while ( selected < ARRAY_LEN( names ) && strcmp( name, names[selected] ) )
		++selected;
	if ( selected == ARRAY_LEN( names ) )
		return reply.Error( "invalid_argument", "$.section", "Use parameters/states/transitions/conditions/events/nodes/masks/joints." );
	const float *parameters;
	const animAsset_t *asset = DevTools_GraphAsset( &parameters );
	if ( !asset )
		return reply.Error( "invalid_state", "$", "Load a cooked graph and step first." );
	uint32_t offset = 0, limit = 16;
	if ( ( JSON_ObjectGetNamedValue( request, end, "offset" ) && !Agent_Integer( request, end, "offset", offset ) ) || offset > 65535 ||
		 ( JSON_ObjectGetNamedValue( request, end, "limit" ) && !Agent_Integer( request, end, "limit", limit ) ) || limit < 1 || limit > 16 )
		return reply.Error( "invalid_argument", "$", "Use offset 0..65535 and limit 1..16." );
	const auto section = sections[selected];
	const uint32_t total = asset->header.sections[section].count;
	reply.Text( ",\"ok\":true,\"result\":{\"total\":" );
	reply.Number( total );
	reply.Text( ",\"items\":[" );
	uint32_t next = offset;
	for ( ; next < total && next - offset < limit; ++next ) {
		if ( next != offset )
			reply.Text( "," );
		reply.Text( "{\"index\":" );
		reply.Number( next );
		switch ( section ) {
		case ANIM_PARAMETERS: {
			const auto row = DevTools_GraphRecord<animFileParameter_t>( asset, section, next );
			reply.Text( ",\"name\":" );
			reply.String( row.name );
			reply.Text( ",\"value\":" );
			reply.Number( parameters[next] );
			reply.Text( ",\"minimum\":" );
			reply.Number( row.minimum );
			reply.Text( ",\"maximum\":" );
			reply.Number( row.maximum );
			break;
		}
		case ANIM_STATES: {
			const auto row = DevTools_GraphRecord<animFileState_t>( asset, section, next );
			reply.Text( ",\"name\":" );
			reply.String( row.name );
			reply.Text( ",\"clip\":" );
			reply.Number( row.clip );
			reply.Text( ",\"flags\":" );
			reply.Number( row.flags );
			reply.Text( ",\"speedQ16\":" );
			reply.Number( row.speedQ16 );
			reply.Text( ",\"firstEvent\":" );
			reply.Number( row.firstEvent );
			reply.Text( ",\"eventCount\":" );
			reply.Number( row.eventCount );
			reply.Text( ",\"node\":" );
			reply.Number( row.node );
			break;
		}
		case ANIM_TRANSITIONS: {
			const auto row = DevTools_GraphRecord<animFileTransition_t>( asset, section, next );
			reply.Text( ",\"from\":" );
			reply.Number( row.from );
			reply.Text( ",\"to\":" );
			reply.Number( row.to );
			reply.Text( ",\"blendMs\":" );
			reply.Number( row.blendMs );
			reply.Text( ",\"firstCondition\":" );
			reply.Number( row.firstCondition );
			reply.Text( ",\"conditionCount\":" );
			reply.Number( row.conditionCount );
			reply.Text( ",\"flags\":" );
			reply.Number( row.flags );
			break;
		}
		case ANIM_CONDITIONS: {
			const auto row = DevTools_GraphRecord<animFileCondition_t>( asset, section, next );
			reply.Text( ",\"parameter\":" );
			reply.Number( row.parameter );
			reply.Text( ",\"operation\":" );
			reply.Number( row.operation );
			reply.Text( ",\"value\":" );
			reply.Number( row.value );
			break;
		}
		case ANIM_EVENTS: {
			const auto row = DevTools_GraphRecord<animFileEvent_t>( asset, section, next );
			reply.Text( ",\"name\":" );
			reply.String( row.name );
			reply.Text( ",\"timeMs\":" );
			reply.Number( row.timeMs );
			reply.Text( ",\"bone\":" );
			reply.Number( row.bone );
			break;
		}
		case ANIM_NODES: {
			const auto row = DevTools_GraphRecord<animFileNode_t>( asset, section, next );
			reply.Text( ",\"name\":" );
			reply.String( row.name );
			reply.Text( ",\"kind\":" );
			reply.Number( row.kind );
			reply.Text( ",\"a\":" );
			reply.Number( row.a );
			reply.Text( ",\"b\":" );
			reply.Number( row.b );
			reply.Text( ",\"reference\":" );
			reply.Number( row.reference );
			reply.Text( ",\"parameter\":" );
			reply.Number( row.parameter );
			reply.Text( ",\"mask\":" );
			reply.Number( row.mask );
			reply.Text( ",\"flags\":" );
			reply.Number( row.flags );
			reply.Text( ",\"weight\":" );
			reply.Number( row.weight );
			break;
		}
		case ANIM_MASKS: {
			const auto row = DevTools_GraphRecord<animFileMask_t>( asset, section, next );
			reply.Text( ",\"name\":" );
			reply.String( row.name );
			reply.Text( ",\"weights\":[" );
			for ( uint32_t joint = 0; joint < asset->header.sections[ANIM_JOINTS].count; ++joint ) {
				if ( joint )
					reply.Text( "," );
				reply.Number( row.weights[joint] );
			}
			reply.Text( "]" );
			break;
		}
		case ANIM_JOINTS: {
			const auto row = DevTools_GraphRecord<animFileJoint_t>( asset, section, next );
			reply.Text( ",\"name\":" );
			reply.String( row.name );
			reply.Text( ",\"parent\":" );
			reply.Number( row.parent );
			break;
		}
		default:
			break;
		}
		reply.Text( "}" );
	}
	reply.Text( "],\"next\":" );
	if ( next < total )
		reply.Number( next );
	else
		reply.Text( "null" );
	reply.Text( "}}" );
	return reply.valid;
}

static void Agent_MaterialParams( agentReply_t &reply, const materialParams_t &params ) {
	reply.Text( "{\"color\":[" );
	for ( int i = 0; i < 4; ++i ) {
		if ( i )
			reply.Text( "," );
		reply.Number( params.color[i] );
	}
	reply.Text( "],\"emissive\":" );
	reply.Vector( params.emissive );
	reply.Text( ",\"metallic\":" );
	reply.Number( params.metallic );
	reply.Text( ",\"roughness\":" );
	reply.Number( params.roughness );
	reply.Text( ",\"normalScale\":" );
	reply.Number( params.normalScale );
	reply.Text( ",\"alphaCutoff\":" );
	reply.Number( params.alphaCutoff );
	reply.Text( ",\"flags\":" );
	reply.Number( params.flags );
	reply.Text( "}" );
}
static bool Agent_Assets( const char *request, const char *end, agentReply_t &reply ) {
	const auto *renderer = DevTools_Renderer();
	if ( !renderer )
		return reply.Error( "invalid_state", "$", "Step a rendered frame first." );
	char kind[32], filter[128];
	const char *p = JSON_ObjectGetNamedValue( request, end, "kind" );
	if ( !JSON_ReadString( p, end, kind, sizeof( kind ) ) || ( strcmp( kind, "images" ) && strcmp( kind, "materials" ) && strcmp( kind, "models" ) ) )
		return reply.Error( "invalid_argument", "$.kind", "Use images/materials/models." );
	p = JSON_ObjectGetNamedValue( request, end, "filter" );
	filter[0] = 0;
	if ( p && !JSON_ReadString( p, end, filter, sizeof( filter ) ) )
		return reply.Error( "invalid_argument", "$.filter", "Use a name substring shorter than 128 bytes." );
	uint32_t offset = 0, limit = 16;
	if ( ( JSON_ObjectGetNamedValue( request, end, "offset" ) && !Agent_Integer( request, end, "offset", offset ) ) || offset > 65535 ||
		 ( JSON_ObjectGetNamedValue( request, end, "limit" ) && !Agent_Integer( request, end, "limit", limit ) ) || limit < 1 || limit > 16 )
		return reply.Error( "invalid_argument", "$", "Use registry offset 0..65535 and page limit 1..16." );
	reply.Text( ",\"ok\":true,\"result\":{\"items\":[" );
	uint32_t count = 0, next = offset;
	bool finished = false;
	for ( ; next <= 65535 && count < limit; ++next ) {
		devImage_t image{};
		devMaterial_t material{};
		devModel_t model{};
		const bool images = !strcmp( kind, "images" ), materials = !strcmp( kind, "materials" );
		const bool found = images ? renderer->GetDeveloperImage( (int)next, &image ) : materials ? renderer->GetDeveloperMaterial( (int)next, &material )
																								 : renderer->GetDeveloperModel( (int)next, &model );
		if ( !found ) {
			finished = true;
			break;
		}
		const char *name = images ? image.name : materials ? material.name
														   : model.name;
		if ( *filter && !Q_stristr( name, filter ) )
			continue;
		if ( count++ )
			reply.Text( "," );
		reply.Text( "{\"index\":" );
		reply.Number( next );
		reply.Text( ",\"name\":" );
		reply.String( name );
		if ( images ) {
			reply.Text( ",\"texture\":" );
			reply.Number( image.texture );
			reply.Text( ",\"width\":" );
			reply.Number( image.width );
			reply.Text( ",\"height\":" );
			reply.Number( image.height );
			reply.Text( ",\"uploadWidth\":" );
			reply.Number( image.uploadWidth );
			reply.Text( ",\"uploadHeight\":" );
			reply.Number( image.uploadHeight );
			reply.Text( ",\"flags\":" );
			reply.Number( image.flags );
			reply.Text( ",\"format\":" );
			reply.Number( image.format );
			reply.Text( ",\"reloads\":" );
			reply.Number( image.reloads );
		} else if ( materials ) {
			reply.Text( ",\"sort\":" );
			reply.Number( material.sort );
			reply.Text( ",\"stages\":" );
			reply.Number( material.stages );
			reply.Text( ",\"cull\":" );
			reply.Number( material.cull );
			reply.Text( ",\"surfaceFlags\":" );
			reply.Number( material.surfaceFlags );
			reply.Text( ",\"contentFlags\":" );
			reply.Number( material.contentFlags );
			reply.Text( ",\"reloads\":" );
			reply.Number( material.reloads );
			reply.Text( ",\"explicitDefinition\":" );
			reply.Text( material.explicitDefinition ? "true" : "false" );
			reply.Text( ",\"fallback\":" );
			reply.Text( material.fallback ? "true" : "false" );
			reply.Text( ",\"metallicRoughness\":" );
			reply.Text( material.metallicRoughness ? "true" : "false" );
			reply.Text( ",\"params\":" );
			Agent_MaterialParams( reply, material.params );
			reply.Text( ",\"stageInfo\":[" );
			for ( int stage = 0; stage < material.stages; ++stage ) {
				if ( stage )
					reply.Text( "," );
				reply.Text( "{\"present\":" );
				reply.Text( material.present[stage] ? "true" : "false" );
				reply.Text( ",\"stateBits\":" );
				reply.Number( material.stateBits[stage] );
				reply.Text( ",\"textures\":[" );
				for ( int texture = 0; texture < 3; ++texture ) {
					if ( texture )
						reply.Text( "," );
					reply.Number( material.textures[stage][texture] );
				}
				reply.Text( "]}" );
			}
			reply.Text( "]" );
		} else {
			reply.Text( ",\"type\":" );
			reply.Number( model.type );
			reply.Text( ",\"frames\":" );
			reply.Number( model.frames );
			reply.Text( ",\"bytes\":" );
			reply.Number( model.bytes );
			reply.Text( ",\"reloads\":" );
			reply.Number( model.reloads );
			reply.Text( ",\"lods\":" );
			reply.Number( model.lods );
			reply.Text( ",\"lodDraws\":[" );
			for ( uint32_t i = 0; i < 4; i++ ) {
				if ( i )
					reply.Text( "," );
				reply.Number( model.lodDraws[i] );
			}
			reply.Text( "]" );
		}
		reply.Text( "}" );
	}
	reply.Text( "],\"next\":" );
	if ( finished || next > 65535 )
		reply.Text( "null" );
	else
		reply.Number( next );
	reply.Text( "}}" );
	return reply.valid;
}

static void Agent_EditorState( agentReply_t &reply ) {
	devEditorState_t state;
	DevTools_EditorState( &state );
	reply.Text( ",\"ok\":true,\"result\":{\"panel\":" );
	reply.String( state.panel );
	reply.Text( ",\"cvar\":" );
	reply.String( state.cvar );
	reply.Text( ",\"filters\":{\"cvars\":" );
	reply.String( state.filters[0] );
	reply.Text( ",\"images\":" );
	reply.String( state.filters[1] );
	reply.Text( ",\"materials\":" );
	reply.String( state.filters[2] );
	reply.Text( "}" );
	reply.Text( ",\"entity\":" );
	reply.Number( state.selectedEntity );
	reply.Text( ",\"frames\":" );
	reply.Number( state.frames );
	reply.Text( ",\"allocations\":" );
	reply.Number( state.allocations );
	reply.Text( ",\"arena\":" );
	reply.Number( state.arena );
	reply.Text( ",\"enabled\":" );
	reply.Text( state.enabled ? "true" : "false" );
	reply.Text( ",\"inputCaptured\":" );
	reply.Text( state.inputCaptured ? "true" : "false" );
	reply.Text( ",\"lines\":" );
	reply.Number( state.lines );
	reply.Text( ",\"labels\":" );
	reply.Number( state.labels );
	reply.Text( ",\"world\":{\"lines\":" );
	reply.Number( state.worldLines );
	reply.Text( ",\"collision\":" );
	reply.Text( state.collision ? "true" : "false" );
	reply.Text( ",\"navigation\":" );
	reply.Text( state.navigation ? "true" : "false" );
	reply.Text( ",\"entities\":" );
	reply.Text( state.entities ? "true" : "false" );
	reply.Text( "},\"animation\":{\"model\":" );
	reply.Number( state.model );
	reply.Text( ",\"frame\":" );
	reply.Number( state.animationFrame );
	reply.Text( ",\"viewport\":[" );
	for ( int i = 0; i < 4; ++i ) {
		if ( i )
			reply.Text( "," );
		reply.Number( state.viewport[i] );
	}
	reply.Text( "]" );
	reply.Text( ",\"previews\":" );
	reply.Number( state.animationPreviews );
	reply.Text( ",\"play\":" );
	reply.Text( state.animationPlay ? "true" : "false" );
	reply.Text( ",\"clip\":" );
	reply.String( state.clip );
	reply.Text( "},\"graph\":{\"state\":" );
	reply.String( state.graphState );
	reply.Text( ",\"event\":" );
	reply.String( state.graphEvent );
	reply.Text( ",\"result\":" );
	reply.String( state.graphResult );
	reply.Text( ",\"tab\":" );
	reply.String( state.graphTab );
	reply.Text( ",\"previews\":" );
	reply.Number( state.graphPreviews );
	reply.Text( ",\"time\":" );
	reply.Number( state.graphTime );
	reply.Text( ",\"dirty\":" );
	reply.Text( state.graphDirty ? "true" : "false" );
	reply.Text( ",\"play\":" );
	reply.Text( state.graphPlay ? "true" : "false" );
	reply.Text( "},\"effects\":{\"result\":" );
	reply.String( state.effectResult );
	reply.Text( ",\"dirty\":" );
	reply.Text( state.effectDirty ? "true" : "false" );
	reply.Text( "},\"range\":{\"loaded\":" );
	reply.Text( state.rangeLoaded ? "true" : "false" );
	reply.Text( ",\"ads\":" );
	reply.Text( state.rangeAds ? "true" : "false" );
	reply.Text( ",\"slot\":" );
	reply.Number( state.rangeSlot );
	reply.Text( ",\"attachments\":" );
	reply.Number( state.rangeAttachments );
	reply.Text( ",\"name\":" );
	reply.String( state.rangeName );
	reply.Text( "}}}" );
}
#endif

static void Agent_Memory( agentReply_t &reply ) {
	devMemory_t memory;
	Com_DeveloperMemory( &memory );
	reply.Text( "{\"tags\":[" );
	for ( int tag = 0; tag < TAG_COUNT; ++tag ) {
		if ( tag )
			reply.Text( "," );
		reply.Text( "{\"name\":" );
		reply.String( memory.names[tag] );
		reply.Text( ",\"bytes\":" );
		reply.Number( memory.bytes[tag] );
		reply.Text( ",\"blocks\":" );
		reply.Number( memory.blocks[tag] );
		reply.Text( "}" );
	}
	reply.Text( "]" );
	reply.Text( ",\"hunkTotal\":" );
	reply.Number( memory.hunkTotal );
	reply.Text( ",\"hunkPermanent\":" );
	reply.Number( memory.hunkPermanent );
	reply.Text( ",\"hunkTemporary\":" );
	reply.Number( memory.hunkTemporary );
	reply.Text( ",\"hunkFree\":" );
	reply.Number( memory.hunkFree );
	reply.Text( "}" );
}

static void Agent_Profile( agentReply_t &reply ) {
	int64_t sorted[ARRAY_LEN( agentFrameTimes )];
	const uint32_t count = MIN( agentSamples, (uint32_t)ARRAY_LEN( sorted ) );
	memcpy( sorted, agentFrameTimes, count * sizeof( sorted[0] ) );
	std::sort( sorted, sorted + count );
	reply.Text( ",\"ok\":true,\"result\":{\"samples\":" );
	reply.Number( count );
	reply.Text( ",\"p50_ms\":" );
	reply.Number( count ? double( sorted[( count - 1 ) * 50 / 100] ) / 1000 : 0 );
	reply.Text( ",\"p95_ms\":" );
	reply.Number( count ? double( sorted[( count - 1 ) * 95 / 100] ) / 1000 : 0 );
	reply.Text( ",\"p99_ms\":" );
	reply.Number( count ? double( sorted[( count - 1 ) * 99 / 100] ) / 1000 : 0 );
	reply.Text( ",\"cpu\":[" );
	const devCpuTiming_t *cpu;
	const uint32_t scopes = DevTools_CpuTimings( &cpu );
	for ( uint32_t i = 0; i < scopes; ++i ) {
		if ( i )
			reply.Text( "," );
		reply.Text( "{\"name\":" );
		reply.String( cpu[i].name );
		reply.Text( ",\"milliseconds\":" );
		reply.Number( double( cpu[i].microseconds ) / 1000 );
		reply.Text( "}" );
	}
	reply.Text( "],\"gpu\":[" );
#ifndef DEDICATED
	if ( const auto *renderer = DevTools_Renderer() ) {
		devGpuTiming_t timings[32];
		const uint32_t gpuCount = renderer->GetDeveloperTimings( timings, ARRAY_LEN( timings ) );
		for ( uint32_t i = 0; i < gpuCount; ++i ) {
			if ( i )
				reply.Text( "," );
			reply.Text( "{\"name\":" );
			reply.String( timings[i].name );
			reply.Text( ",\"ms\":" );
			reply.Number( timings[i].microseconds / 1000 );
			reply.Text( "}" );
		}
	}
#endif
	reply.Text( "],\"memory\":" );
	Agent_Memory( reply );
#ifndef DEDICATED
	if ( const auto *renderer = DevTools_Renderer() ) {
		postRenderStats_t post;
		renderer->PostStats( &post );
		char status[384];
		snprintf( status, sizeof( status ), ",\"presentationCpuUsec\":{\"effects\":%" PRIu64 ",\"decals\":%" PRIu64 ",\"effectsDraw\":%" PRIu64 ",\"lod\":%" PRIu64 "}",
			post.effectsCpuUsec, post.decalsCpuUsec, post.effectsDrawCpuUsec, post.lodCpuUsec );
		reply.Text( status );
		snprintf( status, sizeof( status ), ",\"post\":{\"loads\":%u,\"draws\":%u,\"dropped\":%u}", post.loads, post.draws, post.dropped );
		reply.Text( status );
		snprintf( status, sizeof( status ), ",\"temporal\":{\"frames\":%u,\"dropped\":%u,\"motionDraws\":%u,\"reactiveDraws\":%u,\"stored\":%u,\"matched\":%u,\"rejected\":%u,\"overflow\":%u}",
			post.temporalFrames, post.temporalDropped, post.motionDraws, post.reactiveDraws,
			post.historyStored, post.historyMatched, post.historyRejected, post.historyOverflow );
		reply.Text( status );
		textureStreamingStats_t textures;
		renderer->TextureStats( &textures );
		char residency[1024];
		snprintf( residency, sizeof( residency ),
			",\"textureStreaming\":{\"budgetBytes\":%" PRIu64 ",\"usedBytes\":%" PRIu64 ",\"peakBytes\":%" PRIu64 ",\"retiredBytes\":%" PRIu64
			",\"sourceBytes\":%" PRIu64 ",\"sourceBudgetBytes\":%" PRIu64 ",\"cpuUsec\":%" PRIu64 ",\"cpuPeakUsec\":%" PRIu64
			",\"gpuUsec\":%.6f,\"gpuSamples\":%" PRIu64 ",\"uploadSubmissions\":%" PRIu64 ",\"uploadBytes\":%" PRIu64
			",\"images\":%u,\"fullResolution\":%u,\"promotions\":%u,\"demotions\":%u,\"deferred\":%u,\"failures\":%u,\"pending\":%u,\"reloads\":%u}",
			textures.budgetBytes, textures.usedBytes, textures.peakBytes, textures.retiredBytes, textures.sourceBytes, textures.sourceBudgetBytes,
			textures.cpuUsec, textures.cpuPeakUsec, textures.gpuUsec, textures.gpuSamples, textures.uploadSubmissions, textures.uploadBytes, textures.images, textures.fullResolution, textures.promotions, textures.demotions, textures.deferred, textures.failures, textures.pending, textures.reloads );
		reply.Text( residency );
	}
#endif
	reply.Text( ",\"network\":{" );
	const auto *net = DevTools_Network();
	char counters[512];
	snprintf( counters, sizeof( counters ), "\"snapshots\":%" PRIu64 ",\"predictions\":%" PRIu64 ",\"rewindReports\":%" PRIu64 ",\"rewindHits\":%" PRIu64 ",\"incomingBytes\":%" PRIu64 ",\"outgoingBytes\":%" PRIu64,
		net->snapshots, net->predictions, net->rewindReports, net->rewindHits, net->bytes[0], net->bytes[1] );
	reply.Text( counters );
	reply.Text( ",\"predictionError\":" );
	reply.Number( net->predictionError );
	reply.Text( ",\"predictionPeak\":" );
	reply.Number( net->predictionPeak );
	reply.Text( ",\"snapshotBits\":" );
	reply.Number( net->snapshotBits );
	reply.Text( ",\"predictionSum\":" );
	reply.Number( net->predictionSum );
	reply.Text( ",\"rewindClamped\":" );
	reply.Number( net->rewindClamped );
	reply.Text( ",\"rewindAge\":" );
	reply.Number( net->rewindAge );
	reply.Text( ",\"rewindLimit\":" );
	reply.Number( net->rewindLimit );
	reply.Text( ",\"incomingPackets\":" );
	reply.Number( net->packets[0] );
	reply.Text( ",\"incomingLastPacket\":" );
	reply.Number( net->lastPacket[0] );
	reply.Text( ",\"outgoingPackets\":" );
	reply.Number( net->packets[1] );
	reply.Text( ",\"outgoingLastPacket\":" );
	reply.Number( net->lastPacket[1] );
	reply.Text( ",\"delta\":" );
	reply.Text( net->delta ? "true" : "false" );
	reply.Text( "},\"events\":{" );
	snprintf( counters, sizeof( counters ), "\"hits\":%" PRIu64 ",\"kills\":%" PRIu64 ",\"errors\":%" PRIu64 ",\"warnings\":%" PRIu64, agentHits, agentKills, agentErrors, agentWarnings );
	reply.Text( counters );
	reply.Text( "}}}" );
}

static void Agent_AnimationState( agentReply_t &reply, const animState_t &state, const char *name ) {
	reply.Text( "{\"state\":" );
	reply.String( name );
	reply.Text( ",\"current\":" );
	reply.Number( state.current );
	reply.Text( ",\"previous\":" );
	reply.Number( state.previous );
	reply.Text( ",\"entered\":" );
	reply.Number( state.entered );
	reply.Text( ",\"previousEntered\":" );
	reply.Number( state.previousEntered );
	reply.Text( ",\"blendStarted\":" );
	reply.Number( state.blendStarted );
	reply.Text( ",\"blendDuration\":" );
	reply.Number( state.blendDuration );
	reply.Text( ",\"lastTime\":" );
	reply.Number( state.lastTime );
	reply.Text( ",\"eventSequence\":" );
	reply.Number( state.eventSequence );
	reply.Text( ",\"initialized\":" );
	reply.Number( state.initialized );
	reply.Text( "}" );
}
static void Agent_Actor( agentReply_t &reply, int owner ) {
	const auto *game = DevTools_Game();
	reply.Text( ",\"ok\":true,\"result\":{\"owner\":" );
	reply.Number( owner );
	reply.Text( ",\"authority\":\"server\",\"weapons\":[" );
	for ( int hand = 0; hand < 2; ++hand ) {
		if ( hand )
			reply.Text( "," );
		devWeaponState_t weapon;
		if ( !game || !game->ReadWeapon || !game->ReadWeapon( owner, hand, &weapon ) ) {
			reply.Text( "null" );
			continue;
		}
		reply.Text( "{\"name\":" );
		reply.String( weapon.name );
		reply.Text( ",\"selected\":" );
		reply.Number( weapon.selected );
		reply.Text( ",\"attachments\":" );
		reply.Number( weapon.attachments );
		reply.Text( ",\"time\":" );
		reply.Number( weapon.state.time );
		reply.Text( ",\"random\":" );
		reply.Number( weapon.state.random );
		reply.Text( ",\"sequence\":" );
		reply.Number( weapon.state.sequence );
		reply.Text( ",\"nextFire\":" );
		reply.Number( weapon.state.nextFire );
		reply.Text( ",\"magazine\":" );
		reply.Number( weapon.state.magazine );
		reply.Text( ",\"reserve\":" );
		reply.Number( weapon.state.reserve );
		reply.Text( ",\"chamber\":" );
		reply.Number( weapon.state.chamber );
		reply.Text( ",\"previousButtons\":" );
		reply.Number( weapon.state.previousButtons );
		reply.Text( ",\"reloadStage\":" );
		reply.Number( weapon.state.reloadStage );
		reply.Text( ",\"reloadStart\":" );
		reply.Number( weapon.state.reloadStart );
		reply.Text( ",\"burstRemaining\":" );
		reply.Number( weapon.state.burstRemaining );
		reply.Text( ",\"adsQ16\":" );
		reply.Number( weapon.state.adsQ16 );
		reply.Text( ",\"nextMelee\":" );
		reply.Number( weapon.state.nextMelee );
		reply.Text( ",\"switchUntil\":" );
		reply.Number( weapon.state.switchUntil );
		reply.Text( ",\"animation\":" );
		Agent_AnimationState( reply, weapon.animation, weapon.animationName );
		reply.Text( "}" );
	}
	reply.Text( "],\"animation\":[" );
	for ( int rig = 0; rig < 2; ++rig ) {
		if ( rig )
			reply.Text( "," );
		devAnimationState_t animation;
		if ( !game || !game->ReadAnimation || !game->ReadAnimation( owner, rig, &animation ) )
			reply.Text( "null" );
		else
			Agent_AnimationState( reply, animation.state, animation.name );
	}
	reply.Text( "]}}" );
}

static bool Agent_Entity( const char *op, const char *request, const char *end, agentReply_t &reply ) {
	const devGameTools_t *tools = DevTools_Game();
	if ( !tools )
		return reply.Error( "invalid_state", "$", "Load a local native developer map first." );
	// Mutation responses have bounded fields. Reserve before calling into the game.
	if ( reply.capacity < 1024 )
		return false;
	if ( !strcmp( op, "entity.list" ) ) {
		uint32_t offset = 0, limit = 32;
		if ( ( JSON_ObjectGetNamedValue( request, end, "offset" ) && !Agent_Integer( request, end, "offset", offset ) ) || offset >= MAX_GENTITIES )
			return reply.Error( "invalid_argument", "$.offset", "Use an entity offset from 0 to 1023." );
		if ( ( JSON_ObjectGetNamedValue( request, end, "limit" ) && !Agent_Integer( request, end, "limit", limit ) ) || limit < 1 || limit > 32 )
			return reply.Error( "invalid_argument", "$.limit", "Request 1 to 32 entities per page." );
		reply.Text( ",\"ok\":true,\"result\":{\"entities\":[" );
		uint32_t count = 0, next = offset;
		for ( ; next < MAX_GENTITIES && count < limit; ++next ) {
			devEntity_t entity;
			if ( !tools->ReadEntity( (int)next, &entity ) )
				continue;
			if ( count++ )
				reply.Text( "," );
			reply.Text( "{\"entity\":" );
			reply.Number( next );
			reply.Text( ",\"classname\":" );
			reply.String( entity.classname );
			reply.Text( ",\"origin\":" );
			reply.Vector( entity.origin );
			reply.Text( ",\"mins\":" );
			reply.Vector( entity.mins );
			reply.Text( ",\"maxs\":" );
			reply.Vector( entity.maxs );
			reply.Text( ",\"health\":" );
			reply.Number( entity.health );
			reply.Text( ",\"model\":" );
			reply.Number( entity.model );
			reply.Text( ",\"frame\":" );
			reply.Number( entity.frame );
			reply.Text( ",\"sound\":" );
			reply.Number( entity.sound );
			reply.Text( ",\"contents\":" );
			reply.Number( entity.contents );
			reply.Text( ",\"source\":" );
			reply.Number( entity.source );
			reply.Text( ",\"linked\":" );
			reply.Text( entity.linked ? "true" : "false" );
			reply.Text( "}" );
		}
		reply.Text( "],\"next\":" );
		if ( next < MAX_GENTITIES )
			reply.Number( next );
		else
			reply.Text( "null" );
		reply.Text( "}}" );
		return reply.valid;
	}
#ifndef DEDICATED
	if ( !strcmp( op, "entity.at_camera" ) ) {
		float origin[3];
		if ( !DevTools_EntityAtCamera( origin ) )
			return reply.Error( "invalid_state", "$", "Step a local map to establish a camera." );
		reply.Text( ",\"ok\":true,\"result\":{\"origin\":" );
		reply.Vector( origin );
		reply.Text( "}}" );
		return reply.valid;
	}
	if ( !strcmp( op, "entity.pick" ) ) {
		const int entity = DevTools_PickCrosshair();
		reply.Text( ",\"ok\":true,\"result\":{\"entity\":" );
		reply.Number( entity );
		reply.Text( "}}" );
		return reply.valid;
	}
	if ( !strcmp( op, "entity.reload" ) ) {
		if ( !DevTools_ReloadEntities() )
			return reply.Error( "rejected", "$", "Save entities in a local developer map before reloading." );
		reply.Text( ",\"ok\":true,\"result\":{\"accepted\":true}}" );
		return reply.valid;
	}
#endif
	if ( !strcmp( op, "entity.spawn" ) ) {
		char classname[64];
		const char *p = JSON_ObjectGetNamedValue( request, end, "classname" );
		float origin[3];
		if ( !JSON_ReadString( p, end, classname, sizeof( classname ) ) || !classname[0] )
			return reply.Error( "invalid_argument", "$.classname", "Use a supported point-entity class shorter than 64 bytes." );
		if ( !Agent_Number( request, end, "x", origin[0], -32752, 32752 ) || !Agent_Number( request, end, "y", origin[1], -32752, 32752 ) || !Agent_Number( request, end, "z", origin[2], -32752, 32752 ) )
			return reply.Error( "invalid_argument", "$", "Provide finite x, y and z coordinates within [-32752,32752]." );
		const int entity = tools->Spawn( classname, origin );
		if ( entity < 0 )
			return reply.Error( "rejected", "$.classname", "Check local cheats, supported spawn classes and entity capacity." );
#ifndef DEDICATED
		DevTools_SelectEntity( entity );
#endif
		reply.Text( ",\"ok\":true,\"result\":{\"entity\":" );
		reply.Number( entity );
		reply.Text( "}}" );
		return reply.valid;
	}
	if ( !strcmp( op, "entity.save" ) ) {
		if ( !DevTools_SaveEntities() )
			return reply.Error( "rejected", "$", "Check local cheats, source retention and writable home directory." );
		reply.Text( ",\"ok\":true,\"result\":{\"path\":" );
		reply.String( Cvar_VariableString( "dev_entityFile" ) );
		reply.Text( "}}" );
		return reply.valid;
	}
	uint32_t entity;
	if ( !Agent_Integer( request, end, "entity", entity ) || entity >= MAX_GENTITIES )
		return reply.Error( "invalid_argument", "$.entity", "Use an entity id from entity.list." );
#ifndef DEDICATED
	if ( !strcmp( op, "entity.select" ) ) {
		if ( !DevTools_SelectEntity( (int)entity ) )
			return reply.Error( "not_found", "$.entity", "Select a live entity id from entity.list." );
		reply.Text( ",\"ok\":true,\"result\":{\"accepted\":true}}" );
		return reply.valid;
	}
#endif
	if ( !strcmp( op, "entity.delete" ) ) {
		if ( !tools->Delete( (int)entity ) )
			return reply.Error( "rejected", "$.entity", "Delete a mutable map entity in a local developer map." );
	} else if ( !strcmp( op, "entity.get" ) || !strcmp( op, "entity.set" ) ) {
		char key[128], value[1024];
		const char *p = JSON_ObjectGetNamedValue( request, end, "key" );
		if ( !JSON_ReadString( p, end, key, sizeof( key ) ) )
			return reply.Error( "invalid_argument", "$.key", "Provide the name of an editable entity field." );
		if ( !strcmp( op, "entity.get" ) ) {
			if ( !tools->ReadField( (int)entity, key, value, sizeof( value ) ) )
				return reply.Error( "not_found", "$.key", "Choose an existing entity and supported field." );
			reply.Text( ",\"ok\":true,\"result\":{\"value\":" );
			reply.String( value );
			reply.Text( "}}" );
			return reply.valid;
		}
		p = JSON_ObjectGetNamedValue( request, end, "value" );
		if ( !JSON_ReadString( p, end, value, sizeof( value ) ) )
			return reply.Error( "invalid_argument", "$.value", "Provide a string shorter than 1024 bytes." );
		if ( !tools->WriteField( (int)entity, key, value ) )
			return reply.Error( "rejected", "$.value", "Check field type, entity mutability and local cheats." );
	} else
		return reply.Error( "unknown_operation", "$.op", "Use an entity operation from hello." );
	reply.Text( ",\"ok\":true,\"result\":{\"accepted\":true}}" );
	return reply.valid;
}

bool DevTools_AgentRequest( const char *request, uint32_t length, char *response, uint32_t capacity ) {
	if ( !response || capacity < 2 )
		return false;
	response[0] = 0;
	agentReply_t reply{ response, capacity, 0, true };
	reply.Text( "{\"id\":" );
	if ( !request || !length || length >= agentLimit ) {
		reply.Text( "null" );
		return reply.Error( "invalid_request", "$", "Send one JSON object shorter than 524288 bytes." );
	}
	const char *end = request + length;
	JSON_Whitespace( request, end );
	const char *p = request;
	if ( !JSON_ValidateObject( request, end ) ) {
		reply.Text( "null" );
		return reply.Error( "invalid_request", "$", "Send a JSON object with valid strings and at most 16 nested containers." );
	}
	const char *id = JSON_ObjectGetNamedValue( request, end, "id" );
	uint32_t sequence = 0;
	const char *idEnd = id ? JSON_SkipValue( id, end ) : end;
	const auto parsedId = id ? std::from_chars( id, idEnd, sequence ) : std::from_chars_result{};
	if ( !id || parsedId.ec != std::errc{} || parsedId.ptr != idEnd ) {
		reply.Text( "null" );
		return reply.Error( "invalid_argument", "$.id", "Use an unsigned 32-bit integer request id." );
	}
	char integer[16];
	snprintf( integer, sizeof( integer ), "%u", sequence );
	reply.Text( integer );
	char op[64], name[256], value[4096];
	p = JSON_ObjectGetNamedValue( request, end, "op" );
	if ( !JSON_ReadString( p, end, op, sizeof( op ) ) )
		return reply.Error( "invalid_argument", "$.op", "Use a command name from hello." );
	if ( !strcmp( op, "hello" ) ) {
		reply.Text( ",\"ok\":true,\"result\":{\"protocol\":1,\"commands\":[\"hello\",\"effects\",\"effects.load\",\"effects.start\",\"effects.stop\",\"effects.edit\",\"decals\",\"decals.load\",\"decals.project\",\"decals.clear\",\"exec\",\"cvar.get\",\"cvar.set\",\"cvar.list\",\"cvar.select\",\"editor.filter\",\"session\",\"step\",\"map\",\"state\",\"input\",\"key\",\"usercmd\",\"camera\",\"panel\",\"world\",\"trace\",\"editor.state\",\"animation.load\",\"animation.set\",\"graph\",\"graph.table\",\"range\",\"actor\",\"assets\",\"asset.select\",\"material.set\",\"material.preview\",\"profile\",\"subscribe\",\"capture\",\"entity.list\",\"entity.pick\",\"entity.select\",\"entity.at_camera\",\"entity.reload\",\"entity.spawn\",\"entity.get\",\"entity.set\",\"entity.delete\",\"entity.save\"]}}" );
	} else if ( !strcmp( op, "decals" ) || !strcmp( op, "decals.load" ) || !strcmp( op, "decals.project" ) || !strcmp( op, "decals.clear" ) ) {
#ifdef DEDICATED
		return reply.Error( "unsupported", "$", "Projected decals require a client build." );
#else
		const refexport_t *renderer = DevTools_Renderer();
		if ( !renderer )
			return reply.Error( "invalid_state", "$", "Load a rendered map before controlling decals." );
		uint32_t handle = 0;
		if ( !strcmp( op, "decals.load" ) ) {
			p = JSON_ObjectGetNamedValue( request, end, "path" );
			if ( !JSON_ReadString( p, end, name, MAX_QPATH ) )
				return reply.Error( "invalid_argument", "$.path", "Use a cooked .asdc qpath." );
			handle = (uint32_t)renderer->RegisterDecal( name );
			if ( !handle )
				return reply.Error( "load_failed", "$.path", "Use a cooked decal with available color and normal textures." );
		} else if ( !strcmp( op, "decals.project" ) ) {
			uint32_t asset;
			vec3_t origin, angles, axis[3];
			if ( !Agent_Integer( request, end, "asset", asset ) || asset > INT32_MAX ||
				 !Agent_Vector( request, end, "origin", origin, -65536, 65536 ) || !Agent_Vector( request, end, "angles", angles, -360, 360 ) )
				return reply.Error( "invalid_argument", "$", "Use an asset handle, origin and projection angles; local Z is the surface normal." );
			AnglesToAxis( angles, axis );
			handle = renderer->ProjectDecal( (qhandle_t)asset, origin, axis );
			if ( !handle )
				return reply.Error( "project_failed", "$.asset", "Use a loaded decal and finite projection axes." );
		} else {
			if ( !strcmp( op, "decals.clear" ) )
				renderer->ClearDecals();
			decalRenderStats_t stats;
			renderer->DecalStats( &stats );
			reply.Text( ",\"ok\":true,\"result\":{\"active\":" );
			reply.Number( stats.active );
			reply.Text( ",\"registered\":" );
			reply.Number( stats.registered );
			reply.Text( ",\"reloads\":" );
			reply.Number( stats.reloads );
			reply.Text( ",\"draws\":" );
			reply.Number( stats.draws );
			reply.Text( ",\"dropped\":" );
			reply.Number( stats.dropped );
			reply.Text( ",\"replaced\":" );
			reply.Number( (double)stats.replaced );
			reply.Text( "}}" );
			return reply.valid;
		}
		reply.Text( ",\"ok\":true,\"result\":{\"handle\":" );
		reply.Number( handle );
		reply.Text( "}}" );
#endif
	} else if ( !strcmp( op, "effects" ) || !strcmp( op, "effects.load" ) || !strcmp( op, "effects.start" ) || !strcmp( op, "effects.stop" ) ) {
#ifdef DEDICATED
		return reply.Error( "unsupported", "$", "Presentation effects require a client build." );
#else
		if ( capacity < 1024 )
			return false;
		const refexport_t *renderer = DevTools_Renderer();
		if ( !renderer )
			return reply.Error( "invalid_state", "$", "Load a rendered map before controlling effects." );
		uint32_t handle = 0;
		if ( !strcmp( op, "effects.load" ) ) {
			p = JSON_ObjectGetNamedValue( request, end, "path" );
			if ( !JSON_ReadString( p, end, name, MAX_QPATH ) )
				return reply.Error( "invalid_argument", "$.path", "Use a cooked .asfx qpath." );
			handle = (uint32_t)renderer->RegisterEffect( name );
			if ( !handle )
				return reply.Error( "load_failed", "$.path", "Load a map and a valid cooked effect with available materials/models." );
		} else if ( !strcmp( op, "effects.start" ) ) {
			uint32_t asset, seed;
			vec3_t origin, angles;
			if ( !Agent_Integer( request, end, "asset", asset ) || asset > INT32_MAX || !Agent_Integer( request, end, "seed", seed ) ||
				 !Agent_Vector( request, end, "origin", origin, -65536, 65536 ) || !Agent_Vector( request, end, "angles", angles, -360, 360 ) )
				return reply.Error( "invalid_argument", "$", "Use an asset handle, unsigned seed, origin and angles." );
			vec3_t axis[3];
			AnglesToAxis( angles, axis );
			handle = renderer->StartEffect( (qhandle_t)asset, origin, axis, seed );
			if ( !handle )
				return reply.Error( "start_failed", "$.asset", "Use a loaded effect; the fixed instance pool may be full." );
		} else if ( !strcmp( op, "effects.stop" ) ) {
			if ( !Agent_Integer( request, end, "handle", handle ) )
				return reply.Error( "invalid_argument", "$.handle", "Use the handle returned by effects.start." );
			const bool stopped = renderer->StopEffect( handle );
			reply.Text( stopped ? ",\"ok\":true,\"result\":{\"stopped\":true}}" : ",\"ok\":true,\"result\":{\"stopped\":false}}" );
			return reply.valid;
		} else {
			fxRenderStats_t stats;
			renderer->EffectStats( &stats );
			reply.Text( ",\"ok\":true,\"result\":{\"particles\":" );
			reply.Number( stats.pool.particles );
			reply.Text( ",\"instances\":" );
			reply.Number( stats.pool.instances );
			reply.Text( ",\"dropped\":" );
			reply.Number( (double)stats.pool.dropped );
			reply.Text( ",\"collisions\":" );
			reply.Number( (double)stats.pool.collisions );
			reply.Text( ",\"registered\":" );
			reply.Number( stats.registered );
			reply.Text( ",\"reloads\":" );
			reply.Number( stats.reloads );
			reply.Text( ",\"draws\":" );
			reply.Number( stats.draws );
			reply.Text( ",\"lightDraws\":" );
			reply.Number( stats.lightDraws );
			reply.Text( ",\"lightDrops\":" );
			reply.Number( stats.lightDrops );
			reply.Text( ",\"softDraws\":" );
			reply.Number( stats.softDraws );
			reply.Text( ",\"softDrops\":" );
			reply.Number( stats.softDrops );
			reply.Text( "}}" );
			return reply.valid;
		}
		reply.Text( ",\"ok\":true,\"result\":{\"handle\":" );
		reply.Number( handle );
		reply.Text( "}}" );
#endif
	} else if ( !strcmp( op, "assets" ) || !strcmp( op, "asset.select" ) || !strcmp( op, "material.set" ) || !strcmp( op, "material.preview" ) ) {
#ifdef DEDICATED
		return reply.Error( "unsupported", "$", "Renderer assets require a client build." );
#else
		if ( !strcmp( op, "assets" ) )
			return Agent_Assets( request, end, reply );
		if ( capacity < 1024 )
			return false;
		uint32_t index;
		if ( !Agent_Integer( request, end, "index", index ) || index > 65535 )
			return reply.Error( "invalid_argument", "$.index", "Use a registry index from assets." );
		bool accepted = false;
		if ( !strcmp( op, "asset.select" ) ) {
			p = JSON_ObjectGetNamedValue( request, end, "kind" );
			if ( !JSON_ReadString( p, end, name, sizeof( name ) ) )
				return reply.Error( "invalid_argument", "$.kind", "Use images/materials/models." );
			accepted = DevTools_SelectAsset( name, (int)index );
		} else if ( !strcmp( op, "material.preview" ) ) {
			bool enabled;
			if ( !Agent_Bool( request, end, "enabled", enabled ) )
				return reply.Error( "invalid_argument", "$.enabled", "Use true to override the preview instance, false to use shared factors." );
			accepted = DevTools_MaterialPreview( (int)index, enabled );
		} else {
			materialParams_t params{};
			if ( !Agent_Vector( request, end, "color", params.color, 0, 1, 4 ) || !Agent_Vector( request, end, "emissive", params.emissive, 0, 1 ) ||
				 !Agent_Number( request, end, "metallic", params.metallic, 0, 1 ) || !Agent_Number( request, end, "roughness", params.roughness, 0, 1 ) ||
				 !Agent_Number( request, end, "normalScale", params.normalScale, -1e6f, 1e6f ) || !Agent_Number( request, end, "alphaCutoff", params.alphaCutoff, 0, 1 ) ||
				 !Agent_Integer( request, end, "flags", params.flags ) )
				return reply.Error( "invalid_argument", "$", "Copy assets.params; use color[4], emissive[3], metallic/roughness/cutoff in [0,1], finite normalScale, and unchanged flags." );
			accepted = DevTools_SetMaterial( (int)index, &params );
		}
		if ( !accepted )
			return reply.Error( "rejected", "$", "Select a loaded asset and valid PBR parameters; pipeline flags must stay unchanged." );
		reply.Text( ",\"ok\":true,\"result\":{\"accepted\":true}}" );
#endif
	} else if ( !strcmp( op, "key" ) ) {
#ifdef DEDICATED
		return reply.Error( "unsupported", "$", "Keyboard events require a client build." );
#else
		bool down;
		p = JSON_ObjectGetNamedValue( request, end, "name" );
		if ( !JSON_ReadString( p, end, name, sizeof( name ) ) || !Agent_Bool( request, end, "down", down ) )
			return reply.Error( "invalid_argument", "$", "Use a key name (e.g. F8/ESCAPE) and boolean down." );
		const int key = Key_StringToKeynum( name );
		if ( key < 0 || key >= MAX_KEYS )
			return reply.Error( "invalid_argument", "$.name", "Use an engine binding key name." );
		reply.Text( ",\"ok\":true,\"result\":{\"queued\":true}}" );
		if ( reply.valid )
			Sys_QueEvent( 0, SE_KEY, key, down ? 1 : 0, 0, nullptr );
#endif
	} else if ( !strcmp( op, "actor" ) ) {
		int owner = DevTools_ViewClient();
		if ( JSON_ObjectGetNamedValue( request, end, "owner" ) ) {
			uint32_t specified;
			if ( !Agent_Integer( request, end, "owner", specified ) || specified >= MAX_CLIENTS )
				return reply.Error( "invalid_argument", "$.owner", "Choose a client id from 0 to 63." );
			owner = (int)specified;
		}
		if ( owner < 0 || !DevTools_Game() )
			return reply.Error( "invalid_state", "$", "Load a local map and select an active client." );
		Agent_Actor( reply, owner );
	} else if ( !strcmp( op, "range" ) ) {
#ifdef DEDICATED
		return reply.Error( "unsupported", "$", "Range controls require a client build." );
#else
		if ( capacity < 1024 )
			return false;
		p = JSON_ObjectGetNamedValue( request, end, "action" );
		if ( !JSON_ReadString( p, end, name, sizeof( name ) ) )
			return reply.Error( "invalid_argument", "$.action", "Use inspect/target/select/fire/reload/melee/offhand/ads/attachments/restart/capture/close." );
		p = JSON_ObjectGetNamedValue( request, end, "path" );
		value[0] = 0;
		if ( p && !JSON_ReadString( p, end, value, MAX_QPATH ) )
			return reply.Error( "invalid_argument", "$.path", "Use a cooked weapon path shorter than 64 bytes." );
		uint32_t number = 0;
		if ( JSON_ObjectGetNamedValue( request, end, "value" ) && ( !Agent_Integer( request, end, "value", number ) || number > 255 ) )
			return reply.Error( "invalid_argument", "$.value", "Use an integer slot, attachment mask or ADS 0/1." );
		if ( !DevTools_Range( name, value, (int)number ) )
			return reply.Error( "rejected", "$", "Check action/value, loaded weapon and local devmap; step before another queued range action." );
		reply.Text( ",\"ok\":true,\"result\":{\"accepted\":true}}" );
#endif
	} else if ( !strcmp( op, "graph.table" ) ) {
#ifdef DEDICATED
		return reply.Error( "unsupported", "$", "Graph tables require a client build." );
#else
		return Agent_GraphTable( request, end, reply );
#endif
	} else if ( !strcmp( op, "graph" ) || !strcmp( op, "effects.edit" ) ) {
#ifdef DEDICATED
		return reply.Error( "unsupported", "$", "Source editor actions require a client build." );
#else
		if ( capacity < 1024 )
			return false;
		static char text[65536];
		p = JSON_ObjectGetNamedValue( request, end, "action" );
		if ( !JSON_ReadString( p, end, name, sizeof( name ) ) )
			return reply.Error( "invalid_argument", "$.action", "Use source/text/save/undo, or a supported preview action." );
		p = JSON_ObjectGetNamedValue( request, end, "text" );
		text[0] = 0;
		if ( p && !JSON_ReadString( p, end, text, sizeof( text ) ) )
			return reply.Error( "invalid_argument", "$.text", "Provide text shorter than 65536 UTF-8 bytes." );
		float number = 0;
		if ( ( !strcmp( name, "play" ) || !strcmp( name, "parameter" ) ) && !Agent_Number( request, end, "value", number, -1e30f, 1e30f ) )
			return reply.Error( "invalid_argument", "$.value", "Provide the numeric parameter value, or 0/1 for play." );
		if ( !( !strcmp( op, "effects.edit" ) ? DevTools_EffectEditor( name, text ) : DevTools_Graph( name, text, number ) ) )
			return reply.Error( "rejected", "$", "Check the action and path; load a source before saving. Step to finish queued IO and inspect editor.state." );
		reply.Text( ",\"ok\":true,\"result\":{\"accepted\":true}}" );
#endif
	} else if ( !strcmp( op, "panel" ) || !strcmp( op, "world" ) || !strcmp( op, "editor.state" ) || !strcmp( op, "animation.load" ) || !strcmp( op, "animation.set" ) ) {
#ifdef DEDICATED
		return reply.Error( "unsupported", "$", "Editor actions require a client build." );
#else
		if ( capacity < 1024 )
			return false;
		if ( !strcmp( op, "editor.state" ) ) {
			Agent_EditorState( reply );
			return reply.valid;
		}
		bool accepted = false;
		if ( !strcmp( op, "panel" ) ) {
			p = JSON_ObjectGetNamedValue( request, end, "name" );
			if ( !JSON_ReadString( p, end, name, sizeof( name ) ) )
				return reply.Error( "invalid_argument", "$.name", "Provide the case-sensitive panel name." );
			accepted = DevTools_SelectPanel( name );
		} else if ( !strcmp( op, "world" ) ) {
			bool collision, navigation, entities;
			float radius;
			if ( !Agent_Bool( request, end, "collision", collision ) || !Agent_Bool( request, end, "navigation", navigation ) ||
				 !Agent_Bool( request, end, "entities", entities ) || !Agent_Number( request, end, "radius", radius, 64, 2048 ) )
				return reply.Error( "invalid_argument", "$", "Use boolean collision/navigation/entities and radius in [64,2048]." );
			accepted = DevTools_SetWorld( collision, navigation, entities, radius );
		} else if ( !strcmp( op, "animation.load" ) ) {
			p = JSON_ObjectGetNamedValue( request, end, "path" );
			if ( !JSON_ReadString( p, end, name, MAX_QPATH ) )
				return reply.Error( "invalid_argument", "$.path", "Provide a model path shorter than 64 bytes." );
			p = JSON_ObjectGetNamedValue( request, end, "skin" );
			value[0] = 0;
			if ( p && !JSON_ReadString( p, end, value, MAX_QPATH ) )
				return reply.Error( "invalid_argument", "$.skin", "Provide an optional skin path shorter than 64 bytes." );
			accepted = DevTools_LoadAnimation( name, value );
		} else {
			float number;
			p = JSON_ObjectGetNamedValue( request, end, "field" );
			if ( !JSON_ReadString( p, end, name, sizeof( name ) ) || !Agent_Number( request, end, "value", number, -180, 16777215 ) )
				return reply.Error( "invalid_argument", "$", "Provide field (model/clip/frame/play/fps/yaw) and its numeric value." );
			accepted = DevTools_SetAnimation( name, number );
		}
		if ( !accepted )
			return reply.Error( "rejected", "$", "Check the panel, model, clip/frame bounds or property range; step after loading." );
		reply.Text( ",\"ok\":true,\"result\":{\"accepted\":true}}" );
#endif
	} else if ( !strcmp( op, "subscribe" ) ) {
		bool enabled;
		if ( !Agent_Bool( request, end, "enabled", enabled ) )
			return reply.Error( "invalid_argument", "$.enabled", "Use true to receive structured events, false to stop." );
		reply.Text( ",\"ok\":true,\"result\":{\"enabled\":" );
		reply.Text( enabled ? "true" : "false" );
		reply.Text( "}}" );
		if ( reply.valid )
			agentSubscribed = enabled;
	} else if ( !strncmp( op, "entity.", 7 ) ) {
		return Agent_Entity( op, request, end, reply );
	} else if ( !strcmp( op, "profile" ) ) {
		Agent_Profile( reply );
	} else if ( !strcmp( op, "state" ) ) {
		Agent_State( reply );
	} else if ( !strcmp( op, "capture" ) ) {
#ifdef DEDICATED
		return reply.Error( "unsupported", "$", "Frame capture requires a client build." );
#else
		p = JSON_ObjectGetNamedValue( request, end, "name" );
		if ( !JSON_ReadString( p, end, name, 64 ) || !name[0] )
			return reply.Error( "invalid_argument", "$.name", "Use a capture name shorter than 64 bytes." );
		for ( const char *c = name; *c; ++c )
			if ( !( ( *c >= 'a' && *c <= 'z' ) || ( *c >= 'A' && *c <= 'Z' ) || ( *c >= '0' && *c <= '9' ) || *c == '_' || *c == '-' ) )
				return reply.Error( "invalid_argument", "$.name", "Use letters, digits, underscores or dashes." );
		snprintf( value, sizeof( value ), "screenshots/%s.png", name );
		if ( FS_FileExists( value ) )
			return reply.Error( "exists", "$.name", "Choose a new name; captures never overwrite a file." );
		reply.Text( ",\"ok\":true,\"result\":{\"path\":" );
		reply.String( value );
		reply.Text( ",\"queued\":true}}" );
		if ( reply.valid ) {
			snprintf( value, sizeof( value ), "screenshotPNG %s\n", name );
			Cbuf_AddText( value );
		}
#endif
	} else if ( !strcmp( op, "map" ) ) {
		p = JSON_ObjectGetNamedValue( request, end, "name" );
		if ( !JSON_ReadString( p, end, name, MAX_QPATH ) || !name[0] || strstr( name, ".." ) )
			return reply.Error( "invalid_argument", "$.name", "Use an installed map name without an extension." );
		for ( const char *c = name; *c; ++c )
			if ( !( ( *c >= 'a' && *c <= 'z' ) || ( *c >= 'A' && *c <= 'Z' ) || ( *c >= '0' && *c <= '9' ) || *c == '_' || *c == '-' || *c == '/' ) )
				return reply.Error( "invalid_argument", "$.name", "Use letters, digits, underscores, dashes or slashes." );
		snprintf( value, sizeof( value ), "maps/%s.bsp", name );
		if ( FS_ReadFile( value, nullptr ) <= 0 )
			return reply.Error( "not_found", "$.name", "Install or compile this map in the selected content directory." );
		reply.Text( ",\"ok\":true,\"result\":{\"queued\":true}}" );
		if ( reply.valid ) {
			agentInput = {};
			agentCamera = false;
			snprintf( value, sizeof( value ), "devmap %s\n", name );
			Cbuf_AddText( value );
		}
	} else if ( !strcmp( op, "trace" ) ) {
		if ( CM_NumInlineModels() <= 0 )
			return reply.Error( "invalid_state", "$", "Load a map before tracing its static collision world." );
		vec3_t start, finish, mins{}, maxs{};
		if ( !Agent_Vector( request, end, "start", start, -32752, 32752 ) || !Agent_Vector( request, end, "end", finish, -32752, 32752 ) )
			return reply.Error( "invalid_argument", "$", "Provide start and end [x,y,z] within world bounds." );
		p = JSON_ObjectGetNamedValue( request, end, "hull" );
		if ( !JSON_ReadString( p, end, name, sizeof( name ) ) || ( strcmp( name, "point" ) && strcmp( name, "player" ) ) )
			return reply.Error( "invalid_argument", "$.hull", "Choose point for solid sightlines or player for the standard player box and clip mask." );
		const bool player = !strcmp( name, "player" );
		if ( player ) {
			VectorSet( mins, -15, -15, -24 );
			VectorSet( maxs, 15, 15, 32 );
		}
		trace_t trace{};
		CM_BoxTrace( &trace, start, finish, mins, maxs, 0, player ? CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY : CONTENTS_SOLID, qfalse );
		reply.Text( ",\"ok\":true,\"result\":{\"fraction\":" );
		reply.Number( trace.fraction );
		reply.Text( ",\"end\":" );
		reply.Vector( trace.endpos );
		reply.Text( ",\"normal\":" );
		reply.Vector( trace.plane.normal );
		reply.Text( ",\"start_solid\":" );
		reply.Text( trace.startsolid ? "true" : "false" );
		reply.Text( ",\"all_solid\":" );
		reply.Text( trace.allsolid ? "true" : "false" );
		reply.Text( ",\"contents\":" );
		reply.Number( trace.contents );
		reply.Text( ",\"surface_flags\":" );
		reply.Number( trace.surfaceFlags );
		reply.Text( "}}" );
	} else if ( !strcmp( op, "camera" ) ) {
#ifdef DEDICATED
		return reply.Error( "unsupported", "$", "Camera control requires a client build." );
#else
		p = JSON_ObjectGetNamedValue( request, end, "mode" );
		if ( !JSON_ReadString( p, end, name, sizeof( name ) ) || ( strcmp( name, "pose" ) && strcmp( name, "player" ) ) )
			return reply.Error( "invalid_argument", "$.mode", "Choose player or pose." );
		const bool pose = !strcmp( name, "pose" );
		float origin[3]{}, angles[3]{};
		if ( pose && ( !Agent_Vector( request, end, "origin", origin, -32752, 32752 ) || !Agent_Vector( request, end, "angles", angles, -360, 360 ) ) )
			return reply.Error( "invalid_argument", "$", "Provide origin [x,y,z] within world bounds and angles [pitch,yaw,roll] in degrees." );
		reply.Text( ",\"ok\":true,\"result\":{\"accepted\":true}}" );
		if ( reply.valid ) {
			agentCamera = pose;
			VectorCopy( origin, agentCameraOrigin );
			VectorCopy( angles, agentCameraAngles );
		}
#endif
	} else if ( !strcmp( op, "usercmd" ) ) {
#ifdef DEDICATED
		return reply.Error( "unsupported", "$", "Raw local input requires a client build." );
#else
		devAgentInput_t input{};
		float angles[3];
		if ( !agentActive )
			return reply.Error( "invalid_state", "$", "Launch with --agent." );
		if ( !Agent_Integer( request, end, "forwardmove", input.forward ) || input.forward < -127 || input.forward > 127 ||
			 !Agent_Integer( request, end, "rightmove", input.right ) || input.right < -127 || input.right > 127 ||
			 !Agent_Integer( request, end, "upmove", input.up ) || input.up < -127 || input.up > 127 )
			return reply.Error( "invalid_argument", "$", "Use integer forwardmove/rightmove/upmove in [-127,127]." );
		if ( !Agent_Integer( request, end, "buttons", input.buttons ) || input.buttons < 0 || input.buttons > 65535 ||
			 !Agent_Integer( request, end, "weapon", input.weapon ) || input.weapon < 0 || input.weapon > 255 )
			return reply.Error( "invalid_argument", "$", "Use integer buttons in [0,65535] and weapon in [0,255]." );
		if ( !Agent_Vector( request, end, "angles", angles, 0, 65535 ) )
			return reply.Error( "invalid_argument", "$.angles", "Use three unsigned 16-bit command angles." );
		for ( int i = 0; i < 3; ++i ) {
			if ( std::trunc( angles[i] ) != angles[i] )
				return reply.Error( "invalid_argument", "$.angles", "Raw command angles must be integers." );
			input.angles[i] = (int32_t)angles[i];
		}
		input.active = input.raw = true;
		reply.Text( ",\"ok\":true,\"result\":{\"accepted\":true}}" );
		if ( reply.valid )
			agentInput = input;
#endif
	} else if ( !strcmp( op, "input" ) ) {
		if ( !agentActive )
			return reply.Error( "invalid_state", "$", "Launch a development client with --agent." );
#ifdef DEDICATED
		return reply.Error( "unsupported", "$", "Local player input requires a client build." );
#else
		devAgentInput_t input{};
		float forward, right, up;
		if ( !Agent_Number( request, end, "forward", forward, -1, 1 ) || !Agent_Number( request, end, "right", right, -1, 1 ) || !Agent_Number( request, end, "up", up, -1, 1 ) )
			return reply.Error( "invalid_argument", "$", "Provide forward, right and up in [-1, 1]." );
		if ( !Agent_Number( request, end, "yaw", input.yaw, -360, 360 ) || !Agent_Number( request, end, "pitch", input.pitch, -89, 89 ) )
			return reply.Error( "invalid_argument", "$", "Provide yaw in [-360,360] and pitch in [-89,89] degrees." );
		input.forward = (int32_t)( forward * 127 );
		input.right = (int32_t)( right * 127 );
		input.up = (int32_t)( up * 127 );
		input.weapon = -1;
		static constexpr const char *buttons[] = { "fire", "ads", "reload", "melee", "offhand" };
		static constexpr int bits[] = { 1, 1 << 12, 1 << 13, 1 << 14, 1 << 15 };
		for ( uint32_t i = 0; i < ARRAY_LEN( buttons ); ++i ) {
			bool pressed;
			if ( !Agent_Bool( request, end, buttons[i], pressed ) )
				return reply.Error( "invalid_argument", "$", "Button fields must be JSON booleans." );
			if ( pressed )
				input.buttons |= bits[i];
		}
		input.active = true;
		reply.Text( ",\"ok\":true,\"result\":{\"accepted\":true}}" );
		if ( reply.valid )
			agentInput = input;
#endif
	} else if ( !strcmp( op, "session" ) ) {
		uint32_t dt, seed;
		if ( !agentActive || agentFrame )
			return reply.Error( "invalid_state", "$", "Configure an --agent process before its first step." );
		if ( !Agent_Integer( request, end, "dt", dt ) || dt < 1 || dt > 100 )
			return reply.Error( "invalid_argument", "$.dt", "Use an integer timestep from 1 to 100 milliseconds." );
		if ( !Agent_Integer( request, end, "seed", seed ) || seed > INT32_MAX )
			return reply.Error( "invalid_argument", "$.seed", "Use an integer seed from 0 to 2147483647." );
		char result[128];
		snprintf( result, sizeof( result ), ",\"ok\":true,\"result\":{\"dt\":%u,\"seed\":%u}}", dt, seed );
		reply.Text( result );
		if ( reply.valid ) {
			agentDt = (int)dt;
			agentSeed = (int)seed;
			Q_Srand( seed );
		}
	} else if ( !strcmp( op, "step" ) ) {
		uint32_t frames;
		if ( !agentActive || agentSteps )
			return reply.Error( "invalid_state", "$", "Step an idle --agent process." );
		if ( !Agent_Integer( request, end, "frames", frames ) || frames < 1 || frames > 10000 ||
			 (uint64_t)agentTime + (uint64_t)frames * (uint32_t)agentDt > INT32_MAX )
			return reply.Error( "invalid_argument", "$.frames", "Use 1 to 10000 frames within the session clock range." );
		// Reserve enough room before accepting work. The transport replies after completion.
		reply.Text( ",\"ok\":true,\"result\":{\"pending\":true}}" );
		if ( reply.valid ) {
			agentSteps = frames;
			agentError[0] = 0;
			agentStepId = sequence;
		}
	} else if ( !strcmp( op, "cvar.list" ) ) {
		return Agent_Cvars( request, end, reply );
	} else if ( !strcmp( op, "cvar.select" ) || !strcmp( op, "editor.filter" ) ) {
#ifdef DEDICATED
		return reply.Error( "unsupported", "$", "Editor controls require a client build." );
#else
		if ( capacity < 1024 )
			return false;
		if ( !strcmp( op, "cvar.select" ) ) {
			p = JSON_ObjectGetNamedValue( request, end, "name" );
			if ( !JSON_ReadString( p, end, name, sizeof( name ) ) || !DevTools_SelectCvar( name ) )
				return reply.Error( "invalid_argument", "$.name", "Select a registered cvar from cvar.list." );
		} else {
			p = JSON_ObjectGetNamedValue( request, end, "kind" );
			const char *v = JSON_ObjectGetNamedValue( request, end, "value" );
			if ( !JSON_ReadString( p, end, name, sizeof( name ) ) || !JSON_ReadString( v, end, value, 128 ) || !DevTools_Filter( name, value ) )
				return reply.Error( "invalid_argument", "$", "Use kind cvars/images/materials and a value shorter than 128 bytes." );
		}
		reply.Text( ",\"ok\":true,\"result\":{\"accepted\":true}}" );
#endif
	} else if ( !strcmp( op, "cvar.get" ) || !strcmp( op, "cvar.set" ) ) {
		p = JSON_ObjectGetNamedValue( request, end, "name" );
		if ( !JSON_ReadString( p, end, name, sizeof( name ) ) || !name[0] )
			return reply.Error( "invalid_argument", "$.name", "Use an existing cvar name shorter than 256 bytes." );
		const unsigned flags = Cvar_Flags( name );
		if ( flags == CVAR_NONEXISTENT )
			return reply.Error( "not_found", "$.name", "Use a cvar registered by the running engine." );
		if ( !strcmp( op, "cvar.get" ) ) {
			reply.Text( ",\"ok\":true,\"result\":{\"name\":" );
			reply.String( name );
			reply.Text( ",\"value\":" );
			reply.String( Cvar_VariableString( name ) );
			reply.Text( "}}" );
		} else {
			p = JSON_ObjectGetNamedValue( request, end, "value" );
			if ( !JSON_ReadString( p, end, value, sizeof( value ) ) )
				return reply.Error( "invalid_argument", "$.value", "Use a string shorter than 4096 bytes." );
			if ( flags & ( CVAR_ROM | CVAR_INIT ) )
				return reply.Error( "read_only", "$.name", "Choose a writable cvar; startup-only values belong in launch arguments." );
			if ( ( flags & CVAR_CHEAT ) && !atoi( Cvar_VariableString( "sv_cheats" ) ) )
				return reply.Error( "protected", "$.name", "Start a local developer map with cheats enabled." );
			if ( ( flags & CVAR_DEVELOPER ) && !atoi( Cvar_VariableString( "developer" ) ) )
				return reply.Error( "protected", "$.name", "Enable developer mode before setting this cvar." );
			reply.Text( ",\"ok\":true,\"result\":{\"accepted\":true}}" );
			if ( reply.valid )
				Cvar_Set2( name, value, qfalse );
		}
	} else if ( !strcmp( op, "exec" ) ) {
		p = JSON_ObjectGetNamedValue( request, end, "command" );
		if ( !JSON_ReadString( p, end, value, sizeof( value ) - 1 ) || !value[0] )
			return reply.Error( "invalid_argument", "$.command", "Use a nonempty console command shorter than 4095 bytes." );
		strcat( value, "\n" );
		reply.Text( ",\"ok\":true,\"result\":{\"queued\":true}}" );
		if ( reply.valid )
			Cbuf_AddText( value );
	} else
		return reply.Error( "unknown_operation", "$.op", "Use a command name from hello." );
	return reply.valid;
}

static void Agent_Send( const char *response ) {
	if ( !Sys_AgentWrite( response, (uint32_t)strlen( response ) ) || !Sys_AgentWrite( "\n", 1 ) )
		Com_Quit_f();
}

void DevTools_AgentFlushEvents( void ) {
	for ( uint32_t i = 0; i < agentEventCount; ++i ) {
		const auto &event = agentEvents[i];
		char buffer[2048];
		agentReply_t reply{ buffer, sizeof( buffer ), 0, true };
		reply.Text( "{\"event\":" );
		reply.String( event.type );
		reply.Text( ",\"frame\":" );
		reply.Number( event.frame );
		reply.Text( ",\"time\":" );
		reply.Number( event.time );
		reply.Text( ",\"actor\":" );
		reply.Number( event.actor );
		reply.Text( ",\"target\":" );
		reply.Number( event.target );
		reply.Text( ",\"value\":" );
		reply.Number( event.value );
		reply.Text( ",\"detail\":" );
		reply.String( event.detail );
		reply.Text( "}" );
		Agent_Send( buffer );
	}
	agentEventCount = 0;
	if ( agentEventsDropped ) {
		char overflow[128];
		snprintf( overflow, sizeof( overflow ), "{\"event\":\"overflow\",\"dropped\":%u}", agentEventsDropped );
		Agent_Send( overflow );
		agentEventsDropped = 0;
	}
}

bool DevTools_AgentNextFrame( void ) {
	if ( !agentSteps ) {
		static char request[agentLimit], response[65536];
		const int length = Sys_AgentRead( request, sizeof( request ) );
		if ( length < 0 ) {
			Com_Quit_f();
			return false;
		}
		if ( !DevTools_AgentRequest( request, (uint32_t)length, response, sizeof( response ) ) )
			Agent_Send( "{\"id\":null,\"ok\":false,\"error\":{\"code\":\"response_limit\",\"path\":\"$\",\"hint\":\"Request a smaller result.\"}}" );
		else if ( !agentSteps )
			Agent_Send( response );
		if ( !agentSteps )
			return false;
	}
	agentTime += agentDt;
	agentFrameStart = Sys_Microseconds();
	return true;
}

void DevTools_AgentEndFrame( void ) {
	agentFrameTimes[agentSamples++ % ARRAY_LEN( agentFrameTimes )] = MAX( INT64_C( 0 ), Sys_Microseconds() - agentFrameStart );
	DevTools_AgentFlushEvents();
	if ( agentError[0] ) {
		char buffer[2048], prefix[64];
		snprintf( prefix, sizeof( prefix ), "{\"id\":%u", agentStepId );
		agentReply_t reply{ buffer, sizeof( buffer ), 0, true };
		reply.Text( prefix );
		reply.Error( "engine_error", "$", agentError );
		agentSteps = 0;
		Agent_Send( buffer );
		return;
	}
	++agentFrame;
	if ( --agentSteps == 0 ) {
		char reply[160];
		snprintf( reply, sizeof( reply ), "{\"id\":%u,\"ok\":true,\"result\":{\"frame\":%u,\"time\":%d,\"dt\":%d}}", agentStepId, agentFrame, agentTime, agentDt );
		Agent_Send( reply );
	}
}
