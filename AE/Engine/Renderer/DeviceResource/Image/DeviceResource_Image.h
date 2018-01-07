#pragma once

#include "../../../BUILD_OPTIONS.h"
#include "../../../Platform.h"

#include "../../../Vulkan/Vulkan.h"

#include "../../DeviceMemory/DeviceMemoryInfo.h"
#include "../DeviceResource.h"
#include "../../../FileResource/Image/ImageData.h"

namespace AE
{

// converts raw image data into a format that's supported by the physical device directly
ImageData ConvertImageToPhysicalDeviceSupportedFormat( Renderer * renderer, const ImageData & image_data );

class DeviceResource_Image : public DeviceResource
{
	friend bool ContinueImageLoadTest_1( DeviceResource * resource );
	friend DeviceResource::LoadingState ContinueImageLoad_1( DeviceResource * resource );

public:
	DeviceResource_Image( Engine * engine, DeviceResource::Flags resource_flags = DeviceResource::Flags( 0 ) );
	~DeviceResource_Image();

private:
	LoadingState						Load();
	UnloadingState						Unload();

	VkCommandPool						ref_vk_primary_render_command_pool			= VK_NULL_HANDLE;
	VkCommandPool						ref_vk_secondary_render_command_pool		= VK_NULL_HANDLE;
	VkCommandPool						ref_vk_primary_transfer_command_pool		= VK_NULL_HANDLE;
	VkCommandBuffer						vk_primary_render_command_buffer			= VK_NULL_HANDLE;
	VkCommandBuffer						vk_secondary_render_command_buffer			= VK_NULL_HANDLE;
	VkCommandBuffer						vk_primary_transfer_command_buffer			= VK_NULL_HANDLE;

	VkSemaphore							vk_semaphore_stage_1						= VK_NULL_HANDLE;
	VkSemaphore							vk_semaphore_stage_2						= VK_NULL_HANDLE;

	VkFence								vk_fence_command_buffers_done				= VK_NULL_HANDLE;

	VkImage								vk_image									= VK_NULL_HANDLE;
	VkImageView							vk_image_view								= VK_NULL_HANDLE;
	DeviceMemoryInfo					image_memory								= {};

	VkBuffer							vk_staging_buffer							= VK_NULL_HANDLE;
	DeviceMemoryInfo					staging_buffer_memory						= {};
};

}
