#pragma once

#include "../../../../BUILD_OPTIONS.h"
#include "../../../../Platform.h"

#include "../Object.h"

namespace AE
{

class SceneNode_Shape : public SceneNode_Object
{
public:
	SceneNode_Shape( Engine * engine, SceneManager * scene_manager, DescriptorPoolManager * descriptor_pool_manager, const Path & scene_node_path );
	~SceneNode_Shape();

	void									Update();
	bool									ParseConfigFile();
	ResourcesLoadState						CheckResourcesLoaded();
	bool									Finalize();

private:
};

}
