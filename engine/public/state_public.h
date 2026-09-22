#ifndef STATE_PUBLIC_H
#define STATE_PUBLIC_H
#include <stddef.h>
#include <stdint.h>

enum class stateType_t : uint32_t { Bytes = 1,
	Int32,
	UInt32,
	Float32,
	UInt64 };
struct stateField_t {
	const char *name;
	uint32_t offset, count;
	stateType_t type;
	uint32_t sinceVersion = 1;
};
struct stateSchema_t {
	const char *name;
	uint32_t version, minimumVersion, objectSize;
	const stateField_t *fields;
	uint32_t fieldCount;
};
// Fields carry native offsets only in memory; files carry names, types and values.
// Object and serialized buffers must not overlap.
size_t State_Write( const stateSchema_t &schema, const void *object, void *data, size_t capacity );
// Validate before mutation. Fields introduced after the source version retain
// caller defaults; sourceVersion selects the caller's explicit migration.
bool State_Read( const stateSchema_t &schema, const void *data, size_t size, void *object, uint32_t *sourceVersion );
// Caller-owned checkpoint storage. Initialize a writer with { data, capacity }.
struct stateWriter_t {
	void *data;
	size_t capacity, size = 0;
	uint32_t records = 0;
	bool failed = false;
};
struct stateReader_t {
	const void *data = nullptr;
	size_t size = 0;
	uint32_t records = 0;
};
bool State_Append( stateWriter_t *writer, const stateSchema_t &schema, uint32_t slot, const void *object );
size_t State_Finish( stateWriter_t *writer );
// A reader is published only after the complete archive validates. Its buffer
// must remain alive and unchanged until the last Find call.
bool State_Open( const void *data, size_t size, stateReader_t *reader );
bool State_Find( const stateReader_t &reader, const stateSchema_t &schema, uint32_t slot, void *object, uint32_t *sourceVersion );
#endif
