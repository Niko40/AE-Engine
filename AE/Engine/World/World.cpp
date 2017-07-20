
#include "World.h"
#include "../Memory/Memory.h"
#include "../FileSystem/FileSystem.h"

#include "../Engine.h"
#include "Scene/SceneManager.h"
#include "WorldRenderer/WorldRenderer.h"

namespace AE
{

World::World( Engine * engine, Path & path )
{
	assert( nullptr != engine );
	p_engine		= engine;
	p_filesystem	= engine->GetFileSystem();
	p_logger		= engine->GetLogger();
	assert( nullptr != p_filesystem );
	assert( nullptr != p_logger );

	scene_manager			= MakeUniquePointer<SceneManager>( p_engine, this );
	world_renderer			= MakeUniquePointer<WorldRenderer>( p_engine, this );
}

World::~World()
{
}

void World::Update()
{
	scene_manager->UpdateLogic();
	scene_manager->UpdateAnimations();

	world_renderer->UpdateSecondaryCommandBuffers();
}

void World::Render()
{
	world_renderer->Render();
}

SceneManager * World::GetSceneManager() const
{
	return scene_manager.Get();
}

WorldRenderer * World::GetWorldRenderer() const
{
	return world_renderer.Get();
}

}
