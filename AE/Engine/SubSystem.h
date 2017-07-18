#pragma once

#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include "Memory/Memory.h"

namespace AE
{

class Engine;
class Logger;

class SubSystem
{
public:
					SubSystem( Engine * engine, String name );
	virtual			~SubSystem();

private:
	String			sub_system_name;

protected:
	Engine		*	p_engine			= nullptr;
	Logger		*	p_logger			= nullptr;
};

}
