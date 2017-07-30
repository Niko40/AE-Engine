#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Memory/MemoryTypes.h"

#include "../../FileResource/XML/FileResource_XML.h"

namespace AE
{

class Engine;
class FileResourceManager;
class DeviceResourceManager;
class SceneManager;
class SceneNode;

class FileResource_XML;

class SceneNodeBase
{
public:
	enum class Type
	{
		UNDEFINED							= 0,
		SCENE,								// Special, only used by scene

		// Simple types
		SHAPE,

		// Units

		// Other types
	};

	enum class ResourcesLoadState : uint32_t
	{
		UNDEFINED							= 0,		// Error value

		READY,
		NOT_READY,
		UNABLE_TO_LOAD,
	};

											SceneNodeBase( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path, SceneNodeBase::Type scene_node_type );
	virtual									~SceneNodeBase();

	SceneNode							*	CreateChild( SceneNodeBase::Type scene_node_type, const Path & scene_node_path );

	// General update function, call once a frame
	void									UpdateFromManager();

	// check is the primary file resource parsed, this IS NOT recursive to childs
	bool									IsConfigFileParsed();

	// check is the primary file resource parsed, this IS NOT recursive to childs
	bool									IsConfigFileLoaded();

	// check is the scene node is ready to use in general updates and renders, this IS NOT recursive to childs
	bool									IsSceneNodeUseReady();

	const Path							&	GetConfigFilePath();

protected:
	// General update function, call once a frame
	virtual void							Update()						= 0;

	// Parse config file, this is called from UpdateFromManager()
	virtual bool							ParseConfigFile()				= 0;

	// After parsing the config file there might be some resources that need loading
	// before using the scene node, after this function returns true the Finalize()
	// function is called to finalize all loaded resources or data
	virtual ResourcesLoadState				CheckResourcesLoaded()			= 0;

	// checking that all resources are properly loaded, this is the next function that
	// gets called, the purpose of this is to finalize all data for the scene node to be
	// able to use it when updating or rendering this scene node
	// should return true on success and false if something went wrong in which case
	// this scene node will not participate in updates or rendering operations
	virtual bool							Finalize()						= 0;

	Engine								*	p_engine						= nullptr;
	SceneManager						*	p_scene_manager					= nullptr;
	FileResourceManager					*	p_file_resource_manager			= nullptr;
	DeviceResourceManager				*	p_device_resource_manager		= nullptr;

	FileResourceHandle<FileResource_XML>	config_file						= nullptr;

	String									name;
	bool									is_visible						= true;

private:
	Type									type							= Type::UNDEFINED;
	Path									config_file_path;
	bool									is_config_file_parsed			= false;
	bool									is_scene_node_use_ready			= false;
	bool									is_scene_node_ok				= true;
	List<UniquePointer<SceneNode>>			child_list;
};

}
