
#include <sstream>

#include "FileResourceManager.h"

#include "../Engine.h"
#include "../Logger/Logger.h"

#include "../FileResource/FileResource.h"
#include "RawData/FileResource_RawData.h"
#include "XML/FileResource_XML.h"
#include "Lua/FileResource_Lua.h"
#include "Mesh/FileResource_Mesh.h"
#include "Image/FileResource_Image.h"

namespace AE
{

void FileWorkerThread( Engine * engine, FileResourceManager * file_resource_manager, std::atomic_bool * thread_sleeping )
{
	assert( nullptr != engine );
	assert( nullptr != file_resource_manager );
	Logger					*	p_logger			= engine->GetLogger();
	FileSystem				*	p_filesystem		= engine->GetFileSystem();
	assert( p_filesystem );
	assert( p_logger );

	while( !file_resource_manager->worker_threads_should_exit ) {
		{
			std::unique_lock<std::mutex> wakeup_guard( file_resource_manager->worker_threads_wakeup_mutex );
			*thread_sleeping	= true;
			file_resource_manager->worker_threads_wakeup.wait( wakeup_guard );
			*thread_sleeping	= false;
		}
		// look for loading work first
		bool load_operation_ran				= false;
		if( file_resource_manager->allow_resource_loading ) {
			bool more_work_available		= true;
			// loop and load everything until the load list is empty
			// it's likely the rest of the program is faster at giving
			// tasks than the loader thread is able to process them
			// this way we don't have to wait a call from the resource manager
			// to initiate resource loading if there is pending work
			while( more_work_available ) {
				Path				path;
				FileResource	*	resource	= nullptr;
				{
					std::lock_guard<std::mutex> load_list_guard( file_resource_manager->mutex_load_list );
					auto res	= file_resource_manager->load_list.begin();
					if( res != file_resource_manager->load_list.end() ) {
						path		= res->first;
						resource	= res->second;
						file_resource_manager->load_list.erase( res );
					}
					if( file_resource_manager->load_list.size() == 0 ) {
						more_work_available	= false;
					}
				}
				if( nullptr != resource ) {
					bool can_load	= false;
					{
						std::lock_guard<std::mutex> resource_guard( resource->mutex );
						assert( resource->state == FileResource::State::LOADING_QUEUED );
						can_load	= ( resource->state == FileResource::State::LOADING_QUEUED );
						if( can_load )	resource->state = FileResource::State::LOADING;
					}
					if( can_load ) {
						auto fs		= p_filesystem->OpenFileStream( path );
						if( nullptr != fs ) {
							auto loaded	= resource->LoadFromManager( fs, path );
							resource->SetState( loaded ? FileResource::State::LOADED : FileResource::State::UNABLE_TO_LOAD );
							p_filesystem->CloseFileStream( fs );
							load_operation_ran			= true;
						} else {
							resource->SetState( FileResource::State::UNABLE_TO_LOAD_FILE_NOT_FOUND );
						}
					}
				}
			}
		}
		// look for unloading and destroying work
		if( file_resource_manager->allow_resource_unloading ) {
			if( !load_operation_ran ) {
				UniquePointer<FileResource> resource = nullptr;
				{
					std::lock_guard<std::mutex> resources_guard( file_resource_manager->mutex_resources_list );
					auto it = file_resource_manager->resources_list.begin();
					while( it != file_resource_manager->resources_list.end() ) {
						auto & res = it->second;
						std::lock_guard<std::mutex> resource_guard( res->mutex );
						if( res->users == 0 ) {
							if( res->state != FileResource::State::LOADING && res->state != FileResource::State::LOADING_QUEUED ) {
								res->state		= FileResource::State::UNLOADING;
								resource		= std::move( it->second );
								it				= file_resource_manager->resources_list.erase( it );
								break;
							} else {
								++it;
							}
						} else {
							++it;
						}
					}
				}
				if( resource ) {
					if( resource->UnloadFromManager() ) {
						resource->SetState( FileResource::State::UNLOADED );
					} else {
						resource->SetState( FileResource::State::UNABLE_TO_UNLOAD );
						assert( 0 && "Unable to unload resource, we'll try and delete it anyways" );
					}
				}
			}
		}
	}
};

FileResourceManager::FileResourceManager( Engine * engine )
	: SubSystem( engine, "FileResourceManager")
{
	p_filesystem		= engine->GetFileSystem();
	allow_resource_requests				= false;
	allow_resource_loading				= false;
	allow_resource_unloading			= false;
	worker_threads_should_exit			= false;

	for( uint32_t i=0; i < BUILD_FILE_RESOURCE_MANAGER_WORKER_THREAD_COUNT; ++i ) {
		worker_threads_sleeping[ i ]	= false;
		auto thread						= std::thread( FileWorkerThread, p_engine, this, &worker_threads_sleeping[ i ] );
		worker_threads[ i ]				= std::move( thread );
	}
}

FileResourceManager::~FileResourceManager()
{
	// prevent new resources being allocated, should already be set though
	AllowResourceRequests( false );
	AllowResourceLoading( false );

	// scrap resources, ideally there shouldn't be any left at this point, but just in case
	ScrapFileResources();

	// notify all threads they should close
	worker_threads_should_exit				= true;

	// wait until they're all joinable, keep waiking them up until they become joinable
	uint32_t worker_threads_joinable_count	= 0;
	while( worker_threads_joinable_count != BUILD_FILE_RESOURCE_MANAGER_WORKER_THREAD_COUNT ) {
		SignalWorkers_All();
		std::this_thread::yield();
		std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

		uint32_t worker_threads_joinable_count_temp		= 0;
		for( auto & t : worker_threads ) {
			worker_threads_joinable_count_temp += uint32_t( !!t.joinable() );
		}
		worker_threads_joinable_count	= worker_threads_joinable_count_temp;
	}

	// all worker threads should be joinable
	for( auto & w : worker_threads ) {
		w.join();
	}
}

FileResourceHandle<FileResource> FileResourceManager::RequestResource( const Path & resource_path )
{
	if( !allow_resource_requests ) return nullptr;

	std::lock_guard<std::mutex> lock_guard( mutex_resources_list );

	FileResourceHandle<FileResource>	resource		= nullptr;
	// check if resource already available
	auto resource_at	= resources_list.find( resource_path );
	if( resource_at		!= resources_list.end() ) {
		// found, use this
		resource		= FileResourceHandle<FileResource>( resource_at->second.Get() );
		return resource;
	} else {
		// not found, make a new entry
		auto resource_type				= GetFileResourceTypeFromExtension( resource_path );
		auto resource_unique			= CreateResource( resource_type );
		resource_unique->state			= FileResource::State::LOADING_QUEUED;
		resource_unique->resource_path	= resource_path;
		resource						= FileResourceHandle<FileResource>( resource_unique.Get() );
		if( resource ) {
			std::lock_guard<std::mutex> load_list_guard( mutex_load_list );
			load_list.insert( std::pair<Path, FileResource*>( resource_path, resource.Get() ) );
			resources_list.insert( std::pair<Path, UniquePointer<FileResource>>( resource_path, std::move( resource_unique ) ) );
			SignalWorkers_One();
		}

		return resource;
	}
}

void FileResourceManager::SignalWorkers_One()
{
	worker_threads_wakeup.notify_one();
}

void FileResourceManager::SignalWorkers_All()
{
	worker_threads_wakeup.notify_all();
}

void FileResourceManager::Update()
{
	SignalWorkers_One();
}

bool FileResourceManager::HasPendingLoadWork()
{
	std::lock_guard<std::mutex> load_list_guard( mutex_load_list );
	return !!load_list.size();
}

bool FileResourceManager::HasPendingUnloadWork()
{
	std::lock_guard<std::mutex> resources_list_guard( mutex_resources_list );
	for( auto & r : resources_list ) {
		if( !r.second->GetUsers() ) return true;
	}
	return false;
}

bool FileResourceManager::IsIdle()
{
	for( auto & s : worker_threads_sleeping ) {
		if( !s ) return false;
	}
	return true;
}

void FileResourceManager::WaitIdle()
{
	while( !IsIdle() ) {
		std::this_thread::yield();
	}
}

void FileResourceManager::WaitJobless()
{
	while( HasPendingLoadWork() || HasPendingUnloadWork() || !IsIdle() ) {
		SignalWorkers_All();
		std::this_thread::yield();
		std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	}
}

void FileResourceManager::WaitUntilNoLoadWork()
{
	while( HasPendingLoadWork() || !IsIdle() ) {
		SignalWorkers_All();
		std::this_thread::yield();
		std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	}
}

void FileResourceManager::WaitUntilNoUnloadWork()
{
	while( HasPendingUnloadWork() || !IsIdle() ) {
		SignalWorkers_All();
		std::this_thread::yield();
		std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	}
}

void FileResourceManager::AllowResourceRequests( bool allow )
{
	allow_resource_requests		= allow;
}

void FileResourceManager::AllowResourceLoading( bool allow )
{
	allow_resource_loading		= allow;
	if( !allow ) {
		WaitIdle();
	}
}

void FileResourceManager::AllowResourceUnloading( bool allow )
{
	allow_resource_unloading	= allow;
	if( !allow ) {
		WaitIdle();
	}
}

void FileResourceManager::ScrapFileResources()
{
	// set all resources to have no users
	{
		std::lock_guard<std::mutex> lock_guard( mutex_resources_list );
		for( auto & r : resources_list ) {
			std::lock_guard<std::mutex> resource_guard( r.second->mutex );
			r.second->users		= 0;
		}
	}

	// wait until all resources have been destroyed
	uint32_t resource_count = UINT32_MAX;
	while( resource_count ) {
		SignalWorkers_All();
		std::this_thread::yield();
		std::this_thread::sleep_for( std::chrono::milliseconds( 150 ) );
		{
			std::lock_guard<std::mutex> lock_guard( mutex_resources_list );
			resource_count = uint32_t( resources_list.size() );
		}
	}
}

FileResource::Type FileResourceManager::GetFileResourceTypeFromExtension( const Path & path ) const
{
	Path extension = path.extension();
	
	if( extension == ".xml" ) {
		return FileResource::Type::XML;
	}
	if( extension == ".lua" ) {
		return FileResource::Type::XML;
	}
	if( extension == ".me3d" ) {
		return FileResource::Type::MESH;
	}
	if( extension == ".png" ||
		extension == ".jpg" ||
		extension == ".jpeg" ||
		extension == ".tga" ||
		extension == ".bmp" ||
		extension == ".psd" ||
		extension == ".gif" ||
		extension == ".hdr" ||
		extension == ".pic" ||
		extension == ".pnm" ||
		extension == ".pgm" ||
		extension == ".ppm" ) {
		return FileResource::Type::IMAGE;
	}

	return FileResource::Type::RAW_DATA;
}

UniquePointer<FileResource> FileResourceManager::CreateResource( const FileResource::Type resource_type )
{
	switch( resource_type ) {

	case FileResource::Type::RAW_DATA:
		return MakeUniquePointer<FileResource_RawData>( p_engine, this );

	case FileResource::Type::XML:
		return MakeUniquePointer<FileResource_XML>( p_engine, this );

	case FileResource::Type::LUA:
		return MakeUniquePointer<FileResource_Lua>( p_engine, this );

	case FileResource::Type::MESH:
		return MakeUniquePointer<FileResource_Mesh>( p_engine, this );

	case FileResource::Type::IMAGE:
		return MakeUniquePointer<FileResource_Image>( p_engine, this );

	default:
	{
		std::stringstream ss;
		ss << "Incorrect resource type: " << uint32_t( resource_type );
		p_logger->LogError( ss.str() );
	}
		break;
	}
	return nullptr;
}

UniquePointer<FileResource> FileResourceManager::CreateResource( const Path & path )
{
	return CreateResource( GetFileResourceTypeFromExtension( path ) );
}

}
