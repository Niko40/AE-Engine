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
		UNDEFINED						= 0,
		SCENE,							// Special, only used by scene

		// Simple types
		SHAPE,

		// Units

		// Other types
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

	const Path							&	GetConfigFilePath();

protected:
	// General update function, call once a frame
	virtual void							Update()						= 0;

	// Parse config file, this is called from UpdateFromManager()
	virtual bool							ParseConfigFile()				= 0;

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
	bool									is_scene_node_ok				= true;
	List<UniquePointer<SceneNode>>			child_list;
};

}
