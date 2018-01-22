#pragma once

#include "../BUILD_OPTIONS.h"
#include "../Platform.h"

#include <vulkan/vulkan.h>

#include "../Memory/MemoryTypes.h"

namespace AE
{

#if BUILD_VULKAN_MEMORY_ALLOCATOR_TYPE == 1 || BUILD_VULKAN_MEMORY_ALLOCATOR_TYPE == 2
extern VkAllocationCallbacks	vulkan_allocation_callbacks;

#define VULKAN_ALLOC		&	vulkan_allocation_callbacks
#else
#define VULKAN_ALLOC			nullptr
#endif


struct VulkanDevice
{
	VkDevice					object;
	Mutex					*	mutex;
};

struct VulkanQueue
{
	VkQueue						object;
	Mutex					*	mutex;
};

// Non device blocking function to wait for fences, indefinitely if fences are never set
void VulkanWaitForFencesNonDeviceBlocking( VulkanDevice & ref_vk_device, const Vector<VkFence> & fences, VkBool32 wait_all );

// Non device blocking function to wait for fences and then reset them, will wait indefinitely if fences are never set
void VulkanWaitAndResetFencesNonDeviceBlocking( VulkanDevice & ref_vk_device, const Vector<VkFence> & fences );

// simple reset fences function
void VulkanResetFences( VulkanDevice & ref_vk_device, const Vector<VkFence> & fences );

String VulkanResultToString( VkResult result );
String VulkanFormatToString( VkFormat format );

VkResult VulkanResultCheck( VkResult result );

}
