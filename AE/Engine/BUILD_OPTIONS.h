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

// in debugging modes, in which lines we want to assert? for example if an application detects
// an error and then reports it using the usual interface, and BUILD_DEBUG_LOG_ASSERT_ERROR_LINES
// is 1, then the program will also stop the execution by asserting on that line,
// Critical errors are always asserted.
// Values:
// 1 = ON
// 0 = OFF
#define BUILD_DEBUG_LOG_ASSERT_INFO_LINES								0
#define BUILD_DEBUG_LOG_ASSERT_WARNING_LINES							0
#define BUILD_DEBUG_LOG_ASSERT_ERROR_LINES								0
#define BUILD_DEBUG_LOG_ASSERT_VULKAN_LINES								0
#define BUILD_DEBUG_LOG_ASSERT_OTHER_LINES								0

// this engine tries to create 3 vulkan queues, one for primary rendering (scene, window, framebuffers,
// visible rendering), one for secondary rendering (non visible rendering, image scaling, etc...)
// and one for transfering data from ram to the physical device, priorities for each queue can be set below
// VALUES: float, 0.0 (minimum priority) - 1.0 (maximum priority)
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
#define BUILD_MAX_PER_SHADER_SAMPLED_IMAGE_COUNT						8

// This tells the maximum amount of descriptor sets in a single vulkan descriptor pool
// Practically this tells how often a new memory pool is allocated when the previous one is full
// so we never really run out of descriptor sets, but how often a new pool is allocated can
// impact performance, on the other end you don't want to allocate too much at once either.
// Descriptor pools like all the other vulkan pools in this engine are created per thread
// VALUES: maximum number of how many descriptor sets we can allocate from a single vulkan pool
#define BUILD_MAX_DESCRIPTOR_SETS_IN_POOL								128
