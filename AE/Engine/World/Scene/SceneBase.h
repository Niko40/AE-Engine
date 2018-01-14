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

// SceneBase post creation function call order:
//	- Update_ResoureAvailability()
//	  ( This function will initiate XML parser, handle extra resource loads and finalize resources.
//		After everything is set and loaded a normal update loops can begin.
//		All functions below can be called at odd times to save on CPU resources,
//		first frame calls every function but after that call per frame is not guaranteed.
//		Maximum call frequency is once a frame )
//		- Update_Logic()			= From every frame to once a minute call frequency
//		- Update_Animation()		= Frequency depending on the object AABB size on screen, from every frame to once a second
//		- Update_Buffers()			= Always called just after Update_Animation() and just before RecordCommand_Render() 
//		- RecordCommand_Transfer()	= Can be called once a frame or once in the lifetime of the object
//		- RecordCommand_Render()	= Can be called once a frame or once in the lifetime of the object
			

class SceneBase
{
	friend class SceneManager;
	friend void ConfigResourceCheckerAndLoader( SceneBase * sb );

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

											SceneBase( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path, SceneBase::Type scene_node_type );
	virtual									~SceneBase();

	SceneNode							*	CreateChild( SceneBase::Type scene_node_type, Path scene_node_path = "" );
	Vector<SceneNode*>						GetChildNodes();

	// Resource update function, call every once in a while to check if resources
	// are loaded in and we can use this object, this function IS recursive to childs
	// You only need to call this function until all resources have been loaded in
	void									Update_ResoureAvailability();

	// check is the primary file resource parsed, this IS NOT recursive to childs
	bool									IsConfigFileParsed();

	// check is the primary file resource loaded, this IS NOT recursive to childs
	bool									IsConfigFileLoaded();

	// check is the scene node is ready to use in general updates and renders, this IS NOT recursive to childs
	bool									IsSceneNodeUseReady();

	const Path							&	GetConfigFilePath();

	// Logic update function, called once a frame or as needed.
	// This will update all logic attached to the scene node like
	// scripts, AI and other general updates.
	virtual void							Update_Logic()					= 0;

	// Animation update function, called once a frame or as needed.
	// This will update all animations on the object like bone animations,
	// shape key animations and all updates of a mesh on internal buffers.
	virtual void							Update_Animation()				= 0;

	// GPU update function, this is called once a frame or as needed.
	// This function updates data on the Vulkan buffers that are later copied
	// onto the GPU buffers. This function will also update the GPU about everything
	// else about this scene object.
	virtual void							Update_Buffers()				= 0;

	// We only use one mesh and one graphics pipeline per scene node. The render order is determined by renderer
	// and to help maximize performance, scene nodes are queued up to minimize pipeline swaps.
	// This requires the scene node to report what pipeline it's using.
	// Either return the pipeline used or VK_NULL_HANDLE. Null pipeline will not render anything
	virtual VkPipeline						GetGraphicsPipeline()			= 0;

	// Record transfer commands onto the Vulkan command buffer.
	// This function should only use commands that transfer on-the-fly data between buffers.
	// Provided command buffer in parameters will run in the primary render queue family,
	// meaning that all buffers should be owned by or visible in primary render queue family.
	virtual void							RecordCommand_Transfer( VkCommandBuffer command_buffer )									= 0;

	// Record render commands onto the Vulkan command buffer.
	// This function should only use commands that render objects.
	// Provided command buffer in parameters will run in the primary render queue family,
	// meaning that all buffers should be owned by or visible in primary render queue family.
	// You do NOT need to bind the pipeline, it has already been binded by the renderer
	virtual void							RecordCommand_Render( VkCommandBuffer command_buffer, VkPipelineLayout pipeline_layout )	= 0;

protected:
	// Parse config file, this is first called from Update_ResoureAvailability(),
	// afterwards CheckResourcesLoaded() is called once in a while to check resource
	// availability.
	// Returns true if everything was done correctly, false if there were errors in
	// which case this object will not partake in any actions in the world, including rendering.
	virtual bool							ParseConfigFile()				= 0;

	// After parsing the config file there might be some resources that need loading
	// before using the scene node, after ParseConfigFile() function returns true the Finalize()
	// function is called to finalize all loaded resources or data.
	// Returns the state of the resources, check the enum, it's self explanatory.
	// In case UNABLE_TO_LOAD, this object will not partake in any actions in the world,
	// including rendering operations.
	virtual ResourcesLoadState				CheckResourcesLoaded()			= 0;

	// After all resources are properly loaded in, this is the next function that
	// gets called, the purpose of this is to finalize all data for the scene node to be
	// able to use it when updating or rendering this scene node
	// should return true on success and false if something went wrong in which case
	// this scene node will not participate in any actions in the world,
	// including rendering operations.
	virtual bool							FinalizeResources()				= 0;

	Engine								*	p_engine						= nullptr;
	SceneManager						*	p_scene_manager					= nullptr;
	FileResourceManager					*	p_file_resource_manager			= nullptr;
	Renderer							*	p_renderer						= nullptr;
	DeviceResourceManager				*	p_device_resource_manager		= nullptr;
	DescriptorPoolManager				*	p_descriptor_pool_manager		= nullptr;

	VulkanDevice							ref_vk_device					= {};

	FileResourceHandle<FileResource_XML>	config_file						= nullptr;

	// Calculates a new transformation matrix from position, scale and rotation
	// Matrix is stored in transformation_matrix variable, it's also returned out of convenience
	const Mat4							&	CalculateTransformationMatrix();

	// inversion of CalculateTransformationMatrixFromPosScaleRot, this one takes in a matrix and
	// calculates a new position, scale and rotation from it
	void									CalculatePosScaleRot( const Mat4 & new_transformations );

	// 3d position of the object
	Vec3									position						= Vec3( 0, 0, 0 );

	// Quaternion rotation of the object
	Quat									rotation						= Quat( 1, 0, 0, 0 );

	// 3d scale of the object
	Vec3									scale							= Vec3( 1, 1, 1 );

	// transformation matrix of the object to be used with other calculations that need it or with rendering
	// this value is not calculated automatically, use CalculateTransformationMatrixFromPosScaleRot before using this
	Mat4									transformation_matrix			= Mat4( 1 );

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


// Parser helper function to hide some boiler plate code
bool										ParseConfigFileHelper( tinyxml2::XMLElement * previous_level, String child_element_name, std::function<bool()> child_element_parser );
SceneBase::ResourcesLoadState				CheckResourcesLoadedHelper( SceneBase::ResourcesLoadState previous_level, std::function<SceneBase::ResourcesLoadState( void )> child_element_parser );
bool										FinalizeResourcesHelper( bool previous_level, std::function<bool( void )> child_parser_function );

}
