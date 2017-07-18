#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "SceneNodeBase.h"

namespace AE
{

class Engine;
class SceneManager;

class Scene : public SceneNodeBase
{
public:
	Scene( Engine * engine, SceneManager * scene_manager );
	~Scene();

private:

};

}
