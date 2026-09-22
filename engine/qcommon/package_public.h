#ifndef PACKAGE_PUBLIC_H
#define PACKAGE_PUBLIC_H
#include "q_shared.h"

constexpr uint32_t PACKAGE_MAX_ASSETS = 65536;
constexpr uint64_t PACKAGE_MAX_ASSET_BYTES = 256 * 1024 * 1024;
struct packageHeader_t {
	char magic[8];
	uint32_t version, count;
	uint64_t manifestOffset, manifestSize;
	uint8_t identity[32], base[32], metadataHash[32];
};
struct packageEntry_t {
	char name[64];
	uint64_t offset, size, stored;
	uint8_t hash[32];
	uint32_t codec, flags;
};
struct package_t {
	packageHeader_t header;
	packageEntry_t *entries;
	char path[MAX_OSPATH];
};
struct packageStream_t;
package_t *Package_Load( const char *path );
void Package_Free( package_t *package );
int Package_Find( const package_t *package, const char *name );
// Mount inputs are lowest precedence first. A patch requires the exact current
// modern-package view; legacy loose/pk3 fallback is not part of that identity.
bool Package_ValidateMounts( package_t *const *packages, uint32_t count, uint8_t identity[32] );
// Open verifies the complete asset before exposing bytes. Stored assets retain a
// platform file stream; compressed assets retain a bounded decoded zone buffer.
packageStream_t *Package_OpenAsset( const package_t *package, uint32_t index );
int Package_ReadAsset( packageStream_t *stream, void *data, int size );
bool Package_SeekAsset( packageStream_t *stream, uint64_t position );
uint64_t Package_TellAsset( const packageStream_t *stream );
void Package_CloseAsset( packageStream_t *stream );
#endif
