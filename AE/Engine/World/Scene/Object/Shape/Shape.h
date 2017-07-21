#pragma once

#include "../../../../BUILD_OPTIONS.h"
#include "../../../../Platform.h"

#include "../Object.h"

#include "../../../../Renderer/DeviceResource/Mesh/DeviceResource_Mesh.h"
#include "../../../../Renderer/DeviceResource/Image/DeviceResource_Image.h"

namespace AE
{

class SceneNode_Shape : public SceneNode_Object
{
public:
	SceneNode_Shape( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path );
	~SceneNode_Shape();

	void									Update();

	bool									ParseConfigFile();

private:
};

}
