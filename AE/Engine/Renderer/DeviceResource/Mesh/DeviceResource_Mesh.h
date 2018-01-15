#pragma once

#include "../../../BUILD_OPTIONS.h"
#include "../../../Platform.h"

#include "../../../Vulkan/Vulkan.h"

#include "../DeviceResource.h"
#include "../../../FileResource/Mesh/FileResource_Mesh.h"

namespace AE
{

class Engine;

class DeviceResource_Mesh : public DeviceResource
{
	friend bool ContinueMeshLoadTest_1( DeviceResource * resource );
	friend DeviceResource::LoadingState ContinueMeshLoad_1( DeviceResource * resource );

public:
	DeviceResource_Mesh( Engine * engine, DeviceResource::Flags resource_flags );
	~DeviceResource_Mesh();

	LoadingState						Load();
	UnloadingState						Unload();

	const Vector<Vertex>			&	GetVertices() const;
	const Vector<CopyVertex>		&	GetCopyVertices() const;
	const Vector<Polygon>			&	GetPolygons() const;

	// There isn't a reason why the editable versions should ever be returned, the base model should be kept intact
	/*
	Vector<Vertex>					&	GetEditableVertices();
	Vector<CopyVertex>				&	GetEditableCopyVertices();
	Vector<Polygon>					&	GetEditablePolygons();
	*/

	size_t								GetVerticesByteSize() const;
	size_t								GetCopyVerticesByteSize() const;
	size_t								GetPolygonsByteSize() const;

	void								UpdateVulkanBuffer_Index( const Vector<Polygon> & polygons );
	void								UpdateVulkanBuffer_Vertex( const Vector<Vertex> & vertices );

	void								RecordVulkanCommand_TransferToPhysicalDevice( VkCommandBuffer command_buffer, bool transfer_indices = false );
	void								RecordVulkanCommand_Render( VkCommandBuffer command_buffer, VkPipelineLayout pipeline_layout );

private:
	VkCommandPool						ref_vk_primary_render_command_pool			= VK_NULL_HANDLE;
	VkCommandPool						ref_vk_primary_transfer_command_pool		= VK_NULL_HANDLE;
	VkCommandBuffer						vk_primary_render_command_buffer			= VK_NULL_HANDLE;
	VkCommandBuffer						vk_primary_transfer_command_buffer			= VK_NULL_HANDLE;

	VkSemaphore							vk_semaphore_stage_1						= VK_NULL_HANDLE;
	VkFence								vk_fence_command_buffers_done				= VK_NULL_HANDLE;

	VkBuffer							vk_buffer									= VK_NULL_HANDLE;
	VkBuffer							vk_staging_buffer							= VK_NULL_HANDLE;
	DeviceMemoryInfo					buffer_memory								= {};
	DeviceMemoryInfo					staging_buffer_memory						= {};

	FileResource_Mesh				*	p_file_mesh_resource						= nullptr;

	size_t								index_offset								= 0;
	size_t								vertex_offset								= 0;
	size_t								total_byte_size								= 0;

	bool								warned_editable_static_return_vertices		= false;
	bool								warned_editable_static_return_copy_vertices	= false;
	bool								warned_editable_static_return_polygons		= false;
};

}
