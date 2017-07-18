#pragma once

#include <memory>
#include <mutex>

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../MemoryTypes.h"

namespace AE
{

namespace engine_internal
{

class MemoryPool
{
public:
	MemoryPool();
	~MemoryPool();

	void	*	AllocateRaw( size_t size );
	void		FreeRaw( void * ptr );

	template<typename T, typename ...Args>
	T * Allocate( Args&&... args )
	{
		return new( AllocateRaw( sizeof( T ) ) )T( std::forward<Args>( args )... );
	}

	template<typename T>
	void Free( T * ptr )
	{
		ptr->~T();
		FreeRaw( ptr );
	}

private:
	std::mutex			mutex_general;

	int64_t				allocation_counter			= 0;
};

extern std::unique_ptr<MemoryPool>		memory_pool;

template<typename T, typename ...Args>
T * MemoryPool_Allocate( Args&&... args )
{
	//	std::unique_ptr<T, engine_internal::MemoryDeleter<T>> ret;
	return memory_pool->Allocate<T>( std::forward<Args>( args )... );
}

template<typename T>
void		MemoryPool_Free( T * ptr )
{
	memory_pool->Free( ptr );
}

void	*	MemoryPool_AllocateRaw( size_t size );

void		MemoryPool_FreeRaw( void * ptr );

}

}
