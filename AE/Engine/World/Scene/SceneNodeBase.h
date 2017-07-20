#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Memory/MemoryTypes.h"

namespace AE
{

class Engine;
class SceneManager;
class SceneNode;

class SceneNodeBase
{
public:
	enum class Type
	{
		UNDEFINED						= 0,
		SCENE,							// Special, only used by scene

		// Simple types
		SHAPE,

		// Units

		// Other types
	};

										SceneNodeBase( Engine * engine, SceneManager * scene_manager, SceneNodeBase::Type scene_node_type );
	virtual								~SceneNodeBase();

	SceneNode						*	CreateChild( SceneNodeBase::Type scene_node_type );

private:
	Engine							*	p_engine						= nullptr;
	SceneManager					*	p_scene_manager					= nullptr;

	Type								type							= Type::UNDEFINED;

	List<UniquePointer<SceneNode>>		child_list;
};

}
