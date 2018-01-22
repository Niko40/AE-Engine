#pragma once

// Engine name
// VALUES: string "engine name"
#define BUILD_ENGINE_NAME												"AE Engine"

// Engine version
// VALUES: Vulkan formatted version VK_MAKE_VERSION( MAJOR, MINOR, PATCH )
#define BUILD_ENGINE_VERSION											VK_MAKE_VERSION( 0, 0, 1 )

// Required hardware vulkan version
// VALUES: Vulkan formatted version VK_MAKE_VERSION( MAJOR, MINOR, PATCH )
#define BUILD_VULKAN_VERSION											VK_MAKE_VERSION( 1, 0, 30 )

// Enable vulkan debugging, debug layers must be installed
// VALUES:
// 0 = OFF
// 1 = ON
#define BUILD_ENABLE_VULKAN_DEBUG										1

// Enable vulkan result check logging for warnings or errors
// Result check is performed on every vulkan function that returns a VkResult value
// VALUES:
// 0 = OFF
// 1 = ON
#define BUILD_ENABLE_VULKAN_RESULT_CHECK_LOGGING_WARNINGS				1
#define BUILD_ENABLE_VULKAN_RESULT_CHECK_LOGGING_ERRORS					1

// Enable vulkan result check assertion for warnings or errors
// Result check is performed on every vulkan function that returns a VkResult value
// VALUES:
// 0 = OFF
// 1 = ON
#define BUILD_ENABLE_VULKAN_RESULT_CHECK_ASSERTION_WARNINGS				0
#define BUILD_ENABLE_VULKAN_RESULT_CHECK_ASSERTION_ERRORS				1

// The memory backing for Vulkan can be allocated by Vulkan or in the application that uses Vulkan
// This is mostly useful only as a debug feature as Vulkan memory is pooled internally and external
// memory pools do not bring significant speed benefits
// VALUES:
// 0 = Vulkan internal allocation
// 1 = Application allocates in memory pool
// 2 = Application allocates in memory pool with debugging
#define BUILD_VULKAN_MEMORY_ALLOCATOR_TYPE								2

// Include function name and line on log report
// VALUES:
// 0 = OFF
// 1 = ON
#define BUILD_DEBUG_LOG_FUNCTION_NAME_AND_LINE							0

// In debugging modes, in which lines we want to assert? for example if an application detects
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

// Do you want to show TODO messages when compiling the project?
// You can disable this to reduce clutter in compilation messages,
// enable if you're looking for places to improve the code
// Values:
// 0 = Disabled
// 1 = Enabled
#define BUILD_TODO_SHOW_WHEN_COMPILING									1

// This engine tries to create 3 vulkan queues, one for primary rendering (scene, window, framebuffers,
// visible rendering), one for secondary rendering (non visible rendering, image scaling, etc...)
// and one for transfering data from ram to the physical device, priorities for each queue can be set below
// VALUES: float, 0.0 (minimum priority) - 1.0 (maximum priority)
#define BUILD_VULKAN_PRIMARY_RENDER_QUEUE_PRIORITY						1.0f
#define BUILD_VULKAN_SECONDARY_RENDER_QUEUE_PRIORITY					0.6f
#define BUILD_VULKAN_PRIMARY_TRANSFER_QUEUE_PRIORITY					0.5f

// How many loader and destroyer threads the resource managers have
// 2 should be a good value to keep the engine fed with resources, adjust as needed
// VALUES: number of threads
#define BUILD_FILE_RESOURCE_MANAGER_WORKER_THREAD_COUNT					2
#define BUILD_DEVICE_RESOURCE_MANAGER_WORKER_THREAD_COUNT				2

// Include resource statistics, basically how long the resource took to
// load and unload, engine will collect a medium for each resource type
// and reports mediums at the end of the application in the log file
// VALUES:
// 0 = OFF
// 1 = ON
#define BUILD_INCLUDE_RESOURCE_STATISTICS								1

// This tells the maximum per shader texture count, this does not mean that space is automatically
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

// Because we handle resources on the background that require accessing the Vulkan device
// and because waiting for fences blocks the mutex to the Vulkan device this could interfere
// with the background resource operations, effectively slowing things down, we need a "busy loop"
// that will periodically unlock the mutex, giving background threads a chance to process.
// This tells how long the non-blocking function is going to block at a time, higher values
// block longer periods at a time, smaller values will consume more system resources.
// In theory this will not effect rendering speed, only the balance between background resource
// handling speed and main thread power consumption.
// VALUES: blocking period of a non-blocking "wait for fences" function, in nanoseconds.
#define BUILD_NON_BLOCKING_TIMEOUT_FOR_FENCES							10000
