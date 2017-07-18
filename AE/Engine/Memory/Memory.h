#pragma once

#include "../BUILD_OPTIONS.h"
#include "../Platform.h"

#include "MemoryTypes.h"
#include "MemoryPool/MemoryPool.h"

namespace AE
{

template<typename T, typename ...Args>
UniquePointer<T> MakeUniquePointer( Args&&... args )
{
	static_assert( !std::is_array<T>::value, "Unique Pointer is not meant for arrays, use vector instead" );
	return UniquePointer<T>( engine_internal::MemoryPool_Allocate<T>( std::forward<Args>( args )... ), engine_internal::UniquePointerDeleter<T> );
}

}
