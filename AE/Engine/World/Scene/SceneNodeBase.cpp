
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
#include "Object/Camera/Camera.h"
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
	p_renderer					= p_engine->GetRenderer();
	assert( p_renderer );
	assert( p_file_resource_manager );
	p_device_resource_manager	= p_renderer->GetDeviceResourceManager();
	ref_vk_device				= p_renderer->GetVulkanDevice();
	assert( p_device_resource_manager );
	assert( ref_vk_device.object );

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
	UniquePointer<SceneNode> unique_ptr = nullptr;

	// create scene node
	switch( scene_node_type ) {

	case Type::CAMERA:
	{
		unique_ptr	= MakeUniquePointer<SceneNode_Camera>( p_engine, p_scene_manager, scene_node_path );
		break;
	}

	case Type::SHAPE:
	{
		unique_ptr	= MakeUniquePointer<SceneNode_Shape>( p_engine, p_scene_manager, scene_node_path );
		break;
	}

	default:
		p_engine->GetLogger()->LogError( "Can't create scene node, scene node type not recognized" );
		assert( 0 && "Can't create scene node, scene node type not recognized" );
		break;
	}

	if( unique_ptr ) {
		// add scene node into the child list
		SceneNode * node_ptr	= unique_ptr.Get();
		child_list.push_back( std::move( unique_ptr ) );
		return node_ptr;
	}
	return nullptr;
}

void SceneNodeBase::UpdateResourcesFromManager()
{
	if( is_scene_node_ok ) {
		if( !is_scene_node_use_ready ) {
			if( !IsConfigFileParsed() ) {
				if( IsConfigFileLoaded() ) {
					if( !ParseConfigFile() ) {
						is_scene_node_ok		= false;
						p_engine->GetLogger()->LogError( "Parsing scene node XML file failed" );
					}
					is_config_file_parsed		= true;
				}
			}
			if( IsConfigFileParsed() && is_scene_node_ok ) {
				auto result = CheckResourcesLoaded();
				switch( result ) {
				case ResourcesLoadState::READY:
				{
					if( FinalizeResources() ) {
						is_scene_node_use_ready	= true;
					} else {
						is_scene_node_use_ready	= false;
						is_scene_node_ok		= false;
					}
					break;
				}
				case ResourcesLoadState::NOT_READY:
					break;
				case ResourcesLoadState::UNABLE_TO_LOAD:
					is_scene_node_ok			= false;
					break;
				default:
					assert( !"Illegal value, check if the value is set?" );
					break;
				}
			}
		}
	}
	for( auto & c : child_list ) {
		c->UpdateResourcesFromManager();
	}
}

bool SceneNodeBase::IsConfigFileParsed()
{
	return is_config_file_parsed;
}

bool SceneNodeBase::IsConfigFileLoaded()
{
	return config_file->IsResourceReadyForUse();
}

bool SceneNodeBase::IsSceneNodeUseReady()
{
	return is_scene_node_use_ready && is_scene_node_ok;
}

const Path & SceneNodeBase::GetConfigFilePath()
{
	return config_file_path;
}


bool ParseConfigFileHelper( tinyxml2::XMLElement * previous_level, String child_element_name, std::function<bool()> child_element_parser )
{
	if( previous_level ) {
		auto next_level		= previous_level->FirstChildElement( child_element_name.c_str() );
		if( next_level ) {
			return child_element_parser();
		}
	}
	return false;
}

SceneNodeBase::ResourcesLoadState CheckResourcesLoadedHelper( SceneNodeBase::ResourcesLoadState previous_level, std::function<SceneNodeBase::ResourcesLoadState( void )> child_element_parser )
{
	if( previous_level == SceneNodeBase::ResourcesLoadState::READY ) {
		return child_element_parser();
	}
	return previous_level;
}

bool FinalizeResourcesHelper( bool previous_level, std::function<bool( void )> child_parser_function )
{
	if( previous_level ) {
		return child_parser_function();
	}
	return false;
}

}
