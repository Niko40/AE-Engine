
#include <sstream>

#include "WindowManager.h"
#include "../Logger/Logger.h"
#include "../Renderer/Renderer.h"

#include <assert.h>

namespace AE
{

WindowManager::WindowManager( Engine * engine, Renderer * renderer )
	: SubSystem( engine, "WindowManager")
{
	p_renderer							= renderer;
	assert( p_renderer );
	ref_vk_instance						= p_renderer->GetVulkanInstance();
	ref_vk_physical_device				= p_renderer->GetVulkanPhysicalDevice();
	ref_vk_device						= p_renderer->GetVulkanDevice();
	primary_render_queue_family_index	= p_renderer->GetPrimaryRenderQueueFamilyIndex();
	assert( ref_vk_instance );
	assert( ref_vk_physical_device );
	assert( ref_vk_device.object );
	assert( ref_vk_device.mutex );
	assert( primary_render_queue_family_index != UINT32_MAX );

	p_logger->LogInfo( "Window manager initialized" );
}

WindowManager::~WindowManager()
{
	p_logger->LogInfo( "Window manager terminated" );
}

void WindowManager::ShowWindow( bool show )
{
	if( show ) {
		glfwShowWindow( window );
	} else {
		glfwHideWindow( window );
	}
}

bool WindowManager::Update()
{
	glfwPollEvents();
	if( glfwWindowShouldClose( window ) ) {
		glfwHideWindow( window );
		return false;
	}
	return true;
}

const GLFWwindow * WindowManager::GetGLFWWindow() const
{
	return window;
}

const VkSurfaceFormatKHR & WindowManager::GetVulkanSurfaceFormat() const
{
	return surface_format;
}

const VkSurfaceCapabilitiesKHR & WindowManager::GetVulkanSurfaceCapabilities() const
{
	return surface_capabilities;
}

const Vector<VkImage>& WindowManager::GetSwapchainImages() const
{
	return swapchain_images;
}

const Vector<VkImageView>& WindowManager::GetSwapchainImageViews() const
{
	return swapchain_image_views;
}

uint32_t WindowManager::GetSwapchainImageCount() const
{
	return swapchain_image_count;
}

void WindowManager::OpenWindow( VkExtent2D size, std::string title, bool fullscreen )
{
	glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
	glfwWindowHint( GLFW_VISIBLE, GLFW_FALSE );

	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

	GLFWmonitor		*	monitor		= nullptr;
	if( fullscreen ) {
		monitor						= glfwGetPrimaryMonitor();
	}

	window = glfwCreateWindow( size.width, size.height, title.c_str(), monitor, nullptr );
	int real_size_x = 0, real_size_y = 0;
	glfwGetWindowSize( window, &real_size_x, &real_size_y );

	std::stringstream ss;
	ss << "Window opened: Title->'" << title << "' X->" << real_size_x << " Y->" << real_size_y;
	p_logger->LogInfo( ss.str() );

	glfwMakeContextCurrent( window );
}

void WindowManager::CloseWindow()
{
	if( nullptr != window ) {
		glfwDestroyWindow( window );
		window		= nullptr;

		p_logger->LogInfo( "Window closed" );
	}
}

void WindowManager::CreateWindowSurface()
{
	VkResult result = VkResult( glfwCreateWindowSurface( ref_vk_instance, window, VULKAN_ALLOC, &vk_surface ) );
	if( result != VK_SUCCESS ) {
		p_logger->LogError( "Vulkan surface creation failed with message: " + VulkanResultToString( result ) );
	}

	VkBool32 WSI_supported		= VK_FALSE;
	VulkanResultCheck( vkGetPhysicalDeviceSurfaceSupportKHR( ref_vk_physical_device, primary_render_queue_family_index, vk_surface, &WSI_supported ) );

	if( !WSI_supported ) {
		std::stringstream ss;
		ss << "Vulkan surface is not supported on current physical device->'";
		ss << p_renderer->GetPhysicalDeviceProperties().deviceName;
		ss << "' and queue family->";
		ss << p_renderer->GetPrimaryRenderQueueFamilyIndex();
		p_logger->LogCritical( ss.str() );
	}

	VulkanResultCheck( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( ref_vk_physical_device, vk_surface, &surface_capabilities ) );

	if( surface_capabilities.currentExtent.width == UINT32_MAX ) {
		int x = 0, y = 0;
		glfwGetWindowSize( window, &x, &y );
		surface_capabilities.currentExtent.width	= uint32_t( x );
		surface_capabilities.currentExtent.height	= uint32_t( y );
	}

	uint32_t surface_count		= 0;
	VulkanResultCheck( vkGetPhysicalDeviceSurfaceFormatsKHR( ref_vk_physical_device, vk_surface, &surface_count, nullptr ) );
	Vector<VkSurfaceFormatKHR> surface_formats( surface_count );
	VulkanResultCheck( vkGetPhysicalDeviceSurfaceFormatsKHR( ref_vk_physical_device, vk_surface, &surface_count, surface_formats.data() ) );

	if( surface_formats.size() <= 0 ) {
		p_logger->LogCritical( "Vulkan surface formats missing" );
	}

	if( surface_formats[ 0 ].format == VK_FORMAT_UNDEFINED ) {
		surface_format.format		= VK_FORMAT_B8G8R8A8_UNORM;
		surface_format.colorSpace	= VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	} else {
		surface_format				= surface_formats[ 0 ];
	}
}

void WindowManager::DestroyWindowSurface()
{
	vkDestroySurfaceKHR( ref_vk_instance, vk_surface, VULKAN_ALLOC );
	vk_surface = VK_NULL_HANDLE;
}

void WindowManager::CreateSwapchain()
{
	if( swapchain_vsync ) {
		swapchain_image_count	= 2;
	} else {
		swapchain_image_count	= swapchain_image_count_target;
	}

	if( swapchain_image_count < surface_capabilities.minImageCount )		swapchain_image_count	= surface_capabilities.minImageCount;
	if( surface_capabilities.maxImageCount > 0 ) {
		if( swapchain_image_count > surface_capabilities.maxImageCount )	swapchain_image_count	= surface_capabilities.maxImageCount;
	}

	swapchain_present_mode				= VK_PRESENT_MODE_FIFO_KHR;
	if( !swapchain_vsync ) {
		uint32_t present_mode_count		= 0;
		VulkanResultCheck( vkGetPhysicalDeviceSurfacePresentModesKHR( ref_vk_physical_device, vk_surface, &present_mode_count, nullptr ) );
		Vector<VkPresentModeKHR> present_modes( present_mode_count );
		VulkanResultCheck( vkGetPhysicalDeviceSurfacePresentModesKHR( ref_vk_physical_device, vk_surface, &present_mode_count, present_modes.data() ) );

		TODO( "Add more options to the user to select present modes" );
		for( auto m : present_modes ) {
			if( m == VK_PRESENT_MODE_MAILBOX_KHR ) {
				swapchain_present_mode	= m;
				break;
			}
		}
	}

	VkSwapchainCreateInfoKHR swapchain_CI {};
	swapchain_CI.sType					= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_CI.pNext					= nullptr;
	swapchain_CI.flags					= 0;
	swapchain_CI.surface				= vk_surface;
	swapchain_CI.minImageCount			= swapchain_image_count;
	swapchain_CI.imageFormat			= surface_format.format;
	swapchain_CI.imageColorSpace		= surface_format.colorSpace;
	swapchain_CI.imageExtent			= surface_capabilities.currentExtent;
	swapchain_CI.imageArrayLayers		= 1;
	swapchain_CI.imageUsage				= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_CI.imageSharingMode		= VK_SHARING_MODE_EXCLUSIVE;
	swapchain_CI.queueFamilyIndexCount	= 1;
	swapchain_CI.pQueueFamilyIndices	= &primary_render_queue_family_index;
	swapchain_CI.preTransform			= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchain_CI.compositeAlpha			= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_CI.presentMode			= swapchain_present_mode;
	swapchain_CI.clipped				= VK_TRUE;
	swapchain_CI.oldSwapchain			= nullptr;

	{
		// We don't have device resource threads at this points but better
		// to be sure and start locking the mutex for the vulkan device at this point
		LOCK_GUARD( *ref_vk_device.mutex );
		VulkanResultCheck( vkCreateSwapchainKHR( ref_vk_device.object, &swapchain_CI, VULKAN_ALLOC, &vk_swapchain ) );
	}
	if( !vk_swapchain ) {
		p_logger->LogCritical( "Swapchain creation failed" );
	}
}

void WindowManager::DestroySwapchain()
{
	// We don't have device resource threads at this points but better to be sure
	LOCK_GUARD( *ref_vk_device.mutex );
	vkDestroySwapchainKHR( ref_vk_device.object, vk_swapchain, VULKAN_ALLOC );
	vk_swapchain = VK_NULL_HANDLE;
}

void WindowManager::CreateSwapchainImageViews()
{
	// We don't have device resource threads at this points but better to be sure
	LOCK_GUARD( *ref_vk_device.mutex );

	// getting the images is easy so we'll just do it in here
	VulkanResultCheck( vkGetSwapchainImagesKHR( ref_vk_device.object, vk_swapchain, &swapchain_image_count, nullptr ) );
	swapchain_images.resize( swapchain_image_count );
	VulkanResultCheck( vkGetSwapchainImagesKHR( ref_vk_device.object, vk_swapchain, &swapchain_image_count, swapchain_images.data() ) );

	swapchain_image_views.resize( swapchain_image_count );
	for( uint32_t i=0; i < swapchain_image_count; ++i ) {
		VkImageViewCreateInfo image_view_CI {};
		image_view_CI.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_CI.pNext				= nullptr;
		image_view_CI.flags				= 0;
		image_view_CI.image				= swapchain_images[ i ];
		image_view_CI.viewType			= VK_IMAGE_VIEW_TYPE_2D;
		image_view_CI.format			= surface_format.format;
		image_view_CI.components.r		= VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_CI.components.g		= VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_CI.components.b		= VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_CI.components.a		= VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_CI.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_CI.subresourceRange.baseMipLevel		= 0;
		image_view_CI.subresourceRange.levelCount		= 1;
		image_view_CI.subresourceRange.baseArrayLayer	= 0;
		image_view_CI.subresourceRange.layerCount		= 1;

		VulkanResultCheck( vkCreateImageView( ref_vk_device.object, &image_view_CI, VULKAN_ALLOC, &swapchain_image_views[ i ] ) );
		if( !swapchain_image_views[ i ] ) {
			p_logger->LogCritical( "Swapchain image view creation failed" );
		}
	}
}

void WindowManager::DestroySwapchainImageViews()
{
	// We don't have device resource threads at this points but better to be sure
	LOCK_GUARD( *ref_vk_device.mutex );

	for( auto iv : swapchain_image_views ) {
		vkDestroyImageView( ref_vk_device.object, iv, VULKAN_ALLOC );
	}

	swapchain_images.clear();
	swapchain_image_views.clear();
}

}
