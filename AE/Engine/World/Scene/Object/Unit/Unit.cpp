
#include "Unit.h"

namespace AE
{

SceneNode_Unit::SceneNode_Unit( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path, SceneNodeBase::Type scene_node_type )
	: SceneNode_Object( engine, scene_manager, scene_node_path, scene_node_type )
{
}

SceneNode_Unit::~SceneNode_Unit()
{
}

tinyxml2::XMLElement * SceneNode_Unit::ParseConfigFile_UnitSection()
{
	auto parent_element			= ParseConfigFile_ObjectSection();
	if( nullptr != parent_element ) {
		auto unit_element		= parent_element->FirstChildElement( "UNIT" );
		if( nullptr != unit_element ) {
			// do unit level stuff
		}
		return unit_element;
	}
	return nullptr;
}

}
