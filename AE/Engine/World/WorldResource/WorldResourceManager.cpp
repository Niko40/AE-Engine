
#include <assert.h>

#include "WorldResourceManager.h"

#include "../../Engine.h"

namespace AE
{

WorldResourceManager::WorldResourceManager( Engine * engine, World * world )
{
	assert( nullptr != engine );
	assert( nullptr != world );
	p_engine			= engine;
	p_logger			= p_engine->GetLogger();
	p_resource_manager	= p_engine->GetFileResourceManager();
	p_world				= world;
	assert( nullptr != p_logger );
	assert( nullptr != p_resource_manager );
}

WorldResourceManager::~WorldResourceManager()
{
}

}
