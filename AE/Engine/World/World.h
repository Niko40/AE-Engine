#pragma once

#include "../BUILD_OPTIONS.h"
#include "../Platform.h"

#include "../Memory/MemoryTypes.h"
#include "../FileSystem/FileStream.h"

namespace AE
{

class Engine;
class FileSystem;
class Logger;

class WorldResourceManager;
class SceneManager;
class WorldRenderer;

class World
{
public:
	World( Engine * engine, Path & path );
	~World();

	void									Update();
	void									Render();

	WorldResourceManager				*	GetWorldResourceManager() const;
	SceneManager						*	GetSceneManager() const;
	WorldRenderer						*	GetWorldRenderer() const;

private:
	Engine								*	p_engine							= nullptr;
	FileSystem							*	p_filesystem						= nullptr;
	Logger								*	p_logger							= nullptr;

	UniquePointer<WorldResourceManager>		world_resource_manager;
	UniquePointer<SceneManager>				scene_manager;
	UniquePointer<WorldRenderer>			world_renderer;
};

}
