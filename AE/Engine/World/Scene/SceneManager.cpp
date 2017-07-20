
#include <assert.h>

#include "SceneManager.h"

#include "../../Engine.h"
#include "../../Logger/Logger.h"
#include "../../Memory/Memory.h"
#include "../Scene/Scene.h"

namespace AE
{

SceneManager::SceneManager( Engine * engine, World * world )
{
	p_engine		= engine;
	p_world			= world;
	p_logger		= engine->GetLogger();
	assert( nullptr != p_engine );
	assert( nullptr != p_world );
	assert( nullptr != p_logger );

	TODO( "Add multithreading support for the scene update" );

	active_scene	= MakeUniquePointer<Scene>( p_engine, this );
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
