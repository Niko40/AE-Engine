
#include <assert.h>

#include "SubSystem.h"

#include "Memory/Memory.h"
#include "Engine.h"
#include "Logger/Logger.h"

namespace AE
{

SubSystem::SubSystem( Engine * engine, String name )
{
	assert( nullptr != engine );
	p_engine			= engine;
	p_logger			= engine->GetLogger();
	sub_system_name		= name;
	p_logger->LogInfo( sub_system_name + " sub system created" );
}

SubSystem::~SubSystem()
{
	p_logger->LogInfo( sub_system_name + " sub system destroyed" );
}

}
