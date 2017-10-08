
#include "Camera.h"

#include "../../../../Renderer/Buffer/UniformBufferTypes.h"
#include "../../../../Renderer/Buffer/UniformBuffer.h"
#include "../../../../Renderer/DescriptorSet/DescriptorPoolManager.h"

namespace AE
{

SceneNode_Camera::SceneNode_Camera( Engine * engine, SceneManager * scene_manager, DescriptorPoolManager * descriptor_pool_manager, const Path & scene_node_path )
	: SceneNode_Object( engine, scene_manager, descriptor_pool_manager, scene_node_path, SceneNodeBase::Type::CAMERA )
{
}

SceneNode_Camera::~SceneNode_Camera()
{
}

Mat4 & SceneNode_Camera::CalculateViewMatrix()
{
	view_matrix			= glm::inverse( CalculateTransformationMatrixFromPosScaleRot() );
	return view_matrix;
}

Mat4 & SceneNode_Camera::CalculateProjectionMatrix( double fov_angle, VkExtent2D viewport_size, double near_plane, double far_plane )
{
	projection_matrix	= glm::perspective( glm::radians( fov_angle ), double( viewport_size.width ) / double( viewport_size.height ), near_plane, far_plane );
	return projection_matrix;
}

void SceneNode_Camera::Update_Animation()
{
}

void SceneNode_Camera::Update_Logic()
{
}

bool SceneNode_Camera::ParseConfigFile()
{
	assert( config_file->IsResourceReadyForUse() );		// config file resource should have been loaded before this function is called

	return ParseConfigFileHelper( ParseConfigFile_ObjectLevel(), "CAMERA", [ this ]() {
		// Parse camera level stuff here
		return true;
	} );
}

SceneNodeBase::ResourcesLoadState SceneNode_Camera::CheckResourcesLoaded()
{
	auto object_level_resources_state	= CheckResourcesLoaded_ObjectLevel();
	if( object_level_resources_state == ResourcesLoadState::READY ) {
		// check requested resources on shape level
		return ResourcesLoadState::READY;
	}
	return object_level_resources_state;
}

bool SceneNode_Camera::FinalizeResources()
{
	if( FinalizeResources_ObjectLevel() ) {
		// finalize camera level stuff

		uniform_buffer					= MakeUniquePointer<UniformBuffer>( p_engine, p_renderer );
		assert( uniform_buffer );
		uniform_buffer->Initialize( sizeof( UniformBufferData_Camera ) );
		uniform_buffer_descriptor_set	= p_descriptor_pool_manager->AllocateDescriptorSetForCamera();
		assert( uniform_buffer_descriptor_set );

		vk::DescriptorBufferInfo buffer_writes {};
		buffer_writes.buffer	= uniform_buffer->GetDeviceBuffer();
		buffer_writes.offset	= 0;
		buffer_writes.range		= sizeof( UniformBufferData_Camera );
		vk::WriteDescriptorSet descriptor_set_write {};
		descriptor_set_write.dstSet				= uniform_buffer_descriptor_set;
		descriptor_set_write.dstBinding			= 0;
		descriptor_set_write.dstArrayElement	= 0;
		descriptor_set_write.descriptorCount	= 1;
		descriptor_set_write.descriptorType		= vk::DescriptorType::eUniformBuffer;
		descriptor_set_write.pImageInfo			= nullptr;
		descriptor_set_write.pBufferInfo		= &buffer_writes;
		descriptor_set_write.pTexelBufferView	= nullptr;

		LOCK_GUARD( *ref_vk_device.mutex );
		ref_vk_device.object.updateDescriptorSets( descriptor_set_write, nullptr );

		return true;
	}
	return false;
}

}
