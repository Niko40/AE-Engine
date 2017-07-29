
#include "DeviceMemoryManager.h"

#include "../../Engine.h"
#include "../../Logger/Logger.h"
#include "../Renderer.h"
#include "../QueueInfo.h"

namespace AE
{

DeviceMemoryManager::DeviceMemoryManager( Engine * engine, Renderer * renderer )
{
	p_engine		= engine;
	p_renderer		= renderer;
	assert( nullptr != p_engine );
	assert( nullptr != p_renderer );
	p_logger		= p_engine->GetLogger();
	assert( nullptr != p_logger );

	ref_vk_instance							= p_renderer->GetVulkanInstance();
	ref_vk_device							= p_renderer->GetVulkanDevice();
	ref_vk_physical_device					= p_renderer->GetVulkanPhysicalDevice();
	vk_physical_device_memory_properties	= ref_vk_physical_device.getMemoryProperties();

	vk::BufferCreateInfo c;
}

DeviceMemoryManager::~DeviceMemoryManager()
{
}

vk::Buffer DeviceMemoryManager::CreateBuffer( vk::BufferCreateFlags flags, vk::DeviceSize buffer_size, vk::BufferUsageFlags usage_flags, UsedQueuesFlags shared_between_queues )
{
	auto sharing_mode_info		= p_renderer->GetSharingModeInfo( shared_between_queues );

	vk::BufferCreateInfo CI {};
	CI.flags					= flags;
	CI.size						= buffer_size;
	CI.usage					= usage_flags;
	CI.sharingMode				= sharing_mode_info.sharing_mode;
	CI.queueFamilyIndexCount	= uint32_t( sharing_mode_info.shared_queue_family_indices.size() );
	CI.pQueueFamilyIndices		= sharing_mode_info.shared_queue_family_indices.data();

	LOCK_GUARD( *ref_vk_device.mutex );
	return ref_vk_device.object.createBuffer( CI );
}

DeviceMemoryInfo DeviceMemoryManager::AllocateImageMemory( vk::Image image, vk::MemoryPropertyFlags memory_properties )
{
	vk::MemoryRequirements memory_requirements;
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		memory_requirements		= ref_vk_device.object.getImageMemoryRequirements( image );
	}
	return AllocateMemory( memory_requirements, memory_properties );
}

DeviceMemoryInfo DeviceMemoryManager::AllocateAndBindImageMemory( vk::Image image, vk::MemoryPropertyFlags memory_properties )
{
	auto memory_info = AllocateImageMemory( image, memory_properties );
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		ref_vk_device.object.bindImageMemory( image, memory_info.memory, memory_info.offset );
	}
	return memory_info;
}

DeviceMemoryInfo DeviceMemoryManager::AllocateBufferMemory( vk::Buffer buffer, vk::MemoryPropertyFlags memory_properties )
{
	vk::MemoryRequirements memory_requirements;
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		memory_requirements		= ref_vk_device.object.getBufferMemoryRequirements( buffer );
	}
	return AllocateMemory( memory_requirements, memory_properties );
}

DeviceMemoryInfo DeviceMemoryManager::AllocateAndBindBufferMemory( vk::Buffer buffer, vk::MemoryPropertyFlags memory_properties )
{
	auto memory_info = AllocateBufferMemory( buffer, memory_properties );
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		ref_vk_device.object.bindBufferMemory( buffer, memory_info.memory, memory_info.offset );
	}
	return memory_info;
}

void DeviceMemoryManager::FreeMemory( DeviceMemoryInfo & memory )
{
	TODO( "Memory pooling, we only have limited number of memory allocation times left" );
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		ref_vk_device.object.freeMemory( memory.memory );
	}
	memory		= {};
}

DeviceMemoryInfo DeviceMemoryManager::AllocateMemory( vk::MemoryRequirements & requirements, vk::MemoryPropertyFlags memory_properties )
{
	TODO( "Memory pooling, we only have limited number of memory allocation times left" );

	DeviceMemoryInfo ret {};

	uint32_t memory_index		= FindMemoryTypeIndex( requirements, memory_properties );

	vk::MemoryAllocateInfo memory_AI {};
	memory_AI.allocationSize	= requirements.size;
	memory_AI.memoryTypeIndex	= memory_index;
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		ret.memory				= ref_vk_device.object.allocateMemory( memory_AI );
		ret.offset				= 0;
		ret.size				= requirements.size;
		ret.alignment			= requirements.alignment;
	}
	if( !ret.memory ) {
		p_logger->LogError( "Couldn't allocate device memory" );
	}

	return ret;
}

uint32_t DeviceMemoryManager::FindMemoryTypeIndex( vk::MemoryRequirements & requirements, vk::MemoryPropertyFlags property_flags )
{
	for( uint32_t i=0; i < vk_physical_device_memory_properties.memoryTypeCount; ++i ) {
		if( requirements.memoryTypeBits & ( 1 << i ) ) {
			if( ( vk_physical_device_memory_properties.memoryTypes[ i ].propertyFlags & property_flags ) == property_flags ) {
				return i;
			}
		}
	}
	p_logger->LogError( "Couldn't find memory type index for resource" );
	return UINT32_MAX;
}

}
