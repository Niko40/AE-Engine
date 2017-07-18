
#include <assert.h>

#include "WorldRenderer.h"

#include "../../Engine.h"

namespace AE
{

WorldRenderer::WorldRenderer( Engine * engine, World * world )
{
	assert( nullptr != engine );
	assert( nullptr != world );
	p_engine			= engine;
	p_world				= world;
	p_logger			= engine->GetLogger();
	assert( nullptr != p_logger );

	TODO( "Add multithreading support for the world renderer secondary command buffer update" );
}

WorldRenderer::~WorldRenderer()
{
}

void WorldRenderer::UpdateSecondaryCommandBuffers()
{
}

void WorldRenderer::Render()
{
}

}
