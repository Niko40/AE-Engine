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
	struct MeshInfo
	{
		bool											is_ok			= true;
		bool											visible			= true;
		Path											xml_mesh_path;
		Path											xml_shader_path;
		Path											xml_diffuse_1_path;
		DeviceResourceHandle<DeviceResource_Mesh>		mesh;
		DeviceResourceHandle<DeviceResource_Image>		diffuse_1;
	};

	SceneNode_Shape( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path );
	~SceneNode_Shape();

	void									Update();

	bool									ParseConfigFile();

private:
	Vector<MeshInfo>						mesh_info_list;
};

}
