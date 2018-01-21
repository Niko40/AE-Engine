#pragma once

#include <string>

#include "../BUILD_OPTIONS.h"
#include "../Platform.h"

#include "../Vulkan/Vulkan.h"
#include <GLFW/glfw3.h>

#include "../SubSystem.h"

namespace AE
{

class Engine;
class Logger;
class Renderer;

class WindowManager : public SubSystem
{
	friend class Renderer;

public:
	WindowManager( Engine * engine, Renderer * renderer );
	~WindowManager();

	void									ShowWindow( bool show );

	bool									Update();

	const GLFWwindow					*	GetGLFWWindow() const;
	const VkSurfaceFormatKHR			&	GetVulkanSurfaceFormat() const;

	const VkSurfaceCapabilitiesKHR		&	GetVulkanSurfaceCapabilities() const;

	const Vector<VkImage>				&	GetSwapchainImages() const;
	const Vector<VkImageView>			&	GetSwapchainImageViews() const;

	uint32_t								GetSwapchainImageCount() const;

private:
	void									OpenWindow( VkExtent2D size, std::string title, bool fullscreen );
	void									CloseWindow();
	void									ResizeWindow();

	void									CreateWindowSurface();
	void									DestroyWindowSurface();

	void									CreateSwapchain();
	void									DestroySwapchain();

	void									CreateSwapchainImageViews();
	void									DestroySwapchainImageViews();

	Renderer							*	p_renderer								= nullptr;

	VkInstance								ref_vk_instance							= VK_NULL_HANDLE;
	VkPhysicalDevice						ref_vk_physical_device					= VK_NULL_HANDLE;
	VulkanDevice							ref_vk_device							= {};

	uint32_t								primary_render_queue_family_index		= UINT32_MAX;

	GLFWwindow							*	window									= nullptr;

	VkSurfaceKHR							vk_surface								= nullptr;
	VkSurfaceCapabilitiesKHR				surface_capabilities					= {};
	VkSurfaceFormatKHR						surface_format							= {};

	VkSwapchainKHR							vk_swapchain							= nullptr;
	uint32_t								swapchain_image_count					= 3;
	uint32_t								swapchain_image_count_target			= 3;
	bool									swapchain_vsync							= false;
	VkPresentModeKHR						swapchain_present_mode					= VK_PRESENT_MODE_MAILBOX_KHR;
	Vector<VkImage>							swapchain_images;
	Vector<VkImageView>						swapchain_image_views;
};

}
