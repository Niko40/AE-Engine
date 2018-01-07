#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Vulkan/Vulkan.h"

#include "DescriptorPoolInfo.h"
#include "../../Memory/MemoryTypes.h"

namespace AE
{

class Engine;
class DescriptorPoolManager;

class DescriptorSetHandle
{
	friend class DescriptorPoolManager;

	DescriptorSetHandle( Engine * engine, DescriptorPoolManager * descriptor_pool, DescriptorSubPoolInfo * descriptor_pool_info, VkDescriptorSet descriptor_set );

public:
	DescriptorSetHandle();
	DescriptorSetHandle( nullptr_t );
	DescriptorSetHandle( const DescriptorSetHandle & other ) = delete;
	DescriptorSetHandle( DescriptorSetHandle && other );

	~DescriptorSetHandle();

	DescriptorSetHandle				&	operator=( const DescriptorSetHandle & other ) = delete;
	DescriptorSetHandle				&	operator=( DescriptorSetHandle && other );

	operator							bool();

	operator							VkDescriptorSet();

private:
	void								Swap( DescriptorSetHandle && other );

	Engine							*	p_engine						= nullptr;

	DescriptorPoolManager			*	pool							= nullptr;
	DescriptorSubPoolInfo			*	pool_info						= nullptr;
	VkDescriptorSet						set								= VK_NULL_HANDLE;
};

}
