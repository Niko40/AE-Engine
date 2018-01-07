
#include "Vulkan.h"

#include <assert.h>
#include <iostream>

#include "../Memory/MemoryTypes.h"

#include "../Memory/MemoryPool/MemoryPool.h"

namespace AE
{

#if BUILD_VULKAN_MEMORY_ALLOCATOR_TYPE == 1 || BUILD_VULKAN_MEMORY_ALLOCATOR_TYPE == 2

#if BUILD_VULKAN_MEMORY_ALLOCATOR_TYPE == 2
int64_t vulkan_allocation_counter		= 0;
#endif

void * VulkanMemoryAllocationFunc(
	void					*	pUserData,
	size_t						size,
	size_t						alignment,
	VkSystemAllocationScope		allocationScope )
{
	auto memory					= engine_internal::MemoryPool_AllocateRaw( size, alignment );
	TODO( "All Vulkan memory debuggin reports" );
#if BUILD_VULKAN_MEMORY_ALLOCATOR_TYPE == 2
	++vulkan_allocation_counter;
#endif
	return memory;
}

void * VulkanMemoryReallocationFunc(
	void					*	pUserData,
	void					*	pOriginal,
	size_t						size,
	size_t						alignment,
	VkSystemAllocationScope		allocationScope )
{
	auto memory					= engine_internal::MemoryPool_ReallocateRaw( pOriginal, size, alignment );
#if BUILD_VULKAN_MEMORY_ALLOCATOR_TYPE == 2

#endif
	return memory;
}

void VulkanMemoryFreeFunc(
	void					*	pUserData,
	void					*	pMemory )
{
	if( pMemory ) {
#if BUILD_VULKAN_MEMORY_ALLOCATOR_TYPE == 2
		--vulkan_allocation_counter;
#endif
		engine_internal::MemoryPool_FreeRaw( pMemory );
	}
}

void VulkanMemoryInternalAllocationNotificationFunc(
	void					*	pUserData,
	size_t						size,
	VkInternalAllocationType	allocationType,
	VkSystemAllocationScope		allocationScope )
{
#if BUILD_VULKAN_MEMORY_ALLOCATOR_TYPE == 2

#endif
}

void VulkanMemoryInternalFreeNotificationFunc(
	void					*	pUserData,
	size_t						size,
	VkInternalAllocationType	allocationType,
	VkSystemAllocationScope		allocationScope )
{
#if BUILD_VULKAN_MEMORY_ALLOCATOR_TYPE == 2

#endif
}

VkAllocationCallbacks vulkan_allocation_callbacks {
	nullptr,
	VulkanMemoryAllocationFunc,
	VulkanMemoryReallocationFunc,
	VulkanMemoryFreeFunc,
	VulkanMemoryInternalAllocationNotificationFunc,
	VulkanMemoryInternalFreeNotificationFunc
};
#endif


void VulkanResultCheckLoggerFunction( String msg )
{
	TODO( "Logger should report this instead of printing to the console" );
	std::cout << "Abnormal Vulkan result: " << msg << std::endl;
}

String VulkanToString( VkResult result )
{
	switch( result ) {
	case VK_SUCCESS:
		return "VK_SUCCESS";
	case VK_NOT_READY:
		return "VK_NOT_READY";
	case VK_TIMEOUT:
		return "VK_TIMEOUT";
	case VK_EVENT_SET:
		return "VK_EVENT_SET";
	case VK_EVENT_RESET:
		return "VK_EVENT_RESET";
	case VK_INCOMPLETE:
		return "VK_INCOMPLETE";
	case VK_ERROR_OUT_OF_HOST_MEMORY:
		return "VK_ERROR_OUT_OF_HOST_MEMORY";
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
	case VK_ERROR_INITIALIZATION_FAILED:
		return "VK_ERROR_INITIALIZATION_FAILED";
	case VK_ERROR_DEVICE_LOST:
		return "VK_ERROR_DEVICE_LOST";
	case VK_ERROR_MEMORY_MAP_FAILED:
		return "VK_ERROR_MEMORY_MAP_FAILED";
	case VK_ERROR_LAYER_NOT_PRESENT:
		return "VK_ERROR_LAYER_NOT_PRESENT";
	case VK_ERROR_EXTENSION_NOT_PRESENT:
		return "VK_ERROR_EXTENSION_NOT_PRESENT";
	case VK_ERROR_FEATURE_NOT_PRESENT:
		return "VK_ERROR_FEATURE_NOT_PRESENT";
	case VK_ERROR_INCOMPATIBLE_DRIVER:
		return "VK_ERROR_INCOMPATIBLE_DRIVER";
	case VK_ERROR_TOO_MANY_OBJECTS:
		return "VK_ERROR_TOO_MANY_OBJECTS";
	case VK_ERROR_FORMAT_NOT_SUPPORTED:
		return "VK_ERROR_FORMAT_NOT_SUPPORTED";
	case VK_ERROR_FRAGMENTED_POOL:
		return "VK_ERROR_FRAGMENTED_POOL";
	case VK_ERROR_SURFACE_LOST_KHR:
		return "VK_ERROR_SURFACE_LOST_KHR";
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
		return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
	case VK_SUBOPTIMAL_KHR:
		return "VK_SUBOPTIMAL_KHR";
	case VK_ERROR_OUT_OF_DATE_KHR:
		return "VK_ERROR_OUT_OF_DATE_KHR";
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
		return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
	case VK_ERROR_VALIDATION_FAILED_EXT:
		return "VK_ERROR_VALIDATION_FAILED_EXT";
	case VK_ERROR_INVALID_SHADER_NV:
		return "VK_ERROR_INVALID_SHADER_NV";
	case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
		return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
	case VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR:
		return "VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR";
	default:
		return "<unknown vulkan result>";
	}
}

VkResult VulkanResultCheck( VkResult result )
{
#if BUILD_ENABLE_VULKAN_RESULT_CHECK_LOGGING_WARNINGS || BUILD_ENABLE_VULKAN_RESULT_CHECK_LOGGING_ERRORS
	switch( result ) {
#if BUILD_ENABLE_VULKAN_RESULT_CHECK_LOGGING_WARNINGS
	case VK_NOT_READY:
		VulkanResultCheckLoggerFunction( "VK_NOT_READY" );
		break;
	case VK_TIMEOUT:
		VulkanResultCheckLoggerFunction( "VK_TIMEOUT" );
		break;
	case VK_EVENT_SET:
		VulkanResultCheckLoggerFunction( "VK_EVENT_SET" );
		break;
	case VK_EVENT_RESET:
		VulkanResultCheckLoggerFunction( "VK_EVENT_RESET" );
		break;
	case VK_INCOMPLETE:
		VulkanResultCheckLoggerFunction( "VK_INCOMPLETE" );
		break;
	case VK_SUBOPTIMAL_KHR:
		VulkanResultCheckLoggerFunction( "VK_SUBOPTIMAL_KHR" );
		break;
#endif

#if BUILD_ENABLE_VULKAN_RESULT_CHECK_LOGGING_ERRORS
	case VK_ERROR_OUT_OF_HOST_MEMORY:
		VulkanResultCheckLoggerFunction( "VK_ERROR_OUT_OF_HOST_MEMORY" );
		break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		VulkanResultCheckLoggerFunction( "VK_ERROR_OUT_OF_DEVICE_MEMORY" );
		break;
	case VK_ERROR_INITIALIZATION_FAILED:
		VulkanResultCheckLoggerFunction( "VK_ERROR_INITIALIZATION_FAILED" );
		break;
	case VK_ERROR_DEVICE_LOST:
		VulkanResultCheckLoggerFunction( "VK_ERROR_DEVICE_LOST" );
		break;
	case VK_ERROR_MEMORY_MAP_FAILED:
		VulkanResultCheckLoggerFunction( "VK_ERROR_MEMORY_MAP_FAILED" );
		break;
	case VK_ERROR_LAYER_NOT_PRESENT:
		VulkanResultCheckLoggerFunction( "VK_ERROR_LAYER_NOT_PRESENT" );
		break;
	case VK_ERROR_EXTENSION_NOT_PRESENT:
		VulkanResultCheckLoggerFunction( "VK_ERROR_EXTENSION_NOT_PRESENT" );
		break;
	case VK_ERROR_FEATURE_NOT_PRESENT:
		VulkanResultCheckLoggerFunction( "VK_ERROR_FEATURE_NOT_PRESENT" );
		break;
	case VK_ERROR_INCOMPATIBLE_DRIVER:
		VulkanResultCheckLoggerFunction( "VK_ERROR_INCOMPATIBLE_DRIVER" );
		break;
	case VK_ERROR_TOO_MANY_OBJECTS:
		VulkanResultCheckLoggerFunction( "VK_ERROR_TOO_MANY_OBJECTS" );
		break;
	case VK_ERROR_FORMAT_NOT_SUPPORTED:
		VulkanResultCheckLoggerFunction( "VK_ERROR_FORMAT_NOT_SUPPORTED" );
		break;
	case VK_ERROR_FRAGMENTED_POOL:
		VulkanResultCheckLoggerFunction( "VK_ERROR_FRAGMENTED_POOL" );
		break;
	case VK_ERROR_SURFACE_LOST_KHR:
		VulkanResultCheckLoggerFunction( "VK_ERROR_SURFACE_LOST_KHR" );
		break;
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
		VulkanResultCheckLoggerFunction( "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" );
		break;
	case VK_ERROR_OUT_OF_DATE_KHR:
		VulkanResultCheckLoggerFunction( "VK_ERROR_OUT_OF_DATE_KHR" );
		break;
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
		VulkanResultCheckLoggerFunction( "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" );
		break;
	case VK_ERROR_VALIDATION_FAILED_EXT:
		VulkanResultCheckLoggerFunction( "VK_ERROR_VALIDATION_FAILED_EXT" );
		break;
	case VK_ERROR_INVALID_SHADER_NV:
		VulkanResultCheckLoggerFunction( "VK_ERROR_INVALID_SHADER_NV" );
		break;
	case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
		VulkanResultCheckLoggerFunction( "VK_ERROR_OUT_OF_POOL_MEMORY_KHR" );
		break;
	case VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR:
		VulkanResultCheckLoggerFunction( "VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR" );
		break;
#endif
	default:
		break;
	}
#endif

#if BUILD_ENABLE_VULKAN_RESULT_CHECK_ASSERTION_WARNINGS
	if( result > 0 ) {
		assert( 0 && "Vulkan result check detected a warning" );
	}
#endif

#if BUILD_ENABLE_VULKAN_RESULT_CHECK_ASSERTION_ERRORS
	if( result < 0 ) {
		assert( 0 && "Vulkan result check detected an error" );
	}
#endif

	return result;
}

}
