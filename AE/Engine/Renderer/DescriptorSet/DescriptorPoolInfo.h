#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Vulkan/Vulkan.h"

namespace AE
{

struct DescriptorSubPoolInfo
{
	int32_t							users						= 0;
	vk::DescriptorPool				pool						= nullptr;
	bool							is_image_pool				= false;

	bool							operator==( DescriptorSubPoolInfo other );
};

}
