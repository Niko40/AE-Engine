
#include <iostream>
#include <sstream>
#include <assert.h>

#include "../Memory/Memory.h"
#include "Renderer.h"
#include "DeviceMemory/DeviceMemoryManager.h"
#include "DeviceResource/DeviceResourceManager.h"
#include "../Logger/Logger.h"
#include "../Window/WindowManager.h"


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
	application_info = vk::ApplicationInfo(
		application_name.c_str(),
		application_version,
		BUILD_ENGINE_NAME,
		BUILD_ENGINE_VERSION,
		BUILD_VULKAN_VERSION
	);

	SetupRequiredLayersAndExtensions();
	SetupDebugReporting();
	CreateInstance();
	CreateDebugReporting();
	SelectPhysicalDevices();
	FindQueueFamilies();
	CreateDevice();
	GetQueueHandles();

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

vk::Instance Renderer::GetVulkanInstance() const
{
	return vk_instance;
}

vk::PhysicalDevice Renderer::GetVulkanPhysicalDevice() const
{
	return vk_physical_device;
}

VulkanDevice Renderer::GetVulkanDevice() const
{
	return vk_device;
}

vk::RenderPass Renderer::GetVulkanRenderPass() const
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

Renderer::SharingModeInfo Renderer::GetSharingModeInfo( Renderer::UsedQueuesFlags used_queues )
{
	SharingModeInfo ret;
	if( uint32_t( used_queues & Renderer::UsedQueuesFlags::PRIMARY_RENDER ) ) {
		ret.shared_queue_family_indices.push_back( primary_render_queue_family_index );
	}
	if( uint32_t( used_queues & Renderer::UsedQueuesFlags::SECONDARY_RENDER ) ) {
		ret.shared_queue_family_indices.push_back( secondary_render_queue_family_index );
	}
	if( uint32_t( used_queues & Renderer::UsedQueuesFlags::PRIMARY_TRANSFER ) ) {
		ret.shared_queue_family_indices.push_back( primary_transfer_queue_family_index );
	}
	// remove doubles
	ret.shared_queue_family_indices.erase( std::unique( ret.shared_queue_family_indices.begin(), ret.shared_queue_family_indices.end() ), ret.shared_queue_family_indices.end() );
	if( ret.shared_queue_family_indices.size() > 1 ) {
		ret.sharing_mode	= vk::SharingMode::eConcurrent;
	} else {
		ret.sharing_mode	= vk::SharingMode::eExclusive;
	}
	return ret;
}

const vk::PhysicalDeviceProperties & Renderer::GetPhysicalDeviceProperties() const
{
	return physical_device_properties;
}

const vk::PhysicalDeviceFeatures & Renderer::GetPhysicalDeviceFeatures() const
{
	return physical_device_features;
}

const vk::PhysicalDeviceLimits & Renderer::GetPhysicalDeviceLimits() const
{
	return physical_device_limits;
}

bool Renderer::IsFormatSupported( vk::ImageTiling tiling, vk::Format format, vk::FormatFeatureFlags feature_flags )
{
	auto fp = vk_physical_device.getFormatProperties( format );
	switch( tiling ) {
	case vk::ImageTiling::eOptimal:
		return ( ( fp.optimalTilingFeatures & feature_flags ) == feature_flags );
	case vk::ImageTiling::eLinear:
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
	vk::InstanceCreateInfo instance_CI {};
	instance_CI.pApplicationInfo		= &application_info;
	instance_CI.enabledLayerCount		= uint32_t( instance_layer_names.size() );
	instance_CI.ppEnabledLayerNames		= instance_layer_names.data();
	instance_CI.enabledExtensionCount	= uint32_t( instance_extension_names.size() );
	instance_CI.ppEnabledExtensionNames	= instance_extension_names.data();

	instance_CI.pNext	= &debug_report_callback_create_info;
	vk_instance			= vk::createInstance( instance_CI );
}

void Renderer::DestroyInstance()
{
	vk_instance.destroy();
}

uint32_t RatePhysicalDeviceSuitability( vk::PhysicalDevice physical_device )
{
	uint32_t score						= 0;

	auto properties						= physical_device.getProperties();
	auto memory_properties				= physical_device.getMemoryProperties();

	// rate by type
	if( properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ) {
		score += 4000;
	}
	if( properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu ) {
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

	auto physical_devices				= vk_instance.enumeratePhysicalDevices();
	std::vector<uint32_t>		physical_device_scores( physical_devices.size() );
	for( size_t i=0; i < physical_devices.size(); ++i ) {
		auto pd							= physical_devices[ i ];
		physical_device_scores[ i ]		= RatePhysicalDeviceSuitability( pd );
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

	physical_device_properties					= vk_physical_device.getProperties();
	physical_device_features					= vk_physical_device.getFeatures();
	physical_device_limits						= physical_device_properties.limits;
}

uint32_t CountQueueFamilyFlags( vk::QueueFamilyProperties & fp )
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

	auto queue_family_properties									= vk_physical_device.getQueueFamilyProperties();

	std::vector<std::pair<uint32_t, vk::QueueFamilyProperties*>>	render_families;
	std::vector<std::pair<uint32_t, vk::QueueFamilyProperties*>>	transfer_families;

	auto primary_render_family		= std::pair<uint32_t, vk::QueueFamilyProperties*>( UINT32_MAX, nullptr );
	auto secondary_render_family	= std::pair<uint32_t, vk::QueueFamilyProperties*>( UINT32_MAX, nullptr );
	auto primary_transfer_family	= std::pair<uint32_t, vk::QueueFamilyProperties*>( UINT32_MAX, nullptr );

	for( size_t i=0; i < queue_family_properties.size(); ++i ) {
		auto & f = queue_family_properties[ i ];
		if( f.queueFlags & vk::QueueFlagBits::eGraphics )	render_families.push_back( std::pair<uint32_t, vk::QueueFamilyProperties*>( uint32_t( i ), &f ) );
		if( f.queueFlags & vk::QueueFlagBits::eTransfer )	transfer_families.push_back( std::pair<uint32_t, vk::QueueFamilyProperties*>( uint32_t( i ), &f ) );
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
	case AE::Renderer::QueueAvailability::F3_PR1_SR2_PT3:
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
	case AE::Renderer::QueueAvailability::F2_PR1_SR1_PT2:
		device_queue_create_infos.resize( 2 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 2;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities[ 0 ];
		device_queue_create_infos[ 1 ].queueFamilyIndex			= primary_transfer_queue_family_index;
		device_queue_create_infos[ 1 ].queueCount				= 1;
		device_queue_create_infos[ 1 ].pQueuePriorities			= &queue_priorities[ 2 ];
		break;
	case AE::Renderer::QueueAvailability::F2_PR1_SR2_PT2:
		device_queue_create_infos.resize( 2 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 1;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities[ 0 ];
		device_queue_create_infos[ 1 ].queueFamilyIndex			= secondary_render_queue_family_index;
		device_queue_create_infos[ 1 ].queueCount				= 2;
		device_queue_create_infos[ 1 ].pQueuePriorities			= &queue_priorities[ 1 ];
		break;
	case AE::Renderer::QueueAvailability::F2_PR1_SR2_PT1:
		device_queue_create_infos.resize( 2 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 2;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities_2[ 0 ];
		device_queue_create_infos[ 1 ].queueFamilyIndex			= secondary_render_queue_family_index;
		device_queue_create_infos[ 1 ].queueCount				= 1;
		device_queue_create_infos[ 1 ].pQueuePriorities			= &queue_priorities_2[ 1 ];
		break;
	case AE::Renderer::QueueAvailability::F2_PR1_SR2:
		device_queue_create_infos.resize( 2 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 1;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities[ 0 ];
		device_queue_create_infos[ 1 ].queueFamilyIndex			= secondary_render_queue_family_index;
		device_queue_create_infos[ 1 ].queueCount				= 1;
		device_queue_create_infos[ 1 ].pQueuePriorities			= &queue_priorities[ 1 ];
		break;
	case AE::Renderer::QueueAvailability::F2_PR1_PT2:
		device_queue_create_infos.resize( 2 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 1;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities_2[ 0 ];
		device_queue_create_infos[ 1 ].queueFamilyIndex			= primary_transfer_queue_family_index;
		device_queue_create_infos[ 1 ].queueCount				= 1;
		device_queue_create_infos[ 1 ].pQueuePriorities			= &queue_priorities_2[ 1 ];
		break;
	case AE::Renderer::QueueAvailability::F1_PR1_SR1_PT1:
		device_queue_create_infos.resize( 1 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 3;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities[ 0 ];
		break;
	case AE::Renderer::QueueAvailability::F1_PR1_PT1:
		device_queue_create_infos.resize( 1 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 2;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities_2[ 0 ];
		break;
	case AE::Renderer::QueueAvailability::F1_PR1_SR1:
		device_queue_create_infos.resize( 1 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 2;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities[ 0 ];
		break;
	case AE::Renderer::QueueAvailability::F1_PR1:
		device_queue_create_infos.resize( 1 );
		device_queue_create_infos[ 0 ].queueFamilyIndex			= primary_render_queue_family_index;
		device_queue_create_infos[ 0 ].queueCount				= 1;
		device_queue_create_infos[ 0 ].pQueuePriorities			= &queue_priorities[ 0 ];
		break;
	default:
		p_logger->LogCritical( "Couldn't resolve the queue allocation scheme" );
		break;
	}
}

void Renderer::CreateDevice()
{
	vk::PhysicalDeviceFeatures features;

	vk::DeviceCreateInfo device_CI {};
	device_CI.queueCreateInfoCount		= uint32_t( device_queue_create_infos.size() );
	device_CI.pQueueCreateInfos			= device_queue_create_infos.data();
	device_CI.enabledExtensionCount		= uint32_t( device_extension_names.size() );
	device_CI.ppEnabledExtensionNames	= device_extension_names.data();
	device_CI.pEnabledFeatures			= &features;

	vk_device.object					= vk_physical_device.createDevice( device_CI );
	vk_device.mutex						= &device_mutex;
}

void Renderer::DestroyDevice()
{
	vk_device.object.destroy();
}

void Renderer::GetQueueHandles()
{
	switch( queue_availability ) {
	case AE::Renderer::QueueAvailability::F3_PR1_SR2_PT3:
		vk_primary_render_queue.object		= vk_device.object.getQueue( primary_render_queue_family_index, 0 );
		vk_secondary_render_queue.object	= vk_device.object.getQueue( secondary_render_queue_family_index, 0 );
		vk_primary_transfer_queue.object	= vk_device.object.getQueue( primary_transfer_queue_family_index, 0 );
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &secondary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &primary_transfer_queue_mutex;
		break;
	case AE::Renderer::QueueAvailability::F2_PR1_SR1_PT2:
		vk_primary_render_queue.object		= vk_device.object.getQueue( primary_render_queue_family_index, 0 );
		vk_secondary_render_queue.object	= vk_device.object.getQueue( secondary_render_queue_family_index, 1 );
		vk_primary_transfer_queue.object	= vk_device.object.getQueue( primary_transfer_queue_family_index, 0 );
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &secondary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &primary_transfer_queue_mutex;
		break;
	case AE::Renderer::QueueAvailability::F2_PR1_SR2_PT2:
		vk_primary_render_queue.object		= vk_device.object.getQueue( primary_render_queue_family_index, 0 );
		vk_secondary_render_queue.object	= vk_device.object.getQueue( secondary_render_queue_family_index, 0 );
		vk_primary_transfer_queue.object	= vk_device.object.getQueue( primary_transfer_queue_family_index, 1 );
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &secondary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &primary_transfer_queue_mutex;
		break;
	case AE::Renderer::QueueAvailability::F2_PR1_SR2_PT1:
		vk_primary_render_queue.object		= vk_device.object.getQueue( primary_render_queue_family_index, 0 );
		vk_secondary_render_queue.object	= vk_device.object.getQueue( secondary_render_queue_family_index, 0 );
		vk_primary_transfer_queue.object	= vk_device.object.getQueue( primary_transfer_queue_family_index, 1 );
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &secondary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &primary_transfer_queue_mutex;
		break;
	case AE::Renderer::QueueAvailability::F2_PR1_SR2:
		vk_primary_render_queue.object		= vk_device.object.getQueue( primary_render_queue_family_index, 0 );
		vk_secondary_render_queue.object	= vk_device.object.getQueue( secondary_render_queue_family_index, 0 );
		vk_primary_transfer_queue.object	= vk_secondary_render_queue.object;
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &secondary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &secondary_render_queue_mutex;
		break;
	case AE::Renderer::QueueAvailability::F2_PR1_PT2:
		vk_primary_render_queue.object		= vk_device.object.getQueue( primary_render_queue_family_index, 0 );
		vk_secondary_render_queue.object	= vk_primary_render_queue.object;
		vk_primary_transfer_queue.object	= vk_device.object.getQueue( primary_transfer_queue_family_index, 0 );
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &primary_transfer_queue_mutex;
		break;
	case AE::Renderer::QueueAvailability::F1_PR1_SR1_PT1:
		vk_primary_render_queue.object		= vk_device.object.getQueue( primary_render_queue_family_index, 0 );
		vk_secondary_render_queue.object	= vk_device.object.getQueue( secondary_render_queue_family_index, 1 );
		vk_primary_transfer_queue.object	= vk_device.object.getQueue( primary_transfer_queue_family_index, 2 );
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &secondary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &primary_transfer_queue_mutex;
		break;
	case AE::Renderer::QueueAvailability::F1_PR1_PT1:
		vk_primary_render_queue.object		= vk_device.object.getQueue( primary_render_queue_family_index, 0 );
		vk_secondary_render_queue.object	= vk_primary_render_queue.object;
		vk_primary_transfer_queue.object	= vk_device.object.getQueue( primary_transfer_queue_family_index, 1 );
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &primary_transfer_queue_mutex;
		break;
	case AE::Renderer::QueueAvailability::F1_PR1_SR1:
		vk_primary_render_queue.object		= vk_device.object.getQueue( primary_render_queue_family_index, 0 );
		vk_secondary_render_queue.object	= vk_device.object.getQueue( secondary_render_queue_family_index, 1 );
		vk_primary_transfer_queue.object	= vk_secondary_render_queue.object;
		vk_primary_render_queue.mutex		= &primary_render_queue_mutex;
		vk_secondary_render_queue.mutex		= &secondary_render_queue_mutex;
		vk_primary_transfer_queue.mutex		= &secondary_render_queue_mutex;
		break;
	case AE::Renderer::QueueAvailability::F1_PR1:
		vk_primary_render_queue.object		= vk_device.object.getQueue( primary_render_queue_family_index, 0 );
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

	auto WSI_supported			= vk_physical_device.getSurfaceSupportKHR( primary_render_queue_family_index, vk_surface );
	if( !WSI_supported ) {
		std::stringstream ss;
		ss << "Vulkan surface is not supported on current physical device->'";
		ss << physical_device_properties.deviceName;
		ss << "' and queue family->";
		ss << primary_render_queue_family_index;
		p_logger->LogCritical( ss.str() );
	}

	surface_capabilities		= vk_physical_device.getSurfaceCapabilitiesKHR( vk_surface );

	if( surface_capabilities.currentExtent.width == UINT32_MAX ) {
		int x = 0, y = 0;
		glfwGetWindowSize( p_window->GetGLFWWindow(), &x, &y );
		surface_capabilities.currentExtent.width	= uint32_t( x );
		surface_capabilities.currentExtent.height	= uint32_t( y );
	}

	auto formats				= vk_physical_device.getSurfaceFormatsKHR( vk_surface );
	if( formats.size() <= 0 ) {
		p_logger->LogCritical( "Vulkan surface formats missing" );
	}

	if( formats[ 0 ].format			== vk::Format::eUndefined ) {
		surface_format.format		= vk::Format::eB8G8R8A8Unorm;
		surface_format.colorSpace	= vk::ColorSpaceKHR::eSrgbNonlinear;
	} else {
		surface_format				= formats[ 0 ];
	}
}

void Renderer::DestroyWindowSurface()
{
	vk_instance.destroySurfaceKHR( vk_surface );
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

	swapchain_present_mode				= vk::PresentModeKHR::eFifo;
	if( !swapchain_vsync ) {
		auto present_modes				= vk_physical_device.getSurfacePresentModesKHR( vk_surface );
		for( auto m : present_modes ) {
			if( m == vk::PresentModeKHR::eMailbox ) {
				swapchain_present_mode	= m;
				break;
			}
		}
	}

	vk::SwapchainCreateInfoKHR swapchain_CI {};
	swapchain_CI.surface				= vk_surface;
	swapchain_CI.minImageCount			= swapchain_image_count;
	swapchain_CI.imageFormat			= surface_format.format;
	swapchain_CI.imageColorSpace		= surface_format.colorSpace;
	swapchain_CI.imageExtent			= surface_capabilities.currentExtent;
	swapchain_CI.imageArrayLayers		= 1;
	swapchain_CI.imageUsage				= vk::ImageUsageFlagBits::eColorAttachment;
	swapchain_CI.imageSharingMode		= vk::SharingMode::eExclusive;
	swapchain_CI.queueFamilyIndexCount	= 1;
	swapchain_CI.pQueueFamilyIndices	= &primary_render_queue_family_index;
	swapchain_CI.preTransform			= vk::SurfaceTransformFlagBitsKHR::eIdentity;
	swapchain_CI.compositeAlpha			= vk::CompositeAlphaFlagBitsKHR::eOpaque;
	swapchain_CI.presentMode			= swapchain_present_mode;
	swapchain_CI.clipped				= VK_TRUE;
	swapchain_CI.oldSwapchain			= nullptr;

	{
		LOCK_GUARD( *vk_device.mutex );
		vk_swapchain	= vk_device.object.createSwapchainKHR( swapchain_CI );
	}
	if( !vk_swapchain ) {
		p_logger->LogCritical( "Swapchain creation failed" );
	}
}

void Renderer::DestroySwapchain()
{
	{
		LOCK_GUARD( *vk_device.mutex );
		vk_device.object.destroySwapchainKHR( vk_swapchain );
	}
}

void Renderer::CreateSwapchainImageViews()
{
	LOCK_GUARD( *vk_device.mutex );

	// getting the images is easy one-liner so we'll just do it in here
	swapchain_images		= vk_device.object.getSwapchainImagesKHR( vk_swapchain );

	// also make sure we got the proper amount of images
	swapchain_image_count	= uint32_t( swapchain_images.size() );

	swapchain_image_views.resize( swapchain_image_count );
	for( uint32_t i=0; i < swapchain_image_count; ++i ) {
		vk::ImageViewCreateInfo image_view_CI {};
		image_view_CI.image				= swapchain_images[ i ];
		image_view_CI.viewType			= vk::ImageViewType::e2D;
		image_view_CI.format			= surface_format.format;
		image_view_CI.components		= vk::ComponentMapping();
		image_view_CI.subresourceRange.aspectMask		= vk::ImageAspectFlagBits::eColor;
		image_view_CI.subresourceRange.baseMipLevel		= 0;
		image_view_CI.subresourceRange.levelCount		= 1;
		image_view_CI.subresourceRange.baseArrayLayer	= 0;
		image_view_CI.subresourceRange.layerCount		= 1;

		swapchain_image_views[ i ]	= vk_device.object.createImageView( image_view_CI );
		if( !swapchain_image_views[ i ] ) {
			p_logger->LogCritical( "Swapchain image view creation failed" );
		}
	}
}

void Renderer::DestroySwapchainImageViews()
{
	LOCK_GUARD( *vk_device.mutex );

	for( auto iv : swapchain_image_views ) {
		vk_device.object.destroyImageView( iv );
	}

	swapchain_images.clear();
	swapchain_image_views.clear();
}

void Renderer::CreateDepthStencilImageAndView()
{
	{
		std::vector<vk::Format> try_formats {
			vk::Format::eD32SfloatS8Uint,
			vk::Format::eD24UnormS8Uint,
			vk::Format::eD16UnormS8Uint,
			vk::Format::eD32Sfloat,
			vk::Format::eD16Unorm,
		};

		for( auto f : try_formats ) {
			auto format_properties		= vk_physical_device.getFormatProperties( f );
			if( format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment ) {
				depth_stencil_format = f;
				break;
			}
		}
		if( depth_stencil_format == vk::Format::eUndefined ) {
			p_logger->LogCritical( "Depth stencil format not selected." );
		}
		if( ( depth_stencil_format == vk::Format::eD32SfloatS8Uint ) ||
			( depth_stencil_format == vk::Format::eD24UnormS8Uint ) ||
			( depth_stencil_format == vk::Format::eD16UnormS8Uint ) ||
			( depth_stencil_format == vk::Format::eS8Uint ) ) {
			stencil_available				= true;
		}
	}

	vk::ImageCreateInfo image_CI {};
	image_CI.imageType				= vk::ImageType::e2D;
	image_CI.format					= depth_stencil_format;
	image_CI.extent.width			= surface_capabilities.currentExtent.width;
	image_CI.extent.height			= surface_capabilities.currentExtent.height;
	image_CI.extent.depth			= 1;
	image_CI.mipLevels				= 1;
	image_CI.arrayLayers			= 1;
	image_CI.samples				= vk::SampleCountFlagBits::e1;
	image_CI.tiling					= vk::ImageTiling::eOptimal;
	image_CI.usage					= vk::ImageUsageFlagBits::eDepthStencilAttachment;
	image_CI.sharingMode			= vk::SharingMode::eExclusive;
	image_CI.queueFamilyIndexCount	= 1;
	image_CI.pQueueFamilyIndices	= &primary_render_queue_family_index;
	image_CI.initialLayout			= vk::ImageLayout::eUndefined;

	{
		LOCK_GUARD( *vk_device.mutex );
		depth_stencil_image				= vk_device.object.createImage( image_CI );
	}
	if( !depth_stencil_image ) {
		p_logger->LogCritical( "Depth stencil image creation failed" );
	}
	depth_stencil_image_memory		= device_memory_manager->AllocateAndBindImageMemory( depth_stencil_image, vk::MemoryPropertyFlagBits::eDeviceLocal );
	if( !depth_stencil_image_memory.memory ) {
		p_logger->LogCritical( "Depth stencil image memory allocation failed" );
	}

	vk::ImageViewCreateInfo image_view_CI {};
	image_view_CI.image			= depth_stencil_image;
	image_view_CI.viewType		= vk::ImageViewType::e2D;
	image_view_CI.format		= depth_stencil_format;
	image_view_CI.components	= vk::ComponentMapping();
	image_view_CI.subresourceRange.aspectMask		= vk::ImageAspectFlagBits::eDepth | ( stencil_available ? vk::ImageAspectFlagBits::eStencil : vk::ImageAspectFlagBits( 0 ) );
	image_view_CI.subresourceRange.baseMipLevel		= 0;
	image_view_CI.subresourceRange.levelCount		= 1;
	image_view_CI.subresourceRange.baseArrayLayer	= 0;
	image_view_CI.subresourceRange.layerCount		= 1;

	{
		LOCK_GUARD( *vk_device.mutex );
		depth_stencil_image_view	= vk_device.object.createImageView( image_view_CI );
	}
	if( !depth_stencil_image_view ) {
		p_logger->LogCritical( "Depth stencil image view creation failed" );
	}
}

void Renderer::DestroyDepthStencilImageAndView()
{
	{
		LOCK_GUARD( *vk_device.mutex );
		vk_device.object.destroyImageView( depth_stencil_image_view );
		vk_device.object.destroyImage( depth_stencil_image );
	}
	device_memory_manager->FreeMemory( depth_stencil_image_memory );
	depth_stencil_image_memory = {};
}

void Renderer::CreateRenderPass()
{
	std::vector<vk::AttachmentDescription> attachments( 2 );
	attachments[ 0 ].format				= depth_stencil_format;
	attachments[ 0 ].samples			= vk::SampleCountFlagBits::e1;
	attachments[ 0 ].loadOp				= vk::AttachmentLoadOp::eClear;
	attachments[ 0 ].storeOp			= vk::AttachmentStoreOp::eDontCare;
	attachments[ 0 ].stencilLoadOp		= vk::AttachmentLoadOp::eDontCare;		// Change if we ever use stencils
	attachments[ 0 ].stencilStoreOp		= vk::AttachmentStoreOp::eDontCare;
	attachments[ 0 ].initialLayout		= vk::ImageLayout::eUndefined;			// Change if we ever use stencils
	attachments[ 0 ].finalLayout		= vk::ImageLayout::eDepthStencilAttachmentOptimal;

	attachments[ 1 ].format				= surface_format.format;
	attachments[ 1 ].samples			= vk::SampleCountFlagBits::e1;
	attachments[ 1 ].loadOp				= vk::AttachmentLoadOp::eClear;
	attachments[ 1 ].storeOp			= vk::AttachmentStoreOp::eStore;
	attachments[ 1 ].initialLayout		= vk::ImageLayout::eUndefined;
	attachments[ 1 ].finalLayout		= vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentReference subpass_0_depth_stencil_image_reference {};
	subpass_0_depth_stencil_image_reference.attachment		= 0;
	subpass_0_depth_stencil_image_reference.layout			= vk::ImageLayout::eDepthStencilAttachmentOptimal;

	std::vector<vk::AttachmentReference> subpass_0_color_image_references( 1 );
	subpass_0_color_image_references[ 0 ].attachment		= 1;
	subpass_0_color_image_references[ 0 ].layout			= vk::ImageLayout::eColorAttachmentOptimal;

	std::vector<vk::SubpassDescription>	subpasses( 1 );
	subpasses[ 0 ].flags					= vk::SubpassDescriptionFlags();
	subpasses[ 0 ].pipelineBindPoint		= vk::PipelineBindPoint::eGraphics;
	subpasses[ 0 ].inputAttachmentCount		= 0;
	subpasses[ 0 ].pInputAttachments		= nullptr;
	subpasses[ 0 ].colorAttachmentCount		= uint32_t( subpass_0_color_image_references.size() );
	subpasses[ 0 ].pColorAttachments		= subpass_0_color_image_references.data();
	subpasses[ 0 ].pResolveAttachments		= nullptr;		// Change if we ever use hardware antialiazing
	subpasses[ 0 ].pDepthStencilAttachment	= &subpass_0_depth_stencil_image_reference;
	subpasses[ 0 ].preserveAttachmentCount	= 0;
	subpasses[ 0 ].pPreserveAttachments		= nullptr;

	vk::RenderPassCreateInfo render_pass_CI {};
	render_pass_CI.attachmentCount		= uint32_t( attachments.size() );
	render_pass_CI.pAttachments			= attachments.data();
	render_pass_CI.subpassCount			= uint32_t( subpasses.size() );
	render_pass_CI.pSubpasses			= subpasses.data();
	render_pass_CI.dependencyCount		= 0;
	render_pass_CI.pDependencies		= nullptr;

	{
		LOCK_GUARD( *vk_device.mutex );
		vk_render_pass					= vk_device.object.createRenderPass( render_pass_CI );
	}
	if( !vk_render_pass ) {
		p_logger->LogCritical( "Render pass creation failed" );
	}
}

void Renderer::DestroyRenderPass()
{
	LOCK_GUARD( *vk_device.mutex );
	vk_device.object.destroyRenderPass( vk_render_pass );
}

void Renderer::CreateWindowFramebuffers()
{
	LOCK_GUARD( *vk_device.mutex );

	vk_framebuffers.resize( swapchain_image_count );
	for( uint32_t i=0; i < swapchain_image_count; ++i ) {
		std::vector<vk::ImageView> attachments( 2 );
		attachments[ 0 ]	= depth_stencil_image_view;
		attachments[ 1 ]	= swapchain_image_views[ i ];

		vk::FramebufferCreateInfo framebuffer_CI {};
		framebuffer_CI.renderPass			= vk_render_pass;
		framebuffer_CI.attachmentCount		= uint32_t( attachments.size() );
		framebuffer_CI.pAttachments			= attachments.data();
		framebuffer_CI.width				= surface_capabilities.currentExtent.width;
		framebuffer_CI.height				= surface_capabilities.currentExtent.height;
		framebuffer_CI.layers				= 1;

		vk_framebuffers[ i ] = vk_device.object.createFramebuffer( framebuffer_CI );
		if( !vk_framebuffers[ i ] ) {
			p_logger->LogCritical( "Framebuffer creation failed" );
		}
	}
}

void Renderer::DestroyWindowFramebuffers()
{
	LOCK_GUARD( *vk_device.mutex );

	for( auto fb : vk_framebuffers ) {
		vk_device.object.destroyFramebuffer( fb );
	}
	vk_framebuffers.clear();
}

Renderer::UsedQueuesFlags operator|( Renderer::UsedQueuesFlags f1, Renderer::UsedQueuesFlags f2 )
{
	return Renderer::UsedQueuesFlags( uint32_t( f1 ) | uint32_t( f2 ) );
}

Renderer::UsedQueuesFlags operator&( Renderer::UsedQueuesFlags f1, Renderer::UsedQueuesFlags f2 )
{
	return Renderer::UsedQueuesFlags( uint32_t( f1 ) & uint32_t( f2 ) );
}

Renderer::UsedQueuesFlags operator|=( Renderer::UsedQueuesFlags f1, Renderer::UsedQueuesFlags f2 )
{
	return f1 | f2;
}

}
