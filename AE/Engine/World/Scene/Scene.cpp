
#include "Scene.h"

namespace AE
{

Scene::Scene( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path )
	: SceneBase( engine, scene_manager, scene_node_path, SceneBase::Type::SCENE )
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
