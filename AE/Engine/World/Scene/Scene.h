#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "SceneBase.h"

namespace AE
{

class Engine;
class SceneManager;

class Scene : public SceneBase
{
public:
	Scene( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path );
	~Scene();

	void							Update_Logic();
	void							Update_Animation();
	void							Update_Buffers();

	VkPipeline						GetGraphicsPipeline();

	void							RecordCommand_Transfer( VkCommandBuffer command_buffer );
	void							RecordCommand_Render( VkCommandBuffer command_buffer, VkPipelineLayout pipeline_layout );

private:
	bool							ParseConfigFile();
	ResourcesLoadState				CheckResourcesLoaded();
	bool							FinalizeResources();
};

}
