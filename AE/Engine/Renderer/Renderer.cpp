
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


Renderer::Renderer( Engine * engine, std::string application_name, uint32_t application_version )
	: SubSystem( engine, "Renderer" )
{
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

	device_resource_manager->AllowResourceRequests( true );
	device_resource_manager->AllowResourceLoading( true );
	device_resource_manager->AllowResourceUnloading( true );
}

Renderer::~Renderer()
{
	if( render_initialized_to_window ) DeInitializeRenderToWindow();

	device_resource_manager->WaitJobless();

	device_resource_manager->AllowResourceRequests( false );
	device_resource_manager->AllowResourceLoading( false );

	device_resource_manager		= nullptr;
	device_memory_manager		= nullptr;
	descriptor_pool_manager		= nullptr;

	DestroyGraphicsPipelineLayouts();
	DestroyDescriptorSetLayouts();
	DestroyDevice();
	DestroyDebugReporting();
	DestroyInstance();
}

void Renderer::Update()
{
	device_resource_manager->Update();
}

void Renderer::InitializeRenderToWindow( WindowManager * window_manager )
{
	assert( window_manager );

	if( render_initialized_to_window ) DeInitializeRenderToWindow();

	p_window		= window_manager;

	CreateWindowSurface();
	CreateSwapchain();
	CreateSwapchainImageViews();
	CreateDepthStencilImageAndView();
	CreateRenderPass();
	CreateWindowFramebuffers();

	render_initialized_to_window		= true;
}

void Renderer::DeInitializeRenderToWindow()
{
	if( !render_initialized_to_window ) return;

	render_initialized_to_window		= false;

	DestroyWindowFramebuffers();
	DestroyRenderPass();
	DestroyDepthStencilImageAndView();
	DestroySwapchainImageViews();
	DestroySwapchain();
	DestroyWindowSurface();
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

DeviceMemoryManager * Renderer::GetDeviceMemoryManager() const
{
	return device_memory_manager.Get();
}

DeviceResourceManager * Renderer::GetDeviceResourceManager() const
{
	return device_resource_manager.Get();
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

void Renderer::CreateWindowSurface()
{
	vk_surface					= p_window->CreateVulkanSurface( vk_instance );
	if( !vk_surface ) {
		p_logger->LogCritical( "Couldn't create Vulkan surface" );
	}

	VkBool32 WSI_supported		= VK_FALSE;
	VulkanResultCheck( vkGetPhysicalDeviceSurfaceSupportKHR( vk_physical_device, primary_render_queue_family_index, vk_surface, &WSI_supported ) );

	if( !WSI_supported ) {
		std::stringstream ss;
		ss << "Vulkan surface is not supported on current physical device->'";
		ss << physical_device_properties.deviceName;
		ss << "' and queue family->";
		ss << primary_render_queue_family_index;
		p_logger->LogCritical( ss.str() );
	}

	VulkanResultCheck( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( vk_physical_device, vk_surface, &surface_capabilities ) );

	if( surface_capabilities.currentExtent.width == UINT32_MAX ) {
		int x = 0, y = 0;
		glfwGetWindowSize( p_window->GetGLFWWindow(), &x, &y );
		surface_capabilities.currentExtent.width	= uint32_t( x );
		surface_capabilities.currentExtent.height	= uint32_t( y );
	}

	uint32_t surface_count		= 0;
	VulkanResultCheck( vkGetPhysicalDeviceSurfaceFormatsKHR( vk_physical_device, vk_surface, &surface_count, nullptr ) );
	Vector<VkSurfaceFormatKHR> surface_formats( surface_count );
	VulkanResultCheck( vkGetPhysicalDeviceSurfaceFormatsKHR( vk_physical_device, vk_surface, &surface_count, surface_formats.data() ) );

	if( surface_formats.size() <= 0 ) {
		p_logger->LogCritical( "Vulkan surface formats missing" );
	}

	if( surface_formats[ 0 ].format	== VK_FORMAT_UNDEFINED ) {
		surface_format.format		= VK_FORMAT_B8G8R8A8_UNORM;
		surface_format.colorSpace	= VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	} else {
		surface_format				= surface_formats[ 0 ];
	}
}

void Renderer::DestroyWindowSurface()
{
	vkDestroySurfaceKHR( vk_instance, vk_surface, VULKAN_ALLOC );
	vk_surface = VK_NULL_HANDLE;
}

void Renderer::CreateSwapchain()
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
		VulkanResultCheck( vkGetPhysicalDeviceSurfacePresentModesKHR( vk_physical_device, vk_surface, &present_mode_count, nullptr ) );
		Vector<VkPresentModeKHR> present_modes( present_mode_count );
		VulkanResultCheck( vkGetPhysicalDeviceSurfacePresentModesKHR( vk_physical_device, vk_surface, &present_mode_count, present_modes.data() ) );

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
		LOCK_GUARD( *vk_device.mutex );
		VulkanResultCheck( vkCreateSwapchainKHR( vk_device.object, &swapchain_CI, VULKAN_ALLOC, &vk_swapchain ) );
	}
	if( !vk_swapchain ) {
		p_logger->LogCritical( "Swapchain creation failed" );
	}
}

void Renderer::DestroySwapchain()
{
	// We don't have device resource threads at this points but better to be sure
	LOCK_GUARD( *vk_device.mutex );
	vkDestroySwapchainKHR( vk_device.object, vk_swapchain, VULKAN_ALLOC );
	vk_swapchain = VK_NULL_HANDLE;
}

void Renderer::CreateSwapchainImageViews()
{
	// We don't have device resource threads at this points but better to be sure
	LOCK_GUARD( *vk_device.mutex );

	// getting the images is easy so we'll just do it in here
	VulkanResultCheck( vkGetSwapchainImagesKHR( vk_device.object, vk_swapchain, &swapchain_image_count, nullptr ) );
	swapchain_images.resize( swapchain_image_count );
	VulkanResultCheck( vkGetSwapchainImagesKHR( vk_device.object, vk_swapchain, &swapchain_image_count, swapchain_images.data() ) );

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

		VulkanResultCheck( vkCreateImageView( vk_device.object, &image_view_CI, VULKAN_ALLOC, &swapchain_image_views[ i ] ) );
		if( !swapchain_image_views[ i ] ) {
			p_logger->LogCritical( "Swapchain image view creation failed" );
		}
	}
}

void Renderer::DestroySwapchainImageViews()
{
	// We don't have device resource threads at this points but better to be sure
	LOCK_GUARD( *vk_device.mutex );

	for( auto iv : swapchain_image_views ) {
		vkDestroyImageView( vk_device.object, iv, VULKAN_ALLOC );
	}

	swapchain_images.clear();
	swapchain_image_views.clear();
}

void Renderer::CreateDepthStencilImageAndView()
{
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

	VkImageCreateInfo image_CI {};
	image_CI.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_CI.pNext					= nullptr;
	image_CI.flags					= 0;
	image_CI.imageType				= VK_IMAGE_TYPE_2D;
	image_CI.format					= depth_stencil_format;
	image_CI.extent.width			= surface_capabilities.currentExtent.width;
	image_CI.extent.height			= surface_capabilities.currentExtent.height;
	image_CI.extent.depth			= 1;
	image_CI.mipLevels				= 1;
	image_CI.arrayLayers			= 1;
	image_CI.samples				= VK_SAMPLE_COUNT_1_BIT;
	image_CI.tiling					= VK_IMAGE_TILING_OPTIMAL;
	image_CI.usage					= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image_CI.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
	image_CI.queueFamilyIndexCount	= 1;
	image_CI.pQueueFamilyIndices	= &primary_render_queue_family_index;
	image_CI.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;

	{
		// We don't have device resource threads at this points but better to be sure
		LOCK_GUARD( *vk_device.mutex );
		VulkanResultCheck( vkCreateImage( vk_device.object, &image_CI, VULKAN_ALLOC, &depth_stencil_image ) );
	}
	if( !depth_stencil_image ) {
		p_logger->LogCritical( "Depth stencil image creation failed" );
	}
	depth_stencil_image_memory		= device_memory_manager->AllocateAndBindImageMemory( depth_stencil_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	if( !depth_stencil_image_memory.memory ) {
		p_logger->LogCritical( "Depth stencil image memory allocation failed" );
	}

	VkImageViewCreateInfo image_view_CI {};
	image_view_CI.sType			= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_CI.pNext			= nullptr;
	image_view_CI.flags			= 0;
	image_view_CI.image			= depth_stencil_image;
	image_view_CI.viewType		= VK_IMAGE_VIEW_TYPE_2D;
	image_view_CI.format		= depth_stencil_format;
	image_view_CI.components.r	= VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_CI.components.g	= VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_CI.components.b	= VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_CI.components.a	= VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_CI.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_DEPTH_BIT | ( stencil_available ? VK_IMAGE_ASPECT_STENCIL_BIT : 0 );
	image_view_CI.subresourceRange.baseMipLevel		= 0;
	image_view_CI.subresourceRange.levelCount		= 1;
	image_view_CI.subresourceRange.baseArrayLayer	= 0;
	image_view_CI.subresourceRange.layerCount		= 1;

	{
		// We don't have device resource threads at this points but better to be sure
		LOCK_GUARD( *vk_device.mutex );
		VulkanResultCheck( vkCreateImageView( vk_device.object, &image_view_CI, VULKAN_ALLOC, &depth_stencil_image_view ) );
	}
	if( !depth_stencil_image_view ) {
		p_logger->LogCritical( "Depth stencil image view creation failed" );
	}
}

void Renderer::DestroyDepthStencilImageAndView()
{
	{
		// We don't have device resource threads at this points but better to be sure
		LOCK_GUARD( *vk_device.mutex );
		vkDestroyImageView( vk_device.object, depth_stencil_image_view, VULKAN_ALLOC );
		vkDestroyImage( vk_device.object, depth_stencil_image, VULKAN_ALLOC );
	}
	device_memory_manager->FreeMemory( depth_stencil_image_memory );
	depth_stencil_image_memory = {};
}

void Renderer::CreateRenderPass()
{
	Vector<VkAttachmentDescription> attachments( 2 );
	attachments[ 0 ].flags				= 0;
	attachments[ 0 ].format				= depth_stencil_format;
	attachments[ 0 ].samples			= VK_SAMPLE_COUNT_1_BIT;
	attachments[ 0 ].loadOp				= VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[ 0 ].storeOp			= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[ 0 ].stencilLoadOp		= VK_ATTACHMENT_LOAD_OP_DONT_CARE;		// Change if we ever use stencils
	attachments[ 0 ].stencilStoreOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[ 0 ].initialLayout		= VK_IMAGE_LAYOUT_UNDEFINED;			// Change if we ever use stencils
	attachments[ 0 ].finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	attachments[ 1 ].flags				= 0;
	attachments[ 1 ].format				= surface_format.format;
	attachments[ 1 ].samples			= VK_SAMPLE_COUNT_1_BIT;
	attachments[ 1 ].loadOp				= VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[ 1 ].storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	attachments[ 1 ].initialLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[ 1 ].finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference subpass_0_depth_stencil_image_reference {};
	subpass_0_depth_stencil_image_reference.attachment		= 0;
	subpass_0_depth_stencil_image_reference.layout			= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	Vector<VkAttachmentReference> subpass_0_color_image_references( 1 );
	subpass_0_color_image_references[ 0 ].attachment		= 1;
	subpass_0_color_image_references[ 0 ].layout			= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Vector<VkSubpassDescription>	subpasses( 1 );
	subpasses[ 0 ].flags					= 0;
	subpasses[ 0 ].pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[ 0 ].inputAttachmentCount		= 0;
	subpasses[ 0 ].pInputAttachments		= nullptr;
	subpasses[ 0 ].colorAttachmentCount		= uint32_t( subpass_0_color_image_references.size() );
	subpasses[ 0 ].pColorAttachments		= subpass_0_color_image_references.data();
	subpasses[ 0 ].pResolveAttachments		= nullptr;		// Change if we ever use hardware antialiazing
	subpasses[ 0 ].pDepthStencilAttachment	= &subpass_0_depth_stencil_image_reference;
	subpasses[ 0 ].preserveAttachmentCount	= 0;
	subpasses[ 0 ].pPreserveAttachments		= nullptr;

	VkRenderPassCreateInfo render_pass_CI {};
	render_pass_CI.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_CI.pNext				= nullptr;
	render_pass_CI.flags				= 0;
	render_pass_CI.attachmentCount		= uint32_t( attachments.size() );
	render_pass_CI.pAttachments			= attachments.data();
	render_pass_CI.subpassCount			= uint32_t( subpasses.size() );
	render_pass_CI.pSubpasses			= subpasses.data();
	render_pass_CI.dependencyCount		= 0;
	render_pass_CI.pDependencies		= nullptr;

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

	vk_framebuffers.resize( swapchain_image_count );
	for( uint32_t i=0; i < swapchain_image_count; ++i ) {
		Vector<VkImageView> attachments( 2 );
		attachments[ 0 ]	= depth_stencil_image_view;
		attachments[ 1 ]	= swapchain_image_views[ i ];

		VkFramebufferCreateInfo framebuffer_CI {};
		framebuffer_CI.sType				= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_CI.pNext				= nullptr;
		framebuffer_CI.flags				= 0;
		framebuffer_CI.renderPass			= vk_render_pass;
		framebuffer_CI.attachmentCount		= uint32_t( attachments.size() );
		framebuffer_CI.pAttachments			= attachments.data();
		framebuffer_CI.width				= surface_capabilities.currentExtent.width;
		framebuffer_CI.height				= surface_capabilities.currentExtent.height;
		framebuffer_CI.layers				= 1;

		VulkanResultCheck( vkCreateFramebuffer( vk_device.object, &framebuffer_CI, VULKAN_ALLOC, &vk_framebuffers[ i ] ) );
		if( !vk_framebuffers[ i ] ) {
			p_logger->LogCritical( "Framebuffer creation failed" );
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

}
