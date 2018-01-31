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

enum class GBUFFERS : uint32_t
{
	DEPTH_STENCIL,			// Format determined later
	COLOR_RGB__EMIT_R,		// R16_G16_B16_A16_SNORM
//	SPECULAR_RGB__EMIT_G,	// R16_G16_B16_A16_SNORM
//	NORMAL_RGB__EMIT_B,		// R16_G16_B16_A16_SNORM
//	LOCATION_XYZ,			// R32_G32_B32_A32_SFLOAT
	COUNT,					// NOT A G-BUFFER, this is a counter of how many G-Buffers there actually are
};

TODO;
// Planning
enum class GRAPHICS_PIPELINE_TYPE : uint32_t
{
	VERTEX_CAMERA_MESH,
	VERTEX_CAMERA_MESH_IMAGE_1,
	VERTEX_CAMERA_MESH_IMAGE_2,
	VERTEX_CAMERA_MESH_IMAGE_3,
	VERTEX_CAMERA_MESH_IMAGE_4,
	VERTEX_CAMERA_MESH_IMAGE_5,
	VERTEX_CAMERA_MESH_IMAGE_6,
	VERTEX_CAMERA_MESH_IMAGE_7,
	VERTEX_CAMERA_MESH_IMAGE_8,
};

constexpr uint32_t GBUFFERS_COUNT				= static_cast<uint32_t>( GBUFFERS::COUNT );
constexpr uint32_t SWAPCHAIN_ATTACHMENT_INDEX	= GBUFFERS_COUNT;	// Swapchain attachment comes after all GBUFFERS so it's index is the G-Buffer count

enum class FORMAT_PROPERTIES_FEATURE : uint32_t
{
	OPTIMAL_IMAGE,
	LINEAR_IMAGE,
	BUFFER,
};

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

	bool									IsFormatSupported( FORMAT_PROPERTIES_FEATURE feature, VkFormat format, VkFormatFeatureFlags feature_flags );

	// Begin render. Aquire a new swapchain image from the window manager, begins
	// and returns a command buffer that we should use to render our scene with.
	VkCommandBuffer							BeginRender();

	// End the render. Submits the command buffer to the primary render queue
	// and gives the correct swapchain image to the presentation engine as soon as it's done rendering.
	void									EndRender( VkCommandBuffer command_buffer_from_begin_render );

	// Command: BeginRenderPass:
	// Convenience function that will record the vkBeginRenderPass() function into the command buffer that you provided.
	// Parameters are collected from the current renderer object.
	void									Command_BeginRenderPass( VkCommandBuffer command_buffer, VkSubpassContents subpass_contents );

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

	void									CreatePrimaryCommandBuffers();
	void									DestroyPrimaryCommandBuffers();

	void									CreateSynchronizationObjects();
	void									DestroySynchronizationObjects();

	VkInstance								vk_instance								= VK_NULL_HANDLE;
	VkPhysicalDevice						vk_physical_device						= VK_NULL_HANDLE;
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
	uint32_t								swapchain_image_count					= 0;
	uint32_t								current_swapchain_image					= 0;
	uint32_t								previous_swapchain_image				= 0;
	Vector<VkClearValue>					clear_values;

	VkCommandPool							vk_primary_command_pool					= VK_NULL_HANDLE;
	Vector<VkCommandBuffer>					vk_primary_command_buffers;
	Vector<VkFence>							vk_primary_command_buffer_fences;

	VkSemaphore								vk_semaphore_next_swapchain_image_available		= VK_NULL_HANDLE;
	VkSemaphore								vk_semaphore_render_complete			= VK_NULL_HANDLE;
//	VkFence									vk_fence_render_complete				= VK_NULL_HANDLE;

	Vector<const char*>						instance_layer_names;
	Vector<const char*>						instance_extension_names;
	Vector<const char*>						device_extension_names;

	VkFormat								depth_stencil_format					= VK_FORMAT_UNDEFINED;
	bool									stencil_available						= false;

	VkRenderPass							vk_render_pass							= VK_NULL_HANDLE;

	// related to graphics pipeline layouts
	VkDescriptorSetLayout					vk_descriptor_set_layout_for_camera		= VK_NULL_HANDLE;
	VkDescriptorSetLayout					vk_descriptor_set_layout_for_mesh		= VK_NULL_HANDLE;
	VkDescriptorSetLayout					vk_descriptor_set_layout_for_pipeline	= VK_NULL_HANDLE;
	Vector<VkDescriptorSetLayout>			vk_descriptor_set_layouts_for_images;

	// graphics pipeline layouts, we only deal with static sets so we don't need a custom layout per pipeline
	// only thing that can change in a set is the amount of textures,
	// for only 1 texture, choose layout at index 0, for 2 textures, choose layout at index 1...
	// the amount of layouts matches BUILD_MAX_PER_SHADER_SAMPLED_IMAGE_COUNT
	Vector<VkPipelineLayout>				vk_graphics_pipeline_layouts;

	Array<UniquePointer<GBuffer>, GBUFFERS_COUNT>									gbuffers						= {};
	Vector<VkFramebuffer>					vk_framebuffers;

	UniquePointer<DescriptorPoolManager>	descriptor_pool_manager;

	VkDebugReportCallbackEXT				debug_report_callback					= VK_NULL_HANDLE;
	VkDebugReportCallbackCreateInfoEXT		debug_report_callback_create_info		= {};

	bool									do_synchronization						= false;
};

}
