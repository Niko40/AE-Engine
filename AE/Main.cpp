
#include <vector>
#include <string>

#include <lua.hpp>
#include <iostream>
#include <tinyxml2.h>

#include "Engine/IncludeAll.h"

int main( int argc, char ** argv )
{
	// parse arguments into a vector
	AE::Vector<AE::String> arguments;
	for( int i=0; i < argc; i++ ) {
		arguments.push_back( AE::String( argv[ i ] ) );
	}

	AE::Engine engine;
	auto world			= engine.CreateWorld( "data/worlds/test.world" );
	auto scene_manager	= world->GetSceneManager();
	auto renderer		= engine.GetRenderer();

	auto format_support_image_r8_unorm					= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE, VK_FORMAT_R8_UNORM, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR );
	auto format_support_image_r8g8_unorm				= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE, VK_FORMAT_R8G8_UNORM, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR );
	auto format_support_image_r8g8b8_unorm				= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE, VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR );
	auto format_support_image_r8g8b8a8_unorm			= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR );
	auto format_support_image_r16_snorm					= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE, VK_FORMAT_R16_SNORM, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR );
	auto format_support_image_r16g16_snorm				= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE, VK_FORMAT_R16G16_SNORM, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR );
	auto format_support_image_r16g16b16_snorm			= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE, VK_FORMAT_R16G16B16_SNORM, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR );
	auto format_support_image_r16g16b16a16_snorm		= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR );
	auto format_support_image_r32_sfloat				= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE, VK_FORMAT_R32_SFLOAT, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR );
	auto format_support_image_r32g32_sfloat				= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR );
	auto format_support_image_r32g32b32_sfloat			= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR );
	auto format_support_image_r32g32b32a32_sfloat		= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::OPTIMAL_IMAGE, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR );

	auto format_support_buffer_r32_sfloat				= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::BUFFER, VK_FORMAT_R32_SFLOAT, VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT );
	auto format_support_buffer_r32g32_sfloat			= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::BUFFER, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT );
	auto format_support_buffer_r32g32b32_sfloat			= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::BUFFER, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT );
	auto format_support_buffer_r32g32b32a32_sfloat		= renderer->IsFormatSupported( AE::FORMAT_PROPERTIES_FEATURE::BUFFER, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT );

	/*
	auto file_resman	= engine.GetFileResourceManager();
	auto renderer		= engine.GetRenderer();
	auto device_resman	= renderer->GetDeviceResourceManager();

	auto descriptor_pool	= renderer->GetDescriptorPoolManager();
	auto set				= descriptor_pool->AllocateDescriptorSetForCamera();
	*/

	{
		auto scene_camera	= scene_manager->GetActiveScene()->CreateChild( AE::SceneBase::Type::CAMERA );
		auto scene_node		= scene_manager->GetActiveScene()->CreateChild( AE::SceneBase::Type::SHAPE, "data/scene_nodes/objects/shapes/torus_knot.xml" );

		auto tim	= std::chrono::high_resolution_clock();
		auto t1		= tim.now();
		auto t2		= t1;
		uint64_t	fps		= 0;

		while( engine.Run() ) {

			// simulate one frame in about 60 fps
			std::this_thread::sleep_for( std::chrono::milliseconds( 16 ) );

			auto command_buffer		= renderer->BeginRender();
			renderer->Command_BeginRenderPass( command_buffer, VK_SUBPASS_CONTENTS_INLINE );
			vkCmdNextSubpass( command_buffer, VK_SUBPASS_CONTENTS_INLINE );
			vkCmdEndRenderPass( command_buffer );

			renderer->EndRender( command_buffer );
			
			++fps;

			t2 = tim.now();
			if( std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count() > 1000 ) {
				t1 = t2;
				std::cout << fps << std::endl;
				fps = 0;
			}
		}
	}

	return 0;
}
