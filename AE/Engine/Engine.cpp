
#include "Engine.h"

// shared
#include "Memory/Memory.h"
#include "Logger/Logger.h"

// sub systems
#include "FileResource/FileResourceManager.h"
#include "Window/WindowManager.h"
#include "Renderer/Renderer.h"
#include "Renderer/DeviceResource/DeviceResourceManager.h"

// specific
#include "World/World.h"

namespace AE
{

EngineBase::EngineBase()
{
	logger				= MakeUniquePointer<Logger>( "Engine.log" );
	logger->LogInfo( "Engine created" );
}

EngineBase::~EngineBase()
{
	logger->LogInfo( "Engine destroyed" );
}

Logger * EngineBase::GetLogger()
{
	return logger.Get();
}

Engine::Engine()
{
	filesystem				= MakeUniquePointer<FileSystem>( this );

	file_resource_manager	= MakeUniquePointer<FileResourceManager>( this );

	file_resource_manager->AllowResourceRequests( true );
	file_resource_manager->AllowResourceLoading( true );
	file_resource_manager->AllowResourceUnloading( true );

	window_manager			= MakeUniquePointer<WindowManager>( this );

	renderer				= MakeUniquePointer<Renderer>( this, "testapp", VK_MAKE_VERSION( 0, 0, 1 ) );

	window_manager->OpenWindow( 800, 600, "testwindow", false );
	renderer->InitializeRenderToWindow( window_manager.Get() );

	window_manager->ShowWindow( true );
}


Engine::~Engine()
{
	active_world	= nullptr;

	renderer->GetDeviceResourceManager()->AllowResourceRequests( false );
	renderer->GetDeviceResourceManager()->WaitJobless();
	renderer->GetDeviceResourceManager()->ScrapDeviceResources();

	renderer->DeInitializeRenderToWindow();
	window_manager->CloseWindow();

	file_resource_manager->AllowResourceRequests( false );
	file_resource_manager->WaitJobless();
	file_resource_manager->ScrapFileResources();
}

bool Engine::Run()
{
	bool keep_running = true;
	file_resource_manager->Update();
	if( active_world ) {
		active_world->Update();
	}
	renderer->Update();
	if( !window_manager->Update() ) {
		keep_running = false;
	}

	return keep_running;
}

FileSystem * Engine::GetFileSystem()
{
	return filesystem.Get();
}

FileResourceManager * Engine::GetFileResourceManager()
{
	return file_resource_manager.Get();
}

WindowManager * Engine::GetWindowManager()
{
	return window_manager.Get();
}

Renderer * Engine::GetRenderer()
{
	return renderer.Get();
}

World * Engine::CreateWorld( Path path )
{
	active_world		= MakeUniquePointer<World>( this, path );
	return active_world.Get();
}

World * Engine::GetWorld()
{
	return active_world.Get();
}

}
