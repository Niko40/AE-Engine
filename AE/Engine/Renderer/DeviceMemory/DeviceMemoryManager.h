#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Vulkan/Vulkan.h"

#include "DeviceMemoryInfo.h"

namespace AE
{

class Engine;
class Logger;
class Renderer;

class DeviceMemoryManager
{
public:
	DeviceMemoryManager( Engine * engine, Renderer * renderer );
	~DeviceMemoryManager();

	DeviceMemoryInfo						AllocateImageMemory( vk::Image image, vk::MemoryPropertyFlags memory_properties );
	DeviceMemoryInfo						AllocateAndBindImageMemory( vk::Image image, vk::MemoryPropertyFlags memory_properties );
	DeviceMemoryInfo						AllocateBufferMemory( vk::Buffer buffer, vk::MemoryPropertyFlags memory_properties );
	DeviceMemoryInfo						AllocateAndBindBufferMemory( vk::Buffer buffer, vk::MemoryPropertyFlags memory_properties );
	void									FreeMemory( DeviceMemoryInfo & memory );

private:
	DeviceMemoryInfo						AllocateMemory( vk::MemoryRequirements & requirements, vk::MemoryPropertyFlags memory_properties );
	uint32_t								FindMemoryTypeIndex( vk::MemoryRequirements &requirements, vk::MemoryPropertyFlags property_flags );

	Engine								*	p_engine					= nullptr;
	Logger								*	p_logger					= nullptr;
	Renderer							*	p_renderer					= nullptr;

	vk::Instance							ref_vk_instance				= nullptr;
	vk::PhysicalDevice						ref_vk_physical_device		= nullptr;
	VulkanDevice							ref_vk_device				= {};

	vk::PhysicalDeviceMemoryProperties		vk_physical_device_memory_properties;
};

}