
#include <vector>
#include <string>

#include <lua.hpp>
#include <tinyxml2.h>

#include "Engine/IncludeAll.h"


int main( int argc, char ** argv )
{
	AE::DynamicGrid2D<int> grid;
	grid[ {0, 0} ] = 5;
	std::cout << grid[ {0, 0} ];

	// parse arguments into a vector
	AE::Vector<AE::String> arguments;
	for( int i=0; i < argc; i++ ) {
		arguments.push_back( AE::String( argv[ i ] ) );
	}

	AE::Engine engine;

	auto world			= engine.CreateWorld( "data/worlds/test.world" );

	auto file_resman	= engine.GetFileResourceManager();
	auto renderer		= engine.GetRenderer();
	auto device_resman	= renderer->GetDeviceResourceManager();

	{
		while( engine.Run() ) {

			// stress test the resource managers
			{
				auto device_image_1		= device_resman->RequestResource_Image( { "data/images/test3.png" } );
				auto device_image_2		= device_resman->RequestResource_Image( { "data/images/test4.png" } );
				auto device_image_3		= device_resman->RequestResource_Image( { "data/images/test3.png" } );
				auto device_mesh_2		= device_resman->RequestResource_Mesh( { "data/models/BlackDragonHead.me3d" } );
				auto device_mesh_3		= device_resman->RequestResource_Mesh( { "data/models/Monkey.me3d" } );
				auto device_mesh_4		= device_resman->RequestResource_Mesh( { "data/models/Monkey.me3d" } );

				std::this_thread::sleep_for( std::chrono::milliseconds( 8 ) );	// simulate one frame time.
			}
			std::this_thread::sleep_for( std::chrono::milliseconds( 8 ) );
		}
	}

	return 0;
}
