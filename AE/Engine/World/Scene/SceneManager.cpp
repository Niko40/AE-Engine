
#include <assert.h>

#include "SceneManager.h"

#include "../../Engine.h"
#include "../../Logger/Logger.h"
#include "../../Renderer/Renderer.h"
#include "../../Memory/Memory.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneNode.h"

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

	active_scene	= MakeUniquePointer<Scene>( p_engine, this, "" );
}

SceneManager::~SceneManager()
{
}

void SceneManager::Update()
{
	TODO( "Implement update frequencies for different types of scene updates" );
	Vector<SceneBase*> collection;
	CollectAllChildSceneBases( active_scene.Get(), &collection );
	for( auto sbase : collection ) {
		sbase->Update_ResoureAvailability();
		sbase->Update_Logic();
		sbase->Update_Animation();
		sbase->Update_GPU();
	}

	TODO( "Grid nodes not enabled at this point." );
	/*
	for( auto & s : grid_nodes ) {
		if( nullptr != s.Get() ) {
			s->Update_ResoureAvailability();
		}
	}
	*/
}

Scene * SceneManager::GetActiveScene() const
{
	return active_scene.Get();
}

void CollectAllChildSceneBases( SceneBase * node, Vector<SceneBase*> * return_collection )
{
	return_collection->push_back( node );
	for( auto i : node->GetChildNodes() ) {
		CollectAllChildSceneBases( i, return_collection );
	}
}

}
