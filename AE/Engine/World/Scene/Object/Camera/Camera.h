#pragma once

#include "../../../../BUILD_OPTIONS.h"
#include "../../../../Platform.h"

#include "../Object.h"

namespace AE
{

class SceneNode_Camera : public SceneNode_Object
{
public:
	SceneNode_Camera( Engine * engine, SceneManager * scene_manager, DescriptorPoolManager * descriptor_pool_manager, Path & scene_node_path );
	~SceneNode_Camera();

	Mat4						&	CalculateViewMatrix();
	Mat4						&	CalculateProjectionMatrix( double fov_angle, VkExtent2D viewport_size, double near_plane, double far_plane );

private:
	void							Update();
	bool							ParseConfigFile();
	ResourcesLoadState				CheckResourcesLoaded();
	bool							Finalize();

	Mat4							view_matrix							= Mat4( 1 );
	Mat4							projection_matrix					= Mat4( 1 );

	UniquePointer<UniformBuffer>	uniform_buffer						= nullptr;
	DescriptorSetHandle				uniform_buffer_descriptor_set		= nullptr;
};

}
