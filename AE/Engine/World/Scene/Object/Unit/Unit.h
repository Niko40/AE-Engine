#pragma once

#include "../../../../BUILD_OPTIONS.h"
#include "../../../../Platform.h"

#include "../Object.h"

namespace AE
{
class SceneNode_Unit : public SceneNode_Object
{
public:
						SceneNode_Unit( Engine * engine, SceneManager * scene_manager, SceneNodeBase::Type scene_node_type );
	virtual				~SceneNode_Unit();

private:

};

}
