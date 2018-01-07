#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Vulkan/Vulkan.h"

#include "../../Memory/MemoryTypes.h"

namespace AE
{

struct ImageData
{
	VkFormat				format				= VK_FORMAT_UNDEFINED;
	uint32_t				used_channels		= 0;
	uint32_t				bits_per_channel	= 0;
	uint32_t				bytes_per_pixel		= 0;
	uint32_t				width				= 0;
	uint32_t				height				= 0;
	VkBool32				has_alpha			= VK_FALSE;
	Vector<uint8_t>			image_bytes;
};

}
