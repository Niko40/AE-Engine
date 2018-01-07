#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Vulkan/Vulkan.h"

#include "DeviceMemoryInfo.h"
#include "../QueueInfo.h"

namespace AE
{

class Engine;
class Logger;
class Renderer;

class DeviceMemoryManager
{
public:
	DeviceMemoryManager( Engine * engine, Renderer * renderer );
	~DeviceMemoryManager();

	// Convenience function:
	// Create buffer object in either device memory or host memory,
	// this is non-managed return which means that the receiver is
	// responsible for freeing the object
	VkBuffer								CreateBuffer( VkBufferCreateFlags flags,
														  VkDeviceSize buffer_size,
														  VkBufferUsageFlags usage_flags,
														  UsedQueuesFlags shared_between_queues = UsedQueuesFlags( 0 ) );

	DeviceMemoryInfo						AllocateImageMemory( VkImage image, VkMemoryPropertyFlags memory_properties );
	DeviceMemoryInfo						AllocateAndBindImageMemory( VkImage image, VkMemoryPropertyFlags memory_properties );
	DeviceMemoryInfo						AllocateBufferMemory( VkBuffer buffer, VkMemoryPropertyFlags memory_properties );
	DeviceMemoryInfo						AllocateAndBindBufferMemory( VkBuffer buffer, VkMemoryPropertyFlags memory_properties );
	void									FreeMemory( DeviceMemoryInfo & memory );

private:
	DeviceMemoryInfo						AllocateMemory( VkMemoryRequirements & requirements, VkMemoryPropertyFlags memory_properties );
	uint32_t								FindMemoryTypeIndex( VkMemoryRequirements &requirements, VkMemoryPropertyFlags property_flags );

	Engine								*	p_engine					= nullptr;
	Logger								*	p_logger					= nullptr;
	Renderer							*	p_renderer					= nullptr;

	VkInstance								ref_vk_instance				= VK_NULL_HANDLE;
	VkPhysicalDevice						ref_vk_physical_device		= VK_NULL_HANDLE;
	VulkanDevice							ref_vk_device				= {};

	VkPhysicalDeviceMemoryProperties		vk_physical_device_memory_properties {};
};

}
