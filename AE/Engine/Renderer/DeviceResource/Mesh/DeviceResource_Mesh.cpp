
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
	if( vkGetFenceStatus( r->ref_vk_device.object, r->vk_fence_command_buffers_done ) == VK_SUCCESS ) {
		VulkanResultCheck( vkResetFences( r->ref_vk_device.object, 1, &r->vk_fence_command_buffers_done ) );
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
		vkDestroySemaphore( r->ref_vk_device.object, r->vk_semaphore_stage_1, VULKAN_ALLOC );
		vkDestroyFence( r->ref_vk_device.object, r->vk_fence_command_buffers_done, VULKAN_ALLOC );
		r->vk_semaphore_stage_1					= nullptr;
		r->vk_fence_command_buffers_done		= nullptr;
	}

	// Free command buffers, not needed anymore
	{
		vkFreeCommandBuffers( r->ref_vk_device.object, r->ref_vk_primary_render_command_pool, 1, &r->vk_primary_render_command_buffer );
		vkFreeCommandBuffers( r->ref_vk_device.object, r->ref_vk_primary_transfer_command_pool, 1, &r->vk_primary_transfer_command_buffer );
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

	p_file_mesh_resource			= dynamic_cast<FileResource_Mesh*>( file_resources[ 0 ].Get() );
	if( !p_file_mesh_resource ) {
		assert( 0 && "Can't load mesh, file resource dynamic_cast failed" );
		return DeviceResource::LoadingState::UNABLE_TO_LOAD;
	}

	TODO( "This is an estimate and might be wrong, Create dummy buffers in the beginning of the application to check the real memory requirements for index and vertex buffers" );
	auto reserve_byte_size =
		p_file_mesh_resource->GetVerticesByteSize() +
		p_file_mesh_resource->GetPolygonsByteSize() +
		p_renderer->GetPhysicalDeviceLimits().minUniformBufferOffsetAlignment * 2;

	// create all buffers, both indices and vertices are in one buffer
	{
		{
			TODO( "We may need to sync the two buffers memory size and alignment, more research and testing is required" );
			vk_staging_buffer	= p_device_memory_manager->CreateBuffer( 0, reserve_byte_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT );
			vk_buffer			= p_device_memory_manager->CreateBuffer( 0, reserve_byte_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );

			LOCK_GUARD( *ref_vk_device.mutex );
			VkMemoryRequirements buffer_memory_requirements {};
			vkGetBufferMemoryRequirements( ref_vk_device.object, vk_buffer, &buffer_memory_requirements );

			total_byte_size		= buffer_memory_requirements.size;
			index_offset		= 0;
			vertex_offset		= uint32_t( RoundToAlignment( p_file_mesh_resource->GetPolygonsByteSize(), buffer_memory_requirements.alignment ) );
		}
		staging_buffer_memory	= p_device_memory_manager->AllocateAndBindBufferMemory( vk_staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
		buffer_memory			= p_device_memory_manager->AllocateAndBindBufferMemory( vk_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

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
			VulkanResultCheck( vkMapMemory( ref_vk_device.object, staging_buffer_memory.memory, staging_buffer_memory.offset, staging_buffer_memory.size, 0, (void**)&data ) );
		}
		assert( nullptr != data );
		if( nullptr != data ) {
			std::memcpy( data + index_offset, p_file_mesh_resource->GetPolygons().data(), GetPolygonsByteSize() );
			std::memcpy( data + vertex_offset, p_file_mesh_resource->GetVertices().data(), GetVerticesByteSize() );
			{
				LOCK_GUARD( *ref_vk_device.mutex );
				vkUnmapMemory( ref_vk_device.object, staging_buffer_memory.memory );
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
			command_buffer_AI.commandPool			= ref_vk_primary_transfer_command_pool;
			command_buffer_AI.level					= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			command_buffer_AI.commandBufferCount	= 1;
			VulkanResultCheck( vkAllocateCommandBuffers( ref_vk_device.object, &command_buffer_AI, &vk_primary_transfer_command_buffer ) );
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
			VkCommandBufferBeginInfo command_buffer_BI {};
			command_buffer_BI.sType			= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			command_buffer_BI.pNext			= nullptr;
			command_buffer_BI.flags			= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			VulkanResultCheck( vkBeginCommandBuffer( vk_primary_transfer_command_buffer, &command_buffer_BI ) );

			TODO( "Figure out if we need an exclusive synchronization between host and buffer memory" );
			// Record: Exclusive pipeline barrier between host and device, might not be needed, check implicit synchronization guarantees
			{
				Array<VkBufferMemoryBarrier, 2> buffer_memory_barriers;
				// Staging buffer memory barrier
				buffer_memory_barriers[ 0 ].sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				buffer_memory_barriers[ 0 ].pNext					= nullptr;
				buffer_memory_barriers[ 0 ].srcAccessMask			= VK_ACCESS_HOST_WRITE_BIT;
				buffer_memory_barriers[ 0 ].dstAccessMask			= VK_ACCESS_TRANSFER_READ_BIT;
				buffer_memory_barriers[ 0 ].srcQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
				buffer_memory_barriers[ 0 ].dstQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
				buffer_memory_barriers[ 0 ].buffer					= vk_staging_buffer;
				buffer_memory_barriers[ 0 ].offset					= 0;
				buffer_memory_barriers[ 0 ].size					= staging_buffer_memory.size;

				// Main buffer memory barrier
				buffer_memory_barriers[ 1 ].sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				buffer_memory_barriers[ 1 ].pNext					= nullptr;
				buffer_memory_barriers[ 1 ].srcAccessMask			= 0;
				buffer_memory_barriers[ 1 ].dstAccessMask			= VK_ACCESS_TRANSFER_WRITE_BIT;
				buffer_memory_barriers[ 1 ].srcQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
				buffer_memory_barriers[ 1 ].dstQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
				buffer_memory_barriers[ 1 ].buffer					= vk_buffer;
				buffer_memory_barriers[ 1 ].offset					= 0;
				buffer_memory_barriers[ 1 ].size					= buffer_memory.size;

				vkCmdPipelineBarrier( vk_primary_transfer_command_buffer,
					VK_PIPELINE_STAGE_HOST_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0, nullptr,
					uint32_t( buffer_memory_barriers.size() ), buffer_memory_barriers.data(),
					0, nullptr );
			}

			// Record: Copy staging buffer to actual buffer
			{
				Array<VkBufferCopy, 1> regions;
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

				vkCmdCopyBuffer( vk_primary_transfer_command_buffer,
					vk_staging_buffer,
					vk_buffer,
					uint32_t( regions.size() ), regions.data() );
			}

			TODO( "Figure out if we need this barrier after buffer copy, we don't need this buffer to be accessed in this queue submit" );
			// Record: Pipeline barrier after the copy operation, might not be needed, check implicit synchronization guarantees
			{
				Array<VkBufferMemoryBarrier, 2> buffer_memory_barriers;
				// Staging buffer memory barrier
				buffer_memory_barriers[ 0 ].sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				buffer_memory_barriers[ 0 ].pNext					= nullptr;
				buffer_memory_barriers[ 0 ].srcAccessMask			= VK_ACCESS_TRANSFER_READ_BIT;
				buffer_memory_barriers[ 0 ].dstAccessMask			= VK_ACCESS_HOST_WRITE_BIT;
				buffer_memory_barriers[ 0 ].srcQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
				buffer_memory_barriers[ 0 ].dstQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
				buffer_memory_barriers[ 0 ].buffer					= vk_staging_buffer;
				buffer_memory_barriers[ 0 ].offset					= 0;
				buffer_memory_barriers[ 0 ].size					= staging_buffer_memory.size;

				buffer_memory_barriers[ 1 ].sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				buffer_memory_barriers[ 1 ].pNext					= nullptr;
				buffer_memory_barriers[ 1 ].srcAccessMask			= VK_ACCESS_TRANSFER_WRITE_BIT;
				buffer_memory_barriers[ 1 ].dstAccessMask			= VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				buffer_memory_barriers[ 1 ].srcQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
				buffer_memory_barriers[ 1 ].dstQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
				buffer_memory_barriers[ 1 ].buffer					= vk_buffer;
				buffer_memory_barriers[ 1 ].offset					= 0;
				buffer_memory_barriers[ 1 ].size					= buffer_memory.size;

				vkCmdPipelineBarrier( vk_primary_transfer_command_buffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					0,
					0, nullptr,
					uint32_t( buffer_memory_barriers.size() ), buffer_memory_barriers.data(),
					0, nullptr );
			}

			TODO( "Add a check here to test if the primary transfer queue family index and primary render queue family index are the same, if they are we don't need to release and acquire exclusivity of this buffer" );
			// Record: Release exclusive ownership of both buffers, from now on both buffers will be updated by the primary render queue
			{
				Array<VkBufferMemoryBarrier, 2> buffer_memory_barriers;
				// Staging buffer memory barrier
				buffer_memory_barriers[ 0 ].sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				buffer_memory_barriers[ 0 ].pNext					= nullptr;
				buffer_memory_barriers[ 0 ].srcAccessMask			= VK_ACCESS_HOST_WRITE_BIT;
				buffer_memory_barriers[ 0 ].dstAccessMask			= VK_ACCESS_HOST_WRITE_BIT;
				buffer_memory_barriers[ 0 ].srcQueueFamilyIndex		= p_renderer->GetPrimaryTransferQueueFamilyIndex();
				buffer_memory_barriers[ 0 ].dstQueueFamilyIndex		= p_renderer->GetPrimaryRenderQueueFamilyIndex();
				buffer_memory_barriers[ 0 ].buffer					= vk_staging_buffer;
				buffer_memory_barriers[ 0 ].offset					= 0;
				buffer_memory_barriers[ 0 ].size					= staging_buffer_memory.size;

				buffer_memory_barriers[ 1 ].sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				buffer_memory_barriers[ 1 ].pNext					= nullptr;
				buffer_memory_barriers[ 1 ].srcAccessMask			= VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				buffer_memory_barriers[ 1 ].dstAccessMask			= VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				buffer_memory_barriers[ 1 ].srcQueueFamilyIndex		= p_renderer->GetPrimaryTransferQueueFamilyIndex();
				buffer_memory_barriers[ 1 ].dstQueueFamilyIndex		= p_renderer->GetPrimaryRenderQueueFamilyIndex();
				buffer_memory_barriers[ 1 ].buffer					= vk_buffer;
				buffer_memory_barriers[ 1 ].offset					= 0;
				buffer_memory_barriers[ 1 ].size					= buffer_memory.size;

				vkCmdPipelineBarrier( vk_primary_transfer_command_buffer,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,		// Ignored according to the specification, left here because validation layer complains
					0,
					0, nullptr,
					uint32_t( buffer_memory_barriers.size() ), buffer_memory_barriers.data(),
					0, nullptr );
			}
			VulkanResultCheck( vkEndCommandBuffer( vk_primary_transfer_command_buffer ) );
		}

		TODO( "Add a check here to test if the primary transfer queue family index and primary render queue family index are the same, if they are we don't need to release and acquire exclusivity of this buffer and we don't need to submit anything to the primary render queue" );
		// Primary render command buffer
		{
			VkCommandBufferBeginInfo command_buffer_BI {};
			command_buffer_BI.sType			= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			command_buffer_BI.pNext			= nullptr;
			command_buffer_BI.flags			= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			VulkanResultCheck( vkBeginCommandBuffer( vk_primary_render_command_buffer, &command_buffer_BI ) );

			// Record: < CONTINUE > Acquire exclusive ownership of both buffers, from now on both buffers will be updated by the primary render queue
			{
				Array<VkBufferMemoryBarrier, 2> buffer_memory_barriers;
				// Staging buffer memory barrier
				buffer_memory_barriers[ 0 ].sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				buffer_memory_barriers[ 0 ].pNext					= nullptr;
				buffer_memory_barriers[ 0 ].srcAccessMask			= VK_ACCESS_HOST_WRITE_BIT;
				buffer_memory_barriers[ 0 ].dstAccessMask			= VK_ACCESS_HOST_WRITE_BIT;
				buffer_memory_barriers[ 0 ].srcQueueFamilyIndex		= p_renderer->GetPrimaryTransferQueueFamilyIndex();
				buffer_memory_barriers[ 0 ].dstQueueFamilyIndex		= p_renderer->GetPrimaryRenderQueueFamilyIndex();
				buffer_memory_barriers[ 0 ].buffer					= vk_staging_buffer;
				buffer_memory_barriers[ 0 ].offset					= 0;
				buffer_memory_barriers[ 0 ].size					= staging_buffer_memory.size;

				buffer_memory_barriers[ 1 ].sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				buffer_memory_barriers[ 1 ].pNext					= nullptr;
				buffer_memory_barriers[ 1 ].srcAccessMask			= VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				buffer_memory_barriers[ 1 ].dstAccessMask			= VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				buffer_memory_barriers[ 1 ].srcQueueFamilyIndex		= p_renderer->GetPrimaryTransferQueueFamilyIndex();
				buffer_memory_barriers[ 1 ].dstQueueFamilyIndex		= p_renderer->GetPrimaryRenderQueueFamilyIndex();
				buffer_memory_barriers[ 1 ].buffer					= vk_buffer;
				buffer_memory_barriers[ 1 ].offset					= 0;
				buffer_memory_barriers[ 1 ].size					= buffer_memory.size;

				vkCmdPipelineBarrier( vk_primary_render_command_buffer,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,		// Ignored according to the specification, left here because validation layer complains
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					0,
					0, nullptr,
					uint32_t( buffer_memory_barriers.size() ), buffer_memory_barriers.data(),
					0, nullptr );
			}
			VulkanResultCheck( vkEndCommandBuffer( vk_primary_render_command_buffer ) );
		}

		// Submit command buffers
		{
			// Create synchronization objects
			{
				VkSemaphoreCreateInfo semaphore_CI {};
				semaphore_CI.sType	= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				semaphore_CI.pNext	= nullptr;
				semaphore_CI.flags	= 0;
				VkFenceCreateInfo fence_CI {};
				fence_CI.sType		= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fence_CI.pNext		= nullptr;
				fence_CI.flags		= 0;

				LOCK_GUARD( *ref_vk_device.mutex );
				VulkanResultCheck( vkCreateSemaphore( ref_vk_device.object, &semaphore_CI, VULKAN_ALLOC, &vk_semaphore_stage_1 ) );
				VulkanResultCheck( vkCreateFence( ref_vk_device.object, &fence_CI, VULKAN_ALLOC, &vk_fence_command_buffers_done ) );
			}

			// Submit transfer command buffer
			{
				VkSubmitInfo submit_info {};
				submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submit_info.pNext					= nullptr;
				submit_info.waitSemaphoreCount		= 0;
				submit_info.pWaitSemaphores			= nullptr;
				submit_info.pWaitDstStageMask		= nullptr;
				submit_info.commandBufferCount		= 1;
				submit_info.pCommandBuffers			= &vk_primary_transfer_command_buffer;
				submit_info.signalSemaphoreCount	= 1;
				submit_info.pSignalSemaphores		= &vk_semaphore_stage_1;

				auto queue = p_renderer->GetPrimaryTransferQueue();
				LOCK_GUARD( *queue.mutex );
				VulkanResultCheck( vkQueueSubmit( queue.object, 1, &submit_info, VK_NULL_HANDLE ) );
			}

			// Submit primary render command buffer
			{
				VkPipelineStageFlags dst_stages		= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

				VkSubmitInfo submit_info {};
				submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submit_info.pNext					= nullptr;
				submit_info.waitSemaphoreCount		= 1;
				submit_info.pWaitSemaphores			= &vk_semaphore_stage_1;
				submit_info.pWaitDstStageMask		= &dst_stages;
				submit_info.commandBufferCount		= 1;
				submit_info.pCommandBuffers			= &vk_primary_render_command_buffer;
				submit_info.signalSemaphoreCount	= 0;
				submit_info.pSignalSemaphores		= nullptr;

				auto queue = p_renderer->GetPrimaryRenderQueue();
				LOCK_GUARD( *queue.mutex );
				VulkanResultCheck( vkQueueSubmit( queue.object, 1, &submit_info, vk_fence_command_buffers_done ) );
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
			vkFreeCommandBuffers( ref_vk_device.object, ref_vk_primary_render_command_pool, 1, &vk_primary_render_command_buffer );
			vkFreeCommandBuffers( ref_vk_device.object, ref_vk_primary_transfer_command_pool, 1, &vk_primary_transfer_command_buffer );
			vk_primary_render_command_buffer	= VK_NULL_HANDLE;
			vk_primary_transfer_command_buffer	= VK_NULL_HANDLE;
		}

		// Destroy synchronization objects
		{
			vkDestroySemaphore( ref_vk_device.object, vk_semaphore_stage_1, VULKAN_ALLOC );
			vkDestroyFence( ref_vk_device.object, vk_fence_command_buffers_done, VULKAN_ALLOC );
			vk_semaphore_stage_1				= VK_NULL_HANDLE;
			vk_fence_command_buffers_done		= VK_NULL_HANDLE;
		}

		// Destroy buffers
		{
			vkDestroyBuffer( ref_vk_device.object, vk_staging_buffer, VULKAN_ALLOC );
			vkDestroyBuffer( ref_vk_device.object, vk_buffer, VULKAN_ALLOC );
			vk_staging_buffer	= VK_NULL_HANDLE;
			vk_buffer			= VK_NULL_HANDLE;
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

	return DeviceResource::UnloadingState::UNLOADED;
}

const Vector<Vertex>& DeviceResource_Mesh::GetVertices() const
{
	return p_file_mesh_resource->GetVertices();
}

const Vector<CopyVertex>& DeviceResource_Mesh::GetCopyVertices() const
{
	return p_file_mesh_resource->GetCopyVertices();
}

const Vector<Polygon>& DeviceResource_Mesh::GetPolygons() const
{
	return p_file_mesh_resource->GetPolygons();
}

/*
Vector<Vertex>& DeviceResource_Mesh::GetEditableVertices()
{
	if( GetResourceFlags() == DeviceResource::Flags::STATIC ) {
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
	if( GetResourceFlags() == DeviceResource::Flags::STATIC ) {
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
	if( GetResourceFlags() == DeviceResource::Flags::STATIC ) {
		assert( 0 && "Requested editable polygons on static resource, mesh will not be updated" );
		if( !warned_editable_static_return_polygons ) {
			p_engine->GetLogger()->LogWarning( "DeviceResource_Mesh, requested editable polygons on static resource: " + Debug_GetHexAddressOfThisAsString() + ", mesh will not be updated" );
			warned_editable_static_return_polygons = true;
		}
	}
	return polygons;
}
*/

size_t DeviceResource_Mesh::GetVerticesByteSize() const
{
	return p_file_mesh_resource->GetVerticesByteSize();
}

size_t DeviceResource_Mesh::GetCopyVerticesByteSize() const
{
	return p_file_mesh_resource->GetCopyVerticesByteSize();
}

size_t DeviceResource_Mesh::GetPolygonsByteSize() const
{
	return p_file_mesh_resource->GetPolygonsByteSize();
}

void DeviceResource_Mesh::UpdateVulkanBuffer_Index( const Vector<Polygon>& polygons )
{
	if( GetResourceFlags() & Flags::STATIC ) return;	// We shouldn't update a static device resource, it's already in memory anyways

	if( polygons.size() != p_file_mesh_resource->GetPolygons().size() ) {
		p_logger->LogWarning( "Tried updating mesh vertex data from differently sized array than what is currently in memory" );
		assert( 0 );
	}

	{
		char * data;
		{
			LOCK_GUARD( *ref_vk_device.mutex );
			VulkanResultCheck( vkMapMemory( ref_vk_device.object, staging_buffer_memory.memory, staging_buffer_memory.offset, staging_buffer_memory.size, 0, (void**)&data ) );
		}
		assert( nullptr != data );
		if( nullptr != data ) {
			std::memcpy( data + index_offset, polygons.data(), GetPolygonsByteSize() );
			{
				LOCK_GUARD( *ref_vk_device.mutex );
				vkUnmapMemory( ref_vk_device.object, staging_buffer_memory.memory );
			}
		} else {
			assert( 0 && "Can't map staging buffer memory" );
		}
	}
}

void DeviceResource_Mesh::UpdateVulkanBuffer_Vertex( const Vector<Vertex>& vertices )
{
	if( GetResourceFlags() & Flags::STATIC ) return;	// We shouldn't update a static device resource, it's already in memory anyways

	if( vertices.size() != p_file_mesh_resource->GetVertices().size() ) {
		p_logger->LogWarning( "Tried updating mesh vertex data from differently sized array than what is currently in memory" );
		assert( 0 );
	}

	{
		char * data;
		{
			LOCK_GUARD( *ref_vk_device.mutex );
			VulkanResultCheck( vkMapMemory( ref_vk_device.object, staging_buffer_memory.memory, staging_buffer_memory.offset, staging_buffer_memory.size, 0, (void**)&data ) );
		}
		assert( nullptr != data );
		if( nullptr != data ) {
			std::memcpy( data + vertex_offset, vertices.data(), GetVerticesByteSize() );
			{
				LOCK_GUARD( *ref_vk_device.mutex );
				vkUnmapMemory( ref_vk_device.object, staging_buffer_memory.memory );
			}
		} else {
			assert( 0 && "Can't map staging buffer memory" );
		}
	}
}

void DeviceResource_Mesh::RecordVulkanCommand_TransferToPhysicalDevice( VkCommandBuffer command_buffer, bool transfer_indices )
{
	VkBufferCopy region {};
	if( transfer_indices ) {
		region.srcOffset	= 0;
		region.dstOffset	= 0;
		region.size			= total_byte_size;
	} else {
		region.srcOffset	= vertex_offset;
		region.dstOffset	= vertex_offset;
		region.size			= GetVerticesByteSize();
	}
	vkCmdCopyBuffer( command_buffer,
		vk_staging_buffer,
		vk_buffer,
		1, &region );
}

void DeviceResource_Mesh::RecordVulkanCommand_Render( VkCommandBuffer command_buffer, VkPipelineLayout pipeline_layout )
{
	vkCmdBindIndexBuffer(
		command_buffer,
		vk_buffer,
		index_offset,
		VK_INDEX_TYPE_UINT32 );

	vkCmdBindVertexBuffers(
		command_buffer,
		0, 1,
		&vk_buffer,
		&vertex_offset );

	vkCmdDrawIndexed(
		command_buffer,
		uint32_t( p_file_mesh_resource->GetPolygons().size() * 3 ),
		1, 0, 0, 0 );
}

}
