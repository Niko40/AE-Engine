#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include <vulkan/vulkan.h>
#include <thread>
#include <mutex>
#include <atomic>

#include "../DeviceMemory/DeviceMemoryInfo.h"
#include "../DeviceResource/DeviceResource.h"
#include "../../Memory/MemoryTypes.h"

// To add a device resource, follow steps 1 and 2

namespace AE
{

class Engine;
class Logger;
class Renderer;
class DeviceMemoryManager;
class DeviceResource;

// 1: Add device resource declarations here
class DeviceResource_Mesh;
class DeviceResource_Image;

class DeviceResourceManager
{
	friend class Engine;
	friend class DeviceResource;
	friend void DeviceWorkerThread( Engine * engine, DeviceResourceManager * device_resource_manager, std::atomic_bool * thread_sleeping );

public:
	DeviceResourceManager( Engine * engine, Renderer * renderer, DeviceMemoryManager * device_memory_manager );
	~DeviceResourceManager();

	DeviceResourceHandle<DeviceResource>		RequestResource( DeviceResource::Type resource_type, const Vector<Path> & file_resource_paths, DeviceResource::Flags resource_flags = DeviceResource::Flags( 0 ) );

	// 2: Add device resource specialized request functions here
	DeviceResourceHandle<DeviceResource_Mesh>	RequestResource_Mesh( const Vector<Path> & file_resource_paths, DeviceResource::Flags resource_flags = DeviceResource::Flags( 0 ) );
	DeviceResourceHandle<DeviceResource_Image>	RequestResource_Image( const Vector<Path> & file_resource_paths, DeviceResource::Flags resource_flags = DeviceResource::Flags( 0 ) );

	void										SignalWorkers_One();
	void										SignalWorkers_All();

	void										ParsePreloadList();
	void										Update();					// general update of the resource manager, should be called once in every frame
	bool										HasPendingLoadWork();		// fast, just checks the size of a load list
	bool										HasPendingUnloadWork();		// relatively slow, loops through every resource in memory
	bool										IsIdle();					// tells if the worker threads are idle, fast but doesn't tell if there's pending work
	void										WaitIdle();					// pauses the excecution of the calling thread until resource manager becomes idle, can be used in realtime but not preferred
	void										WaitJobless();				// pauses the excecution of the calling thread until resource manager is completely without work and worker threads become idle, NO REALTIME USE
	void										WaitUntilNoLoadWork();		// pauses the excecution of the calling thread until all pending resources have been loaded and worker threads become idle, NO REALTIME USE
	void										WaitUntilNoUnloadWork();	// pauses the excecution of the calling thread until all pending resources have been unloaded and worker threads become idle, NO REALTIME USE

	uint32_t									GetThisTreadResourceIndex();

	void										AllowResourceRequests( bool allow );
	void										AllowResourceLoading( bool allow );
	void										AllowResourceUnloading( bool allow );

	vk::CommandPool								GetPrimaryRenderCommandPoolForThisThread( uint32_t thread_index = UINT32_MAX );
	vk::CommandPool								GetSecondaryRenderCommandPoolForThisThread( uint32_t thread_index = UINT32_MAX );
	vk::CommandPool								GetPrimaryTransferCommandPoolForThisThread( uint32_t thread_index = UINT32_MAX );

private:
	void										ScrapDeviceResources();

	DeviceResourceHandle<DeviceResource>		RequestExistingResource( DeviceResource::Type resource_type, const Vector<Path> & file_resource_paths, DeviceResource::Flags resource_flags );
	DeviceResourceHandle<DeviceResource>		RequestNewResource( DeviceResource::Type resource_type, DeviceResource::Flags resource_flags );

	UniquePointer<DeviceResource>				CreateResource( DeviceResource::Type resource_type, DeviceResource::Flags resource_flags );

	Engine									*	p_engine					= nullptr;
	Logger									*	p_logger					= nullptr;
	Renderer								*	p_renderer					= nullptr;
	DeviceMemoryManager						*	p_device_memory_manager		= nullptr;
	FileResourceManager						*	p_file_resource_manager		= nullptr;

	vk::Instance								ref_vk_instance;
	vk::PhysicalDevice							ref_vk_physical_device;
	VulkanDevice								ref_vk_device;

	uint32_t									primary_render_queue_family_index			= UINT32_MAX;
	uint32_t									secondary_render_queue_family_index			= UINT32_MAX;
	uint32_t									primary_transfer_queue_family_index			= UINT32_MAX;
	vk::Bool32									queue_families_are_same						= VK_FALSE;

	std::condition_variable						worker_threads_wakeup;
	std::mutex									worker_threads_wakeup_mutex;
	std::atomic_bool							worker_threads_should_exit;
	std::array<vk::CommandPool, BUILD_DEVICE_RESOURCE_MANAGER_WORKER_THREAD_COUNT>			vk_thread_command_pools_primary_render;
	std::array<vk::CommandPool, BUILD_DEVICE_RESOURCE_MANAGER_WORKER_THREAD_COUNT>			vk_thread_command_pools_secondary_render;
	std::array<vk::CommandPool, BUILD_DEVICE_RESOURCE_MANAGER_WORKER_THREAD_COUNT>			vk_thread_command_pools_primary_transfer;
	std::array<std::atomic_bool, BUILD_DEVICE_RESOURCE_MANAGER_WORKER_THREAD_COUNT>			worker_threads_sleeping;
	std::array<std::thread, BUILD_DEVICE_RESOURCE_MANAGER_WORKER_THREAD_COUNT>				worker_threads;

	std::mutex									mutex_resources_list;
	std::mutex									mutex_preload_list;
	std::mutex									mutex_load_and_continue_load_list;
	std::mutex									mutex_continue_unload_list;

	List<UniquePointer<DeviceResource>>			resources_list;
	List<DeviceResource*>						preload_list;
	List<DeviceResource*>						load_list;
	List<DeviceResource*>						continue_load_list;
	List<UniquePointer<DeviceResource>>			continue_unload_list;

	std::atomic_bool							allow_resource_requests;
	std::atomic_bool							allow_resource_loading;
	std::atomic_bool							allow_resource_unloading;
};

}
