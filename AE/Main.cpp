
#include <vector>
#include <string>

#include <lua.hpp>
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

	auto file_resman	= engine.GetFileResourceManager();
	auto renderer		= engine.GetRenderer();
	auto device_resman	= renderer->GetDeviceResourceManager();

	{
		auto scene_node		= scene_manager->GetActiveScene()->CreateChild( AE::SceneNodeBase::Type::SHAPE, "data/scene_nodes/objects/shapes/torus_knot.xml" );

		while( engine.Run() ) {

			// simulate one frame in about 60 fps
			std::this_thread::sleep_for( std::chrono::milliseconds( 17 ) );
		}
	}

	return 0;
}
