#ifndef CONTENT_PUBLIC_H
#define CONTENT_PUBLIC_H
#include <stddef.h>
#include <stdint.h>

// Read-only main-thread stream. The owner closes it explicitly before reuse.
struct sysContentFile_t {
	void *handle;
	uint64_t size;
};
bool Sys_OpenContentFile( const char *path, sysContentFile_t *file );
bool Sys_ReadContentFile( const sysContentFile_t &file, uint64_t offset, void *data, size_t size );
void Sys_CloseContentFile( sysContentFile_t *file );
#endif
