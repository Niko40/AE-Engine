
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

		while( engine.Run() ) {

			// simulate one frame in about 60 fps
			std::this_thread::sleep_for( std::chrono::milliseconds( 17 ) );

			auto command_buffer		= renderer->BeginRender();
			renderer->Command_BeginRenderPass( command_buffer, VK_SUBPASS_CONTENTS_INLINE );
			vkCmdNextSubpass( command_buffer, VK_SUBPASS_CONTENTS_INLINE );
			vkCmdEndRenderPass( command_buffer );

			renderer->EndRender( command_buffer );
		}
	}

	return 0;
}
