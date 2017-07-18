#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Memory/MemoryTypes.h"
#include "../../Math/Math.h"

namespace AE
{

class Engine;
class Logger;
class World;
class WorldResourceManager;
class Scene;
class SceneNode;

// Scene manager is responsible for allocating, removing and updating individual objects on the world
// It's also responsible for updating all logic, SceneNode_Unit behaviours and AI, handling world events and triggers, physics and animations
class SceneManager
{
public:
	SceneManager( Engine * engine, World * world, WorldResourceManager * world_resource_manager );
	~SceneManager();

	void								UpdateLogic();
	void								UpdateAnimations();

	Scene							*	GetActiveScene() const;
	Scene							*	GetGridScene( Vec3 world_coords ) const;

private:
	Engine							*	p_engine					= nullptr;
	Logger							*	p_logger					= nullptr;
	World							*	p_world						= nullptr;
	WorldResourceManager			*	p_world_resource_manager	= nullptr;

	UniquePointer<Scene>				active_scene;
//	Grid2D<Scene>						grid_nodes;
};

}
