#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Vulkan/Vulkan.h"
#include "../../Renderer/DeviceMemory/DeviceMemoryInfo.h"

namespace AE
{

class Engine;
class Logger;
class Renderer;

class UniformBuffer
{
public:
	UniformBuffer( Engine * engine, Renderer * renderer );
	~UniformBuffer();

	void						Initialize( VkDeviceSize uniform_buffer_size );
	void						DeInitialize();

	void						CopyDataToHostBuffer( const void * data, VkDeviceSize byte_size );
	void						RecordHostToDeviceBufferCopy( VkCommandBuffer command_buffer );

	VkBuffer					GetHostBuffer() const;
	VkBuffer					GetDeviceBuffer() const;

private:
	Engine					*	p_engine						= nullptr;
	Logger					*	p_logger						= nullptr;
	Renderer				*	p_renderer						= nullptr;
	VulkanDevice				ref_vk_device					= {};

	VkBuffer					vk_buffer_host					= VK_NULL_HANDLE;
	VkBuffer					vk_buffer_device				= VK_NULL_HANDLE;
	DeviceMemoryInfo			buffer_host_memory				= {};
	DeviceMemoryInfo			buffer_device_memory			= {};

	VkDeviceSize				buffer_size						= 0;
};

}
