
#include "Camera.h"

#include "../../../../Renderer/Buffer/UniformBufferTypes.h"
#include "../../../../Renderer/Buffer/UniformBuffer.h"
#include "../../../../Renderer/DescriptorSet/DescriptorPoolManager.h"

namespace AE
{

SceneNode_Camera::SceneNode_Camera( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path )
	: SceneNode_Object( engine, scene_manager, scene_node_path, SceneBase::Type::CAMERA )
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

void SceneNode_Camera::Update_Logic()
{
}

void SceneNode_Camera::Update_Animation()
{
}

void SceneNode_Camera::Update_GPU()
{
}

void SceneNode_Camera::RecordCommand_Transfer( VkCommandBuffer command_buffer )
{
}

void SceneNode_Camera::RecordCommand_Render( VkCommandBuffer command_buffer )
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

SceneBase::ResourcesLoadState SceneNode_Camera::CheckResourcesLoaded()
{
	return CheckResourcesLoadedHelper( CheckResourcesLoaded_ObjectLevel(), [ this ]() {
		// check requested resources on shape level
		return ResourcesLoadState::READY;
	} );
}

bool SceneNode_Camera::FinalizeResources()
{
	return FinalizeResourcesHelper( FinalizeResources_ObjectLevel(), [ this ]() {
		// finalize camera level stuff

		uniform_buffer					= MakeUniquePointer<UniformBuffer>( p_engine, p_renderer );
		assert( uniform_buffer );
		uniform_buffer->Initialize( sizeof( UniformBufferData_Camera ) );
		uniform_buffer_descriptor_set	= p_descriptor_pool_manager->AllocateDescriptorSetForCamera();
		assert( uniform_buffer_descriptor_set );

		VkDescriptorBufferInfo buffer_writes {};
		buffer_writes.buffer	= uniform_buffer->GetDeviceBuffer();
		buffer_writes.offset	= 0;
		buffer_writes.range		= sizeof( UniformBufferData_Camera );
		VkWriteDescriptorSet descriptor_set_write {};
		descriptor_set_write.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_set_write.pNext				= nullptr;
		descriptor_set_write.dstSet				= uniform_buffer_descriptor_set;
		descriptor_set_write.dstBinding			= 0;
		descriptor_set_write.dstArrayElement	= 0;
		descriptor_set_write.descriptorCount	= 1;
		descriptor_set_write.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_set_write.pImageInfo			= nullptr;
		descriptor_set_write.pBufferInfo		= &buffer_writes;
		descriptor_set_write.pTexelBufferView	= nullptr;

		LOCK_GUARD( *ref_vk_device.mutex );
		vkUpdateDescriptorSets( ref_vk_device.object, 1, &descriptor_set_write, 0, nullptr );

		return true;
	} );
}

}
