#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

namespace AE
{

class Engine;
class Logger;
class FileResourceManager;
class World;

// World resource manager is responsible for "on the fly" data allocations,
// uploads to the physical devices and basic memory management
class WorldResourceManager
{
public:
	WorldResourceManager( Engine * engine, World * world );
	~WorldResourceManager();

private:
	Engine							*	p_engine					= nullptr;
	Logger							*	p_logger					= nullptr;
	FileResourceManager					*	p_resource_manager			= nullptr;
	World							*	p_world						= nullptr;
};

}
