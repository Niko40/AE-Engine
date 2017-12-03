
#include "Shape.h"

#include "../../../../Engine.h"
#include "../../../../Logger/Logger.h"
#include "../../../../Renderer/DeviceResource/DeviceResourceManager.h"
#include "../../../../Renderer/DeviceResource/Mesh/DeviceResource_Mesh.h"
#include "../../../../FileResource/XML/FileResource_XML.h"

namespace AE
{

SceneNode_Shape::SceneNode_Shape( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path )
	: SceneNode_Object( engine, scene_manager, scene_node_path, SceneNodeBase::Type::SHAPE )
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
	return CheckResourcesLoadedHelper( CheckResourcesLoaded_ObjectLevel(), [ this ]() {
		// check requested resources on shape level
		return SceneNodeBase::ResourcesLoadState::READY;
	} );
}

bool SceneNode_Shape::FinalizeResources()
{
	return FinalizeResourcesHelper( FinalizeResources_ObjectLevel(), [ this ]() {
		// finalize shape level stuff
		return true;
	} );
}

void SceneNode_Shape::Update_Animation()
{
}

void SceneNode_Shape::Update_Logic()
{
}

}
