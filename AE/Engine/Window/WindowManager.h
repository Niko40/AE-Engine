#pragma once

#include <string>

#include "../BUILD_OPTIONS.h"
#include "../Platform.h"

#include "../Vulkan/Vulkan.h"
#include <GLFW/glfw3.h>

#include "../SubSystem.h"

namespace AE
{

class Logger;

class WindowManager : public SubSystem
{
public:
	WindowManager( Engine * engine );
	~WindowManager();

	void				OpenWindow( uint32_t size_x, uint32_t size_y, std::string title, bool fullscreen );
	void				ShowWindow( bool show );
	void				CloseWindow();
	void				ResizeWindow();

	bool				Update();

	VkSurfaceKHR		CreateVulkanSurface( VkInstance vk_instance );

	GLFWwindow		*	GetGLFWWindow();

private:
	GLFWwindow		*	window				= nullptr;
};

}
