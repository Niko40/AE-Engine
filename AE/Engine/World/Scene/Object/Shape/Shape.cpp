
#include "Shape.h"

#include "../../../../Engine.h"
#include "../../../../Logger/Logger.h"
#include "../../../../Renderer/DeviceResource/DeviceResourceManager.h"
#include "../../../../Renderer/DeviceResource/Mesh/DeviceResource_Mesh.h"
#include "../../../../Renderer/DeviceResource/GraphicsPipeline/DeviceResource_GraphicsPipeline.h"
#include "../../../../FileResource/XML/FileResource_XML.h"
#include "../../../../Renderer/Buffer/UniformBuffer.h"
#include "../../../../Renderer/Buffer/UniformBufferTypes.h"

namespace AE
{

SceneNode_Shape::SceneNode_Shape( Engine * engine, SceneManager * scene_manager, SceneBase * parent, const Path & scene_node_path )
	: SceneNode_Object( engine, scene_manager, parent, scene_node_path, SceneBase::Type::SHAPE )
{
}

SceneNode_Shape::~SceneNode_Shape()
{
}

void SceneNode_Shape::Update_Logic()
{
}

void SceneNode_Shape::Update_Animation()
{
	// Shapes are static objects and no mesh updates are required, it's already in physical device memory
}

void SceneNode_Shape::Update_Buffers()
{
	if( mesh_info ) {
		UniformBufferData_Mesh ub_data {};
		TODO( "double check the matrix multiplication order" );
		ub_data.model_matrix	= inherited_transformation_matrix;

		mesh_info->uniform_buffer->CopyDataToHostBuffer( &ub_data, sizeof( ub_data ) );
	}
}

VkPipeline SceneNode_Shape::GetGraphicsPipeline()
{
	if( mesh_info ) {
		return mesh_info->render_info.graphics_pipeline_resource->GetVulkanPipeline();
	}
	return VK_NULL_HANDLE;
}

void SceneNode_Shape::RecordCommand_Transfer( VkCommandBuffer command_buffer )
{
	if( mesh_info ) {
		mesh_info->uniform_buffer->RecordHostToDeviceBufferCopy( command_buffer );
	}
}

void SceneNode_Shape::RecordCommand_Render( VkCommandBuffer command_buffer, VkPipelineLayout pipeline_layout )
{
	if( mesh_info ) {
		VkDescriptorSet set = mesh_info->uniform_buffer_descriptor_set;

		vkCmdBindDescriptorSets(
			command_buffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline_layout,
			0, 1, &set,
			0, nullptr );

		mesh_info->mesh_resource->RecordVulkanCommand_Render( command_buffer, pipeline_layout );
	}
}

bool SceneNode_Shape::ParseConfigFile()
{
	assert( config_file->IsResourceReadyForUse() );		// config file resource should have been loaded before this function is called

	return ParseConfigFileHelper( ParseConfigFile_ObjectLevel(), "SHAPE", [ this ]() {
		// Parse shape level stuff here
		return true;
	} );
}

SceneBase::ResourcesLoadState SceneNode_Shape::CheckResourcesLoaded()
{
	return CheckResourcesLoadedHelper( CheckResourcesLoaded_ObjectLevel(), [ this ]() {
		// check requested resources on shape level
		return SceneBase::ResourcesLoadState::READY;
	} );
}

bool SceneNode_Shape::FinalizeResources()
{
	return FinalizeResourcesHelper( FinalizeResources_ObjectLevel(), [ this ]() {
		// finalize shape level stuff
		return true;
	} );
}

}
