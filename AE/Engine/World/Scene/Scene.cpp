
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

void Scene::Update()
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

bool Scene::Finalize()
{
	return true;
}

}
