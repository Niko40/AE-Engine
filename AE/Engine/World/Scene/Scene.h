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
	Scene( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path );
	~Scene();

private:
	void							Update();

	bool							ParseConfigFile();
};

}
