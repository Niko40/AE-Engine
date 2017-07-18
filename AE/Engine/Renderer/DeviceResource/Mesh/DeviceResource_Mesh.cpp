
#include "DeviceResource_Mesh.h"

#include "../../../Engine.h"
#include "../../../Logger/Logger.h"
#include "../../Renderer.h"
#include "../../DeviceMemory/DeviceMemoryManager.h"
#include "../../DeviceResource/DeviceResourceManager.h"
#include "../../../FileResource/Mesh/FileResource_Mesh.h"
#include "../../../Math/Math.h"

namespace AE
{

DeviceResource_Mesh::DeviceResource_Mesh( Engine * engine, DeviceResource::Flags resource_flags )
	: DeviceResource( engine, DeviceResource::Type::MESH, resource_flags )
{
}

DeviceResource_Mesh::~DeviceResource_Mesh()
{
	Unload();
}

bool ContinueMeshLoadTest_1( DeviceResource * resource )
{
	assert( nullptr != resource );
	auto r		= dynamic_cast<DeviceResource_Mesh*>( resource );

	LOCK_GUARD( *r->ref_vk_device.mutex );
	if( r->ref_vk_device.object.getFenceStatus( r->vk_fence_command_buffers_done ) == vk::Result::eSuccess ) {
		r->ref_vk_device.object.resetFences( r->vk_fence_command_buffers_done );
		return true;
	}
	return false;
}

DeviceResource::LoadingState ContinueMeshLoad_1( DeviceResource * resource )
{
	assert( nullptr != resource );
	auto r		= dynamic_cast<DeviceResource_Mesh*>( resource );

	LOCK_GUARD( *r->ref_vk_device.mutex );

	// Destroy synchronization objects, not needed anymore
	{
//		r->ref_vk_device.resetFences( r->vk_fence_command_buffers_done );
		r->ref_vk_device.object.destroySemaphore( r->vk_semaphore_stage_1 );
		r->ref_vk_device.object.destroyFence( r->vk_fence_command_buffers_done );
		r->vk_semaphore_stage_1					= nullptr;
		r->vk_fence_command_buffers_done		= nullptr;
	}

	// Free command buffers, not needed anymore
	{
		r->ref_vk_device.object.freeCommandBuffers( r->ref_vk_primary_render_command_pool, r->vk_primary_render_command_buffer );
		r->ref_vk_device.object.freeCommandBuffers( r->ref_vk_primary_transfer_command_pool, r->vk_primary_transfer_command_buffer );
		r->vk_primary_render_command_buffer		= nullptr;
		r->vk_primary_transfer_command_buffer	= nullptr;
	}

	return DeviceResource::LoadingState::LOADED;
}

DeviceResource::LoadingState DeviceResource_Mesh::Load()
{
	if( file_resources.size() != 1 ) {
		assert( 0 && "Can't load mesh, file resource count doesn't match requirements" );
		return DeviceResource::LoadingState::UNABLE_TO_LOAD;
	}

	if( file_resources[ 0 ]->GetResourceType() != FileResource::Type::MESH ) {
		assert( 0 && "Can't load mesh, file resource type is not MESH" );
		return DeviceResource::LoadingState::UNABLE_TO_LOAD;
	}

	auto mesh_resource			= dynamic_cast<FileResource_Mesh*>( file_resources[ 0 ].Get() );
	if( !mesh_resource ) {
		assert( 0 && "Can't load mesh, file resource dynamic_cast failed" );
		return DeviceResource::LoadingState::UNABLE_TO_LOAD;
	}

	vertices		= mesh_resource->GetVertices();
	copy_vertices	= mesh_resource->GetCopyVertices();
	polygons		= mesh_resource->GetPolygons();

	auto total_device_memory_reserve_size	=
		mesh_resource->GetVerticesByteSize() +
		mesh_resource->GetPolygonsByteSize() +
		p_renderer->GetPhysicalDeviceLimits().minUniformBufferOffsetAlignment * 2;

	// create all buffers, both indices and vertices are in one buffer
	{
		{
			LOCK_GUARD( *ref_vk_device.mutex );

			vk::BufferCreateInfo	buffer_CI {};
			buffer_CI.flags					= vk::BufferCreateFlagBits( 0 );
			buffer_CI.size					= total_device_memory_reserve_size;
			buffer_CI.usage					= vk::BufferUsageFlagBits::eTransferSrc;
			buffer_CI.sharingMode			= vk::SharingMode::eExclusive;
			buffer_CI.queueFamilyIndexCount	= 0;
			buffer_CI.pQueueFamilyIndices	= nullptr;
			vk_staging_buffer				= ref_vk_device.object.createBuffer( buffer_CI );

			buffer_CI.usage					= vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer;
			vk_buffer						= ref_vk_device.object.createBuffer( buffer_CI );

			index_offset					= 0;
			vertex_offset					= uint32_t( RoundToAlignment( mesh_resource->GetPolygonsByteSize(), ref_vk_device.object.getBufferMemoryRequirements( vk_buffer ).alignment ) );
		}
		staging_buffer_memory				= p_device_memory_manager->AllocateAndBindBufferMemory( vk_staging_buffer, vk::MemoryPropertyFlagBits::eHostVisible );
		buffer_memory						= p_device_memory_manager->AllocateAndBindBufferMemory( vk_buffer, vk::MemoryPropertyFlagBits::eDeviceLocal );

		if( !( staging_buffer_memory.memory && buffer_memory.memory ) ) {
			assert( 0 && "Can't load mesh, can't allocate memory" );
			return DeviceResource::LoadingState::UNABLE_TO_LOAD;
		}

		assert( staging_buffer_memory.size == buffer_memory.size );
	}

	// copy the contents to the staging buffer
	{
		char * data;
		{
			LOCK_GUARD( *ref_vk_device.mutex );
			data		= reinterpret_cast<char*>( ref_vk_device.object.mapMemory( staging_buffer_memory.memory, staging_buffer_memory.offset, staging_buffer_memory.size ) );
		}
		assert( nullptr != data );
		if( nullptr != data ) {
			std::memcpy( data + index_offset, polygons.data(), GetPolygonsByteSize() );
			std::memcpy( data + vertex_offset, vertices.data(), GetVerticesByteSize() );
			{
				LOCK_GUARD( *ref_vk_device.mutex );
				ref_vk_device.object.unmapMemory( staging_buffer_memory.memory );
			}
		} else {
			assert( 0 && "Can't load mesh, can't map staging buffer memory" );
			return DeviceResource::LoadingState::UNABLE_TO_LOAD;
		}
	}

	// allocate command buffers
	{
		ref_vk_primary_render_command_pool		= p_device_resource_manager->GetPrimaryRenderCommandPoolForThisThread();
		ref_vk_primary_transfer_command_pool	= p_device_resource_manager->GetPrimaryTransferCommandPoolForThisThread();

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
			command_buffer_AI.commandPool			= ref_vk_primary_transfer_command_pool;
			command_buffer_AI.level					= vk::CommandBufferLevel::ePrimary;
			command_buffer_AI.commandBufferCount	= 1;
			auto command_buffers					= ref_vk_device.object.allocateCommandBuffers( command_buffer_AI );
			vk_primary_transfer_command_buffer		= command_buffers[ 0 ];
		}

		if( !( vk_primary_render_command_buffer && vk_primary_transfer_command_buffer ) ) {
			assert( 0 && "Can't load mesh, command buffer allocation failed" );
			return DeviceResource::LoadingState::UNABLE_TO_LOAD;
		}
	}

	// Record and submit command buffers
	{
		// Transfer command buffer
		{
			vk::CommandBufferBeginInfo command_buffer_BI {};
			command_buffer_BI.flags			= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			vk_primary_transfer_command_buffer.begin( command_buffer_BI );

			TODO( "Figure out if we need an exclusive synchronization between host and buffer memory" );
			// Record: Exclusive pipeline barrier between host and device, might not be needed, check implicit synchronization guarantees
			{
				vk::BufferMemoryBarrier staging_buffer_memory_barrier {};
				staging_buffer_memory_barrier.srcAccessMask			= vk::AccessFlagBits( 0 );
				staging_buffer_memory_barrier.dstAccessMask			= vk::AccessFlagBits::eTransferWrite;
				staging_buffer_memory_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				staging_buffer_memory_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				staging_buffer_memory_barrier.buffer				= vk_buffer;
				staging_buffer_memory_barrier.offset				= 0;
				staging_buffer_memory_barrier.size					= buffer_memory.size;

				vk::BufferMemoryBarrier buffer_memory_barrier {};
				buffer_memory_barrier.srcAccessMask					= vk::AccessFlagBits::eHostWrite;
				buffer_memory_barrier.dstAccessMask					= vk::AccessFlagBits::eTransferRead;
				buffer_memory_barrier.srcQueueFamilyIndex			= VK_QUEUE_FAMILY_IGNORED;
				buffer_memory_barrier.dstQueueFamilyIndex			= VK_QUEUE_FAMILY_IGNORED;
				buffer_memory_barrier.buffer						= vk_staging_buffer;
				buffer_memory_barrier.offset						= 0;
				buffer_memory_barrier.size							= staging_buffer_memory.size;

				vk_primary_transfer_command_buffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eHost,
					vk::PipelineStageFlagBits::eTransfer,
					vk::DependencyFlagBits( 0 ),
					nullptr,
					{ staging_buffer_memory_barrier, buffer_memory_barrier },
					nullptr );
			}

			// Record: Copy staging buffer to actual buffer
			{
				Array<vk::BufferCopy, 1> regions;
				/*
				regions[ 0 ].srcOffset	= index_offset;
				regions[ 0 ].dstOffset	= index_offset;
				regions[ 0 ].size		= GetPolygonsByteSize();

				regions[ 1 ].srcOffset	= vertex_offset;
				regions[ 1 ].dstOffset	= vertex_offset;
				regions[ 1 ].size		= GetVerticesByteSize();
				*/
				regions[ 0 ].srcOffset	= 0;
				regions[ 0 ].dstOffset	= 0;
				regions[ 0 ].size		= std::min( staging_buffer_memory.size, buffer_memory.size );
				vk_primary_transfer_command_buffer.copyBuffer(
					vk_staging_buffer,
					vk_buffer,
					regions );
			}

			TODO( "Figure out if we need this barrier after buffer copy, we don't need this buffer to be accessed in this queue submit" );
			// Record: Pipeline barrier after the copy operation, might not be needed, check implicit synchronization guarantees
			{
				vk::BufferMemoryBarrier staging_buffer_memory_barrier {};
				staging_buffer_memory_barrier.srcAccessMask			= vk::AccessFlagBits::eTransferRead;
				staging_buffer_memory_barrier.dstAccessMask			= vk::AccessFlagBits::eHostWrite;
				staging_buffer_memory_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				staging_buffer_memory_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				staging_buffer_memory_barrier.buffer				= vk_staging_buffer;
				staging_buffer_memory_barrier.offset				= 0;
				staging_buffer_memory_barrier.size					= staging_buffer_memory.size;

				vk::BufferMemoryBarrier buffer_memory_barrier {};
				buffer_memory_barrier.srcAccessMask					= vk::AccessFlagBits::eTransferWrite;
				buffer_memory_barrier.dstAccessMask					= vk::AccessFlagBits::eIndexRead | vk::AccessFlagBits::eVertexAttributeRead;
				buffer_memory_barrier.srcQueueFamilyIndex			= VK_QUEUE_FAMILY_IGNORED;
				buffer_memory_barrier.dstQueueFamilyIndex			= VK_QUEUE_FAMILY_IGNORED;
				buffer_memory_barrier.buffer						= vk_buffer;
				buffer_memory_barrier.offset						= 0;
				buffer_memory_barrier.size							= buffer_memory.size;

				vk_primary_transfer_command_buffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer,
					vk::PipelineStageFlagBits::eAllCommands,
					vk::DependencyFlagBits( 0 ),
					nullptr,
					{ staging_buffer_memory_barrier, buffer_memory_barrier },
					nullptr );
			}

			TODO( "Add a check here to test if the primary transfer queue family index and primary render queue family index are the same, if they are we don't need to release and acquire exclusivity of this buffer" );
			// Record: Release exclusive ownership of both buffers, from now on both buffers will be updated by the primary render queue
			{
				vk::BufferMemoryBarrier staging_buffer_memory_barrier {};
				staging_buffer_memory_barrier.srcAccessMask			= vk::AccessFlagBits::eHostWrite;
				staging_buffer_memory_barrier.dstAccessMask			= vk::AccessFlagBits::eHostWrite;
				staging_buffer_memory_barrier.srcQueueFamilyIndex	= p_renderer->GetPrimaryTransferQueueFamilyIndex();
				staging_buffer_memory_barrier.dstQueueFamilyIndex	= p_renderer->GetPrimaryRenderQueueFamilyIndex();
				staging_buffer_memory_barrier.buffer				= vk_staging_buffer;
				staging_buffer_memory_barrier.offset				= 0;
				staging_buffer_memory_barrier.size					= staging_buffer_memory.size;

				vk::BufferMemoryBarrier buffer_memory_barrier {};
				buffer_memory_barrier.srcAccessMask					= vk::AccessFlagBits::eIndexRead | vk::AccessFlagBits::eVertexAttributeRead;
				buffer_memory_barrier.dstAccessMask					= vk::AccessFlagBits::eIndexRead | vk::AccessFlagBits::eVertexAttributeRead;
				buffer_memory_barrier.srcQueueFamilyIndex			= p_renderer->GetPrimaryTransferQueueFamilyIndex();
				buffer_memory_barrier.dstQueueFamilyIndex			= p_renderer->GetPrimaryRenderQueueFamilyIndex();
				buffer_memory_barrier.buffer						= vk_buffer;
				buffer_memory_barrier.offset						= 0;
				buffer_memory_barrier.size							= buffer_memory.size;

				vk_primary_transfer_command_buffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eAllCommands,
					vk::PipelineStageFlagBits::eAllCommands,	// Ignored according to the specification, left here because validation layers complain
					vk::DependencyFlagBits( 0 ),
					nullptr,
					{ staging_buffer_memory_barrier, buffer_memory_barrier },
					nullptr );
			}

			vk_primary_transfer_command_buffer.end();
		}

		TODO( "Add a check here to test if the primary transfer queue family index and primary render queue family index are the same, if they are we don't need to release and acquire exclusivity of this buffer and we don't need to submit anything to the primary render queue" );
		// Primary render command buffer
		{
			vk::CommandBufferBeginInfo command_buffer_BI {};
			command_buffer_BI.flags			= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			vk_primary_render_command_buffer.begin( command_buffer_BI );

			// Record: < CONTINUE > Acquire exclusive ownership of both buffers, from now on both buffers will be updated by the primary render queue
			{
				vk::BufferMemoryBarrier staging_buffer_memory_barrier {};
				staging_buffer_memory_barrier.srcAccessMask			= vk::AccessFlagBits::eHostWrite;
				staging_buffer_memory_barrier.dstAccessMask			= vk::AccessFlagBits::eHostWrite;
				staging_buffer_memory_barrier.srcQueueFamilyIndex	= p_renderer->GetPrimaryTransferQueueFamilyIndex();
				staging_buffer_memory_barrier.dstQueueFamilyIndex	= p_renderer->GetPrimaryRenderQueueFamilyIndex();
				staging_buffer_memory_barrier.buffer				= vk_staging_buffer;
				staging_buffer_memory_barrier.offset				= 0;
				staging_buffer_memory_barrier.size					= staging_buffer_memory.size;

				vk::BufferMemoryBarrier buffer_memory_barrier {};
				buffer_memory_barrier.srcAccessMask					= vk::AccessFlagBits::eIndexRead | vk::AccessFlagBits::eVertexAttributeRead;
				buffer_memory_barrier.dstAccessMask					= vk::AccessFlagBits::eIndexRead | vk::AccessFlagBits::eVertexAttributeRead;
				buffer_memory_barrier.srcQueueFamilyIndex			= p_renderer->GetPrimaryTransferQueueFamilyIndex();
				buffer_memory_barrier.dstQueueFamilyIndex			= p_renderer->GetPrimaryRenderQueueFamilyIndex();
				buffer_memory_barrier.buffer						= vk_buffer;
				buffer_memory_barrier.offset						= 0;
				buffer_memory_barrier.size							= buffer_memory.size;

				vk_primary_render_command_buffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eAllCommands,	// Ignored according to the specification, left here because validation layers complain
					vk::PipelineStageFlagBits::eAllCommands,
					vk::DependencyFlagBits( 0 ),
					nullptr,
					{ staging_buffer_memory_barrier, buffer_memory_barrier },
					nullptr );
			}

			vk_primary_render_command_buffer.end();
		}

		// Submit command buffers
		{
			// Create synchronization objects
			{
				LOCK_GUARD( *ref_vk_device.mutex );
				vk_semaphore_stage_1				= ref_vk_device.object.createSemaphore( vk::SemaphoreCreateInfo() );
				vk_fence_command_buffers_done		= ref_vk_device.object.createFence( vk::FenceCreateInfo() );
			}

			// Submit transfer command buffer
			{
				LOCK_GUARD( *p_renderer->GetPrimaryTransferQueue().mutex );
				vk::SubmitInfo submit_info {};
				submit_info.waitSemaphoreCount		= 0;
				submit_info.pWaitSemaphores			= nullptr;
				submit_info.pWaitDstStageMask		= nullptr;
				submit_info.commandBufferCount		= 1;
				submit_info.pCommandBuffers			= &vk_primary_transfer_command_buffer;
				submit_info.signalSemaphoreCount	= 1;
				submit_info.pSignalSemaphores		= &vk_semaphore_stage_1;
				p_renderer->GetPrimaryTransferQueue().object.submit( submit_info, nullptr );
			}

			// Submit primary render command buffer
			{
				LOCK_GUARD( *p_renderer->GetPrimaryRenderQueue().mutex );
				vk::SubmitInfo submit_info {};
				vk::PipelineStageFlags dst_stages	= vk::PipelineStageFlagBits::eAllCommands;
				submit_info.waitSemaphoreCount		= 1;
				submit_info.pWaitSemaphores			= &vk_semaphore_stage_1;
				submit_info.pWaitDstStageMask		= &dst_stages;
				submit_info.commandBufferCount		= 1;
				submit_info.pCommandBuffers			= &vk_primary_render_command_buffer;
				submit_info.signalSemaphoreCount	= 0;
				submit_info.pSignalSemaphores		= nullptr;
				p_renderer->GetPrimaryRenderQueue().object.submit( submit_info, vk_fence_command_buffers_done );
			}
		}
	}

	SetNextLoadOperation( ContinueMeshLoadTest_1, ContinueMeshLoad_1 );
	return DeviceResource::LoadingState::CONTINUE_LOADING;
}

DeviceResource::UnloadingState DeviceResource_Mesh::Unload()
{
	{
		LOCK_GUARD( *ref_vk_device.mutex );

		// Free command buffers
		{
			ref_vk_device.object.freeCommandBuffers( ref_vk_primary_render_command_pool, vk_primary_render_command_buffer );
			ref_vk_device.object.freeCommandBuffers( ref_vk_primary_transfer_command_pool, vk_primary_transfer_command_buffer );
			vk_primary_render_command_buffer	= nullptr;
			vk_primary_transfer_command_buffer	= nullptr;
		}

		// Destroy synchronization objects
		{
			//		ref_vk_device.resetFences( vk_fence_command_buffers_done );
			ref_vk_device.object.destroySemaphore( vk_semaphore_stage_1 );
			ref_vk_device.object.destroyFence( vk_fence_command_buffers_done );
			vk_semaphore_stage_1				= nullptr;
			vk_fence_command_buffers_done		= nullptr;
		}

		// Destroy buffers
		{
			ref_vk_device.object.destroyBuffer( vk_staging_buffer );
			ref_vk_device.object.destroyBuffer( vk_buffer );
			vk_staging_buffer	= nullptr;
			vk_buffer			= nullptr;
		}
	}

	// free memory
	{
		p_device_memory_manager->FreeMemory( staging_buffer_memory );
		p_device_memory_manager->FreeMemory( buffer_memory );
		staging_buffer_memory	= {};
		buffer_memory			= {};
	}

	// clear other values
	{
		index_offset		= 0;
		vertex_offset		= 0;
		warned_editable_static_return_vertices		= false;
		warned_editable_static_return_copy_vertices	= false;
		warned_editable_static_return_polygons		= false;
	}

	// clear local copy of the mesh
	{
		vertices.clear();
		copy_vertices.clear();
		polygons.clear();
	}

	return DeviceResource::UnloadingState::UNLOADED;
}

const Vector<Vertex>& DeviceResource_Mesh::GetVertices() const
{
	return vertices;
}

const Vector<CopyVertex>& DeviceResource_Mesh::GetCopyVertices() const
{
	return copy_vertices;
}

const Vector<Polygon>& DeviceResource_Mesh::GetPolygons() const
{
	return polygons;
}

Vector<Vertex>& DeviceResource_Mesh::GetEditableVertices()
{
	if( GetFlags() == DeviceResource::Flags::STATIC ) {
		assert( 0 && "Requested editable vertices on static resource, mesh will not be updated" );
		if( !warned_editable_static_return_vertices ) {
			p_engine->GetLogger()->LogWarning( "DeviceResource_Mesh, requested editable vertices on static resource: " + Debug_GetHexAddressOfThisAsString() + ", mesh will not be updated" );
			warned_editable_static_return_vertices = true;
		}
	}
	return vertices;
}

Vector<CopyVertex>& DeviceResource_Mesh::GetEditableCopyVertices()
{
	if( GetFlags() == DeviceResource::Flags::STATIC ) {
		assert( 0 && "Requested editable copy vertices on static resource, mesh will not be updated" );
		if( !warned_editable_static_return_copy_vertices ) {
			p_engine->GetLogger()->LogWarning( "DeviceResource_Mesh, requested editable copy vertices on static resource: " + Debug_GetHexAddressOfThisAsString() + ", mesh will not be updated" );
			warned_editable_static_return_copy_vertices = true;
		}
	}
	return copy_vertices;
}

Vector<Polygon>& DeviceResource_Mesh::GetEditablePolygons()
{
	if( GetFlags() == DeviceResource::Flags::STATIC ) {
		assert( 0 && "Requested editable polygons on static resource, mesh will not be updated" );
		if( !warned_editable_static_return_polygons ) {
			p_engine->GetLogger()->LogWarning( "DeviceResource_Mesh, requested editable polygons on static resource: " + Debug_GetHexAddressOfThisAsString() + ", mesh will not be updated" );
			warned_editable_static_return_polygons = true;
		}
	}
	return polygons;
}

size_t DeviceResource_Mesh::GetVerticesByteSize() const
{
	return vertices.size() * sizeof( Vertex );
}

size_t DeviceResource_Mesh::GetCopyVerticesByteSize() const
{
	return copy_vertices.size() * sizeof( CopyVertex );
}

size_t DeviceResource_Mesh::GetPolygonsByteSize() const
{
	return polygons.size() * sizeof( Polygon );
}

}
