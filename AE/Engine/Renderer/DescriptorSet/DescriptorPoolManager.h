#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Vulkan/Vulkan.h"

#include "DescriptorPoolInfo.h"
#include "../../Memory/MemoryTypes.h"
#include "../../Threading/Threading.h"

namespace AE
{

class Engine;
class Logger;
class Renderer;
class DescriptorSetHandle;

class DescriptorPoolManager
{
public:
	DescriptorPoolManager( Engine * engine, Renderer * renderer );
	~DescriptorPoolManager();

	DescriptorSetHandle					AllocateDescriptorSetForCamera();
	DescriptorSetHandle					AllocateDescriptorSetForMesh();
	DescriptorSetHandle					AllocateDescriptorSetForPipeline();
	DescriptorSetHandle					AllocateDescriptorSetForImages( uint32_t shader_image_count );

	void								FreeDescriptorSet( DescriptorSubPoolInfo * pool_info, VkDescriptorSet set );

private:
	DescriptorSetHandle					AllocateDescriptorSet( VkDescriptorSetLayout layout, bool is_image_pool );

	Engine							*	p_engine					= nullptr;
	Logger							*	p_logger					= nullptr;
	Renderer						*	p_renderer					= nullptr;

	VulkanDevice						ref_vk_device				= {};

	Mutex								allocator_mutex;

	List<DescriptorSubPoolInfo>			uniform_pool_list;
	List<DescriptorSubPoolInfo>			image_pool_list;
};

}
