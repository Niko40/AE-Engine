#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

namespace AE
{

class Engine;
class Logger;
class World;
class Renderer;

// World renderer is mostly responsible for partitioning the world into segments 
class WorldRenderer
{
public:
	WorldRenderer( Engine * engine, World * world );
	~WorldRenderer();

	void UpdateSecondaryCommandBuffers();
	void Render();
private:
	Engine						*	p_engine					= nullptr;
	Logger						*	p_logger					= nullptr;
	World						*	p_world						= nullptr;
	Renderer					*	p_renderer					= nullptr;
};

}
