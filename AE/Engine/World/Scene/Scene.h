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

private:
	void							Update_Animation();
	void							Update_Logic();

	bool							ParseConfigFile();
	ResourcesLoadState				CheckResourcesLoaded();
	bool							FinalizeResources();
};

}
