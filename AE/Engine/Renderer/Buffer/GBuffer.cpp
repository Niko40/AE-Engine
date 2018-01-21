
#include "GBuffer.h"

#include <assert.h>

#include "../../Engine.h"
#include "../../Logger/Logger.h"
#include "../../Renderer/Renderer.h"
#include "../../Renderer/DeviceMemory/DeviceMemoryManager.h"

namespace AE
{

GBuffer::GBuffer( Engine * engine, Renderer * renderer, VkFormat image_format, VkImageUsageFlags image_usage_flags, VkImageAspectFlags image_aspect_flags, VkExtent2D size )
{
	p_engine		= engine;
	p_renderer		= renderer;
	format			= image_format;
	assert( p_engine );
	assert( p_renderer );
	assert( format != VK_FORMAT_UNDEFINED );
	p_logger		= p_engine->GetLogger();
	ref_vk_device	= p_renderer->GetVulkanDevice();
	primary_render_queue_family_index	= p_renderer->GetPrimaryRenderQueueFamilyIndex();
	assert( p_logger );
	assert( ref_vk_device.object );
	assert( ref_vk_device.mutex );
	assert( primary_render_queue_family_index != UINT32_MAX );


	VkImageCreateInfo image_CI {};
	image_CI.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_CI.pNext					= nullptr;
	image_CI.flags					= 0;
	image_CI.imageType				= VK_IMAGE_TYPE_2D;
	image_CI.format					= format;
	image_CI.extent.width			= size.width;
	image_CI.extent.height			= size.height;
	image_CI.extent.depth			= 1;
	image_CI.mipLevels				= 1;
	image_CI.arrayLayers			= 1;
	image_CI.samples				= VK_SAMPLE_COUNT_1_BIT;
	image_CI.tiling					= VK_IMAGE_TILING_OPTIMAL;
	image_CI.usage					= image_usage_flags;
	image_CI.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
	image_CI.queueFamilyIndexCount	= 1;
	image_CI.pQueueFamilyIndices	= &primary_render_queue_family_index;
	image_CI.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;

	{
		// We don't have device resource threads at this points but better to be sure
		LOCK_GUARD( *ref_vk_device.mutex );
		VulkanResultCheck( vkCreateImage( ref_vk_device.object, &image_CI, VULKAN_ALLOC, &vk_image ) );
	}
	if( !vk_image ) {
		p_logger->LogCritical( "G-Buffer image creation failed, format: " + VulkanFormatToString( format ) );
	}
	image_memory				= p_renderer->GetDeviceMemoryManager()->AllocateAndBindImageMemory( vk_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	if( !image_memory.memory ) {
		p_logger->LogCritical( "G-Buffer image memory allocation failed, format: " + VulkanFormatToString( format ) );
	}

	VkImageViewCreateInfo image_view_CI {};
	image_view_CI.sType			= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_CI.pNext			= nullptr;
	image_view_CI.flags			= 0;
	image_view_CI.image			= vk_image;
	image_view_CI.viewType		= VK_IMAGE_VIEW_TYPE_2D;
	image_view_CI.format		= format;
	image_view_CI.components.r	= VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_CI.components.g	= VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_CI.components.b	= VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_CI.components.a	= VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_CI.subresourceRange.aspectMask		= image_aspect_flags;
	image_view_CI.subresourceRange.baseMipLevel		= 0;
	image_view_CI.subresourceRange.levelCount		= 1;
	image_view_CI.subresourceRange.baseArrayLayer	= 0;
	image_view_CI.subresourceRange.layerCount		= 1;

	{
		// We don't have device resource threads at this points but better to be sure
		LOCK_GUARD( *ref_vk_device.mutex );
		VulkanResultCheck( vkCreateImageView( ref_vk_device.object, &image_view_CI, VULKAN_ALLOC, &vk_image_view ) );
	}
	if( !vk_image_view ) {
		p_logger->LogCritical( "Depth stencil image view creation failed" );
	}
}

GBuffer::~GBuffer()
{
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		vkDestroyImageView( ref_vk_device.object, vk_image_view, VULKAN_ALLOC );
		vkDestroyImage( ref_vk_device.object, vk_image, VULKAN_ALLOC );
	}
	p_renderer->GetDeviceMemoryManager()->FreeMemory( image_memory );
	vk_image_view		= VK_NULL_HANDLE;
	vk_image			= VK_NULL_HANDLE;
	image_memory		= {};
}

VkImage GBuffer::GetVulkanImage() const
{
	return vk_image;
}

VkImageView GBuffer::GetVulkanImageView() const
{
	return vk_image_view;
}

VkFormat GBuffer::GetVulkanFormat() const
{
	return format;
}

}
