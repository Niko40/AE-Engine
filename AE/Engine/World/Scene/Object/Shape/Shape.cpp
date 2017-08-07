
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

void SceneNode_Shape::Update()
{
}

bool SceneNode_Shape::ParseConfigFile()
{
	assert( config_file->IsResourceReadyForUse() );		// config file resource should have been loaded before this function is called

	auto object_level	= ParseConfigFile_ObjectLevel();
	if( nullptr != object_level ) {
		auto shape_level	= object_level->FirstChildElement( "SHAPE" );
		if( nullptr != shape_level ) {
			// Parse shape level stuff

			return true;
		}
	}
	return false;
}

SceneNodeBase::ResourcesLoadState SceneNode_Shape::CheckResourcesLoaded()
{
	auto object_level	= CheckResourcesLoaded_ObjectLevel();
	if( object_level == ResourcesLoadState::READY ) {
		// check requested resources on shape level
		return ResourcesLoadState::READY;
	}
	return object_level;
}

bool SceneNode_Shape::Finalize()
{
	if( Finalize_ObjectLevel() ) {
		// finalize shape level stuff
		return true;
	}
	return false;
}

}
