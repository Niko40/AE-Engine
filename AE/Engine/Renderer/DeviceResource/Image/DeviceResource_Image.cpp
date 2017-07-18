
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
	if( r->ref_vk_device.object.getFenceStatus( r->vk_fence_command_buffers_done ) == vk::Result::eSuccess ) {
		r->ref_vk_device.object.resetFences( r->vk_fence_command_buffers_done );
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

		// free command pools
		{
			r->ref_vk_device.object.freeCommandBuffers( r->ref_vk_primary_render_command_pool, r->vk_primary_render_command_buffer );
			r->ref_vk_device.object.freeCommandBuffers( r->ref_vk_secondary_render_command_pool, r->vk_secondary_render_command_buffer );
			r->ref_vk_device.object.freeCommandBuffers( r->ref_vk_primary_transfer_command_pool, r->vk_primary_transfer_command_buffer );
			r->vk_primary_render_command_buffer		= nullptr;
			r->vk_secondary_render_command_buffer	= nullptr;
			r->vk_primary_transfer_command_buffer	= nullptr;
		}

		// destroy synchronization objects, not needed anymore
		{
			r->ref_vk_device.object.destroyFence( r->vk_fence_command_buffers_done );
			r->ref_vk_device.object.destroySemaphore( r->vk_semaphore_stage_1 );
			r->ref_vk_device.object.destroySemaphore( r->vk_semaphore_stage_2 );
			r->vk_fence_command_buffers_done	= nullptr;
			r->vk_semaphore_stage_1				= nullptr;
			r->vk_semaphore_stage_2				= nullptr;
		}

		// destroy staging buffer, not needed anymore
		{
			r->ref_vk_device.object.destroyBuffer( r->vk_staging_buffer );
			r->vk_staging_buffer		= nullptr;
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

	assert( image_resource->IsReadyForUse() );

	auto image_data = ConvertImageToPhysicalDeviceSupportedFormat( p_renderer, image_resource->GetImageData() );
	if( image_data.width == 0 || image_data.height == 0 ) {
		return DeviceResource::LoadingState::UNABLE_TO_LOAD;
	}
	auto ref_vk_device = p_renderer->GetVulkanDevice();

	// Staging buffer creation
	{
		LOCK_GUARD( *ref_vk_device.mutex );

		vk::BufferCreateInfo buffer_CI {};
		buffer_CI.flags					= vk::BufferCreateFlagBits( 0 );
		buffer_CI.size					= uint32_t( image_data.image_bytes.size() );
		buffer_CI.usage					= vk::BufferUsageFlagBits::eTransferSrc;
		buffer_CI.sharingMode			= vk::SharingMode::eExclusive;
		buffer_CI.queueFamilyIndexCount	= 0;
		buffer_CI.pQueueFamilyIndices	= nullptr;
		vk_staging_buffer				= ref_vk_device.object.createBuffer( buffer_CI );
	}
	{
		staging_buffer_memory			= p_device_memory_manager->AllocateAndBindBufferMemory( vk_staging_buffer, vk::MemoryPropertyFlagBits::eHostVisible );
		if( staging_buffer_memory.memory ) {
			void * mapped_memory;
			{
				LOCK_GUARD( *ref_vk_device.mutex );
				mapped_memory			= ref_vk_device.object.mapMemory( staging_buffer_memory.memory, staging_buffer_memory.offset, staging_buffer_memory.size );
			}
			if( nullptr != mapped_memory ) {
				std::memcpy( mapped_memory, image_data.image_bytes.data(), image_data.image_bytes.size() );
				LOCK_GUARD( *ref_vk_device.mutex );
				ref_vk_device.object.unmapMemory( staging_buffer_memory.memory );
			} else {
				LOCK_GUARD( *ref_vk_device.mutex );
				ref_vk_device.object.destroyBuffer( vk_staging_buffer );
				p_device_memory_manager->FreeMemory( staging_buffer_memory );
				vk_staging_buffer		= nullptr;
				p_device_memory_manager	= {};
				return DeviceResource::LoadingState::UNABLE_TO_LOAD;
			}
		}
	}

	// Image creation
	Vector<vk::Extent3D>			mip_levels;
	vk::ImageSubresourceRange		image_sub_resource_range_first_mip_only {};
	vk::ImageSubresourceRange		image_sub_resource_range_complete {};
	image_sub_resource_range_first_mip_only.aspectMask		= vk::ImageAspectFlagBits::eColor;
	image_sub_resource_range_first_mip_only.baseMipLevel	= 0;
	image_sub_resource_range_first_mip_only.levelCount		= 1;
	image_sub_resource_range_first_mip_only.baseArrayLayer	= 0;
	image_sub_resource_range_first_mip_only.layerCount		= 1;

	image_sub_resource_range_complete						= image_sub_resource_range_first_mip_only;	// changed later to cover all mip levels
	{
		mip_levels.push_back( vk::Extent3D( image_data.width, image_data.height, 1 ) );
		{
			uint32_t mwidth				= image_data.width;
			uint32_t mheight			= image_data.height;
			while( mwidth > 1 && mheight > 1 ) {
				mwidth		/= 2;
				mheight		/= 2;
				if( mwidth < 1 )		mwidth		= 1;
				if( mheight < 1 )		mheight		= 1;
				mip_levels.push_back( vk::Extent3D( mwidth, mheight, 1 ) );
			}
		}
		image_sub_resource_range_complete.levelCount			= uint32_t( mip_levels.size() );

		vk::ImageCreateInfo image_CI {};
		image_CI.flags					= vk::ImageCreateFlagBits( 0 );
		image_CI.imageType				= vk::ImageType::e2D;
		image_CI.format					= image_data.format;
		image_CI.extent					= vk::Extent3D( image_data.width, image_data.height, 1 );
		image_CI.mipLevels				= uint32_t( mip_levels.size() );
		image_CI.arrayLayers			= 1;
		image_CI.samples				= vk::SampleCountFlagBits::e1;
		image_CI.tiling					= vk::ImageTiling::eOptimal;
		image_CI.usage					= vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
		image_CI.sharingMode			= vk::SharingMode::eExclusive;		// we use exclusive sharing mode to enable full speed access to the image at render-time
		image_CI.queueFamilyIndexCount	= 0;
		image_CI.pQueueFamilyIndices	= nullptr;
		image_CI.initialLayout			= vk::ImageLayout::eUndefined;
		{
			LOCK_GUARD( *ref_vk_device.mutex );
			vk_image						= ref_vk_device.object.createImage( image_CI );
		}
		image_memory					= p_device_memory_manager->AllocateAndBindImageMemory( vk_image, vk::MemoryPropertyFlagBits::eDeviceLocal );

		vk::ComponentMapping component_mapping {};
		if( image_data.has_alpha ) {
			if( image_data.used_channels == 1 ) {
				assert( 0 && "We can't have only alpha" );
			}
			if( image_data.used_channels == 2 ) {
				component_mapping.a		= vk::ComponentSwizzle::eG;
			}
			if( image_data.used_channels == 3 ) {
				component_mapping.a		= vk::ComponentSwizzle::eB;
			}
			if( image_data.used_channels == 4 ) {
				component_mapping.a		= vk::ComponentSwizzle::eA;
			}
		}
		vk::ImageViewCreateInfo image_view_CI {};
		image_view_CI.flags				= vk::ImageViewCreateFlagBits( 0 );
		image_view_CI.image				= vk_image;
		image_view_CI.viewType			= vk::ImageViewType::e2D;
		image_view_CI.format			= image_data.format;
		image_view_CI.components		= component_mapping;
		image_view_CI.subresourceRange	= image_sub_resource_range_complete;
		{
			LOCK_GUARD( *ref_vk_device.mutex );
			vk_image_view					= ref_vk_device.object.createImageView( image_view_CI );
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
			vk::CommandBufferAllocateInfo command_buffer_AI {};
			command_buffer_AI.commandPool			= ref_vk_primary_render_command_pool;
			command_buffer_AI.level					= vk::CommandBufferLevel::ePrimary;
			command_buffer_AI.commandBufferCount	= 1;
			auto command_buffers					= ref_vk_device.object.allocateCommandBuffers( command_buffer_AI );
			vk_primary_render_command_buffer		= command_buffers[ 0 ];
		}
		{
			vk::CommandBufferAllocateInfo command_buffer_AI {};
			command_buffer_AI.commandPool			= ref_vk_secondary_render_command_pool;
			command_buffer_AI.level					= vk::CommandBufferLevel::ePrimary;
			command_buffer_AI.commandBufferCount	= 1;
			auto command_buffers					= ref_vk_device.object.allocateCommandBuffers( command_buffer_AI );
			vk_secondary_render_command_buffer		= command_buffers[ 0 ];
		}
		{
			vk::CommandBufferAllocateInfo command_buffer_AI {};
			command_buffer_AI.commandPool			= ref_vk_primary_transfer_command_pool;
			command_buffer_AI.level					= vk::CommandBufferLevel::ePrimary;
			command_buffer_AI.commandBufferCount	= 1;
			auto command_buffers					= ref_vk_device.object.allocateCommandBuffers( command_buffer_AI );
			vk_primary_transfer_command_buffer		= command_buffers[ 0 ];
		}
		if( !( vk_primary_render_command_buffer && vk_secondary_render_command_buffer && vk_primary_transfer_command_buffer ) ) {
			return DeviceResource::LoadingState::UNABLE_TO_LOAD;
		}
	}

	// Begin: Transfer command buffer
	{
		vk::CommandBufferBeginInfo command_buffer_BI {};
		command_buffer_BI.flags					= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		vk_primary_transfer_command_buffer.begin( command_buffer_BI );

		// Record: Set buffer to act as a source for transfer and translate image layout from undefined to transfer destination optimal
		{
			vk::BufferMemoryBarrier buffer_memory_barrier {};
			buffer_memory_barrier.srcAccessMask			= vk::AccessFlagBits::eHostWrite;
			buffer_memory_barrier.dstAccessMask			= vk::AccessFlagBits::eTransferRead;
			buffer_memory_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			buffer_memory_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			buffer_memory_barrier.buffer				= vk_staging_buffer;
			buffer_memory_barrier.offset				= 0;
			buffer_memory_barrier.size					= staging_buffer_memory.size;

			vk::ImageMemoryBarrier image_memory_barrier {};
			image_memory_barrier.srcAccessMask			= vk::AccessFlagBits( 0 );
			image_memory_barrier.dstAccessMask			= vk::AccessFlagBits::eTransferWrite;
			image_memory_barrier.oldLayout				= vk::ImageLayout::eUndefined;
			image_memory_barrier.newLayout				= vk::ImageLayout::eTransferDstOptimal;
			image_memory_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			image_memory_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			image_memory_barrier.image					= vk_image;
			image_memory_barrier.subresourceRange		= image_sub_resource_range_complete;
			TODO( "Possible optimization, translate only one mip level at a time before we fill them all out" );

			vk_primary_transfer_command_buffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eHost,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlagBits( 0 ),
				nullptr,
				buffer_memory_barrier,
				image_memory_barrier );
		}

		// Record: Copy buffer to first mip level of the image
		{
			Vector<vk::BufferImageCopy> regions( 1 );
			regions[ 0 ].bufferOffset						= 0;
			regions[ 0 ].bufferRowLength					= 0;
			regions[ 0 ].bufferImageHeight					= 0;
			regions[ 0 ].imageSubresource.aspectMask		= vk::ImageAspectFlagBits::eColor;
			regions[ 0 ].imageSubresource.mipLevel			= 0;
			regions[ 0 ].imageSubresource.baseArrayLayer	= 0;
			regions[ 0 ].imageSubresource.layerCount		= 1;
			regions[ 0 ].imageOffset						= { 0, 0, 0 };
			regions[ 0 ].imageExtent						= { image_data.width, image_data.height, 1 };
			vk_primary_transfer_command_buffer.copyBufferToImage(
				vk_staging_buffer, vk_image,
				vk::ImageLayout::eTransferDstOptimal,
				regions );
		}

		// Record: Translate image layout for blitting
		{
			vk::ImageMemoryBarrier image_memory_barrier {};
			image_memory_barrier.srcAccessMask			= vk::AccessFlagBits::eTransferWrite;
			image_memory_barrier.dstAccessMask			= vk::AccessFlagBits::eTransferRead;
			image_memory_barrier.oldLayout				= vk::ImageLayout::eTransferDstOptimal;
			image_memory_barrier.newLayout				= vk::ImageLayout::eTransferSrcOptimal;
			image_memory_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			image_memory_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			image_memory_barrier.image					= vk_image;
			image_memory_barrier.subresourceRange		= image_sub_resource_range_complete;

			vk_primary_transfer_command_buffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eAllCommands,
				vk::DependencyFlagBits( 0 ),
				nullptr,
				nullptr,
				image_memory_barrier );
		}

		// Record: Release exclusive ownership of the image
		// separated from the previous memory barrier to shut up validation layers,
		// shouldn't be much of a performance impact but might want to fix later
		{
			vk::ImageMemoryBarrier image_memory_barrier {};
			image_memory_barrier.srcAccessMask			= vk::AccessFlagBits::eTransferRead;
			image_memory_barrier.dstAccessMask			= vk::AccessFlagBits::eTransferRead;
			image_memory_barrier.oldLayout				= vk::ImageLayout::eTransferSrcOptimal;
			image_memory_barrier.newLayout				= vk::ImageLayout::eTransferSrcOptimal;
			image_memory_barrier.srcQueueFamilyIndex	= p_renderer->GetPrimaryTransferQueueFamilyIndex();
			image_memory_barrier.dstQueueFamilyIndex	= p_renderer->GetSecondaryRenderQueueFamilyIndex();
			image_memory_barrier.image					= vk_image;
			image_memory_barrier.subresourceRange		= image_sub_resource_range_complete;

			vk_primary_transfer_command_buffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eAllCommands,
				vk::PipelineStageFlagBits::eAllCommands,	// ignored according to specification, we need a semaphore between queue submits
				vk::DependencyFlagBits( 0 ),
				nullptr,
				nullptr,
				image_memory_barrier );
		}

		vk_primary_transfer_command_buffer.end();
	}

	// Begin: secondary render command buffer
	{
		vk::CommandBufferBeginInfo command_buffer_BI {};
		command_buffer_BI.flags					= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		vk_secondary_render_command_buffer.begin( command_buffer_BI );

		// Record: < CONTINUED > Aquire exclusive ownership of the image
		// This pipeline barrier is not executed twice but is required for the completion of the exclusivity transfer
		{
			vk::ImageMemoryBarrier image_memory_barrier {};
			image_memory_barrier.srcAccessMask			= vk::AccessFlagBits::eTransferRead;
			image_memory_barrier.dstAccessMask			= vk::AccessFlagBits::eTransferRead;
			image_memory_barrier.oldLayout				= vk::ImageLayout::eTransferSrcOptimal;
			image_memory_barrier.newLayout				= vk::ImageLayout::eTransferSrcOptimal;
			image_memory_barrier.srcQueueFamilyIndex	= p_renderer->GetPrimaryTransferQueueFamilyIndex();
			image_memory_barrier.dstQueueFamilyIndex	= p_renderer->GetSecondaryRenderQueueFamilyIndex();
			image_memory_barrier.image					= vk_image;
			image_memory_barrier.subresourceRange		= image_sub_resource_range_complete;

			vk_secondary_render_command_buffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eAllCommands,	// ignored according to specification, we need a semaphore between queue submits
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlagBits( 0 ),
				nullptr,
				nullptr,
				image_memory_barrier );
		}

		for( uint32_t i=1; i < mip_levels.size(); ++i ) {
			auto & m		= mip_levels[ i ];
			auto & mprev	= mip_levels[ i - 1 ];

			// Record: Transition current mip level for writing to it
			{
				vk::ImageMemoryBarrier image_memory_barrier {};
				image_memory_barrier.srcAccessMask			= vk::AccessFlagBits( 0 );
				image_memory_barrier.dstAccessMask			= vk::AccessFlagBits::eTransferWrite;
				image_memory_barrier.oldLayout				= vk::ImageLayout::eUndefined;			// we don't care about the existing contents of this mip level
				image_memory_barrier.newLayout				= vk::ImageLayout::eTransferDstOptimal;
				image_memory_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				image_memory_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				image_memory_barrier.image					= vk_image;
				image_memory_barrier.subresourceRange.aspectMask		= vk::ImageAspectFlagBits::eColor;
				image_memory_barrier.subresourceRange.baseMipLevel		= i;
				image_memory_barrier.subresourceRange.levelCount		= 1;
				image_memory_barrier.subresourceRange.baseArrayLayer	= 0;
				image_memory_barrier.subresourceRange.layerCount		= 1;

				vk_secondary_render_command_buffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eAllCommands,
					vk::PipelineStageFlagBits::eTransfer,
					vk::DependencyFlagBits( 0 ),
					nullptr,
					nullptr,
					image_memory_barrier );
			}

			// Record: blit image mip level from previous higher mip level
			{
				vk::ImageBlit region {};

				// Source
				region.srcSubresource.aspectMask	= vk::ImageAspectFlagBits::eColor;
				region.srcSubresource.layerCount	= 1;
				region.srcSubresource.mipLevel		= i - 1;
				region.srcOffsets[ 0 ]				= { 0, 0, 0 };
				region.srcOffsets[ 1 ]				= { int32_t( mprev.width ), int32_t( mprev.height ), int32_t( mprev.depth ) };

				// Destination
				region.dstSubresource.aspectMask	= vk::ImageAspectFlagBits::eColor;
				region.dstSubresource.layerCount	= 1;
				region.dstSubresource.mipLevel		= i;
				region.dstOffsets[ 0 ]				= { 0, 0, 0 };
				region.dstOffsets[ 1 ]				= { int32_t( m.width ), int32_t( m.height ), int32_t( m.depth ) };

				vk_secondary_render_command_buffer.blitImage(
					vk_image, vk::ImageLayout::eTransferSrcOptimal,
					vk_image, vk::ImageLayout::eTransferDstOptimal,
					region,
					vk::Filter::eCubicIMG );
			}

			// Record: Transition current mip level to be the source for the next mip level
			{
				vk::ImageMemoryBarrier image_memory_barrier {};
				image_memory_barrier.srcAccessMask			= vk::AccessFlagBits::eTransferWrite;
				image_memory_barrier.dstAccessMask			= vk::AccessFlagBits::eTransferRead;
				image_memory_barrier.oldLayout				= vk::ImageLayout::eTransferDstOptimal;
				image_memory_barrier.newLayout				= vk::ImageLayout::eTransferSrcOptimal;
				image_memory_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				image_memory_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				image_memory_barrier.image					= vk_image;
				image_memory_barrier.subresourceRange.aspectMask		= vk::ImageAspectFlagBits::eColor;
				image_memory_barrier.subresourceRange.baseMipLevel		= i;
				image_memory_barrier.subresourceRange.levelCount		= 1;
				image_memory_barrier.subresourceRange.baseArrayLayer	= 0;
				image_memory_barrier.subresourceRange.layerCount		= 1;

				vk_secondary_render_command_buffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer,
					vk::PipelineStageFlagBits::eAllCommands,
					vk::DependencyFlagBits( 0 ),
					nullptr,
					nullptr,
					image_memory_barrier );
			}
		}

		// Record: Transition whole image to be used in a shader
		{
			vk::ImageMemoryBarrier image_memory_barrier {};
			image_memory_barrier.srcAccessMask			= vk::AccessFlagBits::eTransferRead;
			image_memory_barrier.dstAccessMask			= vk::AccessFlagBits::eShaderRead;
			image_memory_barrier.oldLayout				= vk::ImageLayout::eTransferSrcOptimal;
			image_memory_barrier.newLayout				= vk::ImageLayout::eShaderReadOnlyOptimal;
			image_memory_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			image_memory_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			image_memory_barrier.image					= vk_image;
			image_memory_barrier.subresourceRange		= image_sub_resource_range_complete;

			vk_secondary_render_command_buffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eAllCommands,
				vk::PipelineStageFlagBits::eAllCommands,
				vk::DependencyFlagBits( 0 ),
				nullptr,
				nullptr,
				image_memory_barrier );
		}

		// Record: Release exclusive ownership of the image
		// separated from the previous memory barrier to shut up validation layers,
		// shouldn't be much of a performance impact but might want to fix later
		{
			vk::ImageMemoryBarrier image_memory_barrier {};
			image_memory_barrier.srcAccessMask			= vk::AccessFlagBits::eShaderRead;
			image_memory_barrier.dstAccessMask			= vk::AccessFlagBits::eShaderRead;
			image_memory_barrier.oldLayout				= vk::ImageLayout::eShaderReadOnlyOptimal;
			image_memory_barrier.newLayout				= vk::ImageLayout::eShaderReadOnlyOptimal;
			image_memory_barrier.srcQueueFamilyIndex	= p_renderer->GetSecondaryRenderQueueFamilyIndex();
			image_memory_barrier.dstQueueFamilyIndex	= p_renderer->GetPrimaryRenderQueueFamilyIndex();
			image_memory_barrier.image					= vk_image;
			image_memory_barrier.subresourceRange		= image_sub_resource_range_complete;

			vk_secondary_render_command_buffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eAllCommands,
				vk::PipelineStageFlagBits::eAllCommands,	// ignored according to specification, we need a semaphore between queue submits
				vk::DependencyFlagBits( 0 ),
				nullptr,
				nullptr,
				image_memory_barrier );
		}

		vk_secondary_render_command_buffer.end();
	}

	// Begin: primary render command buffer
	{
		vk::CommandBufferBeginInfo command_buffer_BI {};
		command_buffer_BI.flags					= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		vk_primary_render_command_buffer.begin( command_buffer_BI );

		// Record: < CONTINUE > Acquire exclusive ownership of the image
		{
			vk::ImageMemoryBarrier image_memory_barrier {};
			image_memory_barrier.srcAccessMask			= vk::AccessFlagBits::eShaderRead;
			image_memory_barrier.dstAccessMask			= vk::AccessFlagBits::eShaderRead;
			image_memory_barrier.oldLayout				= vk::ImageLayout::eShaderReadOnlyOptimal;
			image_memory_barrier.newLayout				= vk::ImageLayout::eShaderReadOnlyOptimal;
			image_memory_barrier.srcQueueFamilyIndex	= p_renderer->GetSecondaryRenderQueueFamilyIndex();
			image_memory_barrier.dstQueueFamilyIndex	= p_renderer->GetPrimaryRenderQueueFamilyIndex();
			image_memory_barrier.image					= vk_image;
			image_memory_barrier.subresourceRange		= image_sub_resource_range_complete;

			vk_primary_render_command_buffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eAllCommands,	// ignored according to specification, we need a semaphore between queue submits
				vk::PipelineStageFlagBits::eAllCommands,
				vk::DependencyFlagBits( 0 ),
				nullptr,
				nullptr,
				image_memory_barrier );
		}

		vk_primary_render_command_buffer.end();
	}

	// Create synchronization objects
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		vk_semaphore_stage_1				= ref_vk_device.object.createSemaphore( vk::SemaphoreCreateInfo() );
		vk_semaphore_stage_2				= ref_vk_device.object.createSemaphore( vk::SemaphoreCreateInfo() );
		vk_fence_command_buffers_done		= ref_vk_device.object.createFence( vk::FenceCreateInfo() );
	}

	// submit command buffers
	TODO( "Postpone queue submit so that it's batched, to save CPU resources" );
	{
		LOCK_GUARD( *p_renderer->GetPrimaryTransferQueue().mutex );
		vk::SubmitInfo						submit_info {};
		submit_info.waitSemaphoreCount		= 0;
		submit_info.pWaitSemaphores			= nullptr;
		submit_info.pWaitDstStageMask		= nullptr;
		submit_info.commandBufferCount		= 1;
		submit_info.pCommandBuffers			= &vk_primary_transfer_command_buffer;
		submit_info.signalSemaphoreCount	= 1;
		submit_info.pSignalSemaphores		= &vk_semaphore_stage_1;
		p_renderer->GetPrimaryTransferQueue().object.submit( submit_info, nullptr );
	}
	{
		LOCK_GUARD( *p_renderer->GetSecondaryRenderQueue().mutex );
		vk::SubmitInfo						submit_info {};
		vk::PipelineStageFlags				dst_stage_mask			= vk::PipelineStageFlagBits::eBottomOfPipe;
		submit_info.waitSemaphoreCount		= 1;
		submit_info.pWaitSemaphores			= &vk_semaphore_stage_1;
		submit_info.pWaitDstStageMask		= &dst_stage_mask;
		submit_info.commandBufferCount		= 1;
		submit_info.pCommandBuffers			= &vk_secondary_render_command_buffer;
		submit_info.signalSemaphoreCount	= 1;
		submit_info.pSignalSemaphores		= &vk_semaphore_stage_2;
		p_renderer->GetSecondaryRenderQueue().object.submit( submit_info, nullptr );
	}
	{
		LOCK_GUARD( *p_renderer->GetPrimaryRenderQueue().mutex );
		vk::SubmitInfo						submit_info {};
		vk::PipelineStageFlags				dst_stage_mask			= vk::PipelineStageFlagBits::eBottomOfPipe;
		submit_info.waitSemaphoreCount		= 1;
		submit_info.pWaitSemaphores			= &vk_semaphore_stage_2;
		submit_info.pWaitDstStageMask		= &dst_stage_mask;
		submit_info.commandBufferCount		= 1;
		submit_info.pCommandBuffers			= &vk_primary_render_command_buffer;
		submit_info.signalSemaphoreCount	= 0;
		submit_info.pSignalSemaphores		= nullptr;
		p_renderer->GetPrimaryRenderQueue().object.submit( submit_info, vk_fence_command_buffers_done );
	}

	SetNextLoadOperation( ContinueImageLoadTest_1, ContinueImageLoad_1 );
	return DeviceResource::LoadingState::CONTINUE_LOADING;
}

DeviceResource::UnloadingState DeviceResource_Image::Unload()
{
	{
		LOCK_GUARD( *ref_vk_device.mutex );

		ref_vk_device.object.destroyFence( vk_fence_command_buffers_done );
		ref_vk_device.object.destroySemaphore( vk_semaphore_stage_1 );
		ref_vk_device.object.destroySemaphore( vk_semaphore_stage_2 );
		vk_fence_command_buffers_done		= nullptr;
		vk_semaphore_stage_1				= nullptr;
		vk_semaphore_stage_2				= nullptr;

		ref_vk_device.object.freeCommandBuffers( ref_vk_primary_render_command_pool, vk_primary_render_command_buffer );
		ref_vk_device.object.freeCommandBuffers( ref_vk_secondary_render_command_pool, vk_secondary_render_command_buffer );
		ref_vk_device.object.freeCommandBuffers( ref_vk_primary_transfer_command_pool, vk_primary_transfer_command_buffer );
		vk_primary_render_command_buffer	= nullptr;
		vk_secondary_render_command_buffer	= nullptr;
		vk_primary_transfer_command_buffer	= nullptr;

		ref_vk_device.object.destroyBuffer( vk_staging_buffer );
		vk_staging_buffer					= nullptr;

		ref_vk_device.object.destroyImageView( vk_image_view );
		ref_vk_device.object.destroyImage( vk_image );
		vk_image_view						= nullptr;
		vk_image							= nullptr;
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
	case vk::Format::eR8Unorm:
	{
		if( renderer->IsFormatSupported(
			vk::ImageTiling::eOptimal,
			vk::Format::eR8Unorm,
			vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eTransferSrcKHR ) ) {
			// use image data directly
			ret.format				= vk::Format::eR8Unorm;
			ret.bytes_per_pixel		= 1;
			ret.image_bytes			= other.image_bytes;
			return ret;

		} else if( renderer->IsFormatSupported(
			vk::ImageTiling::eOptimal,
			vk::Format::eR8G8Unorm,
			vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eTransferSrcKHR ) ) {
			// add extra channel
			ret.format				= vk::Format::eR8G8Unorm;
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
	case vk::Format::eR8G8B8Unorm:
	{
		if( renderer->IsFormatSupported(
			vk::ImageTiling::eOptimal,
			vk::Format::eR8G8B8Unorm,
			vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eTransferSrcKHR ) ) {
			// use image data directly
			ret.format				= vk::Format::eR8G8B8Unorm;
			ret.bytes_per_pixel		= 3;
			ret.image_bytes			= other.image_bytes;
			return ret;

		} else if( renderer->IsFormatSupported(
			vk::ImageTiling::eOptimal,
			vk::Format::eB8G8R8Unorm,
			vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eTransferSrcKHR ) ) {
			// flip components
			ret.format				= vk::Format::eB8G8R8Unorm;
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
			vk::ImageTiling::eOptimal,
			vk::Format::eR8G8B8A8Unorm,
			vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eTransferSrcKHR ) ) {
			// add alpha
			ret.format				= vk::Format::eR8G8B8A8Unorm;
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
			vk::ImageTiling::eOptimal,
			vk::Format::eB8G8R8A8Unorm,
			vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eTransferSrcKHR ) ) {
			// flip components and add alpha
			ret.format				= vk::Format::eB8G8R8A8Unorm;
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
	case vk::Format::eR8G8Unorm:
	{
		if( renderer->IsFormatSupported(
			vk::ImageTiling::eOptimal,
			vk::Format::eR8G8Unorm,
			vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eTransferSrcKHR ) ) {
			// use image data directly
			ret.format				= vk::Format::eR8G8Unorm;
			ret.bytes_per_pixel		= 2;
			ret.image_bytes			= other.image_bytes;
			return ret;
		}
		break;
	}
	case vk::Format::eR8G8B8A8Unorm:
	{
		if( renderer->IsFormatSupported(
			vk::ImageTiling::eOptimal,
			vk::Format::eR8G8B8A8Unorm,
			vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eTransferSrcKHR ) ) {
			// use image data directly
			ret.format				= vk::Format::eR8G8B8A8Unorm;
			ret.bytes_per_pixel		= 4;
			ret.image_bytes			= other.image_bytes;
			return ret;

		} else if( renderer->IsFormatSupported(
			vk::ImageTiling::eOptimal,
			vk::Format::eB8G8R8A8Unorm,
			vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eTransferSrcKHR ) ) {
			// flip color components but keep alpha
			ret.format				= vk::Format::eB8G8R8A8Unorm;
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
	case vk::Format::eB8G8R8A8Unorm:
	{
		if( renderer->IsFormatSupported(
			vk::ImageTiling::eOptimal,
			vk::Format::eB8G8R8A8Unorm,
			vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eTransferSrcKHR ) ) {
			// use image data directly
			ret.format				= vk::Format::eB8G8R8A8Unorm;
			ret.bytes_per_pixel		= 4;
			ret.image_bytes			= other.image_bytes;
			return ret;

		} else if( renderer->IsFormatSupported(
			vk::ImageTiling::eOptimal,
			vk::Format::eR8G8B8A8Unorm,
			vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eTransferSrcKHR ) ) {
			// flip color components but keep alpha
			ret.format				= vk::Format::eR8G8B8A8Unorm;
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
