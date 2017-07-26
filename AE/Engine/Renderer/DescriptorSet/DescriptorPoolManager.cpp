
#include "DescriptorPoolManager.h"

#include <assert.h>

#include "../../Engine.h"
#include "../Renderer.h"
#include "../../Logger/Logger.h"
#include "DescriptorSetHandle.h"

namespace AE
{

DescriptorPoolManager::DescriptorPoolManager( Engine * engine, Renderer * renderer )
{
	p_engine			= engine;
	p_renderer			= renderer;
	assert( p_engine );
	assert( p_renderer );
	p_logger			= p_engine->GetLogger();
	assert( p_logger );

	ref_vk_device		= p_renderer->GetVulkanDevice();
}

DescriptorPoolManager::~DescriptorPoolManager()
{
	if( uniform_pool_list.size() ) {
		p_logger->LogWarning( "destroying descriptor pool manager with existing uniform sub pools, this might cause a errors later" );
	}
	if( image_pool_list.size() ) {
		p_logger->LogWarning( "destroying descriptor pool manager with existing image sub pools, this might cause a errors later" );
	}

	LOCK_GUARD( *ref_vk_device.mutex );
	for( auto & p : uniform_pool_list ) {
		ref_vk_device.object.destroyDescriptorPool( p.pool );
	}
	for( auto & p : image_pool_list ) {
		ref_vk_device.object.destroyDescriptorPool( p.pool );
	}
}

DescriptorSetHandle DescriptorPoolManager::AllocateDescriptorSetForCamera()
{
	vk::DescriptorSetAllocateInfo descriptor_set_AI {};
	descriptor_set_AI.pSetLayouts			= &p_renderer->GetVulkanDescriptorSetLayoutForCamera();
	return AllocateDescriptorSet( descriptor_set_AI, false );
}

DescriptorSetHandle DescriptorPoolManager::AllocateDescriptorSetForObject()
{
	vk::DescriptorSetAllocateInfo descriptor_set_AI {};
	descriptor_set_AI.pSetLayouts			= &p_renderer->GetVulkanDescriptorSetLayoutForObject();
	return AllocateDescriptorSet( descriptor_set_AI, false );
}

DescriptorSetHandle DescriptorPoolManager::AllocateDescriptorSetForPipeline()
{
	vk::DescriptorSetAllocateInfo descriptor_set_AI {};
	descriptor_set_AI.pSetLayouts			= &p_renderer->GetVulkanDescriptorSetLayoutForPipeline();
	return AllocateDescriptorSet( descriptor_set_AI, false );
}

DescriptorSetHandle DescriptorPoolManager::AllocateDescriptorSetForImages( uint32_t shader_image_count )
{
	vk::DescriptorSetAllocateInfo descriptor_set_AI {};
	descriptor_set_AI.pSetLayouts			= &p_renderer->GetVulkanDescriptorSetLayoutForImageBindingCount( shader_image_count );
	return AllocateDescriptorSet( descriptor_set_AI, true );
}

void DescriptorPoolManager::FreeDescriptorSet( DescriptorSubPoolInfo * pool_info, vk::DescriptorSet set )
{
	if( pool_info && set ) {
		assert( pool_info->users > 0 );
		pool_info->users--;
		if( pool_info->users <= 0 ) {
			// free entire vulkan pool, no need to free the descriptor set
			{
				LOCK_GUARD( *ref_vk_device.mutex );
				ref_vk_device.object.destroyDescriptorPool( pool_info->pool );
			}
			if( pool_info->is_image_pool ) {
				image_pool_list.remove( *pool_info );
			} else {
				uniform_pool_list.remove( *pool_info );
			}
		} else {
			// users not yet 0, free only the descriptor set
			TODO( "This could be optimized so that it frees descriptor sets in batches instead of individually" );
			ref_vk_device.object.freeDescriptorSets( pool_info->pool, set );
		}
	}
}

DescriptorSetHandle DescriptorPoolManager::AllocateDescriptorSet( vk::DescriptorSetAllocateInfo & allocate_info, bool is_image_pool )
{
	VkDescriptorSetLayout	layout		= allocate_info.pSetLayouts[ 0 ];

	VkDescriptorSetAllocateInfo AI {};
	AI.sType					= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	AI.pNext					= nullptr;
	AI.descriptorSetCount		= 1;
	AI.pSetLayouts				= &layout;

	auto lambda_allocate_set	= []( Logger * logger, VulkanDevice & vk_device, VkDescriptorSetAllocateInfo & allocate_info ) {
		VkDescriptorSet	set		= VK_NULL_HANDLE;
		VkResult result			= vkAllocateDescriptorSets( vk_device.object, &allocate_info, &set );
		if( result == VK_SUCCESS ) {
			// all good, return the set
			return set;
		} else if( result == VK_ERROR_FRAGMENTED_POOL || result == VK_ERROR_OUT_OF_POOL_MEMORY_KHR ) {
			// ran out of memory in this pool, try another
		} else {
			// ran out of memory, terminate
			logger->LogCritical( "Ran out of memory" );
		}
		return set;
	};

	VkDescriptorSet		set		= VK_NULL_HANDLE;

	if( is_image_pool ) {
		for( auto p : image_pool_list ) {
			LOCK_GUARD( *ref_vk_device.mutex );
			AI.descriptorPool	= p.pool;
			set = lambda_allocate_set( p_logger, ref_vk_device, AI );
			if( set ) {
				p.users++;
				return DescriptorSetHandle( p_engine, this, &p, set );
			}
		}
	} else {
		for( auto p : uniform_pool_list ) {
			LOCK_GUARD( *ref_vk_device.mutex );
			AI.descriptorPool	= p.pool;
			set = lambda_allocate_set( p_logger, ref_vk_device, AI );
			if( set ) {
				p.users++;
				return DescriptorSetHandle( p_engine, this, &p, set );
			}
		}
	}

	// if we got here, all current pools are full, create a new
	vk::DescriptorPoolCreateInfo pool_CI {};
	pool_CI.flags				= vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	pool_CI.maxSets				= BUILD_MAX_DESCRIPTOR_SETS_IN_POOL;
	if( is_image_pool ) {
		vk::DescriptorPoolSize pool_size {};
		pool_size.type				= vk::DescriptorType::eCombinedImageSampler;
		pool_size.descriptorCount	= BUILD_MAX_DESCRIPTOR_SETS_IN_POOL * ( BUILD_MAX_PER_SHADER_SAMPLED_IMAGE_COUNT / 2 );

		pool_CI.poolSizeCount		= 1;
		pool_CI.pPoolSizes			= &pool_size;

		LOCK_GUARD( *ref_vk_device.mutex );
		vk::DescriptorPool new_pool	= ref_vk_device.object.createDescriptorPool( pool_CI );
		if( new_pool ) {
			image_pool_list.push_back( { 0, new_pool, true } );
			AI.descriptorPool		= new_pool;
			set = lambda_allocate_set( p_logger, ref_vk_device, AI );
			if( set ) {
				image_pool_list.back().users++;
				return DescriptorSetHandle( p_engine, this, &image_pool_list.back(), set );
			} else {
				p_logger->LogCritical( "Couldn't create a new descriptor sets from a new pool, pool size too small" );
			}
		} else {
			p_logger->LogCritical( "Couldn't create a new descriptor pool" );
		}
	} else {
		vk::DescriptorPoolSize pool_size {};
		pool_size.type				= vk::DescriptorType::eUniformBuffer;
		pool_size.descriptorCount	= BUILD_MAX_DESCRIPTOR_SETS_IN_POOL;

		pool_CI.poolSizeCount		= 1;
		pool_CI.pPoolSizes			= &pool_size;

		LOCK_GUARD( *ref_vk_device.mutex );
		vk::DescriptorPool new_pool	= ref_vk_device.object.createDescriptorPool( pool_CI );
		if( new_pool ) {
			uniform_pool_list.push_back( { 0, new_pool, false } );
			AI.descriptorPool		= new_pool;
			set = lambda_allocate_set( p_logger, ref_vk_device, AI );
			if( set ) {
				uniform_pool_list.back().users++;
				return DescriptorSetHandle( p_engine, this, &uniform_pool_list.back(), set );
			} else {
				p_logger->LogCritical( "Couldn't create a new descriptor sets from a new pool, pool size too small" );
			}
		} else {
			p_logger->LogCritical( "Couldn't create a new descriptor pool" );
		}
	}
	return nullptr;
}

}
