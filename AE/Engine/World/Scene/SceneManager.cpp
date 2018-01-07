
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

	TODO( "Add multithreading support for the scene update" );

	active_scene	= MakeUniquePointer<Scene>( p_engine, this, "no file for now, todo" );
}

SceneManager::~SceneManager()
{
}

void SceneManager::Update()
{
	active_scene->UpdateResourcesFromManager();
	TODO( "Grid nodes not enabled at this point." );
	/*
	for( auto & s : grid_nodes ) {
		if( nullptr != s.Get() ) {
			s->UpdateResourcesFromManager();
		}
	}
	*/
}

Scene * SceneManager::GetActiveScene() const
{
	return active_scene.Get();
}

}
