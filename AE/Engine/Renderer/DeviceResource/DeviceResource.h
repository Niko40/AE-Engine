#pragma once

#include <memory>
#include <mutex>
#include <functional>

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Vulkan/Vulkan.h"
#include <atomic>

#include "../DeviceMemory/DeviceMemoryInfo.h"
#include "../../FileSystem/FileStream.h"
#include "../../FileResource/FileResource.h"

// To add device resources seek steps 1 and 2

namespace AE
{

class Engine;
class Logger;
class Renderer;
class FileResourceManager;
class DeviceMemoryManager;
class DeviceResourceManager;
template<typename T>
class DeviceResourceHandle;

class DeviceResource;

// 1: Declare device resource classes here
class DeviceResource_Mesh;
class DeviceResource_Image;

class DeviceResource
{
	friend class DeviceResourceManager;
	friend void DeviceWorkerThread( Engine * engine, DeviceResourceManager * device_resource_manager, std::atomic_bool * thread_sleeping );
	template<typename T>
	friend class DeviceResourceHandle;

public:
	enum class Type : uint32_t
	{
		UNDEFINED				= 0,

		// 2: Add device resource types here
		GRAPHICS_PIPELINE,
//		COMPUTE_PIPELINE,

		MESH,
		IMAGE,
	};

	enum class State : uint32_t
	{
		UNLOADED,
		LOADED,

		LOADING_QUEUED,
		LOADING,
		UNLOADING,

		UNABLE_TO_LOAD,
		UNABLE_TO_UNLOAD,
	};

	enum class Flags : uint32_t
	{
		UNIQUE					= 1 << 0,			// resource should only have one user, for example for a per character deformed mesh
		STATIC					= 1 << 1,			// resource memory or any internal object should not be modified for the lifetime of the resource
	};

	enum class LoadingState : uint32_t
	{
		UNDEFINED				= 0,				// error, should never be

		UNABLE_TO_LOAD,
		CONTINUE_LOADING,
		LOADED,
	};

	enum class UnloadingState : uint32_t
	{
		UNDEFINED				= 0,				// error, should never be

		UNABLE_TO_UNLOAD,
		CONTINUE_UNLOADING,
		UNLOADED,
	};

	DeviceResource( Engine * engine, Type device_resource_type, Flags resource_flags = Flags( 0 ) );
	virtual ~DeviceResource();

	String						Debug_GetHexAddressOfThisAsString();

	bool						IsReadyForUse();
	uint32_t					GetUsers();
	Type						GetResourceType() const;
	Flags						GetFlags() const;

	// if you use DeviceResourceHandle you don't manually need to call this function
	void						Release();

private:
	uint32_t					IncrementUsers();
	uint32_t					DecrementUsers();

	// Load in loader thread
	// Load function should return LOADED when the resource is fully usable,
	// CONTINUE_LOADING if the resource needs further operations before being usable
	// ( for example if the resource needs to be sent to the physical device before finalizing
	// the resource on CPU side ) and UNABLE_TO_LOAD if there was unrecoverable error in which
	// case the resource will not be usable for the application
	// Device resource manager will not call this function until all required file resources are available
	// for use, if they have errors or they're never ready for use, this function will never be called and
	// this device resource will never be ready for use
	virtual LoadingState		Load()							= 0;

	// Unload in loader thread
	// Unload function should return UNLOADED when the resource is fully unloaded and can be deleted from memory,
	// CONTINUE_UNLOADING if the resource needs further operations before unloading is done
	// ( for example if the resource needs to change memory on the physical device before deleting vulkan objects
	// of the resource on CPU side ) and UNABLE_TO_LOAD if there was unrecoverable error in which case the object deletion is not allowed
	// this way the possible crash will not happen during runtime
	virtual UnloadingState		Unload()						= 0;

	LoadingState				LoadFromManager();
	UnloadingState				UnloadFromManager();

	bool						ContinueLoadingFromManagerCanRun();
	bool						ContinueUnloadingFromManagerCanRun();
	LoadingState				ContinueLoadingFromManager();
	UnloadingState				ContinueUnloadingFromManager();

	std::thread::id				GetWorkerThreadID();

protected:
	State						GetState();
	void						SetState( DeviceResource::State new_state );

	// MUST BE SET BEFORE EXITING Load() function or previous call to next unload operation if these functions return CONTINUE_LOADING
	// test function must return true before the operation function is called by the device resource manager
	void						SetNextLoadOperation( std::function<bool( DeviceResource* )> test, std::function<LoadingState( DeviceResource* )> operation );

	// MUST BE SET BEFORE EXITING Unload() function or previous call to next unload operation if these functions return CONTINUE_UNLOADING
	// test function must return true before the operation function is called by the device resource manager
	void						SetNextUnloadOperation( std::function<bool( DeviceResource* )> test, std::function<UnloadingState( DeviceResource* )> operation );

	Engine					*	p_engine						= nullptr;
	Logger					*	p_logger						= nullptr;
	Renderer				*	p_renderer						= nullptr;
	FileResourceManager		*	p_file_resource_manager			= nullptr;
	DeviceResourceManager	*	p_device_resource_manager		= nullptr;
	DeviceMemoryManager		*	p_device_memory_manager			= nullptr;

	VulkanDevice				ref_vk_device					= {};

private:
	// MUST BE SET IF Load() function or previous call to NextLoadOperation() returns CONTINUE_LOADING
	// if this function returns true then the NextLoadOperation() will run
	std::function<bool( DeviceResource* )>						NextLoadOperationCanRun			= nullptr;

	// MUST BE SET IF Load() function returns CONTINUE_LOADING
	// if NextLoadOperationCanRun() returns true then this function is ran next to continue the loading operation
	std::function<LoadingState( DeviceResource* )>				NextLoadOperation				= nullptr;

	// MUST BE SET IF Unload() function of previous call to NextUnloadOperation() returns CONTINUE_UNLOADING
	// if this function returns true then the NextUnloadOperation() will run
	std::function<bool( DeviceResource* )>						NextUnloadOperationCanRun		= nullptr;

	// MUST BE SET IF Unload() function returns CONTINUE_UNLOADING
	// if NextUnloadOperationCanRun() returns true then this function is ran next to continue the unloading operation
	std::function<UnloadingState( DeviceResource* )>			NextUnloadOperation				= nullptr;

	std::mutex					mutex;
	uint32_t					users							= 0;
	State						state							= State::UNLOADED;
	Flags						flags							= Flags( 0 );
	Type						type							= Type::UNDEFINED;

	// Passing around the vulkan objects from thread to thread is generally troublesome
	// we need to lock onto one of the worker threads and only use that thread to do all loading operations
	// Resource manager call to Load() function will lock all future load operations to the thread that called
	// Load() originally. Similarly call to Unload() will lock all future unload operations to
	// the same thread that called Unload() originally. See: NextLoadOperation and NextUnloadOperation
	std::thread::id				locked_worker_thread_id;

protected:
	Vector<FileResourceHandle<FileResource>>					file_resources;
};

template<typename T>
class DeviceResourceHandle
{
//	static_assert( std::is_base_of<DeviceResource, T>::value, "Not base of DeviceResource" );
	friend class DeviceResourceManager;
	template<typename R>
	friend class DeviceResourceHandle;
	friend class DeviceResourceHandle<T>;
	friend class DeviceResource;

private:
	void Destroy()
	{
		if( nullptr	!= res_handle )
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
	DeviceResourceHandle( R * resource )
	{
		static_assert( std::is_base_of<DeviceResource, R>::value, "Not base of DeviceResource" );
		assert( nullptr != resource );
		resource->IncrementUsers();
		Destroy();
		res_handle	= (T*)resource;
	}

public:
	DeviceResourceHandle() {};
	
	template<typename R>
	DeviceResourceHandle( DeviceResourceHandle<R> & other )
	{
		static_assert( std::is_base_of<DeviceResource, R>::value, "Not base of DeviceResource" );
		if( res_handle != other.res_handle ) {
			other.res_handle->IncrementUsers();
			Destroy();
			res_handle		= other.res_handle;
		}
	}
	template<typename R>
	DeviceResourceHandle( DeviceResourceHandle<R> && other )
	{
		static_assert( std::is_base_of<DeviceResource, R>::value, "Not base of DeviceResource" );
		Swap( res_handle, other.res_handle );
	}

	DeviceResourceHandle( nullptr_t )
	{
		Destroy();
	}
	~DeviceResourceHandle()
	{
		Destroy();
	}

	template<typename R>
	void operator=( DeviceResourceHandle<R> & other )
	{
		static_assert( std::is_base_of<DeviceResource, R>::value, "Not base of DeviceResource" );
		if( res_handle != other.res_handle ) {
			other.res_handle->IncrementUsers();
			Destroy();
			res_handle		= other.res_handle;
		}
	}
	template<typename R>
	void operator=( DeviceResourceHandle<R> && other )
	{
		static_assert( std::is_base_of<DeviceResource, R>::value, "Not base of DeviceResource" );
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
		static_assert( std::is_base_of<DeviceResource, R>::value, "Not base of DeviceResource" );
		return ( res_handle == other.res_handle );
	}
	bool operator==( const FileResource * resource ) const
	{
		return ( res_handle == resource );
	}
	template<typename R>
	bool operator!=( const FileResourceHandle<R> & other ) const
	{
		static_assert( std::is_base_of<DeviceResource, R>::value, "Not base of DeviceResource" );
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
	T						*	res_handle					= nullptr;
};

DeviceResource::Flags			operator|( DeviceResource::Flags f1, DeviceResource::Flags f2 );
uint32_t						operator&( DeviceResource::Flags f1, DeviceResource::Flags f2 );
DeviceResource::Flags			operator|=( DeviceResource::Flags f1, DeviceResource::Flags f2 );

}
