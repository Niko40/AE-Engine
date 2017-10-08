
#include "Shape.h"

#include "../../../../Engine.h"
#include "../../../../Logger/Logger.h"
#include "../../../../Renderer/DeviceResource/DeviceResourceManager.h"
#include "../../../../Renderer/DeviceResource/Mesh/DeviceResource_Mesh.h"
#include "../../../../FileResource/XML/FileResource_XML.h"

namespace AE
{

SceneNode_Shape::SceneNode_Shape( Engine * engine, SceneManager * scene_manager, DescriptorPoolManager * descriptor_pool_manager, const Path & scene_node_path )
	: SceneNode_Object( engine, scene_manager, descriptor_pool_manager, scene_node_path, SceneNodeBase::Type::SHAPE )
{
}

SceneNode_Shape::~SceneNode_Shape()
{
}

bool SceneNode_Shape::ParseConfigFile()
{
	assert( config_file->IsResourceReadyForUse() );		// config file resource should have been loaded before this function is called

	return ParseConfigFileHelper( ParseConfigFile_ObjectLevel(), "SHAPE", [ this ]() {
		// Parse shape level stuff here
		return true;
	} );
}

SceneNodeBase::ResourcesLoadState SceneNode_Shape::CheckResourcesLoaded()
{
	auto object_level_resources_state	= CheckResourcesLoaded_ObjectLevel();
	if( object_level_resources_state == ResourcesLoadState::READY ) {
		// check requested resources on shape level
		return ResourcesLoadState::READY;
	}
	return object_level_resources_state;
}

bool SceneNode_Shape::FinalizeResources()
{
	if( FinalizeResources_ObjectLevel() ) {
		// finalize shape level stuff
		return true;
	}
	return false;
}

void SceneNode_Shape::Update_Animation()
{
}

void SceneNode_Shape::Update_Logic()
{
}

}
