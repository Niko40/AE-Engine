#pragma once

#include "../../../../BUILD_OPTIONS.h"
#include "../../../../Platform.h"

#include "../Object.h"

namespace AE
{

class SceneNode_Shape : public SceneNode_Object
{
public:
	SceneNode_Shape( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path );
	~SceneNode_Shape();

	void							Update_Logic();
	void							Update_Animation();
	void							Update_GPU();

	void							RecordCommand_Transfer( VkCommandBuffer command_buffer );
	void							RecordCommand_Render( VkCommandBuffer command_buffer );

protected:
	bool							ParseConfigFile();
	ResourcesLoadState				CheckResourcesLoaded();
	bool							FinalizeResources();
};

}
