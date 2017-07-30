
#include "DeviceResource.h"

#include <sstream>
#include <thread>
#include <mutex>

#include "../../Engine.h"
#include "../Renderer.h"

namespace AE
{

DeviceResource::DeviceResource( Engine * engine, Type device_resource_type, DeviceResource::Flags resource_flags )
{
	p_engine					= engine;
	assert( p_engine );
	p_logger					= p_engine->GetLogger();
	p_renderer					= p_engine->GetRenderer();
	p_file_resource_manager		= p_engine->GetFileResourceManager();
	assert( p_logger );
	assert( p_renderer );
	assert( p_file_resource_manager );
	p_device_memory_manager		= p_renderer->GetDeviceMemoryManager();
	p_device_resource_manager	= p_renderer->GetDeviceResourceManager();
	assert( p_device_memory_manager );
	assert( p_device_resource_manager );

	flags	|= resource_flags;
	type	= device_resource_type;

	ref_vk_device				= p_renderer->GetVulkanDevice();

	assert( type != Type::UNDEFINED );
}

DeviceResource::~DeviceResource()
{
}

String DeviceResource::Debug_GetHexAddressOfThisAsString()
{
	std::stringstream ss;
	ss << reinterpret_cast<const void*>( this );
	return ss.str().c_str();
}

bool DeviceResource::IsResourceReadyForUse()
{
	std::lock_guard<std::mutex> resource_guard( mutex );
	return ( State::LOADED == state );
}

bool DeviceResource::IsResourceOK()
{
	LOCK_GUARD( mutex );
	return !( State::UNABLE_TO_LOAD == state || State::UNABLE_TO_UNLOAD == state );
}

uint32_t DeviceResource::GetResourceUsers()
{
	std::lock_guard<std::mutex> resource_guard( mutex );
	return users;
}

DeviceResource::Type DeviceResource::GetResourceType() const
{
	return type;
}

DeviceResource::Flags DeviceResource::GetResourceFlags() const
{
	return flags;
}

void DeviceResource::Release()
{
	DecrementUsers();
}

uint32_t DeviceResource::IncrementUsers()
{
	std::lock_guard<std::mutex> resource_guard( mutex );
	++users;
	assert( !( uint32_t( flags & Flags::UNIQUE ) && ( users != 1 ) ) );
	return users;
}

uint32_t DeviceResource::DecrementUsers()
{
	std::lock_guard<std::mutex> resource_guard( mutex );
	--users;
	assert( !( uint32_t( flags & Flags::UNIQUE ) && ( users != 0 ) ) );
	return users;
}

DeviceResource::LoadingState DeviceResource::LoadFromManager()
{
	{
		std::lock_guard<std::mutex> resource_guard( mutex );
		locked_worker_thread_id		= std::this_thread::get_id();
	}
	auto result			= Load();
	return result;
}

DeviceResource::UnloadingState DeviceResource::UnloadFromManager()
{
	auto result			= Unload();
	return result;
}

bool DeviceResource::ContinueLoadingFromManagerCanRun()
{
	assert( nullptr != NextLoadOperationCanRun );
	return NextLoadOperationCanRun( this );
}

bool DeviceResource::ContinueUnloadingFromManagerCanRun()
{
	assert( nullptr != NextUnloadOperationCanRun );
	return NextUnloadOperationCanRun( this );
}

DeviceResource::LoadingState DeviceResource::ContinueLoadingFromManager()
{
	assert( locked_worker_thread_id == std::this_thread::get_id() );
	assert( nullptr != NextLoadOperation );
	return NextLoadOperation( this );
}

DeviceResource::UnloadingState DeviceResource::ContinueUnloadingFromManager()
{
	assert( locked_worker_thread_id == std::this_thread::get_id() );
	assert( nullptr != NextUnloadOperation );
	return NextUnloadOperation( this );
}

std::thread::id DeviceResource::GetWorkerThreadID()
{
	std::lock_guard<std::mutex> resource_guard( mutex );
	return locked_worker_thread_id;
}

DeviceResource::State DeviceResource::GetResourceState()
{
	std::lock_guard<std::mutex> resource_guard( mutex );
	return state;
}

void DeviceResource::SetResourceState( DeviceResource::State new_state )
{
	std::lock_guard<std::mutex> resource_guard( mutex );
	state		= new_state;
}

void DeviceResource::SetNextLoadOperation( std::function<bool( DeviceResource* )> test, std::function<LoadingState( DeviceResource* )> operation )
{
	NextLoadOperationCanRun		= test;
	NextLoadOperation			= operation;
}

void DeviceResource::SetNextUnloadOperation( std::function<bool( DeviceResource* )> test, std::function<UnloadingState( DeviceResource* )> operation )
{
	NextUnloadOperationCanRun	= test;
	NextUnloadOperation			= operation;
}

DeviceResource::Flags operator|( DeviceResource::Flags f1, DeviceResource::Flags f2 )
{
	return DeviceResource::Flags( uint32_t( f1 ) | uint32_t( f2 ) );
}

uint32_t operator&( DeviceResource::Flags f1, DeviceResource::Flags f2 )
{
	return uint32_t( f1 ) & uint32_t( f2 );
}

DeviceResource::Flags operator|=( DeviceResource::Flags f1, DeviceResource::Flags f2 )
{
	return f1 | f2;
}

}
