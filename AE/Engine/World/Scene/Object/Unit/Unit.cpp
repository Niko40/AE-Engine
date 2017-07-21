
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

}
