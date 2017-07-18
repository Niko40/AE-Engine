
#include "Scene.h"

namespace AE
{

Scene::Scene( Engine * engine, SceneManager * scene_manager )
	: SceneNodeBase( engine, scene_manager, SceneNodeBase::Type::SCENE )
{
}

Scene::~Scene()
{
}

}
