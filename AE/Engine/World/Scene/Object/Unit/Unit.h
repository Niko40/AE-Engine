#pragma once

#include "../../../../BUILD_OPTIONS.h"
#include "../../../../Platform.h"

#include "../Object.h"

namespace AE
{
class SceneNode_Unit : public SceneNode_Object
{
public:
								SceneNode_Unit( Engine * engine, SceneManager * scene_manager, SceneBase * parent, const Path & scene_node_path, SceneBase::Type scene_node_type );
	virtual						~SceneNode_Unit();

protected:
	tinyxml2::XMLElement	*	ParseConfigFile_UnitLevel();
	ResourcesLoadState			CheckResourcesLoaded_UnitLevel();
	bool						Finalize_UnitLevel();

private:

};

}
