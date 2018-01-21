
#include "Scene.h"

namespace AE
{

Scene::Scene( Engine * engine, SceneManager * scene_manager, SceneBase * parent, const Path & scene_node_path )
	: SceneBase( engine, scene_manager, parent, scene_node_path, SceneBase::Type::SCENE )
{
}

Scene::~Scene()
{
}

void Scene::Update_Logic()
{
}

void Scene::Update_Animation()
{
}

void Scene::Update_Buffers()
{
}

VkPipeline Scene::GetGraphicsPipeline()
{
	return VK_NULL_HANDLE;
}

void Scene::RecordCommand_Transfer( VkCommandBuffer command_buffer )
{
}

void Scene::RecordCommand_Render( VkCommandBuffer command_buffer, VkPipelineLayout pipeline_layout )
{
}

bool Scene::ParseConfigFile()
{
	return false;
}

SceneBase::ResourcesLoadState Scene::CheckResourcesLoaded()
{
	return ResourcesLoadState::READY;
}

bool Scene::FinalizeResources()
{
	return true;
}

}
