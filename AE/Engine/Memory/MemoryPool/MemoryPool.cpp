
#include <memory>

#include "MemoryPool.h"

#include <iostream>

namespace AE
{

namespace engine_internal
{

MemoryPool::MemoryPool()
{
}

MemoryPool::~MemoryPool()
{
	assert( 0 == allocation_counter );
}

void * MemoryPool::AllocateRaw( size_t size, size_t alignment )
{
	LOCK_GUARD( mutex_general );
	TODO( "Implement proper memory pool functionality" );
	++allocation_counter;
//	return ::operator new( size );
	return _aligned_malloc( size, alignment );
}

void * MemoryPool::ReallocateRaw( void * old_ptr, size_t new_size, size_t alignment )
{
	LOCK_GUARD( mutex_general );
	TODO( "Implement proper memory pool functionality" );
	auto memory = _aligned_realloc( old_ptr, new_size, alignment );
	if( memory ) {
		return memory;
	} else {
		FreeRaw( old_ptr );
		return nullptr;
	}
}

void MemoryPool::FreeRaw( void * ptr )
{
	LOCK_GUARD( mutex_general );
	TODO( "Implement proper memory pool functionality" );
//	::operator delete( ptr );
	if( ptr ) {
		_aligned_free( ptr );
		--allocation_counter;
	}
}

std::unique_ptr<MemoryPool>		memory_pool( new MemoryPool );

void * MemoryPool_AllocateRaw( size_t size, size_t alignment )
{
	return memory_pool->AllocateRaw( size, alignment );
}

void * MemoryPool_ReallocateRaw( void * old_ptr, size_t new_size, size_t alignment )
{
	return memory_pool->ReallocateRaw( old_ptr, new_size, alignment );
}

void MemoryPool_FreeRaw( void * ptr )
{
	memory_pool->FreeRaw( ptr );
}

}

}
