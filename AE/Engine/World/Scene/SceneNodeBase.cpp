
#include "SceneNodeBase.h"

#include "../../Engine.h"
#include "../../Logger/Logger.h"
#include "../../Memory/Memory.h"

#include <assert.h>

// include all scene node final derivatives here
// Simple objects
#include "Object/Shape/Shape.h"

// Units


namespace AE
{

SceneNodeBase::SceneNodeBase( Engine * engine, SceneManager * scene_manager, SceneNodeBase::Type scene_node_type )
{
	p_engine				= engine;
	p_scene_manager			= scene_manager;
	assert( p_engine );
	assert( p_scene_manager );

	type					= scene_node_type;
	assert( type != Type::UNDEFINED );
}

SceneNodeBase::~SceneNodeBase()
{
}

SceneNode * SceneNodeBase::CreateChild( SceneNodeBase::Type scene_node_type )
{
	UniquePointer<SceneNode> unique = nullptr;

	// create scene node
	switch( scene_node_type ) {

	case Type::SHAPE:
	{
		unique = MakeUniquePointer<SceneNode_Shape>( p_engine, p_scene_manager );
	}

	default:
		p_engine->GetLogger()->LogError( "Can't create scene node, scene node type not recognized" );
		assert( 0 && "Can't create scene node, scene node type not recognized" );
		break;
	}

	if( unique ) {
		// add scene node into the child list
		SceneNode * node_ptr	= unique.Get();
		child_list.push_back( std::move( unique ) );
		return node_ptr;
	}
	return nullptr;
}

}
