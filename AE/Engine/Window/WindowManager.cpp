
#include <sstream>

#include "WindowManager.h"
#include "../Logger/Logger.h"

namespace AE
{

WindowManager::WindowManager( Engine * engine )
	: SubSystem( engine, "WindowManager")
{
	glfwInit();
	p_logger->LogInfo( "GLFW initialized" );
}

WindowManager::~WindowManager()
{
	glfwTerminate();
	p_logger->LogInfo( "GLFW terminated" );
}

void WindowManager::OpenWindow( uint32_t size_x, uint32_t size_y, std::string title, bool fullscreen )
{
	glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
	glfwWindowHint( GLFW_VISIBLE, GLFW_FALSE );

	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

	GLFWmonitor		*	monitor		= nullptr;
	if( fullscreen ) {
		monitor						= glfwGetPrimaryMonitor();
	}

	window = glfwCreateWindow( size_x, size_y, title.c_str(), monitor, nullptr );
	int real_size_x = 0, real_size_y = 0;
	glfwGetWindowSize( window, &real_size_x, &real_size_y );

	std::stringstream ss;
	ss << "Window opened: Title->'" << title << "' X->" << real_size_x << " Y->" << real_size_y;
	p_logger->LogInfo( ss.str() );

	glfwMakeContextCurrent( window );
}

void WindowManager::ShowWindow( bool show )
{
	if( show ) {
		glfwShowWindow( window );
	} else {
		glfwHideWindow( window );
	}
}

void WindowManager::CloseWindow()
{
	if( nullptr		!= window ) {
		glfwDestroyWindow( window );
		window		= nullptr;

		p_logger->LogInfo( "Window closed" );
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

VkSurfaceKHR WindowManager::CreateVulkanSurface( VkInstance vk_instance )
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	vk::Result result = vk::Result( glfwCreateWindowSurface( vk_instance, window, nullptr, &surface ) );
	if( result != vk::Result::eSuccess )
	{
		p_logger->LogError( "Vulkan surface creation failed with message: " + vk::to_string( result ) );
	}
	return surface;
}

GLFWwindow * WindowManager::GetGLFWWindow()
{
	return window;
}

}
