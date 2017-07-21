
#include "Object.h"

namespace AE
{

SceneNode_Object::SceneNode_Object( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path, SceneNodeBase::Type scene_node_type )
	: SceneNode( engine, scene_manager, scene_node_path, scene_node_type )
{
}

SceneNode_Object::~SceneNode_Object()
{
}

tinyxml2::XMLElement * SceneNode_Object::ParseConfigFile_ObjectSection()
{
	auto parent_element			= ParseConfigFile_SceneNodeSection();
	if( nullptr != parent_element ) {
		auto object_element		= parent_element->FirstChildElement( "OBJECT" );
		if( nullptr != object_element ) {
			// do object level stuff
		}
		return object_element;
	}
	return nullptr;
}

}
