
#include "Object.h"

namespace AE
{

SceneNode_Object::SceneNode_Object( Engine * engine, SceneManager * scene_manager, SceneNodeBase::Type scene_node_type )
	: SceneNode( engine, scene_manager, scene_node_type )
{
}

SceneNode_Object::~SceneNode_Object()
{
}

}
