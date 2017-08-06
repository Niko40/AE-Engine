
#include "UniformBuffer.h"

#include "../../Engine.h"
#include "../../Renderer/Renderer.h"
#include "../../Renderer/DeviceMemory/DeviceMemoryManager.h"

#include <assert.h>

namespace AE
{

UniformBuffer::UniformBuffer( Engine * engine, Renderer * renderer )
{
	p_engine				= engine;
	p_renderer				= renderer;
	assert( p_engine );
	assert( p_renderer );
	p_logger				= p_engine->GetLogger();
	ref_vk_device			= p_renderer->GetVulkanDevice();
}

UniformBuffer::~UniformBuffer()
{
	DeInitialize();
}

void UniformBuffer::Initialize( vk::DeviceSize uniform_buffer_size )
{
	buffer_size						= uniform_buffer_size;
	assert( !buffer_size );

	vk::BufferCreateInfo buffer_CI {};
	buffer_CI.flags					= vk::BufferCreateFlagBits( 0 );
	buffer_CI.size					= buffer_size;
	buffer_CI.sharingMode			= vk::SharingMode::eExclusive;
	buffer_CI.queueFamilyIndexCount	= 0;
	buffer_CI.pQueueFamilyIndices	= nullptr;

	{
		LOCK_GUARD( *ref_vk_device.mutex );
		buffer_CI.usage					= vk::BufferUsageFlagBits::eTransferSrc;
		vk_buffer_host					= ref_vk_device.object.createBuffer( buffer_CI );
		buffer_CI.usage					= vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer;
		vk_buffer_device				= ref_vk_device.object.createBuffer( buffer_CI );
	}

	auto memory_man						= p_renderer->GetDeviceMemoryManager();
	buffer_host_memory					= memory_man->AllocateAndBindBufferMemory( vk_buffer_host, vk::MemoryPropertyFlagBits::eHostVisible );
	buffer_device_memory				= memory_man->AllocateAndBindBufferMemory( vk_buffer_device, vk::MemoryPropertyFlagBits::eDeviceLocal );
}

void UniformBuffer::DeInitialize()
{
	if( buffer_size || vk_buffer_host || vk_buffer_device ) {
		{
			LOCK_GUARD( *ref_vk_device.mutex );
			ref_vk_device.object.destroyBuffer( vk_buffer_host );
			ref_vk_device.object.destroyBuffer( vk_buffer_device );
			vk_buffer_host			= nullptr;
			vk_buffer_device		= nullptr;
		}
		{
			auto memory_man			= p_renderer->GetDeviceMemoryManager();
			memory_man->FreeMemory( buffer_host_memory );
			memory_man->FreeMemory( buffer_device_memory );
			buffer_host_memory		= {};
			buffer_device_memory	= {};
		}
		buffer_size					= 0;
	}
}

void UniformBuffer::CopyDataToHostBuffer( void * data, vk::DeviceSize byte_size )
{
	assert( byte_size <= buffer_host_memory.size );

	LOCK_GUARD( *ref_vk_device.mutex );
	auto mapped_data	= ref_vk_device.object.mapMemory( buffer_host_memory.memory, buffer_host_memory.offset, buffer_host_memory.size );
	if( mapped_data ) {
		std::memcpy( mapped_data, data, std::min( byte_size, buffer_host_memory.size ) );
		ref_vk_device.object.unmapMemory( buffer_host_memory.memory );
	}
}

void UniformBuffer::RecordHostToDeviceBufferCopy( vk::CommandBuffer command_buffer )
{
	assert( command_buffer );

	vk::BufferCopy region {};
	region.srcOffset	= 0;
	region.dstOffset	= 0;
	region.size			= buffer_size;
	command_buffer.copyBuffer( vk_buffer_host, vk_buffer_device, region );
}

}
