#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Vulkan/Vulkan.h"

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

	void						Initialize( vk::DeviceSize uniform_buffer_size );
	void						DeInitialize();

	void						CopyDataToHostBuffer( void * data, vk::DeviceSize byte_size );
	void						RecordHostToDeviceBufferCopy( vk::CommandBuffer command_buffer );

private:
	Engine					*	p_engine						= nullptr;
	Logger					*	p_logger						= nullptr;
	Renderer				*	p_renderer						= nullptr;
	VulkanDevice				ref_vk_device					= {};

	vk::Buffer					vk_buffer_host					= nullptr;
	vk::Buffer					vk_buffer_device				= nullptr;
	DeviceMemoryInfo			buffer_host_memory				= {};
	DeviceMemoryInfo			buffer_device_memory			= {};

	vk::DeviceSize				buffer_size						= 0;
};

}
