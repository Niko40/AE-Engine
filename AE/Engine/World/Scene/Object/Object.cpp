
#include "Object.h"

namespace AE
{

SceneNode_Object::SceneNode_Object( Engine * engine, SceneManager * scene_manager, DescriptorPoolManager * descriptor_pool_manager, const Path & scene_node_path, SceneNodeBase::Type scene_node_type )
	: SceneNode( engine, scene_manager, descriptor_pool_manager, scene_node_path, scene_node_type )
{
}

SceneNode_Object::~SceneNode_Object()
{
}

tinyxml2::XMLElement * SceneNode_Object::ParseConfigFile_ObjectLevel()
{
	auto scene_node_element			= ParseConfigFile_SceneNodeLevel();
	if( nullptr != scene_node_element ) {
		auto object_element		= scene_node_element->FirstChildElement( "OBJECT" );
		if( nullptr != object_element ) {
			// do object level stuff
		}
		return object_element;
	}
	return nullptr;
}

SceneNodeBase::ResourcesLoadState SceneNode_Object::CheckResourcesLoaded_ObjectLevel()
{
	auto scene_node_level	= CheckResourcesLoaded_SceneNodeLevel();
	if( scene_node_level == ResourcesLoadState::READY ) {
		// check requested resources on shape level
		return ResourcesLoadState::READY;
	}
	return scene_node_level;
}

bool SceneNode_Object::Finalize_ObjectLevel()
{
	if( Finalize_SceneNodeLevel() ) {
		// finalize shape level stuff
		return true;
	}
	return false;
}

}
