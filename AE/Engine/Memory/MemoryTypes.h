#pragma once

#include "../Platform.h"

#include <assert.h>
#include <limits>
#include <memory>
#include <array>
#include <vector>
#include <list>
#include <string>
#include <map>
#include <filesystem>
#include <glm/glm.hpp>

#include <iostream>

#undef min
#undef max

namespace AE
{

namespace engine_internal
{

template<typename T>
class MemoryAllocator
{
public:
	// type definitions
	typedef T				value_type;
	typedef T*				pointer;
	typedef const T*		const_pointer;
	typedef T&				reference;
	typedef const T&		const_reference;
	typedef std::size_t		size_type;
	typedef std::ptrdiff_t	difference_type;

	// rebind allocator to type U
	template <class U>
	struct rebind
	{
		typedef MemoryAllocator<U> other;
	};

	// return address of values
	pointer address( reference value ) const
	{
		return &value;
	}
	const_pointer address( const_reference value ) const
	{
		return &value;
	}

	/* constructors and destructor
	* - nothing to do because the allocator has no state
	*/
	MemoryAllocator() throw( )
	{
	}
	MemoryAllocator( const MemoryAllocator& ) throw( )
	{
	}
	template <class U>
	MemoryAllocator( const MemoryAllocator<U>& ) throw( )
	{
	}
	~MemoryAllocator() throw( )
	{
	}

	// return maximum number of elements that can be allocated
	size_type max_size() const throw( )
	{
		return std::numeric_limits<std::size_t>::max() / sizeof( T );
	}

	// allocate but don't initialize num elements of type T
	pointer allocate( size_type num, const void* = 0 )
	{
		return (pointer)( MemoryPool_AllocateRaw( num * sizeof( T ) ) );
	}

	// initialize elements of allocated storage p with value value
	template<typename ... Args>
	void construct( pointer p, const Args&&... args )
	{
		// initialize memory with placement new
		new( (void*)p )T( args... );
	}

	// destroy elements of initialized storage p
	void destroy( pointer p )
	{
		// destroy objects by calling their destructor
		p->~T();
	}

	// deallocate storage p of deleted elements
	void deallocate( pointer p, size_type num )
	{
		MemoryPool_FreeRaw( (void*)p );
	}
};

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
bool operator== ( const MemoryAllocator<T1>&,
	const MemoryAllocator<T2>& ) throw( )
{
	return true;
}
template <class T1, class T2>
bool operator!= ( const MemoryAllocator<T1>&,
	const MemoryAllocator<T2>& ) throw( )
{
	return false;
}



template<typename T>
class UniquePointer;

template<typename T>
class SharedPointer;

template<typename T>
void UniquePointerDeleter( void * ptr )
{
	MemoryPool_Free( static_cast<T*>( ptr ) );
};
template<typename T>
void SharedPointerDeleter( void * ptr )
{
	MemoryPool_Free( static_cast<T*>( ptr ) );
};
template<typename T>
void SharedPointerDataDeleter( void * ptr )
{
	MemoryPool_Free( static_cast<T*>( ptr ) );
};

using UniquePointerDeleterFunction = void( *)( void* );
using SharedPointerDeleterFunction = void( *)( void* );
using SharedPointerDataDeleterFunction = void( *)( void* );

}

// UniquePointer, USE MakeUniquePointer() FUNCTION TO CREATE THIS OBJECT
// othervise you will risk mixing regular memory allocations and pool memory allocations
template<typename T>
class UniquePointer
{
	friend class UniquePointer;

private:
	T													*	ptr			= nullptr;
	engine_internal::UniquePointerDeleterFunction			deleter		= nullptr;

public:
	void Destroy()
	{
		if( nullptr != ptr ) {
			assert( nullptr != deleter );
			deleter( ptr );
			ptr			= nullptr;
			deleter		= nullptr;
		}
	}

	UniquePointer() {}

	UniquePointer( nullptr_t )
	{
		Destroy();
	}

	UniquePointer( T * pointer, engine_internal::UniquePointerDeleterFunction deleter_function )
	{
		Destroy();
		ptr			= pointer;
		deleter		= deleter_function;
	}
	// this function is purely to fix any errors with objects that assign 0 as a value to this object
	UniquePointer( uint64_t na )
	{
		assert( na == 0 );
		Destroy();
	}

	UniquePointer( UniquePointer<T> && other )
	{
		std::swap( ptr,		other.ptr );
		std::swap( deleter,	other.deleter );
	}
	UniquePointer( const UniquePointer<T> & other ) = delete;

	template<typename TO>
	UniquePointer( UniquePointer<TO> && other )
	{
		// We want to assign child objects to base class unique pointers, we can't swap so we just destroy and copy
		Destroy();
		ptr				= other.ptr;
		deleter			= other.deleter;
		other.ptr		= nullptr;
		other.deleter	= nullptr;
	}
	template<typename TO>
	UniquePointer( const UniquePointer<TO> & other ) = delete;

	~UniquePointer()
	{
		Destroy();
	}

	// this operator is purely to fix any errors with objects that assign 0 as a value to this object
	void operator=( uint64_t na )
	{
		assert( na == 0 );
		Destroy();
	}

	void operator=( UniquePointer<T> && other )
	{
		std::swap( ptr, other.ptr );
		std::swap( deleter, other.deleter );
	}
	void operator=( const UniquePointer<T> & other ) = delete;

	template<typename TO>
	void operator=( UniquePointer<TO> && other )
	{
		// We want to assign child objects to base class unique pointers, we can't swap so we just destroy and copy
		Destroy();
		ptr				= other.ptr;
		deleter			= other.deleter;
		other.ptr		= nullptr;
		other.deleter	= nullptr;
	}
	template<typename TO>
	void operator=( const UniquePointer<TO> & other ) = delete;

	T *& operator->()
	{
		return ptr;
	}
	typename std::add_lvalue_reference<T>::type operator*() const
	{
		return *ptr;
	}

	operator bool() const
	{
		return ( nullptr != ptr );
	}

	T * Get() const
	{
		return ptr;
	}
};

template<typename T>
class SharedPointerData
{
	template<typename TO>
	friend class SharedPointer;

	T													*	ptr					= nullptr;
	engine_internal::SharedPointerDeleterFunction			deleter				= nullptr;
	engine_internal::SharedPointerDataDeleterFunction		structure_deleter	= nullptr;
	int64_t													counter				= 0;
};

// SharedPointer, USE MakeSharedPointer() FUNCTION TO CREATE THIS OBJECT
// othervise you will risk mixing regular memory allocations and pool memory allocations
template<typename T>
class SharedPointer
{
	friend class SharedPointer;

private:
	SharedPointerData<T>								*	data			= nullptr;;

public:
	void DecRef()
	{
		if( nullptr != data ) {
			data->counter--;
			if( data->counter < 0 )	data->counter = 0;
			if( data->counter <= 0 ) {
				if( nullptr != data->ptr ) {
					assert( nullptr != data->deleter );
					data->deleter( data->ptr );
				}
				data->structure_deleter( data );
				data = nullptr;
			}
		}
	}
	void IncRef()
	{
		data->counter++;
	}

	SharedPointer() {}

	SharedPointer( nullptr_t )
	{
		DecRef();
	}

	// Do not construct from unique pointers
	template<typename Any>
	SharedPointer( UniquePointer<Any> ) = delete;

	SharedPointer(
		SharedPointerData<T> * structure_pointer,
		T * data_pointer,
		engine_internal::SharedPointerDataDeleterFunction structure_deleter_function,
		engine_internal::SharedPointerDeleterFunction deleter_function )
	{
		DecRef();
		data					= structure_pointer;
		data->ptr				= data_pointer;
		data->deleter			= deleter_function;
		data->structure_deleter	= structure_deleter_function;
		IncRef();
	}
	// this function is purely to fix any errors with objects that assign 0 as a value to this object
	SharedPointer( uint64_t na )
	{
		assert( na == 0 );
		DecRef();
	}

	SharedPointer( SharedPointer<T> && other )
	{
		if( *this != other ) {
			std::swap( data, other.data );
		}
	}
	SharedPointer( const SharedPointer<T> & other )
	{
		if( *this != other ) {
			DecRef();
			data	= other.data;
			IncRef();
		}
	}

	template<typename TO>
	SharedPointer( SharedPointer<TO> && other )
	{
		// We want to assign child objects to base class shared pointers, we can't swap so treat this like normal reference
		if( *this != other ) {
			DecRef();
			data	= other.data;
			IncRef();
		}
	}
	template<typename TO>
	SharedPointer( const SharedPointer<TO> & other )
	{
		if( *this != other ) {
			DecRef();
			data	= other.data;
			IncRef();
		}
	}

	~SharedPointer()
	{
		DecRef();
	}

	// this operator is purely to fix any errors with objects that assign 0 as a value to this object
	void operator=( uint64_t na )
	{
		assert( na == 0 );
		DecRef();
	}

	// Do not copy from unique pointers
	template<typename Any>
	void operator=( UniquePointer<Any> ) = delete;

	void operator=( SharedPointer<T> && other )
	{
		if( *this != other ) {
			std::swap( data, other.data );
		}
	}
	void operator=( const SharedPointer<T> & other )
	{
		if( *this != other ) {
			DecRef();
			data	= other.data;
			IncRef();
		}
	}

	template<typename TO>
	void operator=( SharedPointer<TO> && other )
	{
		// We want to assign child objects to base class shared pointers, we can't swap so treat this like normal reference
		if( *this != other ) {
			DecRef();
			data	= other.data;
			IncRef();
		}
	}
	template<typename TO>
	void operator=( const SharedPointer<TO> & other )
	{
		if( *this != other ) {
			DecRef();
			data	= other.data;
			IncRef();
		}
	}

	bool operator==( const SharedPointer<T> & other )
	{
		if( nullptr != data && nullptr != other.data ) {
			return ( data->ptr == other.data->ptr );
		}
		return ( nullptr == data && nullptr == other.data );
	}
	bool operator==( const SharedPointer<T> && other )
	{
		if( nullptr != data && nullptr != other.data ) {
			return ( data->ptr == other.data->ptr );
		}
		return ( nullptr == data && nullptr == other.data );
	}
	template<typename TO>
	bool operator==( const SharedPointer<TO> & other )
	{
		if( nullptr != data && nullptr != other.data ) {
			return ( data->ptr == other.data->ptr );
		}
		return ( nullptr == data && nullptr == other.data );
	}
	template<typename TO>
	bool operator==( const SharedPointer<TO> && other )
	{
		if( nullptr != data && nullptr != other.data ) {
			return ( data->ptr == other.data->ptr );
		}
		return ( nullptr == data && nullptr == other.data );
	}

	bool operator!=( const SharedPointer<T> & other )
	{
		return !( *this == other );
	}
	bool operator!=( const SharedPointer<T> && other )
	{
		return !( *this == other );
	}
	template<typename TO>
	bool operator!=( const SharedPointer<TO> & other )
	{
		return !( *this == other );
	}
	template<typename TO>
	bool operator!=( const SharedPointer<TO> && other )
	{
		return !( *this == other );
	}

	T *& operator->()
	{
		assert( nullptr != data );
		return data->ptr;
	}
	typename std::add_lvalue_reference<T>::type operator*() const
	{
		assert( nullptr != data );
		return *data->ptr;
	}

	operator bool() const
	{
		if( nullptr != data ) {
			return ( nullptr != data->ptr );
		}
		return false;
	}

	T * Get() const
	{
		if( nullptr != data ) {
			return data->ptr;
		}
		return nullptr;
	}
};



template<typename T, size_t S>
using Array				= std::array<T, S>;

template<typename T>
using Vector			= std::vector<T, engine_internal::MemoryAllocator<T>>;

template<typename T>
using List				= std::list<T, engine_internal::MemoryAllocator<T>>;

using String			= std::basic_string<char, std::char_traits<char>, engine_internal::MemoryAllocator<char>>;
using WString			= std::basic_string<wchar_t, std::char_traits<wchar_t>, engine_internal::MemoryAllocator<wchar_t>>;

template<typename T1, typename T2>
using Pair				= std::pair<T1, T2>;

template<typename T1, typename T2>
using Map				= std::map<T1, T2, std::less<T1>, engine_internal::MemoryAllocator<Pair<T1, T2>>>;

using GridCoords2D		= glm::tvec2<int32_t, glm::highp>;

template<typename T>
class DynamicGrid2D
{
public:
	DynamicGrid2D() {};
	~DynamicGrid2D() {};

	typename Vector<T>::iterator		begin()
	{
		return data.begin();
	}
	typename Vector<T>::iterator		end()
	{
		return data.end();
	}

	void								Resize( GridCoords2D lower, GridCoords2D upper )
	{
		upper.x					+= 1;
		upper.y					+= 1;

		auto cur_size_x			= limit_upper_x - limit_lower_x;
		auto cur_size_y			= limit_upper_y - limit_lower_y;
		auto new_size_x			= upper.x - lower.x;
		auto new_size_y			= upper.y - lower.y;

		auto lowest_common_x	= std::max( lower.x, limit_lower_x );
		auto lowest_common_y	= std::max( lower.y, limit_lower_y );
		auto highest_common_x	= std::min( upper.x, limit_upper_x );
		auto highest_common_y	= std::min( upper.y, limit_upper_y );

		Vector<T>				new_data;
		new_data.resize( new_size_x * new_size_y );

		for( int32_t y = lowest_common_y; y < highest_common_y; ++y ) {
			for( int32_t x = lowest_common_x; x < highest_common_x; ++x ) {
				auto cur_index			= IndexFromRawCoords( cur_size_x, { x - limit_lower_x, y - limit_lower_y } );
				auto new_index			= IndexFromRawCoords( new_size_x, { x - lower.x, y - lower.y } );
				new_data[ new_index ]	=  std::move( data[ cur_index ] );
			}
		}
		data			= std::move( new_data );
		limit_lower_x	= lower.x;
		limit_lower_y	= lower.y;
		limit_upper_x	= upper.x;
		limit_upper_y	= upper.y;
	}

	T								&	operator[]( GridCoords2D coords )
	{
		if( coords.x < limit_lower_x || coords.x >= limit_upper_x ||
			coords.y < limit_lower_y || coords.y >= limit_upper_y ) {
			Resize (
				{ std::min( coords.x, limit_lower_x ), std::min( coords.y, limit_lower_y ) },
				{ std::max( coords.x, limit_upper_x - 1 ), std::max( coords.y, limit_upper_y - 1 ) } );
		}
		auto cur_size_x		= limit_upper_x - limit_lower_x;
		return data[ IndexFromRawCoords( cur_size_x, coords - GridCoords2D( limit_lower_x, limit_lower_y ) ) ];
	}

	DynamicGrid2D<T>				&	operator=( const DynamicGrid2D<T> & other )
	{
		limit_lower_x		= other.limit_lower_x;
		limit_lower_y		= other.limit_lower_y;
		limit_upper_x		= other.limit_upper_x;
		limit_upper_y		= other.limit_upper_y;
		data				= other.data;
		return *this;
	}
	DynamicGrid2D<T>				&	operator=( const DynamicGrid2D<T> && other )
	{
		std::swap( limit_lower_x, other.limit_lower_x );
		std::swap( limit_lower_y, other.limit_lower_y );
		std::swap( limit_upper_x, other.limit_upper_x );
		std::swap( limit_upper_y, other.limit_upper_y );
		std::swap( data, other.data );
		return *this;
	}

private:
	GridCoords2D						RawCoordsFromIndex( int32_t raw_size_x, uint32_t index ) const
	{
		assert( raw_size_x > 0 );
		GridCoords2D ret;
		ret.y				= index / raw_size_x;
		ret.x				= index % raw_size_x;
	}
	int32_t								IndexFromRawCoords( int32_t raw_size_x, const GridCoords2D & raw_coords ) const
	{
		assert( raw_size_x > 0 );
		return raw_coords.y * raw_size_x + raw_coords.x;
	}

	int32_t				limit_lower_x;
	int32_t				limit_upper_x;
	int32_t				limit_lower_y;
	int32_t				limit_upper_y;
	Vector<T>			data;
};

}
