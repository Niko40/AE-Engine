
#include "DeviceResourceManager.h"

#include "../../Engine.h"
#include "../../Logger/Logger.h"
#include "../Renderer.h"
#include "../../FileResource/FileResourceManager.h"

#include "DeviceResource.h"

// To add a device resource seek steps 1 and 2

// 1: include all device resources here
#include "GraphicsPipeline\DeviceResource_GraphicsPipeline.h"
#include "Image/DeviceResource_Image.h"
#include "Mesh/DeviceResource_Mesh.h"

namespace AE
{

void DeviceWorkerThread( Engine * engine, DeviceResourceManager * device_resource_manager, std::atomic_bool * thread_sleeping )
{
	assert( nullptr != engine );
	assert( nullptr != device_resource_manager );
	Logger					*	p_logger			= engine->GetLogger();
	assert( p_logger );

	auto this_thread_index		= device_resource_manager->GetThisTreadResourceIndex();

	while( !device_resource_manager->worker_threads_should_exit ) {
		{
			std::unique_lock<std::mutex> wakeup_guard( device_resource_manager->worker_threads_wakeup_mutex );
			*thread_sleeping	= true;
			device_resource_manager->worker_threads_wakeup.wait( wakeup_guard );
			*thread_sleeping	= false;
		}
		// look for loading work first
		bool load_operation_ran	= false;
		if( device_resource_manager->allow_resource_loading ) {
			// look for continuing load operations
			{
				TODO( "device resource continue loading operations should also loop until all NextLoadOperationCanRun() functions return false within that loop" );
				DeviceResource	*	resource			= nullptr;
				bool				next_op_can_run		= false;
				{
					std::lock_guard<std::mutex> continue_load_list_guard( device_resource_manager->mutex_load_and_continue_load_list );
					auto it = device_resource_manager->continue_load_list.begin();
					while( it != device_resource_manager->continue_load_list.end() ) {
						resource		= *it;
						if( resource->GetWorkerThreadID() == std::this_thread::get_id() ) {
							next_op_can_run		= resource->ContinueLoadingFromManagerCanRun();
						}
						if( next_op_can_run ) {
							it			= device_resource_manager->continue_load_list.erase( it );
							break;
						} else {
							++it;
						}
					}
				}
				if( nullptr != resource && next_op_can_run ) {
					auto loading_state	= resource->ContinueLoadingFromManager();
					switch( loading_state ) {
					case AE::DeviceResource::LoadingState::UNABLE_TO_LOAD:
					{
						resource->SetResourceState( DeviceResource::State::UNABLE_TO_LOAD );
						assert( 0 && "Unable to load resource" );
						break;
					}
					case AE::DeviceResource::LoadingState::CONTINUE_LOADING:
					{
						std::lock_guard<std::mutex> continue_load_list_guard( device_resource_manager->mutex_load_and_continue_load_list );
						device_resource_manager->continue_load_list.push_back( resource );
						break;
					}
					case AE::DeviceResource::LoadingState::LOADED:
					{
						resource->SetResourceState( DeviceResource::State::LOADED );
						break;
					}
					default:
						assert( 0 && "Illegal device resource loading state" );
						break;
					}
				}
			}
			// look for new load operations
			{
				bool more_work_available		= true;
				// loop and load everything until the load list is empty
				// this way we don't have to wait a call from the resource manager
				// to initiate resource loading if there is pending work
				while( more_work_available ) {
					DeviceResource	*	resource	= nullptr;
					{
						std::lock_guard<std::mutex> load_list_guard( device_resource_manager->mutex_load_and_continue_load_list );
						auto res		= device_resource_manager->load_list.begin();
						if( res != device_resource_manager->load_list.end() ) {
							resource	= *res;
							device_resource_manager->load_list.erase( res );
						}
						if( device_resource_manager->load_list.size() == 0 ) {
							more_work_available		= false;
						}
					}
					if( nullptr != resource ) {
						bool can_load	= false;
						{
							std::lock_guard<std::mutex> resource_guard( resource->mutex );
							assert( resource->state == DeviceResource::State::LOADING_QUEUED );
							can_load	= ( resource->state == DeviceResource::State::LOADING_QUEUED );
							if( can_load )	resource->state = DeviceResource::State::LOADING;
						}
						if( can_load ) {
							auto load_state		= resource->LoadFromManager();
							switch( load_state ) {
							case AE::DeviceResource::LoadingState::UNABLE_TO_LOAD:
							{
								resource->SetResourceState( DeviceResource::State::UNABLE_TO_LOAD );
								break;
							}
							case AE::DeviceResource::LoadingState::CONTINUE_LOADING:
							{
								std::lock_guard<std::mutex> continue_load_list_guard( device_resource_manager->mutex_load_and_continue_load_list );
								device_resource_manager->continue_load_list.push_back( resource );
								break;
							}
							case AE::DeviceResource::LoadingState::LOADED:
							{
								resource->SetResourceState( DeviceResource::State::LOADED );
								break;
							}
							default:
								assert( 0 && "Illegal device resource loading state" );
								break;
							}
							load_operation_ran			= true;
						}
					}
				}
			}
		}
		// look for unloading and destroying work
		if( device_resource_manager->allow_resource_unloading ) {
			if( !load_operation_ran ) {
				// look for continuing unload operations
				{
					UniquePointer<DeviceResource>	resource			= nullptr;
					bool							next_op_can_run		= false;
					{
						std::lock_guard<std::mutex> continue_unload_list_guard( device_resource_manager->mutex_continue_unload_list );
						auto it = device_resource_manager->continue_unload_list.begin();
						while( it != device_resource_manager->continue_unload_list.end() ) {
							if( resource->GetWorkerThreadID() == std::this_thread::get_id() ) {
								next_op_can_run		= ( *it )->ContinueUnloadingFromManagerCanRun();
							}
							if( next_op_can_run ) {
								resource	= std::move( *it );
								it			= device_resource_manager->continue_unload_list.erase( it );
								break;
							} else {
								++it;
							}
						}
					}
					if( resource && next_op_can_run ) {
						auto loading_state	= resource->ContinueUnloadingFromManager();
						switch( loading_state ) {
						case AE::DeviceResource::UnloadingState::UNABLE_TO_UNLOAD:
						{
							resource->SetResourceState( DeviceResource::State::UNABLE_TO_UNLOAD );
							assert( 0 && "Failed to unload device resource, we'll try and destroy the object anyway" );
							break;
						}
						case AE::DeviceResource::UnloadingState::CONTINUE_UNLOADING:
						{
							std::lock_guard<std::mutex> continue_unload_list_guard( device_resource_manager->mutex_continue_unload_list );
							device_resource_manager->continue_unload_list.push_back( std::move( resource ) );
							break;
						}
						case AE::DeviceResource::UnloadingState::UNLOADED:
						{
							resource->SetResourceState( DeviceResource::State::UNLOADED );
							break;
						}
						default:
							assert( 0 && "Illegal device resource unloading state" );
							break;
						}
					}
				}
				// look for new unload operations
				{
					UniquePointer<DeviceResource> resource = nullptr;
					{
						std::lock_guard<std::mutex> resources_list_guard( device_resource_manager->mutex_resources_list );
						auto it = device_resource_manager->resources_list.begin();
						while( it != device_resource_manager->resources_list.end() ) {
							auto & res = *it;
							std::lock_guard<std::mutex> resource_guard( res->mutex );
							// only unload a resource if it's users match to 0 AND the resource was created using this same thread originally
							// thread id check is mandatory for some resources depend on per-thread vulkan memory or buffer pools
							if( res->users == 0 && res->locked_worker_thread_id == std::this_thread::get_id() ) {
								if( res->state != DeviceResource::State::LOADING && res->state != DeviceResource::State::LOADING_QUEUED ) {
									res->state		= DeviceResource::State::UNLOADING;
									resource		= std::move( *it );
									it				= device_resource_manager->resources_list.erase( it );
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
						auto state = resource->UnloadFromManager();
						switch( state ) {
						case AE::DeviceResource::UnloadingState::UNABLE_TO_UNLOAD:
						{
							resource->SetResourceState( DeviceResource::State::UNABLE_TO_UNLOAD );
							assert( 0 && "Unable to unload resource, we'll try and delete it anyways" );
							break;
						}
						case AE::DeviceResource::UnloadingState::CONTINUE_UNLOADING:
						{
							std::lock_guard<std::mutex> continue_unload_list_guard( device_resource_manager->mutex_continue_unload_list );
							device_resource_manager->continue_unload_list.push_back( std::move( resource ) );
							break;
						}
						case AE::DeviceResource::UnloadingState::UNLOADED:
						{
							resource->SetResourceState( DeviceResource::State::UNLOADED );
							break;
						}
						default:
							assert( 0 && "Illegal device resource unload state" );
							break;
						}
					}
				}
			}
		}
	}
};


DeviceResourceManager::DeviceResourceManager( Engine * engine, Renderer * renderer, DeviceMemoryManager * device_memory_manager )
{
	p_engine					= engine;
	p_renderer					= renderer;
	p_device_memory_manager		= device_memory_manager;
	assert( nullptr != p_engine );
	assert( nullptr != p_renderer );
	assert( nullptr != p_device_memory_manager );
	p_logger					= p_engine->GetLogger();
	p_file_resource_manager		= p_engine->GetFileResourceManager();
	assert( nullptr != p_logger );
	assert( nullptr != p_file_resource_manager );

	ref_vk_instance							= p_renderer->GetVulkanInstance();
	ref_vk_device							= p_renderer->GetVulkanDevice();
	ref_vk_physical_device					= p_renderer->GetVulkanPhysicalDevice();

	primary_render_queue_family_index		= p_renderer->GetPrimaryRenderQueueFamilyIndex();
	secondary_render_queue_family_index		= p_renderer->GetSecondaryRenderQueueFamilyIndex();
	primary_transfer_queue_family_index		= p_renderer->GetPrimaryTransferQueueFamilyIndex();
	queue_families_are_same					= ( primary_render_queue_family_index == secondary_render_queue_family_index == primary_transfer_queue_family_index );

	allow_resource_requests					= false;
	allow_resource_loading					= false;
	allow_resource_unloading				= false;
	worker_threads_should_exit				= false;
	{
		LOCK_GUARD( *ref_vk_device.mutex );

		for( uint32_t i=0; i < BUILD_DEVICE_RESOURCE_MANAGER_WORKER_THREAD_COUNT; ++i ) {
			worker_threads_sleeping[ i ]		= false;
			{
				// create primary render command pool
				vk::CommandPoolCreateInfo command_pool_CI {};
				command_pool_CI.flags							= vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
				command_pool_CI.queueFamilyIndex				= primary_render_queue_family_index;
				vk_thread_command_pools_primary_render[ i ]		= ref_vk_device.object.createCommandPool( command_pool_CI );
			}
			if( secondary_render_queue_family_index == primary_render_queue_family_index ) {
				vk_thread_command_pools_secondary_render[ i ]	= vk_thread_command_pools_primary_render[ i ];
			} else {
				// create secondary render command pool
				vk::CommandPoolCreateInfo command_pool_CI {};
				command_pool_CI.flags							= vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
				command_pool_CI.queueFamilyIndex				= secondary_render_queue_family_index;
				vk_thread_command_pools_secondary_render[ i ]	= ref_vk_device.object.createCommandPool( command_pool_CI );
			}
			if( primary_transfer_queue_family_index == primary_render_queue_family_index ) {
				vk_thread_command_pools_primary_transfer[ i ]	= vk_thread_command_pools_primary_render[ i ];
			} else if( primary_transfer_queue_family_index == secondary_render_queue_family_index ) {
				vk_thread_command_pools_primary_transfer[ i ]	= vk_thread_command_pools_secondary_render[ i ];
			} else {
				// create primary transfer command pool
				vk::CommandPoolCreateInfo command_pool_CI {};
				command_pool_CI.flags							= vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
				command_pool_CI.queueFamilyIndex				= primary_transfer_queue_family_index;
				vk_thread_command_pools_primary_transfer[ i ]	= ref_vk_device.object.createCommandPool( command_pool_CI );
			}

			auto thread				= std::thread( DeviceWorkerThread, p_engine, this, &worker_threads_sleeping[ i ] );
			worker_threads[ i ]		= std::move( thread );
		}
	}
}

DeviceResourceManager::~DeviceResourceManager()
{
	// prevent new resources being allocated, should already be set though
	AllowResourceRequests( false );
	AllowResourceLoading( false );

	// scrap resources, ideally there shouldn't be any left at this point, but just in case
	ScrapDeviceResources();

	// sync device and CPU
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		ref_vk_device.object.waitIdle();
	}
	// notify all threads they should close
	worker_threads_should_exit				= true;

	// wait until they're all joinable, keep waking them up until they become joinable
	uint32_t worker_threads_joinable_count	= 0;
	while( worker_threads_joinable_count != BUILD_DEVICE_RESOURCE_MANAGER_WORKER_THREAD_COUNT ) {
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

	{
		LOCK_GUARD( *ref_vk_device.mutex );

		// sync device and CPU
		ref_vk_device.object.waitIdle();

		// destroy all command pools
		for( uint32_t i=0; i < BUILD_DEVICE_RESOURCE_MANAGER_WORKER_THREAD_COUNT; ++i ) {
			if( primary_render_queue_family_index == secondary_render_queue_family_index &&
				primary_render_queue_family_index == primary_transfer_queue_family_index ) {
				ref_vk_device.object.destroyCommandPool( vk_thread_command_pools_primary_render[ i ] );

			} else if( primary_render_queue_family_index == secondary_render_queue_family_index &&
				primary_render_queue_family_index != primary_transfer_queue_family_index ) {
				ref_vk_device.object.destroyCommandPool( vk_thread_command_pools_primary_render[ i ] );
				ref_vk_device.object.destroyCommandPool( vk_thread_command_pools_primary_transfer[ i ] );

			} else if( primary_transfer_queue_family_index == primary_render_queue_family_index &&
				primary_transfer_queue_family_index != secondary_render_queue_family_index ) {
				ref_vk_device.object.destroyCommandPool( vk_thread_command_pools_primary_transfer[ i ] );
				ref_vk_device.object.destroyCommandPool( vk_thread_command_pools_secondary_render[ i ] );

			} else if( secondary_render_queue_family_index == primary_transfer_queue_family_index &&
				secondary_render_queue_family_index != primary_render_queue_family_index ) {
				ref_vk_device.object.destroyCommandPool( vk_thread_command_pools_secondary_render[ i ] );
				ref_vk_device.object.destroyCommandPool( vk_thread_command_pools_primary_render[ i ] );
			} else {
				ref_vk_device.object.destroyCommandPool( vk_thread_command_pools_primary_render[ i ] );
				ref_vk_device.object.destroyCommandPool( vk_thread_command_pools_secondary_render[ i ] );
				ref_vk_device.object.destroyCommandPool( vk_thread_command_pools_primary_transfer[ i ] );
			}

			vk_thread_command_pools_primary_render[ i ]		= nullptr;
			vk_thread_command_pools_secondary_render[ i ]	= nullptr;
			vk_thread_command_pools_primary_transfer[ i ]	= nullptr;
		}
	}
}

DeviceResourceHandle<DeviceResource> DeviceResourceManager::RequestResource( DeviceResource::Type resource_type, const Vector<Path> & file_resource_paths, DeviceResource::Flags resource_flags )
{
	if( !allow_resource_requests ) return nullptr;

	DeviceResourceHandle<DeviceResource>	resource		= nullptr;
	{
		std::lock_guard<std::mutex>			resource_list_guard( mutex_resources_list );

		// we should create an unique resource if one was requested, othervise share an existing one if one exists
		if( resource_flags & DeviceResource::Flags::UNIQUE ) {
			resource		= RequestNewResource( resource_type, resource_flags );
		} else {
			resource		= RequestExistingResource( resource_type, file_resource_paths, resource_flags );
			if( resource ) {
				return resource;
			} else {
				resource	= RequestNewResource( resource_type, resource_flags );
			}
		}
		assert( resource );
		resource->state		= DeviceResource::State::LOADING_QUEUED;
	}
	// a new resource has to be created, we need to request it's file resources from file resource manager
	{
		bool all_file_resources_loaded	= true;
		if( resource ) {
			std::lock_guard<std::mutex> resource_guard( resource->mutex );
			resource->file_resources.resize( file_resource_paths.size() );
			for( size_t i=0; i < file_resource_paths.size(); ++i ) {
				resource->file_resources[ i ]	= p_file_resource_manager->RequestResource( file_resource_paths[ i ] );
				if( resource->file_resources[ i ] ) {
					if( !resource->file_resources[ i ]->IsResourceReadyForUse() ) {
						all_file_resources_loaded	= false;
					}
				} else {
					p_logger->LogError( "Device Resource request failed, file resource not found: " + file_resource_paths[ i ].string() );
					resource->state = DeviceResource::State::UNABLE_TO_LOAD;
					assert( 0 && "Device Resource request failed, file resource not found" );
				}
			}
		}
		{
			// check if all file resources are already available, if yes, we start loading the resource immediately
			// othervise we'll put the resource on a preload list where the resource will wait until all file resources become available
			if( all_file_resources_loaded ) {
				std::lock_guard<std::mutex>			load_list_guard( mutex_load_and_continue_load_list );
				load_list.push_back( resource.Get() );
				SignalWorkers_One();
			} else {
				std::lock_guard<std::mutex>			preload_list_guard( mutex_preload_list );
				preload_list.push_back( resource.Get() );
			}
		}
	}
	return resource;
}

DeviceResourceHandle<DeviceResource_Mesh> DeviceResourceManager::RequestResource_Mesh( const Vector<Path>& file_resource_paths, DeviceResource::Flags resource_flags )
{
	return DeviceResourceHandle<DeviceResource_Mesh>( RequestResource( AE::DeviceResource::Type::MESH, file_resource_paths, resource_flags ) );
}

DeviceResourceHandle<DeviceResource_Image> DeviceResourceManager::RequestResource_Image( const Vector<Path>& file_resource_paths, DeviceResource::Flags resource_flags )
{
	return DeviceResourceHandle<DeviceResource_Image>( RequestResource( AE::DeviceResource::Type::IMAGE, file_resource_paths, resource_flags ) );
}

DeviceResourceHandle<DeviceResource_GraphicsPipeline> DeviceResourceManager::RequestResource_GraphicsPipeline( const Vector<Path>& file_resource_paths, DeviceResource::Flags resource_flags )
{
	return DeviceResourceHandle<DeviceResource_GraphicsPipeline>( RequestResource( AE::DeviceResource::Type::GRAPHICS_PIPELINE, file_resource_paths, resource_flags ) );
}

void DeviceResourceManager::SignalWorkers_One()
{
	worker_threads_wakeup.notify_one();
}

void DeviceResourceManager::SignalWorkers_All()
{
	worker_threads_wakeup.notify_all();
}

void DeviceResourceManager::ParsePreloadList()
{
	std::lock_guard<std::mutex>		preload_list_guard( mutex_preload_list );
	auto pl_it			= preload_list.begin();
	while( pl_it != preload_list.end() ) {
		bool file_resources_ready		= true;
		for( auto & f : ( *pl_it )->file_resources ) {
			// check each file resource within this device resource if they're ready
			auto current_file_resource_state		= f->GetResourceState();
			if( current_file_resource_state != FileResource::State::LOADED ) {
				file_resources_ready	= false;
				// check if an error happened
				if( current_file_resource_state == FileResource::State::UNABLE_TO_LOAD ||
					current_file_resource_state == FileResource::State::UNABLE_TO_LOAD_FILE_NOT_FOUND ) {
					// if unable to load any of the file resources, we just remove this device resource from
					// the preload_list, we hang on to the resource handle as usual until it's reference counter reaches 0
					// ( already in resource_list )
					{
						LOCK_GUARD( ( *pl_it )->mutex );
						// Assign a random worker thread id so that the resource gets destroyed and won't hang the program
						( *pl_it )->locked_worker_thread_id		= worker_threads[ rand() % BUILD_DEVICE_RESOURCE_MANAGER_WORKER_THREAD_COUNT ].get_id();
						( *pl_it )->state						= DeviceResource::State::UNABLE_TO_LOAD;
					}
					pl_it										= preload_list.erase( pl_it );
					// side effect: this skips the next resource on the list because
					// pl_it practically gets incremented twice, not worth fixing though
				}
				break;
			}
		}
		if( file_resources_ready ) {
			std::lock_guard<std::mutex>		load_list_guard( mutex_load_and_continue_load_list );
			load_list.push_back( *pl_it );
			pl_it		= preload_list.erase( pl_it );
		} else {
			++pl_it;
		}
	}
}

void DeviceResourceManager::Update()
{
	SignalWorkers_One();
	ParsePreloadList();
}

bool DeviceResourceManager::HasPendingLoadWork()
{
	{
		std::lock_guard<std::mutex> load_list_guard( mutex_load_and_continue_load_list );
		if( load_list.size() ) return true;
		if( continue_load_list.size() ) return true;
	}
	{
		std::lock_guard<std::mutex> preload_list_guard( mutex_preload_list );
		if( preload_list.size() ) return true;
	}
	return false;
}

bool DeviceResourceManager::HasPendingUnloadWork()
{
	{
		std::lock_guard<std::mutex> unload_list_guard( mutex_continue_unload_list );
		if( continue_unload_list.size() ) return true;
	}
	{
		std::lock_guard<std::mutex> resources_list_guard( mutex_resources_list );
		for( auto & r : resources_list ) {
			if( !r->GetResourceUsers() ) return true;
		}
	}
	return false;
}

bool DeviceResourceManager::IsIdle()
{
	for( auto & s : worker_threads_sleeping ) {
		if( !s ) return false;
	}
	return true;
}

void DeviceResourceManager::WaitIdle()
{
	while( !IsIdle() ) {
		std::this_thread::yield();
	}
}

void DeviceResourceManager::WaitJobless()
{
	while( HasPendingLoadWork() || HasPendingUnloadWork() || !IsIdle() ) {
		p_file_resource_manager->SignalWorkers_All();
		ParsePreloadList();
		SignalWorkers_All();
		std::this_thread::yield();
		std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	}
}

void DeviceResourceManager::WaitUntilNoLoadWork()
{
	while( HasPendingLoadWork() || !IsIdle() ) {
		p_file_resource_manager->SignalWorkers_All();
		ParsePreloadList();
		SignalWorkers_All();
		std::this_thread::yield();
		std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	}
}

void DeviceResourceManager::WaitUntilNoUnloadWork()
{
	while( HasPendingUnloadWork() || !IsIdle() ) {
		p_file_resource_manager->SignalWorkers_All();
		SignalWorkers_All();
		std::this_thread::yield();
		std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	}
}

uint32_t DeviceResourceManager::GetThisTreadResourceIndex()
{
	for( size_t i=0; i < worker_threads.size(); ++i ) {
		const auto * const t	= &worker_threads[ i ];
		if( t->get_id() == std::this_thread::get_id() ) {
			return uint32_t( i );
		}
	}
	return UINT32_MAX;
}

void DeviceResourceManager::AllowResourceRequests( bool allow )
{
	allow_resource_requests		= allow;
}

void DeviceResourceManager::AllowResourceLoading( bool allow )
{
	allow_resource_loading		= allow;
	if( !allow ) {
		WaitIdle();
	}
}

void DeviceResourceManager::AllowResourceUnloading( bool allow )
{
	allow_resource_unloading	= allow;
	if( !allow ) {
		WaitIdle();
	}
}

vk::CommandPool DeviceResourceManager::GetPrimaryRenderCommandPoolForThisThread( uint32_t thread_index )
{
	// Get slot number from thread id
	uint32_t slot	= thread_index;
	if( UINT32_MAX == slot ) {
		slot		= GetThisTreadResourceIndex();
	}
	if( UINT32_MAX == slot ) {
		assert( 0 && "thread not found or called from main thread, this is a worker thread function only" );
		return nullptr;
	} else {
		return vk_thread_command_pools_primary_render[ slot ];
	}
}

vk::CommandPool DeviceResourceManager::GetSecondaryRenderCommandPoolForThisThread( uint32_t thread_index )
{
	// Get slot number from thread id
	uint32_t slot	= thread_index;
	if( UINT32_MAX == slot ) {
		slot		= GetThisTreadResourceIndex();
	}
	if( UINT32_MAX == slot ) {
		assert( 0 && "thread not found or called from main thread, this is a worker thread function only" );
		return nullptr;
	} else {
		return vk_thread_command_pools_secondary_render[ slot ];
	}
}

vk::CommandPool DeviceResourceManager::GetPrimaryTransferCommandPoolForThisThread( uint32_t thread_index )
{
	// Get slot number from thread id
	uint32_t slot	= thread_index;
	if( UINT32_MAX	== slot ) {
		slot		= GetThisTreadResourceIndex();
	}
	if( UINT32_MAX	== slot ) {
		assert( 0 && "thread not found or called from main thread, this is a worker thread function only" );
		return nullptr;
	} else {
		return vk_thread_command_pools_primary_transfer[ slot ];
	}
}

void DeviceResourceManager::ScrapDeviceResources()
{
	// set all resources to have no users
	{
		std::lock_guard<std::mutex> lock_guard( mutex_resources_list );
		for( auto & r : resources_list ) {
			std::lock_guard<std::mutex> resource_guard( r->mutex );
			r->users		= 0;
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
			resource_count	= uint32_t( resources_list.size() );
		}
		{
			std::lock_guard<std::mutex> lock_guard( mutex_continue_unload_list );
			resource_count	+= uint32_t( continue_unload_list.size() );
		}
	}
}

DeviceResourceHandle<DeviceResource> DeviceResourceManager::RequestExistingResource( DeviceResource::Type resource_type, const Vector<Path> & file_resource_paths, DeviceResource::Flags resource_flags )
{
	for( auto & r : resources_list ) {
		bool found_suitable = true;
		{
			std::lock_guard<std::mutex> resource_guard( r->mutex );
			if( ( r->type == resource_type ) &&
				!( r->flags & DeviceResource::Flags::UNIQUE ) &&
				( r->file_resources.size() == file_resource_paths.size() ) ) {
				for( size_t i=0; i < file_resource_paths.size(); ++i ) {
					if( r->file_resources[ i ]->GetPath() != file_resource_paths[ i ] ) {
						found_suitable		= false;
						break;
					}
				} 
			} else {
				found_suitable				= false;
			}
		}
		if( found_suitable ) {
			// found compatible resource
			return DeviceResourceHandle<DeviceResource>( r.Get() );
		}
	}
	return nullptr;
};

DeviceResourceHandle<DeviceResource> DeviceResourceManager::RequestNewResource( DeviceResource::Type resource_type, DeviceResource::Flags resource_flags )
{
	DeviceResourceHandle<DeviceResource>	resource	= nullptr;
	auto resource_unique	= CreateResource( resource_type, resource_flags );
	assert( resource_unique );
	if( resource_unique ) {
		resource			= DeviceResourceHandle<DeviceResource>( resource_unique.Get() );
		resources_list.push_back( std::move( resource_unique ) );
	}
	return resource;
}

UniquePointer<DeviceResource> DeviceResourceManager::CreateResource( DeviceResource::Type resource_type, DeviceResource::Flags resource_flags )
{
	switch( resource_type ) {

		// 2: Create all individual device resources here
	case AE::DeviceResource::Type::GRAPHICS_PIPELINE:
		return MakeUniquePointer<DeviceResource_GraphicsPipeline>( p_engine, resource_flags );

	case AE::DeviceResource::Type::MESH:
		return MakeUniquePointer<DeviceResource_Mesh>( p_engine, resource_flags );

	case AE::DeviceResource::Type::IMAGE:
		return MakeUniquePointer<DeviceResource_Image>( p_engine, resource_flags );

	default:
		p_logger->LogError( "Cannot create device resource, resource type is unknown" );
		break;
	}
	return nullptr;
}

}
