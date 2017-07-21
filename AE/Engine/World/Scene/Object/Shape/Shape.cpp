
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

void SceneNode_Shape::Update()
{
}

bool SceneNode_Shape::ParseConfigFile()
{
	assert( config_file->IsReadyForUse() );		// config file resource should be loaded before this function is called

	auto object_level	= ParseConfigFile_ObjectSection();
	if( nullptr != object_level ) {
		auto shape_level	= object_level->FirstChildElement( "SHAPE" );
		if( nullptr != shape_level ) {
			// Do shape level stuff

			return true;
		}
	}
	return false;
}

}
