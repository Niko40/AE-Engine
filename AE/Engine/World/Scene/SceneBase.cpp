
#include "SceneBase.h"

#include "../../Engine.h"
#include "../../Logger/Logger.h"
#include "../../Memory/Memory.h"
#include "../../Renderer/Renderer.h"
#include "../../Renderer/DeviceResource/DeviceResourceManager.h"
#include "../../FileResource/FileResourceManager.h"
#include "../../FileResource/XML/FileResource_XML.h"
#include "SceneManager.h"

#include <assert.h>

// include all scene node final derivatives here
// Simple objects
#include "Object/Camera/Camera.h"
#include "Object/Shape/Shape.h"

// Units


namespace AE
{

SceneBase::SceneBase( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path, SceneBase::Type scene_node_type )
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
	p_descriptor_pool_manager	= p_renderer->GetDescriptorPoolManager();
	ref_vk_device				= p_renderer->GetVulkanDevice();
	assert( p_device_resource_manager );
	assert( ref_vk_device.object );

	type						= scene_node_type;
	assert( type != Type::UNDEFINED );

	config_file_path			= scene_node_path;
	config_file					= p_engine->GetFileResourceManager()->RequestResource( config_file_path );
}

SceneBase::~SceneBase()
{
}

SceneNode * SceneBase::CreateChild( SceneBase::Type scene_node_type, Path scene_node_path )
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

void ConfigResourceCheckerAndLoader( SceneBase * sb )
{
	auto lambda_check_config_is_loaded_and_finalize	= []( SceneBase * sb ) {
		auto result = sb->CheckResourcesLoaded();
		switch( result ) {
		case SceneBase::ResourcesLoadState::READY:
		{
			if( sb->FinalizeResources() ) {
				sb->is_scene_node_use_ready	= true;
			} else {
				sb->is_scene_node_use_ready	= false;
				sb->is_scene_node_ok		= false;
			}
			break;
		}
		case SceneBase::ResourcesLoadState::NOT_READY:
			break;
		case SceneBase::ResourcesLoadState::UNABLE_TO_LOAD:
			sb->is_scene_node_ok			= false;
			break;
		default:
			assert( !"Illegal value, check if the value is set?" );
			break;
		}
	};

	if( sb->config_file ) {
		if( !sb->IsConfigFileParsed() ) {
			if( sb->IsConfigFileLoaded() ) {
				if( !sb->ParseConfigFile() ) {
					sb->is_scene_node_ok		= false;
					sb->p_engine->GetLogger()->LogError( "Parsing scene node XML file failed" );
				}
				sb->is_config_file_parsed		= true;
			}
		}
		if( sb->IsConfigFileParsed() && sb->is_scene_node_ok ) {
			lambda_check_config_is_loaded_and_finalize( sb );
		}
	} else {
		// Config file doesn't exist for this resource
		if( sb->is_scene_node_ok ) {
			lambda_check_config_is_loaded_and_finalize( sb );
		}
	}
}

void SceneBase::UpdateResourcesFromManager()
{
	if( is_scene_node_ok ) {
		if( !is_scene_node_use_ready ) {
			ConfigResourceCheckerAndLoader( this );
		}
	}
	for( auto & c : child_list ) {
		c->UpdateResourcesFromManager();
	}
}

bool SceneBase::IsConfigFileParsed()
{
	return is_config_file_parsed;
}

bool SceneBase::IsConfigFileLoaded()
{
	return config_file->IsResourceReadyForUse();
}

bool SceneBase::IsSceneNodeUseReady()
{
	return is_scene_node_use_ready && is_scene_node_ok;
}

const Path & SceneBase::GetConfigFilePath()
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

SceneBase::ResourcesLoadState CheckResourcesLoadedHelper( SceneBase::ResourcesLoadState previous_level, std::function<SceneBase::ResourcesLoadState( void )> child_element_parser )
{
	if( previous_level == SceneBase::ResourcesLoadState::READY ) {
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
