#pragma once

#include <memory>

#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include "Memory/MemoryTypes.h"
#include "FileSystem/FileStream.h"
#include "CppFileSystem\CppFileSystem.h"

namespace AE
{

class Logger;
class FileSystem;
class FileResourceManager;
class Renderer;
class World;

class EngineBase
{
public:
	EngineBase();
	~EngineBase();

	Logger						*	GetLogger();

protected:
	UniquePointer<Logger>			logger;
};

class Engine : public EngineBase
{
public:
	Engine();
	~Engine();

	bool								Run();

	FileSystem						*	GetFileSystem();
	FileResourceManager				*	GetFileResourceManager();
	Renderer						*	GetRenderer();
	
	World							*	CreateWorld( Path path );
	World							*	GetWorld();

private:
	UniquePointer<FileSystem>			filesystem;
	UniquePointer<FileResourceManager>	file_resource_manager;
	UniquePointer<Renderer>				renderer;

	UniquePointer<World>				active_world;
};

}
