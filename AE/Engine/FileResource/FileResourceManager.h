#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <array>

#include "../BUILD_OPTIONS.h"
#include "../Platform.h"

#include "../Memory/Memory.h"
#include "../SubSystem.h"
#include "../FileSystem/FileSystem.h"

#include "FileResource.h"

namespace AE
{

class Engine;
class Logger;
class FileResource;

class FileResourceManager : SubSystem
{
	friend class Engine;
	friend void FileWorkerThread( Engine * engine, FileResourceManager * file_resource_manager, std::atomic_bool * thread_sleeping );

public:
											FileResourceManager( Engine * engine );
											~FileResourceManager();

	FileResourceHandle<FileResource>		RequestResource( const Path & resource_path );

	void									SignalWorkers_One();
	void									SignalWorkers_All();

	void									Update();					// general update of the resource manager, should be called once in every frame
	bool									HasPendingLoadWork();		// fast, just checks the size of a load and continue load lists
	bool									HasPendingUnloadWork();		// relatively slow, loops through every resource in memory
	bool									IsIdle();					// tells if the worker threads are idle, fast but doesn't tell if there's pending work
	void									WaitIdle();					// pauses the excecution of the calling thread until resource manager becomes idle, can be used in realtime but not preferred
	void									WaitJobless();				// pauses the excecution of the calling thread until resource manager is completely without work and worker threads become idle, NO REALTIME USE
	void									WaitUntilNoLoadWork();		// pauses the excecution of the calling thread until all pending resources have been loaded and worker threads become idle, NO REALTIME USE
	void									WaitUntilNoUnloadWork();	// pauses the excecution of the calling thread until all pending resources have been unloaded and worker threads become idle, NO REALTIME USE

	void									AllowResourceRequests( bool allow );
	void									AllowResourceLoading( bool allow );
	void									AllowResourceUnloading( bool allow );

protected:
	void									ScrapFileResources();

private:
	FileResource::Type						GetFileResourceTypeFromExtension( const Path & path ) const;
	UniquePointer<FileResource>				CreateResource( const FileResource::Type resource_type );
	UniquePointer<FileResource>				CreateResource( const Path & path );

	FileSystem							*	p_filesystem							= nullptr;

	std::condition_variable					worker_threads_wakeup;
	std::mutex								worker_threads_wakeup_mutex;
	std::atomic_bool						worker_threads_should_exit;
	std::array<std::atomic_bool, BUILD_FILE_RESOURCE_MANAGER_WORKER_THREAD_COUNT>	worker_threads_sleeping;
	std::array<std::thread, BUILD_FILE_RESOURCE_MANAGER_WORKER_THREAD_COUNT>		worker_threads;

	std::mutex								mutex_resources_list;
	std::mutex								mutex_load_list;

	Map<Path, UniquePointer<FileResource>>	resources_list;
	Map<Path, FileResource*>				load_list;

	std::atomic_bool						allow_resource_requests;
	std::atomic_bool						allow_resource_loading;
	std::atomic_bool						allow_resource_unloading;
};

}
