
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

void * MemoryPool::AllocateRaw( size_t size )
{
	std::lock_guard<std::mutex> general_guard( mutex_general );
	TODO( "Implement proper memory pool functionality" );
	++allocation_counter;
	return ::operator new( size );
}

void MemoryPool::FreeRaw( void * ptr )
{
	std::lock_guard<std::mutex> general_guard( mutex_general );
	TODO( "Implement proper memory pool functionality" );
	::operator delete( ptr );
	--allocation_counter;
}

std::unique_ptr<MemoryPool>		memory_pool( new MemoryPool );

void * MemoryPool_AllocateRaw( size_t size )
{
	return memory_pool->AllocateRaw( size );
}

void MemoryPool_FreeRaw( void * ptr )
{
	memory_pool->FreeRaw( ptr );
}

}

}
