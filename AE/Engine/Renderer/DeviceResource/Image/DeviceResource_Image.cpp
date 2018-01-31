
#include "DeviceResource_Image.h"

#include "../../Renderer.h"
#include "../../DeviceMemory/DeviceMemoryManager.h"
#include "../../DeviceResource/DeviceResourceManager.h"
#include "../../../FileResource/Image/FileResource_Image.h"

namespace AE
{

DeviceResource_Image::DeviceResource_Image( Engine * engine, DeviceResource::Flags resource_flags )
	: DeviceResource( engine, DeviceResource::Type::IMAGE, resource_flags | DeviceResource::Flags::STATIC )
{
}

DeviceResource_Image::~DeviceResource_Image()
{
	// destroy everything that might still hang around in memory
	Unload();
}

bool ContinueImageLoadTest_1( DeviceResource * resource )
{
	auto r = dynamic_cast<DeviceResource_Image*>( resource );
	LOCK_GUARD( *r->ref_vk_device.mutex );
	if( vkGetFenceStatus( r->ref_vk_device.object, r->vk_fence_command_buffers_done ) == VK_SUCCESS ) {
		VulkanResultCheck( vkResetFences( r->ref_vk_device.object, 1, &r->vk_fence_command_buffers_done ) );
		return true;
	}
	return false;
}

DeviceResource::LoadingState ContinueImageLoad_1( DeviceResource * resource )
{
	auto r = dynamic_cast<DeviceResource_Image*>( resource );
//	r->ref_vk_device.resetFences( r->vk_fence_command_buffers_done );
	{
		LOCK_GUARD( *r->ref_vk_device.mutex );

		// free command buffers used for uploading and manipulating the image in the GPU
		{
			vkFreeCommandBuffers( r->ref_vk_device.object, r->ref_vk_primary_render_command_pool, 1, &r->vk_primary_render_command_buffer );
			vkFreeCommandBuffers( r->ref_vk_device.object, r->ref_vk_secondary_render_command_pool, 1, &r->vk_secondary_render_command_buffer );
			vkFreeCommandBuffers( r->ref_vk_device.object, r->ref_vk_primary_transfer_command_pool, 1, &r->vk_primary_transfer_command_buffer );
			r->vk_primary_render_command_buffer		= VK_NULL_HANDLE;
			r->vk_secondary_render_command_buffer	= VK_NULL_HANDLE;
			r->vk_primary_transfer_command_buffer	= VK_NULL_HANDLE;
		}

		// destroy synchronization objects, not needed anymore
		{
			vkDestroyFence( r->ref_vk_device.object, r->vk_fence_command_buffers_done, VULKAN_ALLOC );
			vkDestroySemaphore( r->ref_vk_device.object, r->vk_semaphore_stage_1, VULKAN_ALLOC );
			vkDestroySemaphore( r->ref_vk_device.object, r->vk_semaphore_stage_2, VULKAN_ALLOC );
			r->vk_fence_command_buffers_done	= VK_NULL_HANDLE;
			r->vk_semaphore_stage_1				= VK_NULL_HANDLE;
			r->vk_semaphore_stage_2				= VK_NULL_HANDLE;
		}

		// destroy staging buffer, not needed anymore
		{
			vkDestroyBuffer( r->ref_vk_device.object, r->vk_staging_buffer, VULKAN_ALLOC );
			r->vk_staging_buffer		= VK_NULL_HANDLE;
		}
	}

	// free staging buffer memory, not needed anymore
	{
		r->p_device_memory_manager->FreeMemory( r->staging_buffer_memory );
		r->staging_buffer_memory	= {};
	}
	return DeviceResource::LoadingState::LOADED;
}

DeviceResource::LoadingState DeviceResource_Image::Load()
{
	if( file_resources.size() == 0 ) {
		return DeviceResource::LoadingState::UNABLE_TO_LOAD;
	}

	if( file_resources[ 0 ]->GetResourceType() != FileResource::Type::IMAGE ) {
		return DeviceResource::LoadingState::UNABLE_TO_LOAD;
	}
	auto image_resource = dynamic_cast<FileResource_Image*>( file_resources[ 0 ].Get() );

	assert( image_resource->IsResourceReadyForUse() );

	auto image_data = ConvertImageToPhysicalDeviceSupportedFormat( p_renderer, image_resource->GetImageData() );
	if( image_data.width == 0 || image_data.height == 0 ) {
		return DeviceResource::LoadingState::UNABLE_TO_LOAD;
	}
	auto ref_vk_device = p_renderer->GetVulkanDevice();

	// Create staging buffer, staging buffer memory, bind memory to buffer and populat the memory with data
	{
		vk_staging_buffer		= p_device_memory_manager->CreateBuffer( 0, uint32_t( image_data.image_bytes.size() ), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, UsedQueuesFlags::PRIMARY_TRANSFER );

		staging_buffer_memory	= p_device_memory_manager->AllocateAndBindBufferMemory( vk_staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
		if( staging_buffer_memory.memory ) {
			void * mapped_memory;
			{
				LOCK_GUARD( *ref_vk_device.mutex );
				VulkanResultCheck( vkMapMemory( ref_vk_device.object, staging_buffer_memory.memory, staging_buffer_memory.offset, staging_buffer_memory.size, 0, &mapped_memory ) );
			}
			if( nullptr != mapped_memory ) {
				std::memcpy( mapped_memory, image_data.image_bytes.data(), image_data.image_bytes.size() );
				LOCK_GUARD( *ref_vk_device.mutex );
				vkUnmapMemory( ref_vk_device.object, staging_buffer_memory.memory );
			} else {
				LOCK_GUARD( *ref_vk_device.mutex );
				vkDestroyBuffer( ref_vk_device.object, vk_staging_buffer, VULKAN_ALLOC );
				p_device_memory_manager->FreeMemory( staging_buffer_memory );
				vk_staging_buffer		= VK_NULL_HANDLE;
				p_device_memory_manager	= {};
				return DeviceResource::LoadingState::UNABLE_TO_LOAD;
			}
		}
	}

	// Image creation
	Vector<VkExtent3D>				mip_levels;
	VkImageSubresourceRange			image_sub_resource_range_first_mip_only {};
	VkImageSubresourceRange			image_sub_resource_range_complete {};
	image_sub_resource_range_first_mip_only.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	image_sub_resource_range_first_mip_only.baseMipLevel	= 0;
	image_sub_resource_range_first_mip_only.levelCount		= 1;
	image_sub_resource_range_first_mip_only.baseArrayLayer	= 0;
	image_sub_resource_range_first_mip_only.layerCount		= 1;

	image_sub_resource_range_complete						= image_sub_resource_range_first_mip_only;	// changed later to cover all mip levels
	{
		mip_levels.push_back( VkExtent3D { image_data.width, image_data.height, 1 } );
		{
			uint32_t mwidth				= image_data.width;
			uint32_t mheight			= image_data.height;
			while( mwidth > 1 && mheight > 1 ) {
				mwidth		/= 2;
				mheight		/= 2;
				if( mwidth < 1 )		mwidth		= 1;
				if( mheight < 1 )		mheight		= 1;
				mip_levels.push_back( VkExtent3D { mwidth, mheight, 1 } );
			}
		}
		image_sub_resource_range_complete.levelCount			= uint32_t( mip_levels.size() );

		VkImageCreateInfo image_CI {};
		image_CI.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_CI.pNext					= nullptr;
		image_CI.flags					= 0;
		image_CI.imageType				= VK_IMAGE_TYPE_2D;
		image_CI.format					= image_data.format;
		image_CI.extent					= VkExtent3D { image_data.width, image_data.height, 1 };
		image_CI.mipLevels				= uint32_t( mip_levels.size() );
		image_CI.arrayLayers			= 1;
		image_CI.samples				= VK_SAMPLE_COUNT_1_BIT;
		image_CI.tiling					= VK_IMAGE_TILING_OPTIMAL;
		image_CI.usage					= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		image_CI.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;		// we use exclusive sharing mode to enable full speed access to the image at render-time
		image_CI.queueFamilyIndexCount	= 0;
		image_CI.pQueueFamilyIndices	= nullptr;
		image_CI.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
		{
			LOCK_GUARD( *ref_vk_device.mutex );
			VulkanResultCheck( vkCreateImage( ref_vk_device.object, &image_CI, VULKAN_ALLOC, &vk_image ) );
		}
		image_memory					= p_device_memory_manager->AllocateAndBindImageMemory( vk_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

		VkComponentMapping component_mapping {};
		if( image_data.has_alpha ) {
			if( image_data.used_channels == 1 ) {
				assert( 0 && "We can't have only alpha" );
			}
			if( image_data.used_channels == 2 ) {
				component_mapping.a		= VK_COMPONENT_SWIZZLE_G;
			}
			if( image_data.used_channels == 3 ) {
				component_mapping.a		= VK_COMPONENT_SWIZZLE_B;
			}
			if( image_data.used_channels == 4 ) {
				component_mapping.a		= VK_COMPONENT_SWIZZLE_A;
			}
		}
		VkImageViewCreateInfo image_view_CI {};
		image_view_CI.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_CI.pNext				= nullptr;
		image_view_CI.flags				= 0;
		image_view_CI.image				= vk_image;
		image_view_CI.viewType			= VK_IMAGE_VIEW_TYPE_2D;
		image_view_CI.format			= image_data.format;
		image_view_CI.components		= component_mapping;
		image_view_CI.subresourceRange	= image_sub_resource_range_complete;
		{
			LOCK_GUARD( *ref_vk_device.mutex );
			VulkanResultCheck( vkCreateImageView( ref_vk_device.object, &image_view_CI, VULKAN_ALLOC, &vk_image_view ) );
		}
	}

	ref_vk_device						= p_renderer->GetVulkanDevice();

	ref_vk_primary_render_command_pool			= p_device_resource_manager->GetPrimaryRenderCommandPoolForThisThread();
	ref_vk_secondary_render_command_pool		= p_device_resource_manager->GetSecondaryRenderCommandPoolForThisThread();
	ref_vk_primary_transfer_command_pool		= p_device_resource_manager->GetPrimaryTransferCommandPoolForThisThread();

	// write command buffer to transfer the image into the physical device
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		{
			VkCommandBufferAllocateInfo command_buffer_AI {};
			command_buffer_AI.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			command_buffer_AI.pNext					= nullptr;
			command_buffer_AI.commandPool			= ref_vk_primary_render_command_pool;
			command_buffer_AI.level					= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			command_buffer_AI.commandBufferCount	= 1;
			VulkanResultCheck( vkAllocateCommandBuffers( ref_vk_device.object, &command_buffer_AI, &vk_primary_render_command_buffer ) );
		}
		{
			VkCommandBufferAllocateInfo command_buffer_AI {};
			command_buffer_AI.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			command_buffer_AI.pNext					= nullptr;
			command_buffer_AI.commandPool			= ref_vk_secondary_render_command_pool;
			command_buffer_AI.level					= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			command_buffer_AI.commandBufferCount	= 1;
			VulkanResultCheck( vkAllocateCommandBuffers( ref_vk_device.object, &command_buffer_AI, &vk_secondary_render_command_buffer ) );
		}
		{
			VkCommandBufferAllocateInfo command_buffer_AI {};
			command_buffer_AI.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			command_buffer_AI.pNext					= nullptr;
			command_buffer_AI.commandPool			= ref_vk_primary_transfer_command_pool;
			command_buffer_AI.level					= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			command_buffer_AI.commandBufferCount	= 1;
			VulkanResultCheck( vkAllocateCommandBuffers( ref_vk_device.object, &command_buffer_AI, &vk_primary_transfer_command_buffer ) );
		}
		if( !( vk_primary_render_command_buffer && vk_secondary_render_command_buffer && vk_primary_transfer_command_buffer ) ) {
			return DeviceResource::LoadingState::UNABLE_TO_LOAD;
		}
	}

	// Begin: Transfer command buffer
	{
		VkCommandBufferBeginInfo command_buffer_BI {};
		command_buffer_BI.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_buffer_BI.pNext					= nullptr;
		command_buffer_BI.flags					= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VulkanResultCheck( vkBeginCommandBuffer( vk_primary_transfer_command_buffer, &command_buffer_BI ) );

		// Record: Set buffer to act as a source for transfer and translate image layout from undefined to transfer destination optimal
		{
			VkBufferMemoryBarrier buffer_memory_barrier {};
			buffer_memory_barrier.sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			buffer_memory_barrier.pNext					= nullptr;
			buffer_memory_barrier.srcAccessMask			= VK_ACCESS_HOST_WRITE_BIT;
			buffer_memory_barrier.dstAccessMask			= VK_ACCESS_TRANSFER_READ_BIT;
			buffer_memory_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			buffer_memory_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			buffer_memory_barrier.buffer				= vk_staging_buffer;
			buffer_memory_barrier.offset				= 0;
			buffer_memory_barrier.size					= staging_buffer_memory.size;

			VkImageMemoryBarrier image_memory_barrier {};
			image_memory_barrier.sType					= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			image_memory_barrier.pNext					= nullptr;
			image_memory_barrier.srcAccessMask			= 0;
			image_memory_barrier.dstAccessMask			= VK_ACCESS_TRANSFER_WRITE_BIT;
			image_memory_barrier.oldLayout				= VK_IMAGE_LAYOUT_UNDEFINED;
			image_memory_barrier.newLayout				= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			image_memory_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			image_memory_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			image_memory_barrier.image					= vk_image;
			image_memory_barrier.subresourceRange		= image_sub_resource_range_complete;
			TODO( "Possible optimization, translate only one mip level at a time before we fill them all out" );

			vkCmdPipelineBarrier( vk_primary_transfer_command_buffer,
				VK_PIPELINE_STAGE_HOST_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				1, &buffer_memory_barrier,
				1, &image_memory_barrier );
		}

		// Record: Copy buffer to first mip level of the image
		{
			Vector<VkBufferImageCopy> regions( 1 );
			regions[ 0 ].bufferOffset						= 0;
			regions[ 0 ].bufferRowLength					= 0;
			regions[ 0 ].bufferImageHeight					= 0;
			regions[ 0 ].imageSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
			regions[ 0 ].imageSubresource.mipLevel			= 0;
			regions[ 0 ].imageSubresource.baseArrayLayer	= 0;
			regions[ 0 ].imageSubresource.layerCount		= 1;
			regions[ 0 ].imageOffset						= { 0, 0, 0 };
			regions[ 0 ].imageExtent						= { image_data.width, image_data.height, 1 };

			vkCmdCopyBufferToImage( vk_primary_transfer_command_buffer,
				vk_staging_buffer, vk_image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				uint32_t( regions.size() ), regions.data() );
		}

		// Record: Translate image layout for blitting
		{
			VkImageMemoryBarrier image_memory_barrier {};
			image_memory_barrier.sType					= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			image_memory_barrier.pNext					= nullptr;
			image_memory_barrier.srcAccessMask			= VK_ACCESS_TRANSFER_WRITE_BIT;
			image_memory_barrier.dstAccessMask			= VK_ACCESS_TRANSFER_READ_BIT;
			image_memory_barrier.oldLayout				= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			image_memory_barrier.newLayout				= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			image_memory_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			image_memory_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			image_memory_barrier.image					= vk_image;
			image_memory_barrier.subresourceRange		= image_sub_resource_range_complete;

			vkCmdPipelineBarrier( vk_primary_transfer_command_buffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &image_memory_barrier );
		}

		// Record: Release exclusive ownership of the image
		// separated from the previous memory barrier to shut up validation layers,
		// shouldn't be much of a performance impact but might want to fix later
		{
			VkImageMemoryBarrier image_memory_barrier {};
			image_memory_barrier.sType					= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			image_memory_barrier.pNext					= nullptr;
			image_memory_barrier.srcAccessMask			= VK_ACCESS_TRANSFER_READ_BIT;
			image_memory_barrier.dstAccessMask			= VK_ACCESS_TRANSFER_READ_BIT;
			image_memory_barrier.oldLayout				= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			image_memory_barrier.newLayout				= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			image_memory_barrier.srcQueueFamilyIndex	= p_renderer->GetPrimaryTransferQueueFamilyIndex();
			image_memory_barrier.dstQueueFamilyIndex	= p_renderer->GetSecondaryRenderQueueFamilyIndex();
			image_memory_barrier.image					= vk_image;
			image_memory_barrier.subresourceRange		= image_sub_resource_range_complete;

			vkCmdPipelineBarrier( vk_primary_transfer_command_buffer,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,		// ignored according to specification, we need a semaphore between queue submits
				0,
				0, nullptr,
				0, nullptr,
				1, &image_memory_barrier );
		}

		VulkanResultCheck( vkEndCommandBuffer( vk_primary_transfer_command_buffer ) );
	}

	// Begin: secondary render command buffer
	{
		VkCommandBufferBeginInfo command_buffer_BI {};
		command_buffer_BI.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_buffer_BI.pNext					= nullptr;
		command_buffer_BI.flags					= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VulkanResultCheck( vkBeginCommandBuffer( vk_secondary_render_command_buffer, &command_buffer_BI ) );

		// Record: < CONTINUED > Aquire exclusive ownership of the image
		// This pipeline barrier is not executed twice but is required for the completion of the exclusivity transfer
		{
			VkImageMemoryBarrier image_memory_barrier {};
			image_memory_barrier.sType					= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			image_memory_barrier.pNext					= nullptr;
			image_memory_barrier.srcAccessMask			= VK_ACCESS_TRANSFER_READ_BIT;
			image_memory_barrier.dstAccessMask			= VK_ACCESS_TRANSFER_READ_BIT;
			image_memory_barrier.oldLayout				= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			image_memory_barrier.newLayout				= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			image_memory_barrier.srcQueueFamilyIndex	= p_renderer->GetPrimaryTransferQueueFamilyIndex();
			image_memory_barrier.dstQueueFamilyIndex	= p_renderer->GetSecondaryRenderQueueFamilyIndex();
			image_memory_barrier.image					= vk_image;
			image_memory_barrier.subresourceRange		= image_sub_resource_range_complete;

			vkCmdPipelineBarrier( vk_secondary_render_command_buffer,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,		// ignored according to specification, we need a semaphore between queue submits
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &image_memory_barrier );
		}

		for( uint32_t i=1; i < mip_levels.size(); ++i ) {
			auto & m		= mip_levels[ i ];
			auto & mprev	= mip_levels[ i - 1 ];

			// Record: Transition current mip level for writing to it
			{
				VkImageMemoryBarrier image_memory_barrier {};
				image_memory_barrier.sType					= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				image_memory_barrier.pNext					= nullptr;
				image_memory_barrier.srcAccessMask			= 0;
				image_memory_barrier.dstAccessMask			= VK_ACCESS_TRANSFER_WRITE_BIT;
				image_memory_barrier.oldLayout				= VK_IMAGE_LAYOUT_UNDEFINED;			// we don't care about the existing contents of this mip level
				image_memory_barrier.newLayout				= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				image_memory_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				image_memory_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				image_memory_barrier.image					= vk_image;
				image_memory_barrier.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
				image_memory_barrier.subresourceRange.baseMipLevel		= i;
				image_memory_barrier.subresourceRange.levelCount		= 1;
				image_memory_barrier.subresourceRange.baseArrayLayer	= 0;
				image_memory_barrier.subresourceRange.layerCount		= 1;

				vkCmdPipelineBarrier( vk_secondary_render_command_buffer,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &image_memory_barrier );
			}

			// Record: blit image mip level from previous higher mip level
			{
				VkImageBlit region {};

				// Source
				region.srcSubresource.aspectMask	= VK_IMAGE_ASPECT_COLOR_BIT;
				region.srcSubresource.layerCount	= 1;
				region.srcSubresource.mipLevel		= i - 1;
				region.srcOffsets[ 0 ]				= { 0, 0, 0 };
				region.srcOffsets[ 1 ]				= { int32_t( mprev.width ), int32_t( mprev.height ), int32_t( mprev.depth ) };

				// Destination
				region.dstSubresource.aspectMask	= VK_IMAGE_ASPECT_COLOR_BIT;
				region.dstSubresource.layerCount	= 1;
				region.dstSubresource.mipLevel		= i;
				region.dstOffsets[ 0 ]				= { 0, 0, 0 };
				region.dstOffsets[ 1 ]				= { int32_t( m.width ), int32_t( m.height ), int32_t( m.depth ) };

				vkCmdBlitImage( vk_secondary_render_command_buffer,
					vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &region,
					VK_FILTER_LINEAR );
			}

			// Record: Transition current mip level to be the source for the next mip level
			{
				VkImageMemoryBarrier image_memory_barrier {};
				image_memory_barrier.sType					= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				image_memory_barrier.pNext					= nullptr;
				image_memory_barrier.srcAccessMask			= VK_ACCESS_TRANSFER_WRITE_BIT;
				image_memory_barrier.dstAccessMask			= VK_ACCESS_TRANSFER_READ_BIT;
				image_memory_barrier.oldLayout				= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				image_memory_barrier.newLayout				= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				image_memory_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				image_memory_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				image_memory_barrier.image					= vk_image;
				image_memory_barrier.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
				image_memory_barrier.subresourceRange.baseMipLevel		= i;
				image_memory_barrier.subresourceRange.levelCount		= 1;
				image_memory_barrier.subresourceRange.baseArrayLayer	= 0;
				image_memory_barrier.subresourceRange.layerCount		= 1;

				vkCmdPipelineBarrier( vk_secondary_render_command_buffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &image_memory_barrier );
			}
		}

		// Record: Transition whole image to be used in a shader
		{
			VkImageMemoryBarrier image_memory_barrier {};
			image_memory_barrier.sType					= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			image_memory_barrier.pNext					= nullptr;
			image_memory_barrier.srcAccessMask			= VK_ACCESS_TRANSFER_READ_BIT;
			image_memory_barrier.dstAccessMask			= VK_ACCESS_SHADER_READ_BIT;
			image_memory_barrier.oldLayout				= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			image_memory_barrier.newLayout				= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_memory_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			image_memory_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			image_memory_barrier.image					= vk_image;
			image_memory_barrier.subresourceRange		= image_sub_resource_range_complete;

			vkCmdPipelineBarrier( vk_secondary_render_command_buffer,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &image_memory_barrier );
		}

		// Record: Release exclusive ownership of the image
		// separated from the previous memory barrier to shut up validation layers,
		// shouldn't be much of a performance impact but might want to fix later
		{
			VkImageMemoryBarrier image_memory_barrier {};
			image_memory_barrier.sType					= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			image_memory_barrier.pNext					= nullptr;
			image_memory_barrier.srcAccessMask			= VK_ACCESS_SHADER_READ_BIT;
			image_memory_barrier.dstAccessMask			= VK_ACCESS_SHADER_READ_BIT;
			image_memory_barrier.oldLayout				= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_memory_barrier.newLayout				= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_memory_barrier.srcQueueFamilyIndex	= p_renderer->GetSecondaryRenderQueueFamilyIndex();
			image_memory_barrier.dstQueueFamilyIndex	= p_renderer->GetPrimaryRenderQueueFamilyIndex();
			image_memory_barrier.image					= vk_image;
			image_memory_barrier.subresourceRange		= image_sub_resource_range_complete;

			vkCmdPipelineBarrier( vk_secondary_render_command_buffer,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,		// ignored according to specification, we need a semaphore between queue submits
				0,
				0, nullptr,
				0, nullptr,
				1, &image_memory_barrier );
		}

		VulkanResultCheck( vkEndCommandBuffer( vk_secondary_render_command_buffer ) );
	}

	// Begin: primary render command buffer
	{
		VkCommandBufferBeginInfo command_buffer_BI {};
		command_buffer_BI.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_buffer_BI.pNext					= nullptr;
		command_buffer_BI.flags					= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VulkanResultCheck( vkBeginCommandBuffer( vk_primary_render_command_buffer, &command_buffer_BI ) );

		// Record: < CONTINUE > Acquire exclusive ownership of the image
		{
			VkImageMemoryBarrier image_memory_barrier {};
			image_memory_barrier.sType					= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			image_memory_barrier.pNext					= nullptr;
			image_memory_barrier.srcAccessMask			= VK_ACCESS_SHADER_READ_BIT;
			image_memory_barrier.dstAccessMask			= VK_ACCESS_SHADER_READ_BIT;
			image_memory_barrier.oldLayout				= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_memory_barrier.newLayout				= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_memory_barrier.srcQueueFamilyIndex	= p_renderer->GetSecondaryRenderQueueFamilyIndex();
			image_memory_barrier.dstQueueFamilyIndex	= p_renderer->GetPrimaryRenderQueueFamilyIndex();
			image_memory_barrier.image					= vk_image;
			image_memory_barrier.subresourceRange		= image_sub_resource_range_complete;

			vkCmdPipelineBarrier( vk_primary_render_command_buffer,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,		// ignored according to specification, we need a semaphore between queue submits
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &image_memory_barrier );
		}

		VulkanResultCheck( vkEndCommandBuffer( vk_primary_render_command_buffer ) );
	}

	// Create synchronization objects
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		VkSemaphoreCreateInfo sepaphore_CI {};
		sepaphore_CI.sType					= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		sepaphore_CI.pNext					= nullptr;
		sepaphore_CI.flags					= 0;
		VkFenceCreateInfo fence_CI {};
		fence_CI.sType						= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_CI.pNext						= nullptr;
		fence_CI.flags						= 0;
		VulkanResultCheck( vkCreateSemaphore( ref_vk_device.object, &sepaphore_CI, VULKAN_ALLOC, &vk_semaphore_stage_1 ) );
		VulkanResultCheck( vkCreateSemaphore( ref_vk_device.object, &sepaphore_CI, VULKAN_ALLOC, &vk_semaphore_stage_2 ) );
		VulkanResultCheck( vkCreateFence( ref_vk_device.object, &fence_CI, VULKAN_ALLOC, &vk_fence_command_buffers_done ) );
	}

	// submit command buffers
	TODO( "Postpone queue submit so that it's batched, to save CPU resources" );
	{
		VkSubmitInfo						submit_info {};
		submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pNext					= nullptr;
		submit_info.waitSemaphoreCount		= 0;
		submit_info.pWaitSemaphores			= nullptr;
		submit_info.pWaitDstStageMask		= nullptr;
		submit_info.commandBufferCount		= 1;
		submit_info.pCommandBuffers			= &vk_primary_transfer_command_buffer;
		submit_info.signalSemaphoreCount	= 1;
		submit_info.pSignalSemaphores		= &vk_semaphore_stage_1;
		LOCK_GUARD( *p_renderer->GetPrimaryTransferQueue().mutex );
		vkQueueSubmit( p_renderer->GetPrimaryTransferQueue().object, 1, &submit_info, VK_NULL_HANDLE );
	}
	{
		VkPipelineStageFlags				dst_stage_mask			= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		VkSubmitInfo						submit_info {};
		submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pNext					= nullptr;
		submit_info.waitSemaphoreCount		= 1;
		submit_info.pWaitSemaphores			= &vk_semaphore_stage_1;
		submit_info.pWaitDstStageMask		= &dst_stage_mask;
		submit_info.commandBufferCount		= 1;
		submit_info.pCommandBuffers			= &vk_secondary_render_command_buffer;
		submit_info.signalSemaphoreCount	= 1;
		submit_info.pSignalSemaphores		= &vk_semaphore_stage_2;
		LOCK_GUARD( *p_renderer->GetSecondaryRenderQueue().mutex );
		vkQueueSubmit( p_renderer->GetSecondaryRenderQueue().object, 1, &submit_info, VK_NULL_HANDLE );
	}
	{
		VkPipelineStageFlags				dst_stage_mask			= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		VkSubmitInfo						submit_info {};
		submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pNext					= nullptr;
		submit_info.waitSemaphoreCount		= 1;
		submit_info.pWaitSemaphores			= &vk_semaphore_stage_2;
		submit_info.pWaitDstStageMask		= &dst_stage_mask;
		submit_info.commandBufferCount		= 1;
		submit_info.pCommandBuffers			= &vk_primary_render_command_buffer;
		submit_info.signalSemaphoreCount	= 0;
		submit_info.pSignalSemaphores		= nullptr;
		LOCK_GUARD( *p_renderer->GetPrimaryRenderQueue().mutex );
		vkQueueSubmit( p_renderer->GetPrimaryRenderQueue().object, 1, &submit_info, vk_fence_command_buffers_done );
	}

	SetNextLoadOperation( ContinueImageLoadTest_1, ContinueImageLoad_1 );
	return DeviceResource::LoadingState::CONTINUE_LOADING;
}

DeviceResource::UnloadingState DeviceResource_Image::Unload()
{
	{
		LOCK_GUARD( *ref_vk_device.mutex );

		vkDestroyFence( ref_vk_device.object, vk_fence_command_buffers_done, VULKAN_ALLOC );
		vkDestroySemaphore( ref_vk_device.object, vk_semaphore_stage_1, VULKAN_ALLOC );
		vkDestroySemaphore( ref_vk_device.object, vk_semaphore_stage_2, VULKAN_ALLOC );
		vk_fence_command_buffers_done		= VK_NULL_HANDLE;
		vk_semaphore_stage_1				= VK_NULL_HANDLE;
		vk_semaphore_stage_2				= VK_NULL_HANDLE;

		vkFreeCommandBuffers( ref_vk_device.object, ref_vk_primary_render_command_pool, 1, &vk_primary_render_command_buffer );
		vkFreeCommandBuffers( ref_vk_device.object, ref_vk_secondary_render_command_pool, 1, &vk_secondary_render_command_buffer );
		vkFreeCommandBuffers( ref_vk_device.object, ref_vk_primary_transfer_command_pool, 1, &vk_primary_transfer_command_buffer );
		vk_primary_render_command_buffer	= VK_NULL_HANDLE;
		vk_secondary_render_command_buffer	= VK_NULL_HANDLE;
		vk_primary_transfer_command_buffer	= VK_NULL_HANDLE;

		vkDestroyBuffer( ref_vk_device.object, vk_staging_buffer, VULKAN_ALLOC );
		vk_staging_buffer					= VK_NULL_HANDLE;

		vkDestroyImageView( ref_vk_device.object, vk_image_view, VULKAN_ALLOC );
		vkDestroyImage( ref_vk_device.object, vk_image, VULKAN_ALLOC );
		vk_image_view						= VK_NULL_HANDLE;
		vk_image							= VK_NULL_HANDLE;
	}
	p_device_memory_manager->FreeMemory( staging_buffer_memory );
	p_device_memory_manager->FreeMemory( image_memory );
	staging_buffer_memory					= {};
	image_memory							= {};

	return DeviceResource::UnloadingState::UNLOADED;
}

ImageData ConvertImageToPhysicalDeviceSupportedFormat( Renderer * renderer, const ImageData & other )
{
	ImageData ret {};
	ret.format				= other.format;
	ret.used_channels		= other.used_channels;
	ret.bits_per_channel	= other.bits_per_channel;
	ret.bytes_per_pixel		= other.bytes_per_pixel;
	ret.width				= other.width;
	ret.height				= other.height;
	ret.has_alpha			= other.has_alpha;

	switch( other.format ) {
	case VK_FORMAT_R8_UNORM:
	{
		if( renderer->IsFormatSupported(
			FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE,
			VK_FORMAT_R8_UNORM,
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR ) ) {
			// use image data directly
			ret.format				= VK_FORMAT_R8_UNORM;
			ret.bytes_per_pixel		= 1;
			ret.image_bytes			= other.image_bytes;
			return ret;

		} else if( renderer->IsFormatSupported(
			FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE,
			VK_FORMAT_R8G8_UNORM,
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR ) ) {
			// add extra channel
			ret.format				= VK_FORMAT_R8G8_UNORM;
			ret.bytes_per_pixel		= 2;

			ret.image_bytes.resize( ret.width * ret.height * ret.bytes_per_pixel );
			for( size_t i=0; i < ret.width * ret.height; ++i ) {
				auto pd		= &ret.image_bytes[ i * 2 ];
				auto ps		= &other.image_bytes[ i * 1 ];
				pd[ 0 ]		= ps[ 0 ];
				pd[ 1 ]		= 255;
			}
			return ret;

		}
		return {};
	}
	case VK_FORMAT_R8G8B8_UNORM:
	{
		if( renderer->IsFormatSupported(
			FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE,
			VK_FORMAT_R8G8B8_UNORM,
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR ) ) {
			// use image data directly
			ret.format				= VK_FORMAT_R8G8B8_UNORM;
			ret.bytes_per_pixel		= 3;
			ret.image_bytes			= other.image_bytes;
			return ret;

		} else if( renderer->IsFormatSupported(
			FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE,
			VK_FORMAT_B8G8R8_UNORM,
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR ) ) {
			// flip components
			ret.format				= VK_FORMAT_B8G8R8_UNORM;
			ret.bytes_per_pixel		= 3;

			ret.image_bytes.resize( ret.width * ret.height * ret.bytes_per_pixel );
			for( size_t i=0; i < ret.width * ret.height; ++i ) {
				auto pd		= &ret.image_bytes[ i * 3 ];
				auto ps		= &other.image_bytes[ i * 3 ];
				pd[ 0 ]		= ps[ 2 ];
				pd[ 1 ]		= ps[ 1 ];
				pd[ 2 ]		= ps[ 0 ];
			}
			return ret;

		} else if( renderer->IsFormatSupported(
			FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR ) ) {
			// add alpha
			ret.format				= VK_FORMAT_R8G8B8A8_UNORM;
			ret.bytes_per_pixel		= 4;

			ret.image_bytes.resize( ret.width * ret.height * ret.bytes_per_pixel );
			for( size_t i=0; i < ret.width * ret.height; ++i ) {
				auto pd		= &ret.image_bytes[ i * 4 ];
				auto ps		= &other.image_bytes[ i * 3 ];
				pd[ 0 ]		= ps[ 0 ];
				pd[ 1 ]		= ps[ 1 ];
				pd[ 2 ]		= ps[ 2 ];
				pd[ 3 ]		= 255;
			}
			return ret;

		} else if( renderer->IsFormatSupported(
			FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE,
			VK_FORMAT_B8G8R8A8_UNORM,
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR ) ) {
			// flip components and add alpha
			ret.format				= VK_FORMAT_B8G8R8A8_UNORM;
			ret.bytes_per_pixel		= 4;

			ret.image_bytes.resize( ret.width * ret.height * ret.bytes_per_pixel );
			for( size_t i=0; i < ret.width * ret.height; ++i ) {
				auto pd		= &ret.image_bytes[ i * 4 ];
				auto ps		= &other.image_bytes[ i * 3 ];
				pd[ 0 ]		= ps[ 2 ];
				pd[ 1 ]		= ps[ 1 ];
				pd[ 2 ]		= ps[ 0 ];
				pd[ 3 ]		= 255;
			}
			return ret;

		}
		break;
	}
	case VK_FORMAT_R8G8_UNORM:
	{
		if( renderer->IsFormatSupported(
			FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE,
			VK_FORMAT_R8G8_UNORM,
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR ) ) {
			// use image data directly
			ret.format				= VK_FORMAT_R8G8_UNORM;
			ret.bytes_per_pixel		= 2;
			ret.image_bytes			= other.image_bytes;
			return ret;
		}
		break;
	}
	case VK_FORMAT_R8G8B8A8_UNORM:
	{
		if( renderer->IsFormatSupported(
			FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR ) ) {
			// use image data directly
			ret.format				= VK_FORMAT_R8G8B8A8_UNORM;
			ret.bytes_per_pixel		= 4;
			ret.image_bytes			= other.image_bytes;
			return ret;

		} else if( renderer->IsFormatSupported(
			FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE,
			VK_FORMAT_B8G8R8A8_UNORM,
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR ) ) {
			// flip color components but keep alpha
			ret.format				= VK_FORMAT_B8G8R8A8_UNORM;
			ret.bytes_per_pixel		= 4;

			ret.image_bytes.resize( ret.width * ret.height * ret.bytes_per_pixel );
			for( size_t i=0; i < ret.width * ret.height; ++i ) {
				auto pd		= &ret.image_bytes[ i * 4 ];
				auto ps		= &other.image_bytes[ i * 4 ];
				pd[ 0 ]		= ps[ 2 ];
				pd[ 1 ]		= ps[ 1 ];
				pd[ 2 ]		= ps[ 0 ];
				pd[ 3 ]		= ps[ 3 ];
			}
			return ret;

		} else {
			return {};
		}
		break;
	}
	case VK_FORMAT_B8G8R8A8_UNORM:
	{
		if( renderer->IsFormatSupported(
			FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE,
			VK_FORMAT_B8G8R8A8_UNORM,
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR ) ) {
			// use image data directly
			ret.format				= VK_FORMAT_B8G8R8A8_UNORM;
			ret.bytes_per_pixel		= 4;
			ret.image_bytes			= other.image_bytes;
			return ret;

		} else if( renderer->IsFormatSupported(
			FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR ) ) {
			// flip color components but keep alpha
			ret.format				= VK_FORMAT_R8G8B8A8_UNORM;
			ret.bytes_per_pixel		= 4;

			ret.image_bytes.resize( ret.width * ret.height * ret.bytes_per_pixel );
			for( size_t i=0; i < ret.width * ret.height; ++i ) {
				auto pd		= &ret.image_bytes[ i * 4 ];
				auto ps		= &other.image_bytes[ i * 4 ];
				pd[ 0 ]		= ps[ 2 ];
				pd[ 1 ]		= ps[ 1 ];
				pd[ 2 ]		= ps[ 0 ];
				pd[ 3 ]		= ps[ 3 ];
			}
			return ret;

		} else {
			return {};
		}
		break;
	}
	default:
		assert( 0 && "Undefined color type" );
		return {};
	}

	return {};
}

}
