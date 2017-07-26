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

	DescriptorSetHandle( Engine * engine, DescriptorPoolManager * descriptor_pool, DescriptorSubPoolInfo * descriptor_pool_info, vk::DescriptorSet descriptor_set );

public:
	DescriptorSetHandle();
	DescriptorSetHandle( nullptr_t );
	DescriptorSetHandle( const DescriptorSetHandle & other ) = delete;
	DescriptorSetHandle( DescriptorSetHandle && other );

	~DescriptorSetHandle();

	DescriptorSetHandle				&	operator=( const DescriptorSetHandle & other ) = delete;
	DescriptorSetHandle				&	operator=( DescriptorSetHandle && other );

	operator							vk::DescriptorSet();

private:
	void								Swap( DescriptorSetHandle && other );

	Engine							*	p_engine						= nullptr;

	DescriptorPoolManager			*	pool							= nullptr;
	DescriptorSubPoolInfo		*	pool_info						= nullptr;
	vk::DescriptorSet					set								= nullptr;
};

}
