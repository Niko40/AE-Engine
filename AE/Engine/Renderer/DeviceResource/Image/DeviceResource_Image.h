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

	vk::CommandPool						ref_vk_primary_render_command_pool			= nullptr;
	vk::CommandPool						ref_vk_secondary_render_command_pool		= nullptr;
	vk::CommandPool						ref_vk_primary_transfer_command_pool		= nullptr;
	vk::CommandBuffer					vk_primary_render_command_buffer			= nullptr;
	vk::CommandBuffer					vk_secondary_render_command_buffer			= nullptr;
	vk::CommandBuffer					vk_primary_transfer_command_buffer			= nullptr;

	vk::Semaphore						vk_semaphore_stage_1						= nullptr;
	vk::Semaphore						vk_semaphore_stage_2						= nullptr;

	vk::Fence							vk_fence_command_buffers_done				= nullptr;

	vk::Image							vk_image									= nullptr;
	vk::ImageView						vk_image_view								= nullptr;
	DeviceMemoryInfo					image_memory								= {};

	vk::Buffer							vk_staging_buffer							= nullptr;
	DeviceMemoryInfo					staging_buffer_memory						= {};
};

}
