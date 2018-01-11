#pragma once

#include <mutex>
#include <atomic>

#include "../BUILD_OPTIONS.h"
#include "../Platform.h"

#include "../Memory/Memory.h"
#include "../FileSystem/FileStream.h"
#include "../CppFileSystem/CppFileSystem.h"

namespace AE
{

class Engine;
class FileResourceManager;

class FileResource
{
	friend class FileResourceManager;
	friend void FileWorkerThread( Engine * engine, FileResourceManager * file_resource_manager, std::atomic_bool * thread_sleeping );
	template<typename R>
	friend class FileResourceHandle;

public:
	enum class State : uint32_t
	{
		UNLOADED,
		LOADED,

		LOADING_QUEUED,
		LOADING,
		UNLOADING,

		UNABLE_TO_LOAD,
		UNABLE_TO_UNLOAD,

		UNABLE_TO_LOAD_FILE_NOT_FOUND,
	};

	enum class Type : uint32_t
	{
		UNDEFINED				= 0,		// error value
		RAW_DATA,

		XML,
		LUA,
		MESH,
		IMAGE,
	};

								FileResource( Engine * engine, FileResourceManager * file_resource_manager, Type file_resource_type );
	virtual						~FileResource();

	bool						IsResourceReadyForUse();
	bool						IsResourceOK();
	uint32_t					GetResourceUsers();
	Type						GetResourceType() const;

	// if you use FileResourceHandle you don't manually need to call this function
	void						Release();

	const Path				&	GetPath() const;

private:
	uint32_t					IncrementUsers();
	uint32_t					DecrementUsers();

	// Load and Unload functions should return true if operation was successful
	// Returning false on Load will result in error, unusable resource and will not get destroyed until the application closes
	// Returning false on Unload will result in resource not destroyed until the application closes
	virtual bool				Load( FileStream * stream, const Path & path )	= 0;	// Load in loader thread
	virtual bool				Unload()														= 0;	// Unload in loader thread

	bool						LoadFromManager( FileStream * stream, const Path & path );
	bool						UnloadFromManager();

public:
	State						GetResourceState();

private:
	void						SetResourceState( State new_state );

protected:
	Engine					*	p_engine					= nullptr;
	FileResourceManager		*	p_resource_manager			= nullptr;

private:
	std::mutex					mutex;
	uint32_t					users						= 0;
	State						state						= State::UNLOADED;
	Type						type						= Type::UNDEFINED;
	Path						resource_path;

#if BUILD_INCLUDE_RESOURCE_STATISTICS
	uint64_t					load_time					= 0;
	uint64_t					unload_time					= 0;
#endif
};

template<typename T>
class FileResourceHandle
{
//	static_assert( std::is_base_of<FileResource, T>::value, "Not base of FileResource" );
	friend class FileResourceManager;
	template<typename R>
	friend class FileResourceHandle;
	friend class FileResourceHandle<T>;
	friend class FileResource;

private:
	void Destroy()
	{
		if( nullptr != res_handle )
			res_handle->Release();
		res_handle	= nullptr;
	}

	template<typename R>
	void Swap( T *& myval, R *& otherval )
	{
		auto other_handle	= otherval;
		otherval			= myval;
		myval				= (T*)other_handle;
	}

	template<typename R>
	FileResourceHandle( R * resource )
	{
		static_assert( std::is_base_of<FileResource, R>::value, "Not base of FileResource" );
		assert( nullptr != resource );
		resource->IncrementUsers();
		Destroy();
		res_handle	= (T*)resource;
	}

public:
	FileResourceHandle() {};

	template<typename R>
	FileResourceHandle( FileResourceHandle<R> & other )
	{
		static_assert( std::is_base_of<FileResource, R>::value, "Not base of FileResource" );
		if( res_handle != other.res_handle ) {
			other.res_handle->IncrementUsers();
			Destroy();
			res_handle		= other.res_handle;
		}
	}
	template<typename R>
	FileResourceHandle( FileResourceHandle<R> && other )
	{
		static_assert( std::is_base_of<FileResource, R>::value, "Not base of FileResource" );
		Swap( res_handle, other.res_handle );
	}

	FileResourceHandle( nullptr_t )
	{
		Destroy();
	}
	~FileResourceHandle()
	{
		Destroy();
	}

	template<typename R>
	void operator=( FileResourceHandle<R> & other )
	{
		static_assert( std::is_base_of<FileResource, R>::value, "Not base of FileResource" );
		if( res_handle != other.res_handle ) {
			other.res_handle->IncrementUsers();
			Destroy();
			res_handle		= other.res_handle;
		}
	}
	template<typename R>
	void operator=( FileResourceHandle<R> && other )
	{
		static_assert( std::is_base_of<FileResource, R>::value, "Not base of FileResource" );
		Swap( res_handle, other.res_handle );
	}
	void operator=( nullptr_t )
	{
		Destroy();
	}
	template<typename R>
	void operator=( R ) = delete;

	template<typename R>
	bool operator==( const FileResourceHandle<R> & other ) const
	{
		static_assert( std::is_base_of<FileResource, R>::value, "Not base of FileResource" );
		return ( res_handle == other.res_handle );
	}
	bool operator==( const FileResource * resource ) const
	{
		return ( res_handle == resource );
	}
	template<typename R>
	bool operator!=( const FileResourceHandle<R> & other ) const
	{
		static_assert( std::is_base_of<FileResource, R>::value, "Not base of FileResource" );
		return ( res_handle != other.res_handle );
	}
	bool operator!=( const FileResource * resource ) const
	{
		return ( res_handle != resource );
	}

	operator bool() const
	{
		return ( nullptr != res_handle );
	}

	T *& operator->()
	{
		return res_handle;
	}

	T * Get()
	{
		return res_handle;
	}
	
protected:
	T					*	res_handle					= nullptr;
};

}
