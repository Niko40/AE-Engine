#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Vulkan/Vulkan.h"
#include "../../Renderer/DeviceMemory/DeviceMemoryInfo.h"

namespace AE
{

class Engine;
class Logger;
class Renderer;

class GBuffer
{
public:
	GBuffer( Engine * engine, Renderer * renderer, VkFormat image_format, VkImageUsageFlags image_usage_flags, VkImageAspectFlags image_aspect_flags, VkExtent2D size );
	~GBuffer();

	VkImage							GetVulkanImage() const;
	VkImageView						GetVulkanImageView() const;

	VkFormat						GetVulkanFormat() const;

private:
	Engine						*	p_engine							= nullptr;
	Logger						*	p_logger							= nullptr;
	Renderer					*	p_renderer							= nullptr;
	VulkanDevice					ref_vk_device						= {};

	uint32_t						primary_render_queue_family_index	= UINT32_MAX;

	VkImage							vk_image							= VK_NULL_HANDLE;
	VkImageView						vk_image_view						= VK_NULL_HANDLE;

	DeviceMemoryInfo				image_memory						= {};

	VkFormat						format								= VK_FORMAT_UNDEFINED;
};

}
