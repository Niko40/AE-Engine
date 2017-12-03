#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Memory/MemoryTypes.h"
#include "../../Vulkan/Vulkan.h"

#include "../../FileResource/XML/FileResource_XML.h"

namespace AE
{

class Engine;
class Renderer;
class FileResourceManager;
class DeviceResourceManager;
class DescriptorPoolManager;
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

		CAMERA,

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

	// Resource update function, call until all resources used by this scene node are fully loaded and ready to use
	void									UpdateResourcesFromManager();

	// check is the primary file resource parsed, this IS NOT recursive to childs
	bool									IsConfigFileParsed();

	// check is the primary file resource loaded, this IS NOT recursive to childs
	bool									IsConfigFileLoaded();

	// check is the scene node is ready to use in general updates and renders, this IS NOT recursive to childs
	bool									IsSceneNodeUseReady();

	const Path							&	GetConfigFilePath();

protected:
	// Animation update function, call once a frame or as needed.
	// This will update all animations on the object and push new data into
	// Vulkan buffers residing in system RAM.
	virtual void							Update_Animation()				= 0;

	// Logic update function, call once a frame or as needed.
	// This will update all logic attached to the scene node, animation changes,
	// AI and scripts.
	virtual void							Update_Logic()					= 0;

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
	virtual bool							FinalizeResources()				= 0;

	Engine								*	p_engine						= nullptr;
	SceneManager						*	p_scene_manager					= nullptr;
	FileResourceManager					*	p_file_resource_manager			= nullptr;
	Renderer							*	p_renderer						= nullptr;
	DeviceResourceManager				*	p_device_resource_manager		= nullptr;
	DescriptorPoolManager				*	p_descriptor_pool_manager		= nullptr;	// Assigned later when we actually know the thread we need one from

	VulkanDevice							ref_vk_device					= {};

	FileResourceHandle<FileResource_XML>	config_file						= nullptr;

	String									name;
	bool									is_visible						= true;

private:
	Type									type							= Type::UNDEFINED;
	Path									config_file_path;
	bool									is_config_file_parsed			= false;
	bool									is_scene_node_use_ready			= false;
	bool									is_scene_node_ok				= true;
	// used to limit certain operations into only one thread, needed for Vulkan pools
	std::thread::id							thread_specific;
	List<UniquePointer<SceneNode>>			child_list;
};


// Parser helper function to hide some boiler plate code
bool										ParseConfigFileHelper( tinyxml2::XMLElement * previous_level, String child_element_name, std::function<bool()> child_element_parser );
SceneNodeBase::ResourcesLoadState			CheckResourcesLoadedHelper( SceneNodeBase::ResourcesLoadState previous_level, std::function<SceneNodeBase::ResourcesLoadState( void )> child_element_parser );
bool										FinalizeResourcesHelper( bool previous_level, std::function<bool( void )> child_parser_function );

}
