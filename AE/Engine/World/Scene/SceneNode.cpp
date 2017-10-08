
#include "SceneNode.h"

#include <glm/gtx/matrix_decompose.hpp>

#include "../../Engine.h"
#include "../../Logger/Logger.h"
#include "../../Renderer/Buffer/UniformBuffer.h"
#include "../../Renderer/Buffer/UniformBufferTypes.h"
#include "../../Renderer/DescriptorSet/DescriptorPoolManager.h"
#include "../../Renderer/DeviceResource/DeviceResourceManager.h"
#include "../../Renderer/DeviceResource/GraphicsPipeline/DeviceResource_GraphicsPipeline.h"
#include "../../Renderer/DeviceResource/Mesh/DeviceResource_Mesh.h"
#include "../../Renderer/DeviceResource/Image/DeviceResource_Image.h"

namespace AE
{

SceneNode::SceneNode( Engine * engine, SceneManager * scene_manager, DescriptorPoolManager * descriptor_pool_manager, const Path & scene_node_path, SceneNodeBase::Type scene_node_type )
	: SceneNodeBase( engine, scene_manager, descriptor_pool_manager, scene_node_path, scene_node_type )
{
}

SceneNode::~SceneNode()
{
}

const Mat4 & SceneNode::CalculateTransformationMatrixFromPosScaleRot()
{
	transformation_matrix = glm::mat4( 1 );
	transformation_matrix = glm::translate( transformation_matrix, position );
	transformation_matrix *= glm::mat4_cast( rotation );
	transformation_matrix = glm::scale( transformation_matrix, scale );
	return transformation_matrix;
}

void SceneNode::CalculatePosScaleRotFromTransformationMatrix( const Mat4 & new_transformations )
{
	Vec3 skew;
	Vec4 perspective;
	glm::decompose( new_transformations, scale, rotation, position, skew, perspective );
	rotation	= glm::conjugate( rotation );		// returned rotation is a conjugate so we need to flip it
}

tinyxml2::XMLElement * SceneNode::ParseConfigFile_SceneNodeLevel()
{
	auto xml_root = config_file->GetRootElement();
	if( nullptr == xml_root ) return nullptr;
	if( String( xml_root->Name() ) != "SCENE_NODE" ) return nullptr;

	name			= config_file->GetFieldValue_Text( xml_root, "name" );
	is_visible		= config_file->GetFieldValue_Bool( xml_root, "visible" );

	// handle meshes
	for( auto xml_mesh = config_file->GetChildElement( xml_root, "MESH" ); xml_mesh; xml_mesh = xml_mesh->NextSiblingElement( "MESH" ) ) {
		mesh_info_list.push_back( MakeSharedPointer<MeshInfo>() );
		auto & mesh = mesh_info_list.back();

		mesh->mesh_resource	= p_device_resource_manager->RequestResource_Mesh( { config_file->GetFieldValue_Text( xml_mesh, "path" ) } );
		mesh->name			= config_file->GetFieldValue_Text( xml_mesh, "name" );
		mesh->is_visible	= config_file->GetFieldValue_Bool( xml_mesh, "visible" );

		auto position		= config_file->GetMultiFieldValues_Double( xml_mesh, "position" );
		auto rotation		= config_file->GetMultiFieldValues_Double( xml_mesh, "rotation" );
		auto scale			= config_file->GetMultiFieldValues_Double( xml_mesh, "scale" );

		mesh->position.x	= ( position.find( "x" ) != position.end() ) ? position[ "x" ] : 0;
		mesh->position.y	= ( position.find( "y" ) != position.end() ) ? position[ "y" ] : 0;
		mesh->position.z	= ( position.find( "z" ) != position.end() ) ? position[ "z" ] : 0;

		mesh->rotation.x	= ( rotation.find( "x" ) != rotation.end() ) ? rotation[ "x" ] : 0;
		mesh->rotation.y	= ( rotation.find( "y" ) != rotation.end() ) ? rotation[ "y" ] : 0;
		mesh->rotation.z	= ( rotation.find( "z" ) != rotation.end() ) ? rotation[ "z" ] : 0;
		mesh->rotation.w	= ( rotation.find( "w" ) != rotation.end() ) ? rotation[ "w" ] : 1;

		mesh->scale.x		= ( scale.find( "x" ) != scale.end() ) ? scale[ "x" ] : 1;
		mesh->scale.y		= ( scale.find( "y" ) != scale.end() ) ? scale[ "y" ] : 1;
		mesh->scale.z		= ( scale.find( "z" ) != scale.end() ) ? scale[ "z" ] : 1;

		// handle render info
		mesh->render_info.image_info.image_count			= -1;
		auto xml_render_info	= config_file->GetChildElement( xml_mesh, "RENDER_INFO" );
		if( nullptr != xml_render_info ) {
			TODO( "Pipeline resources" );
			mesh->render_info.graphics_pipeline_resource	= p_device_resource_manager->RequestResource_GraphicsPipeline( { config_file->GetFieldValue_Text( xml_render_info, "graphics_pipeline_path", "Graphics pipeline path not defined" ) } );

			// handle images
			auto xml_images	= config_file->GetChildElement( xml_render_info, "IMAGES" );
			if( nullptr != xml_images ) {
				for( auto i = xml_images->FirstChildElement( "image" ); i; i = i->NextSiblingElement( "image" ) ) {
					auto attr_binding	= i->Attribute( "binding" );
					auto attr_path		= i->Attribute( "path" );
					if( nullptr != attr_binding && nullptr != attr_path ) {
						auto index		= i->Int64Attribute( "binding", 0 );
						mesh->render_info.image_info.image_resources[ index ]	= p_device_resource_manager->RequestResource_Image( { attr_path } );
						mesh->render_info.image_info.image_count				= std::max( mesh->render_info.image_info.image_count, int32_t( index ) );
					}
				}
			}
		}
		mesh->render_info.image_info.image_count++;
	}

	return xml_root;
}

SceneNodeBase::ResourcesLoadState SceneNode::CheckResourcesLoaded_SceneNodeLevel()
{
	for( auto & m : mesh_info_list ) {
		if( !m->mesh_resource->IsResourceOK() ) return ResourcesLoadState::UNABLE_TO_LOAD;
		if( !m->mesh_resource->IsResourceReadyForUse() ) return ResourcesLoadState::NOT_READY;

		if( !m->render_info.graphics_pipeline_resource->IsResourceOK() ) return ResourcesLoadState::UNABLE_TO_LOAD;
		if( !m->render_info.graphics_pipeline_resource->IsResourceReadyForUse() ) return ResourcesLoadState::NOT_READY;

		for( auto & i : m->render_info.image_info.image_resources ) {
			if( i ) {
				if( !i->IsResourceOK() ) return ResourcesLoadState::UNABLE_TO_LOAD;
				if( !i->IsResourceReadyForUse() ) return ResourcesLoadState::NOT_READY;
			}
		}
	}
	return ResourcesLoadState::READY;
}

bool SceneNode::FinalizeResources_SceneNodeLevel()
{
	for( auto & i : mesh_info_list ) {
		i->uniform_buffer	= MakeUniquePointer<UniformBuffer>( p_engine, p_renderer );
		assert( i->uniform_buffer );
		i->uniform_buffer->Initialize( sizeof( UniformBufferData_Mesh ) );

		i->uniform_buffer_descriptor_set		= p_descriptor_pool_manager->AllocateDescriptorSetForMesh();
		assert( i->uniform_buffer_descriptor_set );

		vk::DescriptorBufferInfo buffer_writes {};
		buffer_writes.buffer	= i->uniform_buffer->GetDeviceBuffer();
		buffer_writes.offset	= 0;
		buffer_writes.range		= sizeof( UniformBufferData_Mesh );
		vk::WriteDescriptorSet descriptor_set_writes {};
		descriptor_set_writes.dstSet			= i->uniform_buffer_descriptor_set;
		descriptor_set_writes.dstBinding		= 0;
		descriptor_set_writes.dstArrayElement	= 0;
		descriptor_set_writes.descriptorCount	= 1;
		descriptor_set_writes.descriptorType	= vk::DescriptorType::eUniformBuffer;
		descriptor_set_writes.pImageInfo		= nullptr;
		descriptor_set_writes.pBufferInfo		= &buffer_writes;
		descriptor_set_writes.pTexelBufferView	= nullptr;

		LOCK_GUARD( *ref_vk_device.mutex );
		ref_vk_device.object.updateDescriptorSets( descriptor_set_writes, nullptr );
	}

	return true;
}

}
