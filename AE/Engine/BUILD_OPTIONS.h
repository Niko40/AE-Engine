#pragma once

// engine name
// VALUES: string "engine name"
#define BUILD_ENGINE_NAME												"AE Engine"

// engine version
// VALUES: Vulkan formatted version VK_MAKE_VERSION( MAJOR, MINOR, PATCH )
#define BUILD_ENGINE_VERSION											VK_MAKE_VERSION( 0, 0, 1 )

// required hardware vulkan version
// VALUES: Vulkan formatted version VK_MAKE_VERSION( MAJOR, MINOR, PATCH )
#define BUILD_VULKAN_VERSION											VK_MAKE_VERSION( 1, 0, 30 )

// enable vulkan debugging
// VALUES:
// 1 = ON
// 0 = OFF
#define BUILD_ENABLE_VULKAN_DEBUG										1

// include function name and line on log report
// VALUES:
// 1 = ON
// 0 = OFF
#define BUILD_DEBUG_LOG_FUNCTION_NAME_AND_LINE							0

// this engine tries to create 3 vulkan queues, one for primary rendering (scene, window, framebuffers,
// visible rendering), one for secondary rendering (non visible rendering, image scaling, etc...)
// and one for transfering data from ram to the physical device, priorities for each queue can be set below
// VALUES: float
#define BUILD_VULKAN_PRIMARY_RENDER_QUEUE_PRIORITY						1.0f
#define BUILD_VULKAN_SECONDARY_RENDER_QUEUE_PRIORITY					0.6f
#define BUILD_VULKAN_PRIMARY_TRANSFER_QUEUE_PRIORITY					0.5f

// how many loader and destroyer threads the resource managers have
// 2 should be a good value to keep the engine fed with resources, adjust as needed
// VALUES: number of threads
#define BUILD_FILE_RESOURCE_MANAGER_WORKER_THREAD_COUNT					2
#define BUILD_DEVICE_RESOURCE_MANAGER_WORKER_THREAD_COUNT				2

// include resource statistics, basically how long the resource took to
// load and unload, engine will collect a medium for each resource type
// and reports mediums at the end of the application in the log file
// VALUES:
// 1 = ON
// 0 = OFF
#define BUILD_INCLUDE_RESOURCE_STATISTICS								1

// this tells the maximum per shader texture count, this does not mean that space is automatically
// allocated for the amount given, but rather the absolute maximum that is possible to allocate
// VALUES: maximum nuber of sampled images per shader
#define BUILD_MAX_PER_SHADER_SAMPLED_IMAGE_COUNT						16
