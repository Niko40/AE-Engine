
#include <assert.h>

#include "SceneManager.h"

#include "../../Engine.h"
#include "../../Logger/Logger.h"
#include "../../Memory/Memory.h"
#include "../WorldResource/WorldResourceManager.h"

namespace AE
{

SceneManager::SceneManager( Engine * engine, World * world, WorldResourceManager * world_resource_manager )
{
	assert( nullptr != engine );
	assert( nullptr != world );
	assert( nullptr != world_resource_manager );
	p_engine					= engine;
	p_world						= world;
	p_world_resource_manager	= world_resource_manager;
	p_logger					= engine->GetLogger();
	assert( nullptr != p_logger );

	TODO( "Add multithreading support for the scene update" );

	active_scene				= MakeUniquePointer<Scene>( p_engine, this );
}

SceneManager::~SceneManager()
{
}

void SceneManager::UpdateLogic()
{
}

void SceneManager::UpdateAnimations()
{
}

Scene * SceneManager::GetActiveScene() const
{
	return active_scene.Get();
}

}
