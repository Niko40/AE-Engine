#pragma once

#include <vector>
#include <string>

#include "../BUILD_OPTIONS.h"
#include "../Platform.h"
#include "../SubSystem.h"

#include "../Vulkan/Vulkan.h"

#include "DeviceMemory/DeviceMemoryInfo.h"
#include "QueueInfo.h"

namespace AE
{

class Logger;
class WindowManager;
class DeviceMemoryManager;
class DeviceResourceManager;
class DescriptorPoolManager;
class SceneBase;
class GBuffer;

class Renderer : public SubSystem
{
public:
	Renderer( Engine * engine, std::string application_name, uint32_t application_version, VkExtent2D resolution, bool fullscreen );
	~Renderer();

	bool									Update();

	VkInstance								GetVulkanInstance() const;
	VkPhysicalDevice						GetVulkanPhysicalDevice() const;
	VulkanDevice							GetVulkanDevice() const;
	VkRenderPass							GetVulkanRenderPass() const;

	DeviceMemoryManager					*	GetDeviceMemoryManager();
	DeviceResourceManager				*	GetDeviceResourceManager();
	WindowManager						*	GetWindowManager();

	uint32_t								GetPrimaryRenderQueueFamilyIndex() const;
	uint32_t								GetSecondaryRenderQueueFamilyIndex() const;
	uint32_t								GetPrimaryTransferQueueFamilyIndex() const;

	VulkanQueue								GetPrimaryRenderQueue() const;
	VulkanQueue								GetSecondaryRenderQueue() const;
	VulkanQueue								GetPrimaryTransferQueue() const;

	SharingModeInfo							GetSharingModeInfo( UsedQueuesFlags used_queues );

	const VkPhysicalDeviceProperties	&	GetPhysicalDeviceProperties() const;
	const VkPhysicalDeviceFeatures		&	GetPhysicalDeviceFeatures() const;
	const VkPhysicalDeviceLimits		&	GetPhysicalDeviceLimits() const;

	VkPipelineLayout						GetVulkanGraphicsPipelineLayout( uint32_t supported_image_count ) const;
	VkDescriptorSetLayout					GetVulkanDescriptorSetLayoutForCamera() const;
	VkDescriptorSetLayout					GetVulkanDescriptorSetLayoutForMesh() const;
	VkDescriptorSetLayout					GetVulkanDescriptorSetLayoutForPipeline() const;
	VkDescriptorSetLayout					GetVulkanDescriptorSetLayoutForImageBindingCount( uint32_t image_binding_count ) const;

	/*
	TODO( "Rendering threads, figure out if rendering threads should be allocated at runtime or creation time" );
	std::thread::id							GetRenderingThreadForSceneBase( SceneBase * node );
	void									FreeRenderingThreadForSceneBase( SceneBase * node );
	*/

	DescriptorPoolManager				*	GetDescriptorPoolManager();

	bool									IsFormatSupported( VkImageTiling tiling, VkFormat format, VkFormatFeatureFlags feature_flags );

private:
	void									SetupDebugReporting();
	void									CreateDebugReporting();
	void									DestroyDebugReporting();

	void									SetupRequiredLayersAndExtensions();

	void									CreateInstance();
	void									DestroyInstance();

	void									SelectPhysicalDevices();

	void									FindQueueFamilies();

	void									CreateDevice();
	void									DestroyDevice();

	void									GetQueueHandles();

	void									FindDepthStencilFormat();

	void									CreateRenderPass();
	void									DestroyRenderPass();

	void									CreateWindowFramebuffers();
	void									DestroyWindowFramebuffers();

	void									CreateDescriptorSetLayouts();
	void									DestroyDescriptorSetLayouts();

	void									CreateGraphicsPipelineLayouts();
	void									DestroyGraphicsPipelineLayouts();

	void									CreateGBuffers();
	void									DestroyGBuffers();

	VkInstance								vk_instance								= nullptr;
	VkPhysicalDevice						vk_physical_device						= nullptr;
	VulkanDevice							vk_device								= {};

	Mutex									device_mutex;

	UniquePointer<DeviceMemoryManager>		device_memory_manager					= nullptr;
	UniquePointer<DeviceResourceManager>	device_resource_manager					= nullptr;
	UniquePointer<WindowManager>			window_manager							= nullptr;

	VkApplicationInfo						application_info						= {};

	VkPhysicalDeviceProperties				physical_device_properties				= {};
	VkPhysicalDeviceFeatures				physical_device_features				= {};
	VkPhysicalDeviceLimits					physical_device_limits					= {};

	VulkanQueue								vk_primary_render_queue					= {};
	VulkanQueue								vk_secondary_render_queue				= {};
	VulkanQueue								vk_primary_transfer_queue				= {};

	Mutex									primary_render_queue_mutex;
	Mutex									secondary_render_queue_mutex;
	Mutex									primary_transfer_queue_mutex;

	VkQueueFamilyProperties					vk_primary_render_queue_family_properties		= {};
	VkQueueFamilyProperties					vk_secondary_render_queue_family_properties		= {};
	VkQueueFamilyProperties					vk_primary_transfer_queue_family_properties		= {};

	Vector<VkSubmitInfo>					vk_primary_render_submit_queue;
	Vector<VkSubmitInfo>					vk_secondary_render_submit_queue;
	Vector<VkSubmitInfo>					vk_primary_transfer_submit_queue;

	Vector<VkDeviceQueueCreateInfo>			device_queue_create_infos;

	uint32_t								primary_render_queue_family_index		= UINT32_MAX;
	uint32_t								secondary_render_queue_family_index		= UINT32_MAX;
	uint32_t								primary_transfer_queue_family_index		= UINT32_MAX;
	QueueAvailability						queue_availability						= QueueAvailability::UNDEFINED;

	VkExtent2D								render_resolution						= { 800, 600 };

	Vector<const char*>						instance_layer_names;
	Vector<const char*>						instance_extension_names;
	Vector<const char*>						device_extension_names;

	VkFormat								depth_stencil_format					= VK_FORMAT_UNDEFINED;
	bool									stencil_available						= false;

	VkRenderPass							vk_render_pass							= nullptr;

	// related to graphics pipeline layouts
	VkDescriptorSetLayout					vk_descriptor_set_layout_for_camera		= nullptr;
	VkDescriptorSetLayout					vk_descriptor_set_layout_for_mesh		= nullptr;
	VkDescriptorSetLayout					vk_descriptor_set_layout_for_pipeline	= nullptr;
	Vector<VkDescriptorSetLayout>			vk_descriptor_set_layouts_for_images;

	// graphics pipeline layouts, we only deal with static sets so we don't need a custom layout per pipeline
	// only thing that can change in a set is the amount of textures,
	// for only 1 texture, choose layout at index 0, for 2 textures, choose layout at index 1...
	// the amount of layouts matches BUILD_MAX_PER_SHADER_SAMPLED_IMAGE_COUNT
	Vector<VkPipelineLayout>				vk_graphics_pipeline_layouts;

	Array<UniquePointer<GBuffer>, 2>		gbuffers								= {};
	Vector<VkFramebuffer>					vk_framebuffers;

	UniquePointer<DescriptorPoolManager>	descriptor_pool_manager;

	VkDebugReportCallbackEXT				debug_report_callback					= VK_NULL_HANDLE;
	VkDebugReportCallbackCreateInfoEXT		debug_report_callback_create_info		= {};

	bool									render_initialized_to_window			= false;
};

}
