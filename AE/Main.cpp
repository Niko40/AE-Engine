
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
