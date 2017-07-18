
#include "Shape.h"

namespace AE
{

SceneNode_Shape::SceneNode_Shape( Engine * engine, SceneManager * scene_manager )
	: SceneNode_Object( engine, scene_manager, SceneNodeBase::Type::SHAPE )
{
}

SceneNode_Shape::~SceneNode_Shape()
{
}

}
