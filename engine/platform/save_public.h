#ifndef SAVE_PUBLIC_H
#define SAVE_PUBLIC_H

#include <stddef.h>

enum class saveKind_t { Profile,
	Game };
enum class saveWriteResult_t { Failed,
	Exists,
	Written };
struct saveProvider_t {
	// Main-thread, synchronous, active user/title namespace. No Com_Error or reentry.
	// Null data queries size; otherwise read the entire record or return -1.
	int ( *read )( const char *path, void *data, int capacity );
	// Commit a new record exclusively; Exists never changes the old bytes.
	// Failed must not publish a partial record. No PC fallback on provider errors.
	saveWriteResult_t ( *create )( const char *path, const void *data, int size );
};
// Install before first IO. A null provider selects the PC user-directory backend.
bool Sys_InstallSaveProvider( const saveProvider_t *provider );
bool Sys_SaveRevision( saveKind_t kind, const char *name, const void *data, int size, char *path, size_t capacity );
int Sys_ReadSave( const char *path, void *data, int capacity );

#endif
