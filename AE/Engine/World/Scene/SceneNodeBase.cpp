
#include "SceneNodeBase.h"

#include "../../Engine.h"
#include "../../Logger/Logger.h"
#include "../../Memory/Memory.h"
#include "../../Renderer/Renderer.h"
#include "../../Renderer/DeviceResource/DeviceResourceManager.h"
#include "../../FileResource/FileResourceManager.h"
#include "../../FileResource/XML/FileResource_XML.h"

#include <assert.h>

// include all scene node final derivatives here
// Simple objects
#include "Object/Shape/Shape.h"

// Units


namespace AE
{

SceneNodeBase::SceneNodeBase( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path, SceneNodeBase::Type scene_node_type )
{
	p_engine					= engine;
	p_scene_manager				= scene_manager;
	assert( p_engine );
	assert( p_scene_manager );
	p_file_resource_manager		= p_engine->GetFileResourceManager();
	p_device_resource_manager	= p_engine->GetRenderer()->GetDeviceResourceManager();
	assert( p_file_resource_manager );
	assert( p_device_resource_manager );

	type					= scene_node_type;
	assert( type != Type::UNDEFINED );

	config_file_path		= scene_node_path;
	config_file				= p_engine->GetFileResourceManager()->RequestResource( scene_node_path );
}

SceneNodeBase::~SceneNodeBase()
{
}

SceneNode * SceneNodeBase::CreateChild( SceneNodeBase::Type scene_node_type, const Path & scene_node_path )
{
	UniquePointer<SceneNode> unique = nullptr;

	// create scene node
	switch( scene_node_type ) {

	case Type::SHAPE:
	{
		unique = MakeUniquePointer<SceneNode_Shape>( p_engine, p_scene_manager, scene_node_path );
		break;
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

void SceneNodeBase::UpdateFromManager()
{
	if( !IsConfigFileParsed() ) {
		if( IsConfigFileLoaded() ) {
			if( !ParseConfigFile() ) {
				is_scene_node_ok		= false;
				p_engine->GetLogger()->LogError( "Parsing scene node XML file failed" );
			}
			is_config_file_parsed		= true;
		}
	}
	Update();
	for( auto & c : child_list ) {
		c->UpdateFromManager();
	}
}

bool SceneNodeBase::IsConfigFileParsed()
{
	return is_config_file_parsed;
}

bool SceneNodeBase::IsConfigFileLoaded()
{
	return config_file->IsReadyForUse();
}

const Path & SceneNodeBase::GetConfigFilePath()
{
	return config_file_path;
}

}
