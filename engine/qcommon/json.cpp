#include <cstdint>
#include <cstring>
#define JSON_IMPLEMENTATION
#include "json.h"

// Shared from the development command owner; keep the same bounded grammar.
void JSON_Whitespace( const char *&p, const char *end ) {
	while ( p < end && ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) )
		++p;
}

static bool JSON_Hex( const char *&p, const char *end, uint32_t &value ) {
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
bool JSON_ReadString( const char *&p, const char *end, char *out, uint32_t capacity ) {
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
				if ( !JSON_Hex( p, end, c ) || !c )
					return false; // Engine strings cannot contain embedded NUL.
				if ( c >= 0xd800 && c <= 0xdbff ) {
					uint32_t low;
					if ( end - p < 6 || p[0] != '\\' || p[1] != 'u' )
						return false;
					p += 2;
					if ( !JSON_Hex( p, end, low ) || low < 0xdc00 || low > 0xdfff )
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
static bool JSON_ParseValue( const char *&p, const char *end, uint32_t depth ) {
	JSON_Whitespace( p, end );
	if ( p == end || depth > 16 )
		return false;
	if ( *p == '"' )
		return JSON_ReadString( p, end, nullptr, 0 );
	if ( *p == '{' || *p == '[' ) {
		const bool object = *p++ == '{';
		const char close = object ? '}' : ']';
		JSON_Whitespace( p, end );
		if ( p < end && *p == close ) {
			++p;
			return true;
		}
		while ( p < end ) {
			if ( object ) {
				if ( !JSON_ReadString( p, end, nullptr, 0 ) )
					return false;
				JSON_Whitespace( p, end );
				if ( p == end || *p++ != ':' )
					return false;
			}
			if ( !JSON_ParseValue( p, end, depth + 1 ) )
				return false;
			JSON_Whitespace( p, end );
			if ( p == end )
				return false;
			if ( *p == close ) {
				++p;
				return true;
			}
			if ( *p++ != ',' )
				return false;
			JSON_Whitespace( p, end );
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

bool JSON_ValidateObject( const char *data, const char *end ) {
	if ( !data || !end || data >= end )
		return false;
	JSON_Whitespace( data, end );
	if ( data == end || *data != '{' || !JSON_ParseValue( data, end, 0 ) )
		return false;
	JSON_Whitespace( data, end );
	return data == end;
}
