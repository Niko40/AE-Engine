
#include <iostream>
#include <sstream>
#include <assert.h>

#include "../Memory/MemoryTypes.h"
#include "../Memory/Memory.h"
#include "Renderer.h"
#include "DeviceMemory/DeviceMemoryManager.h"
#include "DeviceResource/DeviceResourceManager.h"
#include "../Logger/Logger.h"
#include "../Window/WindowManager.h"
#include "../Renderer/DescriptorSet/DescriptorPoolManager.h"
#include "Buffer/GBuffer.h"


namespace AE
{

#if BUILD_ENABLE_VULKAN_DEBUG

VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugReportCallback(
	VkDebugReportFlagsEXT		flags,
	VkDebugReportObjectTypeEXT	obj_type,
	uint64_t					src_obj,
	size_t						location,
	int32_t						msg_code,
	const char *				layer_prefix,
	const char *				msg,
	Logger *					logger
)
{
	std::ostringstream stream;
	stream << "VULKAN DEBUG: ";
	if( flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) {
		stream << "INFO: ";
	}
	if( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT ) {
		stream << "WARNING: ";
	}
	if( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ) {
		stream << "PERFORMANCE: ";
	}
	if( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {
		stream << "ERROR: ";
	}
	if( flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ) {
		stream << "DEBUG: ";
	}
	stream << "@[" << layer_prefix << "]: ";
	stream << msg << std::endl;
	std::cout << stream.str();
	if( nullptr != logger ) {
		auto log = stream.str();
		logger->LogVulkan( log.substr( 0, log.end() - log.begin() - 1 ) );
	}

#if defined( _WIN32 )
	if( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {
		engine_internal::WinMessageBox( stream.str().c_str(), "Vulkan Error!" );
	}
#endif

	return false;
}

PFN_vkCreateDebugReportCallbackEXT		fvkCreateDebugReportCallbackEXT		= nullptr;
PFN_vkDestroyDebugReportCallbackEXT		fvkDestroyDebugReportCallbackEXT	= nullptr;

void Renderer::SetupDebugReporting()
{
	debug_report_callback_create_info.sType			= VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	debug_report_callback_create_info.pfnCallback	= PFN_vkDebugReportCallbackEXT( VulkanDebugReportCallback );
	debug_report_callback_create_info.pUserData		= p_logger;
	debug_report_callback_create_info.flags			=
//		VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT |
//		VK_DEBUG_REPORT_DEBUG_BIT_EXT |
		0;

	instance_layer_names.push_back( "VK_LAYER_LUNARG_standard_validation" );
	instance_extension_names.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
}

void Renderer::CreateDebugReporting()
{
	p_logger->LogInfo( "Vulkan debug reporting starting" );

	fvkCreateDebugReportCallbackEXT		= (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr( vk_instance, "vkCreateDebugReportCallbackEXT" );
	fvkDestroyDebugReportCallbackEXT	= (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr( vk_instance, "vkDestroyDebugReportCallbackEXT" );
	if( nullptr == fvkCreateDebugReportCallbackEXT || nullptr == fvkDestroyDebugReportCallbackEXT ) {
		assert( 0 && "Vulkan ERROR: Can't fetch debug function pointers." );
		std::exit( -1 );
	}

	fvkCreateDebugReportCallbackEXT( vk_instance, &debug_report_callback_create_info, nullptr, &debug_report_callback );

	p_logger->LogInfo( "Vulkan debug reporting started" );
}

void Renderer::DestroyDebugReporting()
{
	fvkDestroyDebugReportCallbackEXT( vk_instance, debug_report_callback, nullptr );
	debug_report_callback = VK_NULL_HANDLE;

	p_logger->LogInfo( "Vulkan debug reporting stopped" );
}

#else

void Renderer::SetupDebugReporting() {};
void Renderer::CreateDebugReporting() {};
void Renderer::DestroyDebugReporting() {};

#endif


Renderer::Renderer( Engine * engine, std::string application_name, uint32_t application_version, VkExtent2D resolution, bool fullscreen )
	: SubSystem( engine, "Renderer" )
{
	glfwInit();

	VkApplicationInfo application_info {};
	application_info.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.pNext				= nullptr;
	application_info.pApplicationName	= application_name.c_str();
	application_info.applicationVersion	= application_version;
	application_info.pEngineName		= BUILD_ENGINE_NAME;
	application_info.engineVersion		= BUILD_ENGINE_VERSION;
	application_info.apiVersion			= BUILD_VULKAN_VERSION;

	SetupRequiredLayersAndExtensions();
	SetupDebugReporting();
	CreateInstance();
	CreateDebugReporting();
	SelectPhysicalDevices();
	FindQueueFamilies();
	CreateDevice();
	GetQueueHandles();
	CreateDescriptorSetLayouts();
	CreateGraphicsPipelineLayouts();

	descriptor_pool_manager		= MakeUniquePointer<DescriptorPoolManager>( p_engine, this );
	device_memory_manager		= MakeUniquePointer<DeviceMemoryManager>( p_engine, this );
	device_resource_manager		= MakeUniquePointer<DeviceResourceManager>( p_engine, this, device_memory_manager.Get() );

	window_manager				= MakeUniquePointer<WindowManager>( p_engine, this );
	window_manager->OpenWindow( resolution, application_name, fullscreen );
	window_manager->CreateWindowSurface();
	window_manager->CreateSwapchain();
	window_manager->CreateSwapchainImageViews();
	render_resolution			= window_manager->GetVulkanSurfaceCapabilities().currentExtent;
	FindDepthStencilFormat();
	CreateGBuffers();
	CreateRenderPass();
	CreateWindowFramebuffers();

	device_resource_manager->AllowResourceRequests( true );
	device_resource_manager->AllowResourceLoading( true );
	device_resource_manager->AllowResourceUnloading( true );
}

Renderer::~Renderer()
{
	device_resource_manager->WaitJobless();

	device_resource_manager->AllowResourceRequests( false );
	device_resource_manager->AllowResourceLoading( false );

	DestroyWindowFramebuffers();
	DestroyRenderPass();
	DestroyGBuffers();
	window_manager->DestroySwapchainImageViews();
	window_manager->DestroySwapchain();
	window_manager->DestroyWindowSurface();
	window_manager->CloseWindow();

	device_resource_manager		= nullptr;
	device_memory_manager		= nullptr;
	descriptor_pool_manager		= nullptr;
	window_manager				= nullptr;

	DestroyGraphicsPipelineLayouts();
	DestroyDescriptorSetLayouts();
	DestroyDevice();
	DestroyDebugReporting();
	DestroyInstance();

	glfwTerminate();
}

bool Renderer::Update()
{
	device_resource_manager->Update();
	return window_manager->Update();
}

VkInstance Renderer::GetVulkanInstance() const
{
	return vk_instance;
}

VkPhysicalDevice Renderer::GetVulkanPhysicalDevice() const
{
	return vk_physical_device;
}

VulkanDevice Renderer::GetVulkanDevice() const
{
	return vk_device;
}

VkRenderPass Renderer::GetVulkanRenderPass() const
{
	return vk_render_pass;
}

DeviceMemoryManager * Renderer::GetDeviceMemoryManager()
{
	return device_memory_manager.Get();
}

DeviceResourceManager * Renderer::GetDeviceResourceManager()
{
	return device_resource_manager.Get();
}

WindowManager * Renderer::GetWindowManager()
{
	return window_manager.Get();
}

uint32_t Renderer::GetPrimaryRenderQueueFamilyIndex() const
{
	return primary_render_queue_family_index;
}

uint32_t Renderer::GetSecondaryRenderQueueFamilyIndex() const
{
	return secondary_render_queue_family_index;
}

uint32_t Renderer::GetPrimaryTransferQueueFamilyIndex() const
{
	return primary_transfer_queue_family_index;
}

VulkanQueue Renderer::GetPrimaryRenderQueue() const
{
	return vk_primary_render_queue;
}

VulkanQueue Renderer::GetSecondaryRenderQueue() const
{
	return vk_secondary_render_queue;
}

VulkanQueue Renderer::GetPrimaryTransferQueue() const
{
	return vk_primary_transfer_queue;
}

SharingModeInfo Renderer::GetSharingModeInfo( UsedQueuesFlags used_queues )
{
	SharingModeInfo ret;
	if( uint32_t( used_queues & UsedQueuesFlags::PRIMARY_RENDER ) ) {
		ret.shared_queue_family_indices.push_back( primary_render_queue_family_index );
	}
	if( uint32_t( used_queues & UsedQueuesFlags::SECONDARY_RENDER ) ) {
		ret.shared_queue_family_indices.push_back( secondary_render_queue_family_index );
	}
	if( uint32_t( used_queues & UsedQueuesFlags::PRIMARY_TRANSFER ) ) {
		ret.shared_queue_family_indices.push_back( primary_transfer_queue_family_index );
	}
	// remove doubles
	ret.shared_queue_family_indices.erase( std::unique( ret.shared_queue_family_indices.begin(), ret.shared_queue_family_indices.end() ), ret.shared_queue_family_indices.end() );
	if( ret.shared_queue_family_indices.size() > 1 ) {
		ret.sharing_mode	= VK_SHARING_MODE_CONCURRENT;
	} else {
		ret.sharing_mode	= VK_SHARING_MODE_EXCLUSIVE;
	}
	return ret;
}

const VkPhysicalDeviceProperties & Renderer::GetPhysicalDeviceProperties() const
{
	return physical_device_properties;
}

const VkPhysicalDeviceFeatures & Renderer::GetPhysicalDeviceFeatures() const
{
	return physical_device_features;
}

const VkPhysicalDeviceLimits & Renderer::GetPhysicalDeviceLimits() const
{
	return physical_device_limits;
}

VkPipelineLayout Renderer::GetVulkanGraphicsPipelineLayout( uint32_t supported_image_count ) const
{
	assert( supported_image_count <= BUILD_MAX_PER_SHADER_SAMPLED_IMAGE_COUNT );
	return vk_graphics_pipeline_layouts[ supported_image_count ];
}

VkDescriptorSetLayout Renderer::GetVulkanDescriptorSetLayoutForCamera() const
{
	return vk_descriptor_set_layout_for_camera;
}

VkDescriptorSetLayout Renderer::GetVulkanDescriptorSetLayoutForMesh() const
{
	return vk_descriptor_set_layout_for_mesh;
}

VkDescriptorSetLayout Renderer::GetVulkanDescriptorSetLayoutForPipeline() const
{
	return vk_descriptor_set_layout_for_pipeline;
}

VkDescriptorSetLayout Renderer::GetVulkanDescriptorSetLayoutForImageBindingCount( uint32_t image_binding_count ) const
{
	return vk_descriptor_set_layouts_for_images[ image_binding_count ];
}

DescriptorPoolManager * Renderer::GetDescriptorPoolManager()
{
	return descriptor_pool_manager.Get();

	// Descriptor pool manager per thread approach was cancelled for now because of unnecessary
	// complexity at this point, might consider this in the future though
	/*
	auto p_iter		= descriptor_pools.find( thread_id );
	if( p_iter != descriptor_pools.end() ) {
		// found, return this pool
		return p_iter->second.Get();
	} else {
		// not found, create new and return that
		auto unique	= MakeUniquePointer<DescriptorPoolManager>( p_engine, this );
		if( unique ) {
			auto ptr = unique.Get();
			descriptor_pools.insert( std::pair<std::thread::id, UniquePointer<DescriptorPoolManager>>( thread_id, std::move( unique ) ) );
			return ptr;
		} else {
			std::stringstream ss;
			ss << "Can't create new descriptor pool manager for thread: "
				<< thread_id;
			p_logger->LogCritical( ss.str().c_str() );
		}
	}
	return nullptr;
	*/
}

bool Renderer::IsFormatSupported( VkImageTiling tiling, VkFormat format, VkFormatFeatureFlags feature_flags )
{
	VkFormatProperties fp {};
	vkGetPhysicalDeviceFormatProperties( vk_physical_device, format, &fp );
	switch( tiling ) {
	case VK_IMAGE_TILING_OPTIMAL:
		return ( ( fp.optimalTilingFeatures & feature_flags ) == feature_flags );
	case VK_IMAGE_TILING_LINEAR:
		return ( ( fp.linearTilingFeatures & feature_flags ) == feature_flags );
	default:
		p_logger->LogWarning( "Enum error: FileResource_Image tiling can only be vk::ImageTiling::eOptimal or vk::ImageTiling::eLinear" );
		return false;
	}
}

void Renderer::SetupRequiredLayersAndExtensions()
{
	// Get reguired WSI extension names directly from GLFW "VK_KHR_surface"
	// is always included, others included are system specific, like
	// VK_KHR_WIN32_SURFACE_EXTENSION_NAME ("VK_KHR_win32_surface") on Windows OS
	{
		uint32_t glfw_extension_count	= 0;
		auto glfw_extensions			= glfwGetRequiredInstanceExtensions( &glfw_extension_count );
		if( glfw_extension_count <= 0 ) {
			p_logger->LogCritical( "glfwGetRequiredInstanceExtensions failed, check GLFW initialized" );
		}
		for( uint32_t i=0; i < glfw_extension_count; ++i ) {
			instance_extension_names.push_back( glfw_extensions[ i ] );
		}
	}

	device_extension_names.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
}

void Renderer::CreateInstance()
{
	VkInstanceCreateInfo instance_CI {};
	instance_CI.sType					= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_CI.pNext					= &debug_report_callback_create_info;
	instance_CI.flags					= 0;
	instance_CI.pApplicationInfo		= &application_info;
	instance_CI.enabledLayerCount		= uint32_t( instance_layer_names.size() );
	instance_CI.ppEnabledLayerNames		= instance_layer_names.data();
	instance_CI.enabledExtensionCount	= uint32_t( instance_extension_names.size() );
	instance_CI.ppEnabledExtensionNames	= instance_extension_names.data();

	VulkanResultCheck( vkCreateInstance( &instance_CI, VULKAN_ALLOC, &vk_instance ) );
	assert( vk_instance );
}

void Renderer::DestroyInstance()
{
	vkDestroyInstance( vk_instance, VULKAN_ALLOC );
	vk_instance = VK_NULL_HANDLE;
}

uint32_t RatePhysicalDeviceSuitability( VkPhysicalDevice physical_device )
{
	uint32_t score						= 0;

	VkPhysicalDeviceProperties properties {};
//	VkPhysicalDeviceMemoryProperties memory_properties {};
	vkGetPhysicalDeviceProperties( physical_device, &properties );
//	vkGetPhysicalDeviceMemoryProperties( physical_device, &memory_properties );

	// rate by type
	if( properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) {
		score += 4000;
	}
	if( properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ) {
		score += 1000;
	}

	// rate by maximum image size
	score += properties.limits.maxImageDimension2D;

	return score;
}

void Renderer::SelectPhysicalDevices()
{
	// TODO: Multi GPU support
	TODO( "Add multi GPU support" );

	uint32_t physical_device_count		= 0;
	VulkanResultCheck( vkEnumeratePhysicalDevices( vk_instance, &physical_device_count, nullptr ) );
	Vector<VkPhysicalDevice> physical_devices( physical_device_count );
	VulkanResultCheck( vkEnumeratePhysicalDevices( vk_instance, &physical_device_count, physical_devices.data() ) );

	Vector<uint32_t>		physical_device_scores( physical_devices.size() );
	for( size_t i=0; i < physical_devices.size(); ++i ) {
		physical_device_scores[ i ]		= RatePhysicalDeviceSuitability( physical_devices[ i ] );
	}

	uint32_t highest_score = 0;
	for( size_t i=0; i < physical_device_scores.size(); ++i ) {
		if( physical_device_scores[ i ] > highest_score ) {
			highest_score		= physical_device_scores[ i ];
			vk_physical_device	= physical_devices[ i ];
		}
	}
	if( !vk_physical_device ) {
		p_logger->LogCritical( "Physical device supporting Vulkan not found" );
	}

	vkGetPhysicalDeviceProperties( vk_physical_device, &physical_device_properties );
	vkGetPhysicalDeviceFeatures( vk_physical_device, &physical_device_features );
	physical_device_limits						= physical_device_properties.limits;
}

uint32_t CountQueueFamilyFlags( VkQueueFamilyProperties & fp )
{
	uint32_t ret = 0;
	for( uint32_t i=0; i < 8; ++i ) {
		ret += ( uint32_t( fp.queueFlags ) >> i ) & 1;
	}
	return ret;
}

float queue_priorities[] {
	BUILD_VULKAN_PRIMARY_RENDER_QUEUE_PRIORITY,
	BUILD_VULKAN_SECONDARY_RENDER_QUEUE_PRIORITY,
	BUILD_VULKAN_PRIMARY_TRANSFER_QUEUE_PRIORITY,
};

float queue_priorities_2[] {
	BUILD_VULKAN_PRIMARY_RENDER_QUEUE_PRIORITY,
	BUILD_VULKAN_PRIMARY_TRANSFER_QUEUE_PRIORITY,
	BUILD_VULKAN_SECONDARY_RENDER_QUEUE_PRIORITY,
};

void Renderer::FindQueueFamilies()
{
	// this engine uses 3 queues, one to draw visible graphics, one to draw
	// non-visible graphics like image scaling, and one to load stuff on the GPU
	// we prefer to use dedicated queue families for all queues if available

	uint32_t queue_family_property_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( vk_physical_device, &queue_family_property_count, nullptr );
	Vector<VkQueueFamilyProperties> queue_family_properties( queue_family_property_count );
	vkGetPhysicalDeviceQueueFamilyProperties( vk_physical_device, &queue_family_property_count, queue_family_properties.data() );

	Vector<Pair<uint32_t, VkQueueFamilyProperties*>>	render_families;
	Vector<Pair<uint32_t, VkQueueFamilyProperties*>>	transfer_families;

	auto primary_render_family		= Pair<uint32_t, VkQueueFamilyProperties*>( UINT32_MAX, nullptr );
	auto secondary_render_family	= Pair<uint32_t, VkQueueFamilyProperties*>( UINT32_MAX, nullptr );
	auto primary_transfer_family	= Pair<uint32_t, VkQueueFamilyProperties*>( UINT32_MAX, nullptr );

	for( size_t i=0; i < queue_family_properties.size(); ++i ) {
		auto & f = queue_family_properties[ i ];
		if( f.queueFlags & VK_QUEUE_GRAPHICS_BIT )	render_families.push_back( Pair<uint32_t, VkQueueFamilyProperties*>( uint32_t( i ), &f ) );
		if( f.queueFlags & VK_QUEUE_TRANSFER_BIT )	transfer_families.push_back( Pair<uint32_t, VkQueueFamilyProperties*>( uint32_t( i ), &f ) );
	}
	{
		// Find all queue families that we can use for primary render ops
		uint32_t	score	= UINT32_MAX;
		for( auto & f : render_families ) {
			uint32_t flag_count					= CountQueueFamilyFlags( *f.second );
			int32_t presentation_support		= glfwGetPhysicalDevicePresentationSupport( vk_instance, vk_physical_device, f.first );
			// presentation must be supported for the primary render queue family
			if( f.second->queueCount && flag_count < score && presentation_support ) {
				score							= flag_count;
				primary_render_family			= f;
			}
		}
		if( primary_render_family.first > UINT32_MAX ) {
			queue_family_properties[ primary_render_family.first ].queueCount--;
		}
	}
	{
		// Find all queue families that we can use for worker render ops
		uint32_t	score	= UINT32_MAX;
		for( auto & f : render_families ) {
			uint32_t flag_count					= CountQueueFamilyFlags( *f.second );
			if( f.second->queueCount && flag_count < score ) {
				score							= flag_count;
				secondary_render_family			= f;
			}
		}
		if( secondary_render_family.first != UINT32_MAX ) {
			queue_family_properties[ secondary_render_family.first ].queueCount--;
		}
	}
	{
		uint32_t score							= UINT32_MAX;
		for( auto & f : transfer_families ) {
			uint32_t flag_count					= CountQueueFamilyFlags( *f.second );
			if( f.second->queueCount && flag_count < score ) {
				score							= flag_count;
				primary_transfer_family			= f;
			}
		}
		if( primary_transfer_family.first != UINT32_MAX ) {
			queue_family_properties[ primary_transfer_family.first ].queueCount--;
		}
	}

	primary_render_queue_family_index							= primary_render_family.first;
	secondary_render_queue_family_index							= secondary_render_family.first;
	primary_transfer_queue_family_index							= primary_transfer_family.first;

	if( primary_render_queue_family_index == UINT32_MAX ) {
		p_logger->LogCritical( "Couldn't find a compatible primary render queue family" );
	}

	if( secondary_render_queue_family_index == UINT32_MAX &&
		primary_transfer_queue_family_index == UINT32_MAX ) {
		// only primary render queue exists
		queue_availability		= QueueAvailability::F1_PR1;

	} else if( primary_render_queue_family_index == secondary_render_queue_family_index &&
		primary_transfer_queue_family_index == UINT32_MAX ) {
		queue_availability		= QueueAvailability::F1_PR1_SR1;

	} else if( primary_render_queue_family_index == primary_transfer_queue_family_index &&
		secondary_render_queue_family_index == UINT32_MAX ) {
		queue_availability		= QueueAvailability::F1_PR1_PT1;


	} else if( primary_render_queue_family_index != secondary_render_queue_family_index &&
		primary_transfer_queue_family_index == UINT32_MAX ) {
		queue_availability		= QueueAvailability::F2_PR1_SR2;

	} else if( primary_render_queue_family_index != primary_transfer_queue_family_index &&
		secondary_render_queue_family_index == UINT32_MAX ) {
		queue_availability		= QueueAvailability::F2_PR1_PT2;


	} else if( primary_render_queue_family_index != secondary_render_queue_family_index &&
		secondary_render_queue_family_index == primary_transfer_queue_family_index ) {
		queue_availability		= QueueAvailability::F2_PR1_SR2_PT2;

	} else if( primary_render_queue_family_index == secondary_render_queue_family_index &&
		secondary_render_queue_family_index != primary_transfer_queue_family_index ) {
		queue_availability		= QueueAvailability::F2_PR1_SR1_PT2;

	} else if( primary_render_queue_family_index == primary_transfer_queue_family_index &&
		secondary_render_queue_family_index != primary_transfer_queue_family_index ) {
		queue_availability		= QueueAvailability::F2_PR1_SR2_PT1;


	} else if( primary_render_queue_family_index != secondary_render_queue_family_index &&
		secondary_render_queue_family_index != primary_transfer_queue_family_index &&
		primary_render_queue_family_index != primary_transfer_queue_family_index ) {
		queue_availability		= QueueAvailability::F3_PR1_SR2_PT3;

	}

	switch( queue_availability ) {
	case QueueAvailability::F3_PR1_SR2_PT3:
		device_queue_create_infos.resize( 3 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 1;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities[ 0 ];
		device_queue_create_infos[ 1 ].queueFamilyIndex			= secondary_render_queue_family_index;
		device_queue_create_infos[ 1 ].queueCount				= 1;
		device_queue_create_infos[ 1 ].pQueuePriorities			= &queue_priorities[ 1 ];
		device_queue_create_infos[ 2 ].queueFamilyIndex			= primary_transfer_queue_family_index;
		device_queue_create_infos[ 2 ].queueCount				= 1;
		device_queue_create_infos[ 2 ].pQueuePriorities			= &queue_priorities[ 2 ];
		break;
	case QueueAvailability::F2_PR1_SR1_PT2:
		device_queue_create_infos.resize( 2 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 2;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities[ 0 ];
		device_queue_create_infos[ 1 ].queueFamilyIndex			= primary_transfer_queue_family_index;
		device_queue_create_infos[ 1 ].queueCount				= 1;
		device_queue_create_infos[ 1 ].pQueuePriorities			= &queue_priorities[ 2 ];
		break;
	case QueueAvailability::F2_PR1_SR2_PT2:
		device_queue_create_infos.resize( 2 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 1;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities[ 0 ];
		device_queue_create_infos[ 1 ].queueFamilyIndex			= secondary_render_queue_family_index;
		device_queue_create_infos[ 1 ].queueCount				= 2;
		device_queue_create_infos[ 1 ].pQueuePriorities			= &queue_priorities[ 1 ];
		break;
	case QueueAvailability::F2_PR1_SR2_PT1:
		device_queue_create_infos.resize( 2 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 2;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities_2[ 0 ];
		device_queue_create_infos[ 1 ].queueFamilyIndex			= secondary_render_queue_family_index;
		device_queue_create_infos[ 1 ].queueCount				= 1;
		device_queue_create_infos[ 1 ].pQueuePriorities			= &queue_priorities_2[ 1 ];
		break;
	case QueueAvailability::F2_PR1_SR2:
		device_queue_create_infos.resize( 2 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 1;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities[ 0 ];
		device_queue_create_infos[ 1 ].queueFamilyIndex			= secondary_render_queue_family_index;
		device_queue_create_infos[ 1 ].queueCount				= 1;
		device_queue_create_infos[ 1 ].pQueuePriorities			= &queue_priorities[ 1 ];
		break;
	case QueueAvailability::F2_PR1_PT2:
		device_queue_create_infos.resize( 2 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 1;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities_2[ 0 ];
		device_queue_create_infos[ 1 ].queueFamilyIndex			= primary_transfer_queue_family_index;
		device_queue_create_infos[ 1 ].queueCount				= 1;
		device_queue_create_infos[ 1 ].pQueuePriorities			= &queue_priorities_2[ 1 ];
		break;
	case QueueAvailability::F1_PR1_SR1_PT1:
		device_queue_create_infos.resize( 1 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 3;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities[ 0 ];
		break;
	case QueueAvailability::F1_PR1_PT1:
		device_queue_create_infos.resize( 1 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 2;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities_2[ 0 ];
		break;
	case QueueAvailability::F1_PR1_SR1:
		device_queue_create_infos.resize( 1 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 2;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities[ 0 ];
		break;
	case QueueAvailability::F1_PR1:
		device_queue_create_infos.resize( 1 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 1;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities[ 0 ];
		break;
	default:
		p_logger->LogCritical( "Couldn't resolve the queue allocation scheme" );
		break;
	}

	// fill in common fields
	for( auto & i : device_queue_create_infos ) {
		i.sType		= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		i.pNext		= nullptr;
		i.flags		= 0;
	}
}

void Renderer::CreateDevice()
{
	VkPhysicalDeviceFeatures features {};

	VkDeviceCreateInfo device_CI {};
	device_CI.sType						= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_CI.pNext						= nullptr;
	device_CI.flags						= 0;
	device_CI.queueCreateInfoCount		= uint32_t( device_queue_create_infos.size() );
	device_CI.pQueueCreateInfos			= device_queue_create_infos.data();
	device_CI.enabledLayerCount			= 0;		// Depricated
	device_CI.ppEnabledLayerNames		= nullptr;	// Depricated
	device_CI.enabledExtensionCount		= uint32_t( device_extension_names.size() );
	device_CI.ppEnabledExtensionNames	= device_extension_names.data();
	device_CI.pEnabledFeatures			= &features;

	VulkanResultCheck( vkCreateDevice( vk_physical_device, &device_CI, VULKAN_ALLOC, &vk_device.object ) );
	vk_device.mutex						= &device_mutex;
}

void Renderer::DestroyDevice()
{
	vkDestroyDevice( vk_device.object, VULKAN_ALLOC );
	vk_device.object = VK_NULL_HANDLE;
}

void Renderer::GetQueueHandles()
{
	switch( queue_availability ) {
	case QueueAvailability::F3_PR1_SR2_PT3:
		vkGetDeviceQueue( vk_device.object, primary_render_queue_family_index, 0, &vk_primary_render_queue.object );
		vkGetDeviceQueue( vk_device.object, secondary_render_queue_family_index, 0, &vk_secondary_render_queue.object );
		vkGetDeviceQueue( vk_device.object, primary_transfer_queue_family_index, 0, &vk_primary_transfer_queue.object );
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &secondary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &primary_transfer_queue_mutex;
		break;
	case QueueAvailability::F2_PR1_SR1_PT2:
		vkGetDeviceQueue( vk_device.object, primary_render_queue_family_index, 0, &vk_primary_render_queue.object );
		vkGetDeviceQueue( vk_device.object, secondary_render_queue_family_index, 1, &vk_secondary_render_queue.object );
		vkGetDeviceQueue( vk_device.object, primary_transfer_queue_family_index, 0, &vk_primary_transfer_queue.object );
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &secondary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &primary_transfer_queue_mutex;
		break;
	case QueueAvailability::F2_PR1_SR2_PT2:
		vkGetDeviceQueue( vk_device.object, primary_render_queue_family_index, 0, &vk_primary_render_queue.object );
		vkGetDeviceQueue( vk_device.object, secondary_render_queue_family_index, 0, &vk_secondary_render_queue.object );
		vkGetDeviceQueue( vk_device.object, primary_transfer_queue_family_index, 1, &vk_primary_transfer_queue.object );
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &secondary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &primary_transfer_queue_mutex;
		break;
	case QueueAvailability::F2_PR1_SR2_PT1:
		vkGetDeviceQueue( vk_device.object, primary_render_queue_family_index, 0, &vk_primary_render_queue.object );
		vkGetDeviceQueue( vk_device.object, secondary_render_queue_family_index, 0, &vk_secondary_render_queue.object );
		vkGetDeviceQueue( vk_device.object, primary_transfer_queue_family_index, 1, &vk_primary_transfer_queue.object );
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &secondary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &primary_transfer_queue_mutex;
		break;
	case QueueAvailability::F2_PR1_SR2:
		vkGetDeviceQueue( vk_device.object, primary_render_queue_family_index, 0, &vk_primary_render_queue.object );
		vkGetDeviceQueue( vk_device.object, secondary_render_queue_family_index, 0, &vk_secondary_render_queue.object );
		vk_primary_transfer_queue.object	= vk_secondary_render_queue.object;
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &secondary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &secondary_render_queue_mutex;
		break;
	case QueueAvailability::F2_PR1_PT2:
		vkGetDeviceQueue( vk_device.object, primary_render_queue_family_index, 0, &vk_primary_render_queue.object );
		vk_secondary_render_queue.object	= vk_primary_render_queue.object;
		vkGetDeviceQueue( vk_device.object, primary_transfer_queue_family_index, 0, &vk_primary_transfer_queue.object );
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &primary_transfer_queue_mutex;
		break;
	case QueueAvailability::F1_PR1_SR1_PT1:
		vkGetDeviceQueue( vk_device.object, primary_render_queue_family_index, 0, &vk_primary_render_queue.object );
		vkGetDeviceQueue( vk_device.object, secondary_render_queue_family_index, 1, &vk_secondary_render_queue.object );
		vkGetDeviceQueue( vk_device.object, primary_transfer_queue_family_index, 2, &vk_primary_transfer_queue.object );
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &secondary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &primary_transfer_queue_mutex;
		break;
	case QueueAvailability::F1_PR1_PT1:
		vkGetDeviceQueue( vk_device.object, primary_render_queue_family_index, 0, &vk_primary_render_queue.object );
		vk_secondary_render_queue.object	= vk_primary_render_queue.object;
		vkGetDeviceQueue( vk_device.object, primary_transfer_queue_family_index, 1, &vk_primary_transfer_queue.object );
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &primary_transfer_queue_mutex;
		break;
	case QueueAvailability::F1_PR1_SR1:
		vkGetDeviceQueue( vk_device.object, primary_render_queue_family_index, 0, &vk_primary_render_queue.object );
		vkGetDeviceQueue( vk_device.object, secondary_render_queue_family_index, 1, &vk_secondary_render_queue.object );
		vk_primary_transfer_queue.object	= vk_secondary_render_queue.object;
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &secondary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &secondary_render_queue_mutex;
		break;
	case QueueAvailability::F1_PR1:
		vkGetDeviceQueue( vk_device.object, primary_render_queue_family_index, 0, &vk_primary_render_queue.object );
		vk_secondary_render_queue.object	= vk_primary_render_queue.object;
		vk_primary_transfer_queue.object	= vk_primary_render_queue.object;
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &primary_render_queue_mutex;
		break;
	default:
		p_logger->LogCritical( "Couldn't resolve the queue allocation scheme" );
		break;
	}
}

void Renderer::FindDepthStencilFormat()
{
	Vector<VkFormat> try_formats {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D16_UNORM,
	};

	for( auto f : try_formats ) {
		VkFormatProperties format_properties {};
		vkGetPhysicalDeviceFormatProperties( vk_physical_device, f, &format_properties );
		if( format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
			depth_stencil_format = f;
			break;
		}
	}
	if( depth_stencil_format == VK_FORMAT_UNDEFINED ) {
		p_logger->LogCritical( "Depth stencil format not selected." );
	}
	if( ( depth_stencil_format == VK_FORMAT_D32_SFLOAT_S8_UINT ) ||
		( depth_stencil_format == VK_FORMAT_D24_UNORM_S8_UINT ) ||
		( depth_stencil_format == VK_FORMAT_D16_UNORM_S8_UINT ) ||
		( depth_stencil_format == VK_FORMAT_S8_UINT ) ) {
		stencil_available				= true;
	}
}

void Renderer::CreateRenderPass()
{
	Vector<VkAttachmentDescription>			attachments;

	Vector<VkSubpassDescription>			subpasses;

	Vector<Vector<VkAttachmentReference>>	input_references;
	Vector<Vector<VkAttachmentReference>>	color_references;
	Vector<Vector<VkAttachmentReference>>	resolve_references;
	Vector<VkAttachmentReference>			depth_stencil_references;
	Vector<Vector<uint32_t>>				preserve_references;

	Vector<VkSubpassDependency>				subpass_dependencies;

	attachments.reserve( 16 );
	subpasses.reserve( 16 );
	input_references.reserve( 16 );
	color_references.reserve( 16 );
	resolve_references.reserve( 16 );
	depth_stencil_references.reserve( 16 );
	preserve_references.reserve( 16 );
	subpass_dependencies.reserve( 16 );

	{
		// Depth stencil attachment
		VkAttachmentDescription				attachment {};
		attachment.flags					= 0;
		attachment.format					= gbuffers[ 0 ]->GetVulkanFormat();
		attachment.samples					= VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp					= VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.storeOp					= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.stencilLoadOp			= VK_ATTACHMENT_LOAD_OP_DONT_CARE;		// Change if we ever use stencils
		attachment.stencilStoreOp			= VK_ATTACHMENT_STORE_OP_DONT_CARE;		// Change if we ever use stencils
		attachment.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;			// Change if we ever use stencils
		attachment.finalLayout				= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments.push_back( attachment );
	}
	{
		// G-buffers
		{
			// Color
			VkAttachmentDescription			attachment {};
			attachment.flags				= 0;
			attachment.format				= gbuffers[ 1 ]->GetVulkanFormat();
			attachment.samples				= VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp				= VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment.storeOp				= VK_ATTACHMENT_STORE_OP_STORE;
			attachment.initialLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
			attachment.finalLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			attachments.push_back( attachment );
		}
	}
	{
		// Final render to the window surface
		VkAttachmentDescription				attachment {};
		attachment.flags					= 0;
		attachment.format					= window_manager->GetVulkanSurfaceFormat().format;
		attachment.samples					= VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp					= VK_ATTACHMENT_LOAD_OP_CLEAR;			// Experiment later if we could just ignore this
		attachment.storeOp					= VK_ATTACHMENT_STORE_OP_STORE;
		attachment.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.finalLayout				= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments.push_back( attachment );
	}

	// Image references for subpasses
	{
		// Subpass 0, G-buffer rendering
		{
			input_references.push_back( {} );
			color_references.push_back( {} );
			resolve_references.push_back( {} );
			depth_stencil_references.push_back( {} );
			preserve_references.push_back( {} );
			auto & input_refs					= input_references.back();
			auto & color_refs					= color_references.back();
			auto & resolve_refs					= resolve_references.back();
			auto & depth_stencil_refs			= depth_stencil_references.back();
			auto & preserve_refs				= preserve_references.back();

			// Color references for this subpass
			{
				VkAttachmentReference cr;
				cr.attachment					= uint32_t( GBUFFERS::COLOR_RGB__EMIT_R );
				cr.layout						= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				color_refs.push_back( cr );
			}
			// Depth stencil reference for this subpass
			{
				VkAttachmentReference depth_stencil_ref {};
				depth_stencil_ref.attachment	= uint32_t( GBUFFERS::DEPTH_STENCIL );
				depth_stencil_ref.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				depth_stencil_refs				= depth_stencil_ref;
			}
		}

		// Subpass 1, G-buffer combining
		{
			input_references.push_back( {} );
			color_references.push_back( {} );
			resolve_references.push_back( {} );
			depth_stencil_references.push_back( {} );
			preserve_references.push_back( {} );
			auto & input_refs					= input_references.back();
			auto & color_refs					= color_references.back();
			auto & resolve_refs					= resolve_references.back();
			auto & depth_stencil_refs			= depth_stencil_references.back();
			auto & preserve_refs				= preserve_references.back();

			// Input references for this subpass
			{
				VkAttachmentReference cr;
				cr.attachment					= uint32_t( GBUFFERS::COLOR_RGB__EMIT_R );
				cr.layout						= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				input_refs.push_back( cr );
			}
			// Color references for this subpass
			{
				VkAttachmentReference cr;
				cr.attachment					= SWAPCHAIN_ATTACHMENT_INDEX;
				cr.layout						= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				color_refs.push_back( cr );
			}
		}
	}

	// Subpasses
	{
		// Subpass 0, rendering into G-Buffers
		uint32_t i							= 0;		// Current subpass
		VkSubpassDescription subpass {};
		subpass.flags						= 0;
		subpass.pipelineBindPoint			= VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.inputAttachmentCount		= uint32_t( input_references[ i ].size() );
		subpass.pInputAttachments			= input_references[ i ].data();
		subpass.colorAttachmentCount		= uint32_t( color_references[ i ].size() );
		subpass.pColorAttachments			= color_references[ i ].data();
		subpass.pResolveAttachments			= resolve_references[ i ].data();
		subpass.pDepthStencilAttachment		= &depth_stencil_references[ i ];
		subpass.preserveAttachmentCount		= uint32_t( preserve_references[ i ].size() );
		subpass.pPreserveAttachments		= preserve_references[ i ].data();
		subpasses.push_back( subpass );
	}
	{
		// Subpass 1, combining G-Buffers into final render
		uint32_t i							= 1;		// Current subpass
		VkSubpassDescription subpass {};
		subpass.flags						= 0;
		subpass.pipelineBindPoint			= VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.inputAttachmentCount		= uint32_t( input_references[ i ].size() );
		subpass.pInputAttachments			= input_references[ i ].data();
		subpass.colorAttachmentCount		= uint32_t( color_references[ i ].size() );
		subpass.pColorAttachments			= color_references[ i ].data();
		subpass.pResolveAttachments			= resolve_references[ i ].data();
		subpass.pDepthStencilAttachment		= nullptr;		// Depth stencil testing disabled for this subpass
		subpass.preserveAttachmentCount		= uint32_t( preserve_references[ i ].size() );
		subpass.pPreserveAttachments		= preserve_references[ i ].data();
		subpasses.push_back( subpass );
	}

	// Introduce dependencies between subpasses
	{
		// Dependency from the commands before beginning of the render pass.
		// Needed because we transfer data before entering the render pass.
		VkSubpassDependency dependency {};
		dependency.srcSubpass			= VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass			= 0;
		dependency.srcStageMask			= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependency.dstStageMask			= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask		= VK_ACCESS_MEMORY_WRITE_BIT;
		dependency.dstAccessMask		= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.dependencyFlags		= 0;
		subpass_dependencies.push_back( dependency );
	}
	{
		// Dependency from the first sub-pass to the second.
		// Needed because we render the G-Buffers on the first sub-pass
		// and use them as input attachments on the second.
		VkSubpassDependency dependency {};
		dependency.srcSubpass			= 0;
		dependency.dstSubpass			= 1;
		dependency.srcStageMask			= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask			= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependency.srcAccessMask		= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.dstAccessMask		= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.dependencyFlags		= 0;
		subpass_dependencies.push_back( dependency );
	}
	{
		// Dependency from the render pass to commands coming after the render pass
		// Not necessarily needed but if we read any of the images after the render pass
		// we need to be sure they're available.
		// For now though it's "just in case" to make one less potential crash
		TODO( "Study if this has any serious negative hit to the performance and take actions accordingly." );
		VkSubpassDependency dependency {};
		dependency.srcSubpass			= 1;
		dependency.dstSubpass			= VK_SUBPASS_EXTERNAL;
		dependency.srcStageMask			= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependency.dstStageMask			= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dependency.srcAccessMask		= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.dstAccessMask		= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dependency.dependencyFlags		= 0;
		subpass_dependencies.push_back( dependency );
	}

	// Create the actual render pass
	VkRenderPassCreateInfo render_pass_CI {};
	render_pass_CI.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_CI.pNext				= nullptr;
	render_pass_CI.flags				= 0;
	render_pass_CI.attachmentCount		= uint32_t( attachments.size() );
	render_pass_CI.pAttachments			= attachments.data();
	render_pass_CI.subpassCount			= uint32_t( subpasses.size() );
	render_pass_CI.pSubpasses			= subpasses.data();
	render_pass_CI.dependencyCount		= uint32_t( subpass_dependencies.size() );
	render_pass_CI.pDependencies		= subpass_dependencies.data();

	{
		// We don't have device resource threads at this points but better to be sure
		LOCK_GUARD( *vk_device.mutex );
		VulkanResultCheck( vkCreateRenderPass( vk_device.object, &render_pass_CI, VULKAN_ALLOC, &vk_render_pass ) );
	}
	if( !vk_render_pass ) {
		p_logger->LogCritical( "Render pass creation failed" );
	}
}

void Renderer::DestroyRenderPass()
{
	// We don't have device resource threads at this points but better to be sure
	LOCK_GUARD( *vk_device.mutex );
	vkDestroyRenderPass( vk_device.object, vk_render_pass, VULKAN_ALLOC );
}

void Renderer::CreateWindowFramebuffers()
{
	// We don't have device resource threads at this points but better to be sure
	LOCK_GUARD( *vk_device.mutex );

	vk_framebuffers.resize( window_manager->GetSwapchainImageCount() );
	for( uint32_t i=0; i < window_manager->GetSwapchainImageCount(); ++i ) {
		Array<VkImageView, GBUFFERS_COUNT + 1> attachments {};
		// G-Buffer attachments, including depth stencil attachment
		for( uint32_t a=0; a < GBUFFERS_COUNT; ++a ) {
			attachments[ a ]						= gbuffers[ a ]->GetVulkanImageView();
		}

		// Final swapchain image is the last attachment
		attachments[ SWAPCHAIN_ATTACHMENT_INDEX ]	= window_manager->GetSwapchainImageViews()[ i ];

		VkFramebufferCreateInfo framebuffer_CI {};
		framebuffer_CI.sType				= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_CI.pNext				= nullptr;
		framebuffer_CI.flags				= 0;
		framebuffer_CI.renderPass			= vk_render_pass;
		framebuffer_CI.attachmentCount		= uint32_t( attachments.size() );
		framebuffer_CI.pAttachments			= attachments.data();
		framebuffer_CI.width				= render_resolution.width;
		framebuffer_CI.height				= render_resolution.height;
		framebuffer_CI.layers				= 1;

		VulkanResultCheck( vkCreateFramebuffer( vk_device.object, &framebuffer_CI, VULKAN_ALLOC, &vk_framebuffers[ i ] ) );
		if( !vk_framebuffers[ i ] ) {
			p_logger->LogCritical( "Window framebuffer creation failed, format: " );
		}
	}
}

void Renderer::DestroyWindowFramebuffers()
{
	// We don't have device resource threads at this points but better to be sure
	LOCK_GUARD( *vk_device.mutex );

	for( auto fb : vk_framebuffers ) {
		vkDestroyFramebuffer( vk_device.object, fb, VULKAN_ALLOC );
	}
	vk_framebuffers.clear();
}

void Renderer::CreateDescriptorSetLayouts()
{
	// We don't have device resource threads at this points but better to be sure
	LOCK_GUARD( *vk_device.mutex );

	// Create camera descriptor set
	{
		// camera descriptor set only has one uniform buffer to send data to the vertex shader
		VkDescriptorSetLayoutBinding descriptor_set_bindings {};
		descriptor_set_bindings.binding				= 0;
		descriptor_set_bindings.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_set_bindings.descriptorCount		= 1;
		descriptor_set_bindings.stageFlags			= VK_SHADER_STAGE_VERTEX_BIT;
		descriptor_set_bindings.pImmutableSamplers	= nullptr;

		VkDescriptorSetLayoutCreateInfo descriptor_set_layout_CI {};
		descriptor_set_layout_CI.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_set_layout_CI.pNext				= nullptr;
		descriptor_set_layout_CI.flags				= 0;
		descriptor_set_layout_CI.bindingCount		= 1;
		descriptor_set_layout_CI.pBindings			= &descriptor_set_bindings;
		VulkanResultCheck( vkCreateDescriptorSetLayout( vk_device.object, &descriptor_set_layout_CI, VULKAN_ALLOC, &vk_descriptor_set_layout_for_camera ) );

		if( !vk_descriptor_set_layout_for_camera ) {
			p_logger->LogCritical( "Unable to create descriptor set layout for camera" );
		}
	}

	// Create object descriptor set
	{
		// object descriptor set only has one uniform buffer to send data to the vertex shader
		VkDescriptorSetLayoutBinding descriptor_set_bindings {};
		descriptor_set_bindings.binding				= 0;
		descriptor_set_bindings.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_set_bindings.descriptorCount		= 1;
		descriptor_set_bindings.stageFlags			= VK_SHADER_STAGE_VERTEX_BIT;
		descriptor_set_bindings.pImmutableSamplers	= nullptr;

		VkDescriptorSetLayoutCreateInfo descriptor_set_layout_CI {};
		descriptor_set_layout_CI.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_set_layout_CI.pNext				= nullptr;
		descriptor_set_layout_CI.flags				= 0;
		descriptor_set_layout_CI.bindingCount		= 1;
		descriptor_set_layout_CI.pBindings			= &descriptor_set_bindings;
		VulkanResultCheck( vkCreateDescriptorSetLayout( vk_device.object, &descriptor_set_layout_CI, VULKAN_ALLOC, &vk_descriptor_set_layout_for_mesh ) );


		if( !vk_descriptor_set_layout_for_mesh ) {
			p_logger->LogCritical( "Unable to create descriptor set layout for object" );
		}
	}

	// Create pipeline descriptor set
	{
		// pipeline descriptor set only has one uniform buffer to send data to the fragment shader
		VkDescriptorSetLayoutBinding descriptor_set_bindings {};
		descriptor_set_bindings.binding				= 0;
		descriptor_set_bindings.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_set_bindings.descriptorCount		= 1;
		descriptor_set_bindings.stageFlags			= VK_SHADER_STAGE_FRAGMENT_BIT;
		descriptor_set_bindings.pImmutableSamplers	= nullptr;

		VkDescriptorSetLayoutCreateInfo descriptor_set_layout_CI {};
		descriptor_set_layout_CI.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_set_layout_CI.pNext				= nullptr;
		descriptor_set_layout_CI.flags				= 0;
		descriptor_set_layout_CI.bindingCount		= 1;
		descriptor_set_layout_CI.pBindings			= &descriptor_set_bindings;
		VulkanResultCheck( vkCreateDescriptorSetLayout( vk_device.object, &descriptor_set_layout_CI, VULKAN_ALLOC, &vk_descriptor_set_layout_for_pipeline ) );

		if( !vk_descriptor_set_layout_for_pipeline ) {
			p_logger->LogCritical( "Unable to create descriptor set layout for pipeline" );
		}
	}

	// Create image descriptor sets
	vk_descriptor_set_layouts_for_images.resize( BUILD_MAX_PER_SHADER_SAMPLED_IMAGE_COUNT + 1 );
	for( uint32_t set=0; set <= BUILD_MAX_PER_SHADER_SAMPLED_IMAGE_COUNT; ++set ) {
		// image descriptor sets can have multiple bindings
		// could also do this with as image arrays...
		Vector<VkDescriptorSetLayoutBinding> descriptor_set_bindings( set );
		for( uint32_t binding=0; binding < set; ++binding ) {
			descriptor_set_bindings[ binding ].binding				= binding;
			descriptor_set_bindings[ binding ].descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptor_set_bindings[ binding ].descriptorCount		= 1;
			descriptor_set_bindings[ binding ].stageFlags			= VK_SHADER_STAGE_FRAGMENT_BIT;
			descriptor_set_bindings[ binding ].pImmutableSamplers	= nullptr;
		}

		VkDescriptorSetLayoutCreateInfo descriptor_set_layout_CI {};
		descriptor_set_layout_CI.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_set_layout_CI.pNext				= nullptr;
		descriptor_set_layout_CI.flags				= 0;
		descriptor_set_layout_CI.bindingCount		= uint32_t( descriptor_set_bindings.size() );
		descriptor_set_layout_CI.pBindings			= descriptor_set_bindings.data();
		VulkanResultCheck( vkCreateDescriptorSetLayout( vk_device.object, &descriptor_set_layout_CI, VULKAN_ALLOC, &vk_descriptor_set_layouts_for_images[ set ] ) );

		if( !vk_descriptor_set_layouts_for_images[ set ] ) {
			p_logger->LogCritical( "Unable to create descriptor set layout for image" );
		}
	}
}

void Renderer::DestroyDescriptorSetLayouts()
{
	// We don't have device resource threads at this points but better to be sure
	LOCK_GUARD( *vk_device.mutex );

	vkDestroyDescriptorSetLayout( vk_device.object, vk_descriptor_set_layout_for_camera, VULKAN_ALLOC );
	vkDestroyDescriptorSetLayout( vk_device.object, vk_descriptor_set_layout_for_mesh, VULKAN_ALLOC );
	vkDestroyDescriptorSetLayout( vk_device.object, vk_descriptor_set_layout_for_pipeline, VULKAN_ALLOC );

	for( auto & d : vk_descriptor_set_layouts_for_images ) {
		vkDestroyDescriptorSetLayout( vk_device.object, d, VULKAN_ALLOC );
	}
	vk_descriptor_set_layouts_for_images.clear();
}

void Renderer::CreateGraphicsPipelineLayouts()
{
	// We don't have device resource threads at this points but better to be sure
	LOCK_GUARD( *vk_device.mutex );

	vk_graphics_pipeline_layouts.resize( BUILD_MAX_PER_SHADER_SAMPLED_IMAGE_COUNT + 1 );
	for( uint32_t pl=0; pl <= BUILD_MAX_PER_SHADER_SAMPLED_IMAGE_COUNT; ++pl ) {
		Vector<VkDescriptorSetLayout>		layouts;
		layouts.reserve( 4 );
		layouts.push_back( vk_descriptor_set_layout_for_camera );
		layouts.push_back( vk_descriptor_set_layout_for_mesh );
		layouts.push_back( vk_descriptor_set_layout_for_pipeline );
		layouts.push_back( vk_descriptor_set_layouts_for_images[ pl ] );

		VkPipelineLayoutCreateInfo layout_CI {};
		layout_CI.sType						= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_CI.pNext						= nullptr;
		layout_CI.flags						= 0;
		layout_CI.setLayoutCount			= uint32_t( layouts.size() );
		layout_CI.pSetLayouts				= layouts.data();
		layout_CI.pushConstantRangeCount	= 0;			// push constants not used by this engine at this time
		layout_CI.pPushConstantRanges		= nullptr;

		VulkanResultCheck( vkCreatePipelineLayout( vk_device.object, &layout_CI, VULKAN_ALLOC, &vk_graphics_pipeline_layouts[ pl ] ) );
		if( !vk_graphics_pipeline_layouts[ pl ] ) {
			p_logger->LogCritical( "Unable to create graphics pipeline layout" );
		}
	}
}

void Renderer::DestroyGraphicsPipelineLayouts()
{
	// We don't have device resource threads at this points but better to be sure
	LOCK_GUARD( *vk_device.mutex );

	for( auto l : vk_graphics_pipeline_layouts ) {
		vkDestroyPipelineLayout( vk_device.object, l, VULKAN_ALLOC );
	}
	vk_graphics_pipeline_layouts.clear();
}

void Renderer::CreateGBuffers()
{
	// Depth stencil
	gbuffers[ 0 ]		= MakeUniquePointer<GBuffer>(
		p_engine, this,
		depth_stencil_format,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT | ( stencil_available ? VK_IMAGE_ASPECT_STENCIL_BIT : 0 ),
		render_resolution );

	// Color
	gbuffers[ 1 ]		= MakeUniquePointer<GBuffer>(
		p_engine, this,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		render_resolution );
}

void Renderer::DestroyGBuffers()
{
	for( auto & g : gbuffers ) {
		g		= nullptr;
	}
}

}
