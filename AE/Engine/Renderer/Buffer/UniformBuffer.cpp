
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

void UniformBuffer::Initialize( VkDeviceSize uniform_buffer_size )
{
	buffer_size						= uniform_buffer_size;
	assert( buffer_size );

	VkBufferCreateInfo buffer_CI {};
	buffer_CI.sType						= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_CI.pNext						= nullptr;
	buffer_CI.flags						= 0;
	buffer_CI.size						= buffer_size;
	buffer_CI.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	buffer_CI.queueFamilyIndexCount		= 0;
	buffer_CI.pQueueFamilyIndices		= nullptr;

	{
		LOCK_GUARD( *ref_vk_device.mutex );
		buffer_CI.usage					= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VulkanResultCheck( vkCreateBuffer( ref_vk_device.object, &buffer_CI, VULKAN_ALLOC, &vk_buffer_host ) );
		assert( vk_buffer_host );
		buffer_CI.usage					= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		VulkanResultCheck( vkCreateBuffer( ref_vk_device.object, &buffer_CI, VULKAN_ALLOC, &vk_buffer_device ) );
		assert( vk_buffer_device );
	}

	auto memory_man						= p_renderer->GetDeviceMemoryManager();
	buffer_host_memory					= memory_man->AllocateAndBindBufferMemory( vk_buffer_host, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
	buffer_device_memory				= memory_man->AllocateAndBindBufferMemory( vk_buffer_device, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
}

void UniformBuffer::DeInitialize()
{
	if( buffer_size || vk_buffer_host || vk_buffer_device ) {
		{
			LOCK_GUARD( *ref_vk_device.mutex );
			vkDestroyBuffer( ref_vk_device.object, vk_buffer_host, VULKAN_ALLOC );
			vkDestroyBuffer( ref_vk_device.object, vk_buffer_device, VULKAN_ALLOC );
			vk_buffer_host			= VK_NULL_HANDLE;
			vk_buffer_device		= VK_NULL_HANDLE;
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

void UniformBuffer::CopyDataToHostBuffer( void * data, VkDeviceSize byte_size )
{
	assert( byte_size <= buffer_host_memory.size );

	LOCK_GUARD( *ref_vk_device.mutex );
	void * mapped_data = nullptr;
	VulkanResultCheck( vkMapMemory( ref_vk_device.object, buffer_host_memory.memory, buffer_host_memory.offset, buffer_host_memory.size, 0, &mapped_data ) );
	if( mapped_data ) {
		std::memcpy( mapped_data, data, std::min( byte_size, buffer_host_memory.size ) );
		vkUnmapMemory( ref_vk_device.object, buffer_host_memory.memory );
	}
}

void UniformBuffer::RecordHostToDeviceBufferCopy( VkCommandBuffer command_buffer )
{
	assert( command_buffer );

	VkBufferCopy region {};
	region.srcOffset	= 0;
	region.dstOffset	= 0;
	region.size			= buffer_size;
	vkCmdCopyBuffer( command_buffer, vk_buffer_host, vk_buffer_device, 1, &region );
}

VkBuffer UniformBuffer::GetHostBuffer() const
{
	return vk_buffer_host;
}

VkBuffer UniformBuffer::GetDeviceBuffer() const
{
	return vk_buffer_device;
}

}
