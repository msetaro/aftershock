#pragma once
#include <cstdint>
#include <cstdlib>
#include <new>

static uint32_t cppAllocations;
static void *CPPAllocate( size_t size, size_t alignment = 0 ) {
	++cppAllocations;
	if ( !size )
		size = 1;
	if ( alignment && size > SIZE_MAX - ( alignment - 1 ) )
		std::abort();
	void *block = alignment ? std::aligned_alloc( alignment, ( size + alignment - 1 ) & ~( alignment - 1 ) ) : std::malloc( size );
	if ( !block )
		std::abort();
	return block;
}
void *operator new( size_t size ) {
	return CPPAllocate( size );
}
void *operator new[]( size_t size ) {
	return CPPAllocate( size );
}
void *operator new( size_t size, std::align_val_t alignment ) {
	return CPPAllocate( size, size_t( alignment ) );
}
void *operator new[]( size_t size, std::align_val_t alignment ) {
	return CPPAllocate( size, size_t( alignment ) );
}
void operator delete( void *block ) noexcept {
	std::free( block );
}
void operator delete[]( void *block ) noexcept {
	std::free( block );
}
void operator delete( void *block, size_t ) noexcept {
	std::free( block );
}
void operator delete[]( void *block, size_t ) noexcept {
	std::free( block );
}
void operator delete( void *block, std::align_val_t ) noexcept {
	std::free( block );
}
void operator delete[]( void *block, std::align_val_t ) noexcept {
	std::free( block );
}
void operator delete( void *block, size_t, std::align_val_t ) noexcept {
	std::free( block );
}
void operator delete[]( void *block, size_t, std::align_val_t ) noexcept {
	std::free( block );
}
