
#include <assert.h>

#include "SceneManager.h"

#include "../../Engine.h"
#include "../../Logger/Logger.h"
#include "../../Renderer/Renderer.h"
#include "../../Memory/Memory.h"
#include "../Scene/Scene.h"

namespace AE
{

SceneManager::SceneManager( Engine * engine, World * world )
{
	p_engine		= engine;
	p_world			= world;
	assert( p_engine );
	assert( p_world );
	p_logger		= p_engine->GetLogger();
	p_renderer		= p_engine->GetRenderer();
	assert( p_logger );
	assert( p_renderer );
	p_active_scene_descriptor_pool_manager	= p_renderer->GetDescriptorPoolManagerForThisThread();
	assert( p_active_scene_descriptor_pool_manager );

	TODO( "Add multithreading support for the scene update" );

	active_scene	= MakeUniquePointer<Scene>( p_engine, this, p_active_scene_descriptor_pool_manager, "no file for now, todo" );
}

SceneManager::~SceneManager()
{
}

void SceneManager::Update()
{
	active_scene->UpdateFromManager();
	for( auto & s : grid_nodes ) {
		if( nullptr != s.Get() ) {
			s->UpdateFromManager();
		}
	}
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
