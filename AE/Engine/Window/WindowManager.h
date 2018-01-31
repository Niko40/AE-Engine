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

	// Aquire the next swapchain image and return it's id. This function may or may not block depending on the presentation engine of the OS.
	// You can provide a semaphore that will be set once the image is ready to be used.
	// If no semaphore is provided (VK_NULL_HANDLE) then this function will not return until the next image is fully available for use
	uint32_t								AquireSwapchainImage( VkSemaphore semaphore_image_ready );

	// Present swapchain image, you should provide one or more semaphores to wait before presenting, typically from the final render completion.
	// if no semaphores are given, the image is presented immediately, in this case make sure the image is ready to be presented.
	void									PresentSwapchainImage( uint32_t image_number, Vector<VkSemaphore> wait_semaphores );

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
	VulkanQueue								ref_primary_render_queue				= {};

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

	VkFence									vk_fence_swapchain_image_ready			= VK_NULL_HANDLE;
};

}
