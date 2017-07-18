#pragma once

#include "../../../BUILD_OPTIONS.h"
#include "../../../Platform.h"

#include "../SceneNode.h"

namespace AE
{

class SceneNode_Object : public SceneNode
{
public:
					SceneNode_Object( Engine * engine, SceneManager * scene_manager, SceneNodeBase::Type scene_node_type );
	virtual			~SceneNode_Object();

private:

};

}
