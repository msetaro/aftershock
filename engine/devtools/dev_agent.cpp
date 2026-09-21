#include "devtools_public.h"
#include "../qcommon/qcommon_public.h"
#define JSON_IMPLEMENTATION
#include "../qcommon/json.h"
#include <charconv>

// Requests and replies are bounded POD data; the inactive channel allocates nothing.
static constexpr uint32_t agentLimit = 16384;
static bool agentActive;
static uint32_t agentFrame, agentSteps, agentStepId;
static int agentTime = 1000, agentDt = 8, agentSeed = 1;

void DevTools_AgentEnable( void ) {
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

static void Agent_Whitespace( const char *&p, const char *end ) {
	while ( p < end && ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) )
		++p;
}

static bool Agent_Hex( const char *&p, const char *end, uint32_t &value ) {
	if ( end - p < 4 )
		return false;
	value = 0;
	for ( int i = 0; i < 4; ++i ) {
		const char c = *p++;
		const int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10
													   : c >= 'A' && c <= 'F'	? c - 'A' + 10
																				: -1;
		if ( digit < 0 )
			return false;
		value = value * 16 + (uint32_t)digit;
	}
	return true;
}

// The existing JSON helper locates members but deliberately leaves strings escaped.
static bool Agent_String( const char *&p, const char *end, char *out, uint32_t capacity ) {
	if ( !p || p >= end || *p++ != '"' )
		return false;
	uint32_t length = 0;
	while ( p < end ) {
		uint32_t c = (unsigned char)*p++;
		if ( c == '"' ) {
			if ( out )
				out[length] = 0;
			return true;
		}
		if ( c < 32 )
			return false;
		char bytes[4];
		uint32_t count = 1;
		bytes[0] = (char)c;
		if ( c == '\\' ) {
			if ( p == end )
				return false;
			c = (unsigned char)*p++;
			switch ( c ) {
			case '"':
			case '\\':
			case '/':
				bytes[0] = (char)c;
				break;
			case 'b':
				bytes[0] = '\b';
				break;
			case 'f':
				bytes[0] = '\f';
				break;
			case 'n':
				bytes[0] = '\n';
				break;
			case 'r':
				bytes[0] = '\r';
				break;
			case 't':
				bytes[0] = '\t';
				break;
			case 'u': {
				if ( !Agent_Hex( p, end, c ) || !c )
					return false; // Engine strings cannot contain embedded NUL.
				if ( c >= 0xd800 && c <= 0xdbff ) {
					uint32_t low;
					if ( end - p < 6 || p[0] != '\\' || p[1] != 'u' )
						return false;
					p += 2;
					if ( !Agent_Hex( p, end, low ) || low < 0xdc00 || low > 0xdfff )
						return false;
					c = 0x10000 + ( ( c - 0xd800 ) << 10 ) + low - 0xdc00;
				} else if ( c >= 0xdc00 && c <= 0xdfff )
					return false;
				count = c < 0x80 ? 1 : c < 0x800 ? 2
								   : c < 0x10000 ? 3
												 : 4;
				for ( uint32_t i = count - 1; i > 0; --i ) {
					bytes[i] = (char)( 0x80 | ( c & 63 ) );
					c >>= 6;
				}
				bytes[0] = (char)( c | ( count == 1 ? 0 : count == 2 ? 0xc0
													  : count == 3	 ? 0xe0
																	 : 0xf0 ) );
				break;
			}
			default:
				return false;
			}
		}
		if ( out ) {
			if ( count >= capacity - length )
				return false;
			memcpy( out + length, bytes, count );
			length += count;
		}
	}
	return false;
}

// Validate delimiters and cap nesting before the permissive JSON member helpers.
static bool Agent_Value( const char *&p, const char *end, uint32_t depth ) {
	Agent_Whitespace( p, end );
	if ( p == end || depth > 16 )
		return false;
	if ( *p == '"' )
		return Agent_String( p, end, nullptr, 0 );
	if ( *p == '{' || *p == '[' ) {
		const bool object = *p++ == '{';
		const char close = object ? '}' : ']';
		Agent_Whitespace( p, end );
		if ( p < end && *p == close ) {
			++p;
			return true;
		}
		while ( p < end ) {
			if ( object ) {
				if ( !Agent_String( p, end, nullptr, 0 ) )
					return false;
				Agent_Whitespace( p, end );
				if ( p == end || *p++ != ':' )
					return false;
			}
			if ( !Agent_Value( p, end, depth + 1 ) )
				return false;
			Agent_Whitespace( p, end );
			if ( p == end )
				return false;
			if ( *p == close ) {
				++p;
				return true;
			}
			if ( *p++ != ',' )
				return false;
			Agent_Whitespace( p, end );
		}
		return false;
	}
	static constexpr const char *literals[] = { "true", "false", "null" };
	for ( const char *literal : literals ) {
		const size_t length = strlen( literal );
		if ( (size_t)( end - p ) >= length && !memcmp( p, literal, length ) ) {
			p += length;
			return true;
		}
	}
	if ( *p == '-' )
		++p;
	if ( p == end || *p < '0' || *p > '9' )
		return false;
	if ( *p++ != '0' )
		while ( p < end && *p >= '0' && *p <= '9' )
			++p;
	if ( p < end && *p == '.' ) {
		++p;
		const char *start = p;
		while ( p < end && *p >= '0' && *p <= '9' )
			++p;
		if ( p == start )
			return false;
	}
	if ( p < end && ( *p == 'e' || *p == 'E' ) ) {
		++p;
		if ( p < end && ( *p == '+' || *p == '-' ) )
			++p;
		const char *start = p;
		while ( p < end && *p >= '0' && *p <= '9' )
			++p;
		if ( p == start )
			return false;
	}
	return true;
}

static bool Agent_Integer( const char *request, const char *end, const char *name, uint32_t &value ) {
	const char *p = JSON_ObjectGetNamedValue( request, end, name );
	if ( !p )
		return false;
	const char *stop = JSON_SkipValue( p, end );
	const auto result = std::from_chars( p, stop, value );
	return result.ec == std::errc{} && result.ptr == stop;
}

bool DevTools_AgentRequest( const char *request, uint32_t length, char *response, uint32_t capacity ) {
	if ( !response || capacity < 2 )
		return false;
	response[0] = 0;
	agentReply_t reply{ response, capacity, 0, true };
	reply.Text( "{\"id\":" );
	if ( !request || !length || length >= agentLimit ) {
		reply.Text( "null" );
		return reply.Error( "invalid_request", "$", "Send one JSON object shorter than 16384 bytes." );
	}
	const char *end = request + length;
	Agent_Whitespace( request, end );
	const char *p = request;
	const bool parsed = p < end && *p == '{' && Agent_Value( p, end, 0 );
	Agent_Whitespace( p, end );
	if ( !parsed || p != end ) {
		reply.Text( "null" );
		return reply.Error( "invalid_request", "$", "Send a JSON object with valid strings and at most 16 nested containers." );
	}
	const char *id = JSON_ObjectGetNamedValue( request, end, "id" );
	uint32_t sequence = 0;
	const char *idEnd = id ? JSON_SkipValue( id, end ) : end;
	const auto number = id ? std::from_chars( id, idEnd, sequence ) : std::from_chars_result{};
	if ( !id || number.ec != std::errc{} || number.ptr != idEnd ) {
		reply.Text( "null" );
		return reply.Error( "invalid_argument", "$.id", "Use an unsigned 32-bit integer request id." );
	}
	char integer[16];
	snprintf( integer, sizeof( integer ), "%u", sequence );
	reply.Text( integer );
	char op[64], name[256], value[4096];
	p = JSON_ObjectGetNamedValue( request, end, "op" );
	if ( !Agent_String( p, end, op, sizeof( op ) ) )
		return reply.Error( "invalid_argument", "$.op", "Use a command name from hello." );
	if ( !strcmp( op, "hello" ) ) {
		reply.Text( ",\"ok\":true,\"result\":{\"protocol\":1,\"commands\":[\"hello\",\"exec\",\"cvar.get\",\"cvar.set\",\"session\",\"step\"]}}" );
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
			srand( seed );
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
			agentStepId = sequence;
		}
	} else if ( !strcmp( op, "cvar.get" ) || !strcmp( op, "cvar.set" ) ) {
		p = JSON_ObjectGetNamedValue( request, end, "name" );
		if ( !Agent_String( p, end, name, sizeof( name ) ) || !name[0] )
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
			if ( !Agent_String( p, end, value, sizeof( value ) ) )
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
		if ( !Agent_String( p, end, value, sizeof( value ) - 1 ) || !value[0] )
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
	return true;
}

void DevTools_AgentEndFrame( void ) {
	++agentFrame;
	if ( --agentSteps == 0 ) {
		char reply[160];
		snprintf( reply, sizeof( reply ), "{\"id\":%u,\"ok\":true,\"result\":{\"frame\":%u,\"time\":%d,\"dt\":%d}}", agentStepId, agentFrame, agentTime, agentDt );
		Agent_Send( reply );
	}
}
