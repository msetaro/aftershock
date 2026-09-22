#include "ui_public.h"
#include "../../third_party/sha256/sha-256.h"
#include <algorithm>
#include <cmath>
#include <string.h>

static bool Range( float value, float low, float high ) {
	return std::isfinite( value ) && value >= low && value <= high;
}
static bool Identifier( const char *value, size_t capacity, const char *extra = "_" ) {
	const char *end = (const char *)memchr( value, 0, capacity );
	if ( !end || end == value )
		return false;
	for ( const char *p = value; p < end; ++p )
		if ( !( *p >= 'a' && *p <= 'z' ) && !( *p >= '0' && *p <= '9' ) && !strchr( extra, *p ) )
			return false;
	return true;
}
int UI_FindPage( const uiDocument_t &document, const char *name ) {
	for ( uint32_t i = 0; i < document.header.pageCount; ++i )
		if ( !strcmp( name, document.pages[i].name ) )
			return int( i );
	return -1;
}
static bool ValidTarget( const uiDocument_t &document, const uiItem_t &item ) {
	if ( !memchr( item.target, 0, sizeof( item.target ) ) )
		return false;
	if ( item.kind == UI_BUTTON ) {
		switch ( item.action ) {
		case UI_ACTION_PAGE:
			return UI_FindPage( document, item.target ) >= 0;
		case UI_ACTION_MAP:
			return Identifier( item.target, sizeof( item.target ), "_-" );
		case UI_ACTION_RESUME:
		case UI_ACTION_QUIT:
			return !item.target[0];
		default:
			return false;
		}
	}
	if ( item.action != UI_ACTION_NONE )
		return false;
	if ( item.kind == UI_LABEL )
		return !item.target[0];
	if ( item.kind == UI_SLIDER ) {
		static const struct {
			const char *name;
			float low, high;
		} limits[] = {
			{ "s_volume", 0, 1 }, { "s_musicvolume", 0, 1 }, { "s_hrtf", 0, 1 },
			{ "sensitivity", .1f, 30 }, { "cl_run", 0, 1 }
		};
		for ( const auto &limit : limits )
			if ( !strcmp( limit.name, item.target ) )
				return Range( item.range[0], limit.low, limit.high ) && Range( item.range[1], limit.low, limit.high ) &&
					   item.range[0] < item.range[1] && Range( item.range[2], 0, item.range[1] - item.range[0] ) && item.range[2] > 0;
		return false;
	}
	static const char *bindings[] = { "+attack", "+forward", "+back", "+moveleft", "+moveright", "+moveup", "+movedown", "+speed", "+voiprecord", "weapnext", "weapprev" };
	static const char *values[] = { "health", "armor", "ammo", "ping", "fps" };
	if ( item.kind == UI_BINDING ) {
		for ( const char *name : bindings )
			if ( !strcmp( name, item.target ) )
				return true;
	} else if ( item.kind == UI_VALUE ) {
		for ( const char *name : values )
			if ( !strcmp( name, item.target ) )
				return true;
	}
	return false;
}

bool UI_ReadDocument( const void *data, size_t size, uiDocument_t *document ) {
	struct envelope_t {
		char magic[8];
		uint32_t version, size;
		uint8_t hash[32];
	} envelope;
	static_assert( sizeof( envelope ) == 48 );
	if ( !data || !document || size < sizeof( envelope ) + sizeof( uiDocumentHeader_t ) || size > sizeof( envelope ) + sizeof( uiDocument_t ) )
		return false;
	memcpy( &envelope, data, sizeof( envelope ) );
	if ( memcmp( envelope.magic, "ASUI\0\0\0\0", 8 ) || envelope.version != 1 || envelope.size != size - sizeof( envelope ) )
		return false;
	const auto *cursor = (const uint8_t *)data + sizeof( envelope );
	uint8_t hash[32];
	calc_sha_256( hash, cursor, envelope.size );
	if ( memcmp( hash, envelope.hash, sizeof( hash ) ) )
		return false;
	uiDocument_t result = {};
	auto &header = result.header;
	memcpy( &header, cursor, sizeof( header ) );
	cursor += sizeof( header );
	if ( !Identifier( header.name, sizeof( header.name ) ) || !Identifier( header.atlas, sizeof( header.atlas ), "_./-" ) ||
		 header.atlas[0] == '/' || strstr( header.atlas, ".." ) || !Range( header.canvas[0], 320, 8192 ) || !Range( header.canvas[1], 320, 8192 ) ||
		 header.pageCount < 3 || header.pageCount > 16 || !header.itemCount || header.itemCount > 128 || !header.textCount || header.textCount > 64 ||
		 !header.localeCount || header.localeCount > 8 || header.glyphCount != 95 || !header.atlasWidth || header.atlasWidth > 2048 || !header.atlasHeight || header.atlasHeight > 2048 )
		return false;
	const uint32_t runs = header.textCount * header.localeCount + header.glyphCount;
	const size_t expected = sizeof( header ) + header.pageCount * sizeof( uiPage_t ) + header.itemCount * sizeof( uiItem_t ) +
							header.textCount * sizeof( uiText_t ) + header.localeCount * sizeof( uiLocale_t ) + runs * sizeof( uiTextRun_t );
	if ( expected != envelope.size )
		return false;
	auto copyRecords = [&]( auto &array, uint32_t count ) {
		const size_t bytes = count * sizeof( array[0] );
		memcpy( array, cursor, bytes );
		cursor += bytes;
	};
	copyRecords( result.pages, header.pageCount );
	copyRecords( result.items, header.itemCount );
	copyRecords( result.texts, header.textCount );
	copyRecords( result.locales, header.localeCount );
	copyRecords( result.runs, runs );
	uint32_t first = 0;
	for ( uint32_t i = 0; i < header.pageCount; ++i ) {
		const auto &page = result.pages[i];
		if ( !Identifier( page.name, sizeof( page.name ) ) || page.first != first || !page.count || page.count > header.itemCount - first )
			return false;
		for ( uint32_t j = 0; j < i; ++j )
			if ( !strcmp( page.name, result.pages[j].name ) )
				return false;
		first += page.count;
	}
	if ( first != header.itemCount || UI_FindPage( result, "main" ) < 0 || UI_FindPage( result, "options" ) < 0 || UI_FindPage( result, "hud" ) < 0 )
		return false;
	for ( uint32_t i = 0; i < header.itemCount; ++i ) {
		const auto &item = result.items[i];
		if ( !Identifier( item.name, sizeof( item.name ) ) || item.kind > UI_VALUE || item.text >= header.textCount || !ValidTarget( result, item ) )
			return false;
		for ( float component : item.rect )
			if ( !Range( component, -8192, 8192 ) )
				return false;
		if ( item.rect[2] <= 0 || item.rect[3] <= 0 )
			return false;
		for ( float component : item.anchor )
			if ( !Range( component, 0, 1 ) )
				return false;
		for ( float component : item.color )
			if ( !Range( component, 0, 1 ) )
				return false;
	}
	for ( uint32_t i = 0; i < header.textCount; ++i )
		if ( !Identifier( result.texts[i].name, sizeof( result.texts[i].name ) ) || !Range( result.texts[i].size, 12, 96 ) )
			return false;
	for ( uint32_t i = 0; i < header.localeCount; ++i )
		if ( !Identifier( result.locales[i].name, sizeof( result.locales[i].name ), "-" ) || result.locales[i].rtl > 1 )
			return false;
	for ( uint32_t i = 0; i < runs; ++i ) {
		const auto &run = result.runs[i];
		if ( run.x >= header.atlasWidth || run.y >= header.atlasHeight || !run.width || !run.height ||
			 run.width > header.atlasWidth - run.x || run.height > header.atlasHeight - run.y ||
			 !Range( run.left, -2048, 2048 ) || !Range( run.top, -2048, 2048 ) || !Range( run.advance, 0, 2048 ) )
			return false;
	}
	*document = result;
	return true;
}
static bool Interactive( const uiItem_t &item ) {
	return item.kind == UI_BUTTON || item.kind == UI_SLIDER || item.kind == UI_BINDING;
}
bool UI_OpenDocument( const uiDocument_t &document, uiDocumentState_t *state, const char *name ) {
	const int page = UI_FindPage( document, name );
	if ( !state || page < 0 )
		return false;
	state->page = uint32_t( page );
	state->focus = UI_NO_FOCUS;
	UI_Navigate( document, state, 1 );
	return true;
}
void UI_Navigate( const uiDocument_t &document, uiDocumentState_t *state, int direction ) {
	if ( !state || state->page >= document.header.pageCount || !direction )
		return;
	const auto &page = document.pages[state->page];
	uint32_t offset = state->focus >= page.first && state->focus < page.first + page.count ? state->focus - page.first : ( direction > 0 ? page.count - 1 : 0 );
	for ( uint32_t i = 0; i < page.count; ++i ) {
		offset = ( offset + ( direction > 0 ? 1 : page.count - 1 ) ) % page.count;
		if ( Interactive( document.items[page.first + offset] ) ) {
			state->focus = page.first + offset;
			return;
		}
	}
	state->focus = UI_NO_FOCUS;
}
float UI_Nudge( const uiItem_t &item, float value, int direction ) {
	if ( item.kind != UI_SLIDER || !std::isfinite( value ) )
		return 0;
	return std::clamp( value + ( direction > 0 ? item.range[2] : direction < 0 ? -item.range[2]
																			   : 0 ),
		item.range[0], item.range[1] );
}
bool UI_Layout( const uiDocument_t &document, const uiItem_t &item, int width, int height, float safeArea, uiRectangle_t *rectangle ) {
	if ( !rectangle || width < 1 || height < 1 || !Range( safeArea, 0, .2f ) )
		return false;
	const float viewportWidth = float( width ), viewportHeight = float( height );
	const float left = viewportWidth * safeArea, top = viewportHeight * safeArea;
	const float availableWidth = viewportWidth - 2 * left, availableHeight = viewportHeight - 2 * top;
	const float scale = std::min( availableWidth / document.header.canvas[0], availableHeight / document.header.canvas[1] );
	const float w = std::min( item.rect[2] * scale, availableWidth ), h = std::min( item.rect[3] * scale, availableHeight );
	*rectangle = { std::clamp( left + availableWidth * item.anchor[0] + item.rect[0] * scale, left, viewportWidth - left - w ),
		std::clamp( top + availableHeight * item.anchor[1] + item.rect[1] * scale, top, viewportHeight - top - h ), w, h, scale };
	return true;
}
