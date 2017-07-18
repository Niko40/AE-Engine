#pragma once

#include <vector>
#include <string>

#include "../BUILD_OPTIONS.h"
#include "../Platform.h"
#include "../SubSystem.h"

#include "../Vulkan/Vulkan.h"

#include "DeviceMemory/DeviceMemoryInfo.h"

namespace AE
{

class Logger;
class WindowManager;
class DeviceMemoryManager;
class DeviceResourceManager;

class Renderer : public SubSystem
{
public:
	enum class QueueAvailability : uint32_t
	{
		UNDEFINED,

		F3_PR1_SR2_PT3,		// 3 families, primary render on family 1, secondary render on family 2, primary transfer on family 3

		F2_PR1_SR1_PT2,		// 2 families, primary render on family 1, secondary render on family 1, primary transfer on family 2
		F2_PR1_SR2_PT2,		// 2 families, primary render on family 1, secondary render on family 2, primary transfer on family 2
		F2_PR1_SR2_PT1,		// 2 families, primary render on family 1, secondary render on family 2, primary transfer on family 1
		F2_PR1_SR2,			// 2 families, primary render on family 1, secondary render on family 2
		F2_PR1_PT2,			// 2 families, primary render on family 1, primary transfer on family 2

		F1_PR1_SR1_PT1,		// 1 families, primary render on family 1, secondary render on family 1, primary transfer on family 1
		F1_PR1_PT1,			// 1 families, primary render on family 1, primary transfer on family 1
		F1_PR1_SR1,			// 1 families, primary render on family 1, secondary render on family 1
		F1_PR1,				// 1 families, primary render on family 1
	};

	enum class UsedQueuesFlags : uint32_t
	{
		PRIMARY_RENDER		= 1 << 0,
		SECONDARY_RENDER	= 1 << 1,
		PRIMARY_TRANSFER	= 1 << 2,
	};

	struct SharingModeInfo
	{
		vk::SharingMode		sharing_mode;
		Vector<uint32_t>	shared_queue_family_indices;
	};

	Renderer( Engine * engine, std::string application_name, uint32_t application_version );
	~Renderer();

	void									Update();

	void									InitializeRenderToWindow( WindowManager * window_manager );
	void									DeInitializeRenderToWindow();

	vk::Instance							GetVulkanInstance() const;
	vk::PhysicalDevice						GetVulkanPhysicalDevice() const;
	VulkanDevice							GetVulkanDevice() const;

	DeviceMemoryManager					*	GetDeviceMemoryManager() const;
	DeviceResourceManager				*	GetDeviceResourceManager() const;

	uint32_t								GetPrimaryRenderQueueFamilyIndex() const;
	uint32_t								GetSecondaryRenderQueueFamilyIndex() const;
	uint32_t								GetPrimaryTransferQueueFamilyIndex() const;

	VulkanQueue								GetPrimaryRenderQueue() const;
	VulkanQueue								GetSecondaryRenderQueue() const;
	VulkanQueue								GetPrimaryTransferQueue() const;

	SharingModeInfo							GetSharingModeInfo( UsedQueuesFlags used_queues );

	const vk::PhysicalDeviceProperties	&	GetPhysicalDeviceProperties() const;
	const vk::PhysicalDeviceFeatures	&	GetPhysicalDeviceFeatures() const;
	const vk::PhysicalDeviceLimits		&	GetPhysicalDeviceLimits() const;

	bool									IsFormatSupported( vk::ImageTiling tiling, vk::Format format, vk::FormatFeatureFlags feature_flags );

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


	void									CreateWindowSurface();
	void									DestroyWindowSurface();

	void									CreateSwapchain();
	void									DestroySwapchain();

	void									CreateSwapchainImageViews();
	void									DestroySwapchainImageViews();

	void									CreateDepthStencilImageAndView();
	void									DestroyDepthStencilImageAndView();

	void									CreateRenderPass();
	void									DestroyRenderPass();

	void									CreateWindowFramebuffers();
	void									DestroyWindowFramebuffers();


	WindowManager						*	p_window								= nullptr;

	vk::Instance							vk_instance								= nullptr;
	vk::PhysicalDevice						vk_physical_device						= nullptr;
	VulkanDevice							vk_device								= {};

	Mutex									device_mutex;

	UniquePointer<DeviceMemoryManager>		device_memory_manager					= nullptr;
	UniquePointer<DeviceResourceManager>	device_resource_manager					= nullptr;

	vk::ApplicationInfo						application_info						= {};

	vk::PhysicalDeviceProperties			physical_device_properties				= {};
	vk::PhysicalDeviceFeatures				physical_device_features				= {};
	vk::PhysicalDeviceLimits				physical_device_limits					= {};

	VulkanQueue								vk_primary_render_queue					= {};
	VulkanQueue								vk_secondary_render_queue				= {};
	VulkanQueue								vk_primary_transfer_queue				= {};

	Mutex									primary_render_queue_mutex;
	Mutex									secondary_render_queue_mutex;
	Mutex									primary_transfer_queue_mutex;

	vk::QueueFamilyProperties				vk_primary_render_queue_family_properties		= {};
	vk::QueueFamilyProperties				vk_secondary_render_queue_family_properties		= {};
	vk::QueueFamilyProperties				vk_primary_transfer_queue_family_properties		= {};

	Vector<vk::SubmitInfo>					vk_primary_render_submit_queue;
	Vector<vk::SubmitInfo>					vk_secondary_render_submit_queue;
	Vector<vk::SubmitInfo>					vk_primary_transfer_submit_queue;

	std::vector<vk::DeviceQueueCreateInfo>	device_queue_create_infos;

	uint32_t								primary_render_queue_family_index		= UINT32_MAX;
	uint32_t								secondary_render_queue_family_index		= UINT32_MAX;
	uint32_t								primary_transfer_queue_family_index		= UINT32_MAX;
	QueueAvailability						queue_availability						= QueueAvailability::UNDEFINED;

	Vector<const char*>						instance_layer_names;
	Vector<const char*>						instance_extension_names;
	Vector<const char*>						device_extension_names;


	vk::SurfaceKHR							vk_surface								= nullptr;
	vk::SurfaceCapabilitiesKHR				surface_capabilities					= {};
	uint32_t								surface_size_x							= 0;
	uint32_t								surface_size_y							= 0;
	vk::SurfaceFormatKHR					surface_format							= {};

	vk::SwapchainKHR						vk_swapchain							= nullptr;
	uint32_t								swapchain_image_count					= 3;
	uint32_t								swapchain_image_count_target			= 3;
	bool									swapchain_vsync							= false;
	vk::PresentModeKHR						swapchain_present_mode					= vk::PresentModeKHR::eMailbox;
	std::vector<vk::Image>					swapchain_images;
	std::vector<vk::ImageView>				swapchain_image_views;

	vk::Image								depth_stencil_image						= nullptr;
	DeviceMemoryInfo						depth_stencil_image_memory				= {};
	vk::ImageView							depth_stencil_image_view				= nullptr;
	vk::Format								depth_stencil_format					= vk::Format::eUndefined;
	bool									stencil_available						= false;

	vk::RenderPass							vk_render_pass							= nullptr;

	Vector<vk::Framebuffer>					vk_framebuffers;

	VkDebugReportCallbackEXT				debug_report_callback					= VK_NULL_HANDLE;
	VkDebugReportCallbackCreateInfoEXT		debug_report_callback_create_info		= {};

	bool									render_initialized_to_window			= false;
};

Renderer::UsedQueuesFlags operator|( Renderer::UsedQueuesFlags f1, Renderer::UsedQueuesFlags f2 );
Renderer::UsedQueuesFlags operator&( Renderer::UsedQueuesFlags f1, Renderer::UsedQueuesFlags f2 );
Renderer::UsedQueuesFlags operator|=( Renderer::UsedQueuesFlags f1, Renderer::UsedQueuesFlags f2 );

}
