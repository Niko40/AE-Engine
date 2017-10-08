
#include "Scene.h"

namespace AE
{

Scene::Scene( Engine * engine, SceneManager * scene_manager, DescriptorPoolManager * descriptor_pool_manager, const Path & scene_node_path )
	: SceneNodeBase( engine, scene_manager, descriptor_pool_manager, scene_node_path, SceneNodeBase::Type::SCENE )
{
}

Scene::~Scene()
{
}

void Scene::Update_Animation()
{
}

void Scene::Update_Logic()
{
}

bool Scene::ParseConfigFile()
{
	return false;
}

SceneNodeBase::ResourcesLoadState Scene::CheckResourcesLoaded()
{
	return ResourcesLoadState::READY;
}

bool Scene::FinalizeResources()
{
	return true;
}

}
