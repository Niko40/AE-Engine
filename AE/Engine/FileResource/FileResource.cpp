
#include <assert.h>
#include <chrono>

#include "FileResource.h"

namespace AE
{

FileResource::FileResource( Engine * engine, FileResourceManager * file_resource_manager, FileResource::Type file_resource_type )
{
	p_engine				= engine;
	p_resource_manager		= file_resource_manager;
	type					= file_resource_type;
	assert( nullptr != p_engine );
	assert( nullptr != p_resource_manager );
	assert( type != FileResource::Type::UNDEFINED );
}

FileResource::~FileResource()
{
}

bool FileResource::IsReadyForUse()
{
	std::lock_guard<std::mutex> guard( mutex );
	return ( State::LOADED == state );
}

uint32_t FileResource::GetUsers()
{
	std::lock_guard<std::mutex> guard( mutex );
	return users;
}

FileResource::Type FileResource::GetResourceType() const
{
	return type;
}

void FileResource::Release()
{
	DecrementUsers();
}

const Path & FileResource::GetPath() const
{
	return resource_path;
}

uint32_t FileResource::IncrementUsers()
{
	std::lock_guard<std::mutex> guard( mutex );
	assert( users < UINT32_MAX );
	++users;
	return users;
}

uint32_t FileResource::DecrementUsers()
{
	std::lock_guard<std::mutex> guard( mutex );
	--users;
	assert( users < UINT32_MAX );
	return users;
}

bool FileResource::LoadFromManager( FileStream * stream, const Path & path )
{
#if BUILD_INCLUDE_RESOURCE_STATISTICS
	auto time_point1	= std::chrono::high_resolution_clock::now();
#endif
	auto result			= Load( stream, path );
#if BUILD_INCLUDE_RESOURCE_STATISTICS
	auto time_point2	= std::chrono::high_resolution_clock::now();
	auto time_span		= std::chrono::duration_cast<std::chrono::milliseconds>( time_point2 - time_point1 );
	load_time			= time_span.count();
#endif
	return result;
}

bool FileResource::UnloadFromManager()
{
#if BUILD_INCLUDE_RESOURCE_STATISTICS
	auto time_point1	= std::chrono::high_resolution_clock::now();
#endif
	auto result			= Unload();
#if BUILD_INCLUDE_RESOURCE_STATISTICS
	auto time_point2	= std::chrono::high_resolution_clock::now();
	auto time_span		= std::chrono::duration_cast<std::chrono::milliseconds>( time_point2 - time_point1 );
	unload_time			= time_span.count();
#endif
	return result;
}

FileResource::State FileResource::GetState()
{
	std::lock_guard<std::mutex> guard( mutex );
	return state;
}

void FileResource::SetState( State new_state )
{
	std::lock_guard<std::mutex> guard( mutex );
	state		= new_state;
}

}
