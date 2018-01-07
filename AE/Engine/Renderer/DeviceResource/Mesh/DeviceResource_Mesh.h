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

	Vector<Vertex>					&	GetEditableVertices();
	Vector<CopyVertex>				&	GetEditableCopyVertices();
	Vector<Polygon>					&	GetEditablePolygons();

	size_t								GetVerticesByteSize() const;
	size_t								GetCopyVerticesByteSize() const;
	size_t								GetPolygonsByteSize() const;

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

	uint32_t							index_offset								= 0;
	uint32_t							vertex_offset								= 0;

	Vector<Vertex>						vertices;
	Vector<CopyVertex>					copy_vertices;
	Vector<Polygon>						polygons;

	bool								warned_editable_static_return_vertices		= false;
	bool								warned_editable_static_return_copy_vertices	= false;
	bool								warned_editable_static_return_polygons		= false;
};

}
