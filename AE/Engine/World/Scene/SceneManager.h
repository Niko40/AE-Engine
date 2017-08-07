#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Memory/MemoryTypes.h"
#include "../../Math/Math.h"

namespace AE
{

class Engine;
class Logger;
class Renderer;
class DescriptorPoolManager;
class World;
class Scene;
class SceneNode;

// Scene manager is responsible for allocating, removing and updating individual objects on the world
// It's also responsible for updating all logic, SceneNode_Unit behaviours and AI, handling world events and triggers, physics and animations
class SceneManager
{
public:
	SceneManager( Engine * engine, World * world );
	~SceneManager();

	// general update called once a frame
	void									Update();

	void									UpdateLogic();
	void									UpdateAnimations();

	Scene								*	GetActiveScene() const;
	Scene								*	GetGridScene( Vec3 world_coords ) const;

private:
	Engine								*	p_engine					= nullptr;
	Logger								*	p_logger					= nullptr;
	Renderer							*	p_renderer					= nullptr;
	World								*	p_world						= nullptr;

	DescriptorPoolManager				*	p_active_scene_descriptor_pool_manager	= nullptr;

	UniquePointer<Scene>					active_scene;
	DynamicGrid2D<SharedPointer<Scene>>		grid_nodes;
};

}
