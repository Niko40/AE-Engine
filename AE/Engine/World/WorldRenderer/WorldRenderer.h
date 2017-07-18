#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

namespace AE
{

class Engine;
class Logger;
class World;

// World renderer is mostly responsible for partitioning the world into chunks,
// creating, deleting and updating secondary command buffers on those chunks
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
};

}
