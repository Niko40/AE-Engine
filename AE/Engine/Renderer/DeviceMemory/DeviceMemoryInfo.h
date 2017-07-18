#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Vulkan/Vulkan.h"

struct DeviceMemoryInfo
{
	vk::DeviceMemory		memory			= nullptr;
	vk::DeviceSize			offset			= 0;
	vk::DeviceSize			size			= 0;
	vk::DeviceSize			alignment		= 0;
};
