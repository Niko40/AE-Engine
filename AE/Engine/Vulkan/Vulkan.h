#pragma once

#include "../BUILD_OPTIONS.h"
#include "../Platform.h"

#include <vulkan/vulkan.hpp>

namespace AE
{

struct VulkanDevice
{
	vk::Device					object;
	Mutex					*	mutex;
};

struct VulkanQueue
{
	vk::Queue					object;
	Mutex					*	mutex;
};

}
