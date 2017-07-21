
#include "Shape.h"

#include "../../../../Engine.h"
#include "../../../../Logger/Logger.h"
#include "../../../../Renderer/DeviceResource/DeviceResourceManager.h"
#include "../../../../Renderer/DeviceResource/Mesh/DeviceResource_Mesh.h"
#include "../../../../FileResource/XML/FileResource_XML.h"

namespace AE
{

SceneNode_Shape::SceneNode_Shape( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path )
	: SceneNode_Object( engine, scene_manager, scene_node_path, SceneNodeBase::Type::SHAPE )
{
}

SceneNode_Shape::~SceneNode_Shape()
{
}

void SceneNode_Shape::Update()
{
}

bool SceneNode_Shape::ParseConfigFile()
{
	assert( config_file->IsReadyForUse() );		// config file resource should be loaded before this function is called
	auto xml			= config_file->GetRawXML();
	auto root_node		= xml->FirstChildElement( "SCENE_NODE" );

	TODO( "All of this could be wrapped to a set of predefined structures and save typing a custom parser for every object type" );

	if( nullptr != root_node ) {
		{
			auto xml_type = root_node->FirstChildElement( "type" )->GetText();
			if( nullptr != xml_type ) {
				if( String( xml_type ) != "SHAPE" ) {
					return false;
				}
			}

			auto xml_name = root_node->FirstChildElement( "name" )->GetText();
			if( nullptr != xml_name ) {
				name		= xml_name;
			}

			is_visible	= root_node->FirstChildElement( "visible" )->BoolText( true );
		}

		tinyxml2::XMLElement * mesh_node	= root_node->FirstChildElement( "mesh" );
		while( nullptr != mesh_node ) {
			MeshInfo buffer		{};

			buffer.xml_mesh_path		= mesh_node->FirstChildElement( "path" )->GetText();
			if( buffer.xml_mesh_path	== "" ) {
				buffer.is_ok			= false;
				p_engine->GetLogger()->LogWarning( String( "Loading shape: Mesh path missing for shape object: " ) + GetConfigFilePath().string().c_str() );
			}

			if( buffer.is_ok ) {
				buffer.visible			= mesh_node->FirstChildElement( "visible" )->BoolText( true );
			}

			auto xml_renderer	= mesh_node->FirstChildElement( "renderer" );
			if( nullptr			== xml_renderer ) {
				buffer.is_ok	= false;
			}

			if( buffer.is_ok ) {
				auto xml_shader_path		= xml_renderer->FirstChildElement( "shader_path" )->GetText();
				if( nullptr != xml_shader_path ) {
					buffer.xml_shader_path	= xml_shader_path;
				} else {
					buffer.is_ok			= false;
				}
			}

			if( buffer.is_ok ) {
				auto xml_shader_images_node			= xml_renderer->FirstChildElement( "images" );
				if( nullptr != xml_shader_images_node ) {
					
					auto xml_diffuse_1_path			= xml_shader_images_node->FirstChildElement( "diffuse_1" )->GetText();
					if( nullptr != xml_diffuse_1_path ) {
						buffer.xml_diffuse_1_path	= xml_diffuse_1_path;
					} else {
						buffer.is_ok	= false;
					}

				} else {
					buffer.is_ok		= false;
				}
			}

			if( buffer.is_ok ) {
				buffer.mesh				= p_device_resource_manager->RequestResource( DeviceResource::Type::MESH, { buffer.xml_mesh_path } );
				buffer.diffuse_1		= p_device_resource_manager->RequestResource( DeviceResource::Type::IMAGE, { buffer.xml_diffuse_1_path } );
				mesh_info_list.push_back( std::move( buffer ) );
			}
			mesh_node	= mesh_node->FirstChildElement( "mesh" );
		}
		return true;
	} else {
		return false;
	}
	return false;
}

}
