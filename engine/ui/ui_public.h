#ifndef UI_DOCUMENT_PUBLIC_H
#define UI_DOCUMENT_PUBLIC_H

#include <stddef.h>
#include <stdint.h>
#include <type_traits>

enum uiItemKind_t : uint32_t { UI_LABEL,
	UI_BUTTON,
	UI_SLIDER,
	UI_BINDING,
	UI_VALUE };
enum uiAction_t : uint32_t { UI_ACTION_NONE,
	UI_ACTION_PAGE,
	UI_ACTION_RESUME,
	UI_ACTION_MAP,
	UI_ACTION_QUIT,
	UI_ACTION_BACKEND };
struct uiDocumentHeader_t {
	char name[32], atlas[64];
	float canvas[2];
	uint32_t pageCount, itemCount, textCount, localeCount, glyphCount, atlasWidth, atlasHeight;
};
struct uiPage_t {
	char name[32];
	uint32_t first, count;
};
struct uiItem_t {
	char name[32];
	uiItemKind_t kind;
	uiAction_t action;
	uint32_t text;
	char target[64];
	float rect[4], anchor[2], color[4], range[3];
};
struct uiText_t {
	char name[32];
	float size;
};
struct uiLocale_t {
	char name[16];
	uint32_t rtl;
};
struct uiTextRun_t {
	uint32_t x, y, width, height;
	float left, top, advance;
};
struct uiDocument_t {
	uiDocumentHeader_t header;
	uiPage_t pages[16];
	uiItem_t items[128];
	uiText_t texts[64];
	uiLocale_t locales[8];
	uiTextRun_t runs[64 * 8 + 95];
};
static_assert( sizeof( uiDocumentHeader_t ) == 132 && offsetof( uiDocumentHeader_t, pageCount ) == 104 );
static_assert( sizeof( uiPage_t ) == 40 && sizeof( uiItem_t ) == 160 && offsetof( uiItem_t, rect ) == 108 );
static_assert( sizeof( uiText_t ) == 36 && sizeof( uiLocale_t ) == 20 && sizeof( uiTextRun_t ) == 28 );
static_assert( std::is_trivially_copyable_v<uiDocument_t> );
constexpr uint32_t UI_NO_FOCUS = UINT32_MAX;
struct uiDocumentState_t {
	uint32_t page, focus;
};
struct uiRectangle_t {
	float x, y, width, height, scale;
};
bool UI_ReadDocument( const void *data, size_t size, uiDocument_t *document );
int UI_FindPage( const uiDocument_t &document, const char *name );
bool UI_OpenDocument( const uiDocument_t &document, uiDocumentState_t *state, const char *page );
void UI_Navigate( const uiDocument_t &document, uiDocumentState_t *state, int direction );
float UI_Nudge( const uiItem_t &item, float value, int direction );
bool UI_Layout( const uiDocument_t &document, const uiItem_t &item, int width, int height, float safeArea, uiRectangle_t *rectangle );

#endif
