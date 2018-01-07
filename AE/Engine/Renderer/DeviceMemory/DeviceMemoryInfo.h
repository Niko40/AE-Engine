#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Vulkan/Vulkan.h"

struct DeviceMemoryInfo
{
	VkDeviceMemory			memory			= VK_NULL_HANDLE;
	VkDeviceSize			offset			= 0;
	VkDeviceSize			size			= 0;
	VkDeviceSize			alignment		= 0;
};
