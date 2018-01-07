
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
	vkGetPhysicalDeviceMemoryProperties( ref_vk_physical_device, &vk_physical_device_memory_properties );
}

DeviceMemoryManager::~DeviceMemoryManager()
{
}

VkBuffer DeviceMemoryManager::CreateBuffer( VkBufferCreateFlags flags, VkDeviceSize buffer_size, VkBufferUsageFlags usage_flags, UsedQueuesFlags shared_between_queues )
{
	auto sharing_mode_info		= p_renderer->GetSharingModeInfo( shared_between_queues );

	VkBufferCreateInfo CI {};
	CI.sType					= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	CI.pNext					= nullptr;
	CI.flags					= flags;
	CI.size						= buffer_size;
	CI.usage					= usage_flags;
	CI.sharingMode				= sharing_mode_info.sharing_mode;
	CI.queueFamilyIndexCount	= uint32_t( sharing_mode_info.shared_queue_family_indices.size() );
	CI.pQueueFamilyIndices		= sharing_mode_info.shared_queue_family_indices.data();

	LOCK_GUARD( *ref_vk_device.mutex );
	VkBuffer ret = VK_NULL_HANDLE;
	VulkanResultCheck( vkCreateBuffer( ref_vk_device.object, &CI, VULKAN_ALLOC, &ret ) );
	return ret;
}

DeviceMemoryInfo DeviceMemoryManager::AllocateImageMemory( VkImage image, VkMemoryPropertyFlags memory_properties )
{
	VkMemoryRequirements memory_requirements {};
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		vkGetImageMemoryRequirements( ref_vk_device.object, image, &memory_requirements );
	}
	return AllocateMemory( memory_requirements, memory_properties );
}

DeviceMemoryInfo DeviceMemoryManager::AllocateAndBindImageMemory( VkImage image, VkMemoryPropertyFlags memory_properties )
{
	auto memory_info = AllocateImageMemory( image, memory_properties );
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		VulkanResultCheck( vkBindImageMemory( ref_vk_device.object, image, memory_info.memory, memory_info.offset ) );
	}
	return memory_info;
}

DeviceMemoryInfo DeviceMemoryManager::AllocateBufferMemory( VkBuffer buffer, VkMemoryPropertyFlags memory_properties )
{
	VkMemoryRequirements memory_requirements {};
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		vkGetBufferMemoryRequirements( ref_vk_device.object, buffer, &memory_requirements );
	}
	return AllocateMemory( memory_requirements, memory_properties );
}

DeviceMemoryInfo DeviceMemoryManager::AllocateAndBindBufferMemory( VkBuffer buffer, VkMemoryPropertyFlags memory_properties )
{
	auto memory_info = AllocateBufferMemory( buffer, memory_properties );
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		VulkanResultCheck( vkBindBufferMemory( ref_vk_device.object, buffer, memory_info.memory, memory_info.offset ) );
	}
	return memory_info;
}

void DeviceMemoryManager::FreeMemory( DeviceMemoryInfo & memory )
{
	TODO( "Memory pooling, we only have limited number of memory allocation times left" );
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		vkFreeMemory( ref_vk_device.object, memory.memory, VULKAN_ALLOC );
	}
	memory		= {};
}

DeviceMemoryInfo DeviceMemoryManager::AllocateMemory( VkMemoryRequirements & requirements, VkMemoryPropertyFlags memory_properties )
{
	TODO( "Memory pooling, we only have limited number of memory allocation times left" );

	DeviceMemoryInfo ret {};

	uint32_t memory_index		= FindMemoryTypeIndex( requirements, memory_properties );

	VkMemoryAllocateInfo memory_AI {};
	memory_AI.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_AI.pNext				= nullptr;
	memory_AI.allocationSize	= requirements.size;
	memory_AI.memoryTypeIndex	= memory_index;
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		vkAllocateMemory( ref_vk_device.object, &memory_AI, VULKAN_ALLOC, &ret.memory );
		ret.offset				= 0;
		ret.size				= requirements.size;
		ret.alignment			= requirements.alignment;
	}
	if( !ret.memory ) {
		p_logger->LogError( "Couldn't allocate device memory" );
	}

	return ret;
}

uint32_t DeviceMemoryManager::FindMemoryTypeIndex( VkMemoryRequirements & requirements, VkMemoryPropertyFlags property_flags )
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
