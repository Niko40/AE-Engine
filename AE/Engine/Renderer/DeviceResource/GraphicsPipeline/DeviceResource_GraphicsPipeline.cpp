
#include "DeviceResource_GraphicsPipeline.h"

#include "../../../Logger/Logger.h"
#include "../../Renderer.h"

#include "../../../FileResource/FileResourceManager.h"
#include "../../../FileResource/RawData/FileResource_RawData.h"
#include "../../../FileResource/XML/FileResource_XML.h"
#include "../../../FileResource/Mesh/MeshInfo.h"

namespace AE
{

DeviceResource_GraphicsPipeline::DeviceResource_GraphicsPipeline( Engine * engine, DeviceResource::Flags resource_flags )
	: DeviceResource( engine, DeviceResource::Type::GRAPHICS_PIPELINE, resource_flags )
{
}

DeviceResource_GraphicsPipeline::~DeviceResource_GraphicsPipeline()
{
}

uint32_t DeviceResource_GraphicsPipeline::GetImageCount() const
{
	return image_count;
}

bool ContinueGraphicsPipelineLoadTest_1( DeviceResource * resource )
{
	auto res			= static_cast<DeviceResource_GraphicsPipeline*>( resource );
	assert( res );

	if( res->vertex_shader_resource ) {
		if( !res->vertex_shader_resource->IsReadyForUse() ) {
			return false;
		}
	}
	if( res->tessellation_control_shader_resource ) {
		if( !res->tessellation_control_shader_resource->IsReadyForUse() ) {
			return false;
		}
	}
	if( res->tessellation_evaluation_shader_resource ) {
		if( !res->tessellation_evaluation_shader_resource->IsReadyForUse() ) {
			return false;
		}
	}
	if( res->geometry_shader_resource ) {
		if( !res->geometry_shader_resource->IsReadyForUse() ) {
			return false;
		}
	}
	if( res->fragment_shader_resource ) {
		if( !res->fragment_shader_resource->IsReadyForUse() ) {
			return false;
		}
	}
	return true;
}

DeviceResource::LoadingState ContinueGraphicsPipelineLoad_1( DeviceResource * resource )
{
	auto res			= static_cast<DeviceResource_GraphicsPipeline*>( resource );
	assert( res );

	auto xml_file		= static_cast<FileResource_XML*>( res->file_resources[ 0 ].Get() );
	auto xml_root		= xml_file->GetRootElement();


	// Pipeline flags
	vk::PipelineCreateFlags									vulkan_flags = vk::PipelineCreateFlagBits( 0 );
	{
		auto flags_str		= xml_file->GetMultiFieldValues_Text( xml_root, "flags" );
		for( auto & f : flags_str ) {
			if( f.second == "DISABLE_OPTIMIZATION_BIT" )	vulkan_flags |= vk::PipelineCreateFlagBits::eDisableOptimization;
			else if( f.second == "ALLOW_DERIVATIVES_BIT" )	vulkan_flags |= vk::PipelineCreateFlagBits::eAllowDerivatives;
			else if( f.second == "DERIVATIVE_BIT" )			vulkan_flags |= vk::PipelineCreateFlagBits::eDerivative;
		}
	}

	// Graphics shaders
	Vector<vk::PipelineShaderStageCreateInfo>				shader_stages;
	String vert_shader_entry_point;
	String tesc_shader_entry_point;
	String tese_shader_entry_point;
	String geom_shader_entry_point;
	String frag_shader_entry_point;
	{
		auto xml_shaders									= xml_root->FirstChildElement( "SHADERS" );
		if( !xml_shaders ) {
			res->p_logger->LogError( String( "Graphics pipeline xml file: " ) + xml_file->GetPath().string().c_str() + " missing required <SHADERS> element, loading cannot continue" );
			return DeviceResource::LoadingState::UNABLE_TO_LOAD;
		}

		// Create shader modules
		if( res->vertex_shader_resource ) {
			vk::ShaderModuleCreateInfo shader_CI {};
			shader_CI.codeSize		= res->vertex_shader_resource->GetData().size();
			shader_CI.pCode			= reinterpret_cast<const uint32_t*>( res->vertex_shader_resource->GetData().data() );

			LOCK_GUARD( *res->ref_vk_device.mutex );
			res->vk_vertex_shader_module					= res->ref_vk_device.object.createShaderModule( shader_CI );
			if( !res->vk_vertex_shader_module ) return DeviceResource::LoadingState::UNABLE_TO_LOAD;
		}
		if( res->tessellation_control_shader_resource ) {
			vk::ShaderModuleCreateInfo shader_CI {};
			shader_CI.codeSize		= res->tessellation_control_shader_resource->GetData().size();
			shader_CI.pCode			= reinterpret_cast<const uint32_t*>( res->tessellation_control_shader_resource->GetData().data() );

			LOCK_GUARD( *res->ref_vk_device.mutex );
			res->vk_tessellation_control_shader_module		= res->ref_vk_device.object.createShaderModule( shader_CI );
			if( !res->vk_tessellation_control_shader_module ) return DeviceResource::LoadingState::UNABLE_TO_LOAD;
		}
		if( res->tessellation_evaluation_shader_resource ) {
			vk::ShaderModuleCreateInfo shader_CI {};
			shader_CI.codeSize		= res->tessellation_evaluation_shader_resource->GetData().size();
			shader_CI.pCode			= reinterpret_cast<const uint32_t*>( res->tessellation_evaluation_shader_resource->GetData().data() );

			LOCK_GUARD( *res->ref_vk_device.mutex );
			res->vk_tessellation_evaluation_shader_module	= res->ref_vk_device.object.createShaderModule( shader_CI );
			if( !res->vk_tessellation_evaluation_shader_module ) return DeviceResource::LoadingState::UNABLE_TO_LOAD;
		}
		if( res->geometry_shader_resource ) {
			vk::ShaderModuleCreateInfo shader_CI {};
			shader_CI.codeSize		= res->geometry_shader_resource->GetData().size();
			shader_CI.pCode			= reinterpret_cast<const uint32_t*>( res->geometry_shader_resource->GetData().data() );

			LOCK_GUARD( *res->ref_vk_device.mutex );
			res->vk_geometry_shader_module					= res->ref_vk_device.object.createShaderModule( shader_CI );
			if( !res->vk_geometry_shader_module ) return DeviceResource::LoadingState::UNABLE_TO_LOAD;
		}
		if( res->fragment_shader_resource ) {
			vk::ShaderModuleCreateInfo shader_CI {};
			shader_CI.codeSize		= res->fragment_shader_resource->GetData().size();
			shader_CI.pCode			= reinterpret_cast<const uint32_t*>( res->fragment_shader_resource->GetData().data() );

			LOCK_GUARD( *res->ref_vk_device.mutex );
			res->vk_fragment_shader_module					= res->ref_vk_device.object.createShaderModule( shader_CI );
			if( !res->vk_fragment_shader_module ) return DeviceResource::LoadingState::UNABLE_TO_LOAD;
		}

		auto entry_points		= xml_file->GetMultiFieldValues_Text( xml_shaders, "entry_points" );
		auto vert_iter			= entry_points.find( "vertex" );
		auto tesc_iter			= entry_points.find( "tessellation_control" );
		auto tese_iter			= entry_points.find( "tessellation_evaluation" );
		auto geom_iter			= entry_points.find( "geometry" );
		auto frag_iter			= entry_points.find( "fragment" );
		vert_shader_entry_point		= ( vert_iter != entry_points.end() ) ? std::move( vert_iter->second ) : "main";
		tesc_shader_entry_point		= ( tesc_iter != entry_points.end() ) ? std::move( tesc_iter->second ) : "main";
		tese_shader_entry_point		= ( tese_iter != entry_points.end() ) ? std::move( tese_iter->second ) : "main";
		geom_shader_entry_point		= ( geom_iter != entry_points.end() ) ? std::move( geom_iter->second ) : "main";
		frag_shader_entry_point		= ( frag_iter != entry_points.end() ) ? std::move( frag_iter->second ) : "main";

		if( res->vk_vertex_shader_module ) {
			shader_stages.push_back( {} );
			auto & stage				= shader_stages.back();
			stage.stage					= vk::ShaderStageFlagBits::eVertex;
			stage.module				= res->vk_vertex_shader_module;
			stage.pName					= vert_shader_entry_point.c_str();
			stage.pSpecializationInfo	= nullptr;	// not used at this time
		}
		if( res->vk_tessellation_control_shader_module ) {
			shader_stages.push_back( {} );
			auto & stage				= shader_stages.back();
			stage.stage					= vk::ShaderStageFlagBits::eTessellationControl;
			stage.module				= res->vk_tessellation_control_shader_module;
			stage.pName					= tesc_shader_entry_point.c_str();
			stage.pSpecializationInfo	= nullptr;	// not used at this time
		}
		if( res->vk_tessellation_evaluation_shader_module ) {
			shader_stages.push_back( {} );
			auto & stage				= shader_stages.back();
			stage.stage					= vk::ShaderStageFlagBits::eTessellationEvaluation;
			stage.module				= res->vk_tessellation_evaluation_shader_module;
			stage.pName					= tese_shader_entry_point.c_str();
			stage.pSpecializationInfo	= nullptr;	// not used at this time
		}
		if( res->vk_geometry_shader_module ) {
			shader_stages.push_back( {} );
			auto & stage				= shader_stages.back();
			stage.stage					= vk::ShaderStageFlagBits::eGeometry;
			stage.module				= res->vk_geometry_shader_module;
			stage.pName					= geom_shader_entry_point.c_str();
			stage.pSpecializationInfo	= nullptr;	// not used at this time
		}
		if( res->vk_fragment_shader_module ) {
			shader_stages.push_back( {} );
			auto & stage				= shader_stages.back();
			stage.stage					= vk::ShaderStageFlagBits::eFragment;
			stage.module				= res->vk_fragment_shader_module;
			stage.pName					= frag_shader_entry_point.c_str();
			stage.pSpecializationInfo	= nullptr;	// not used at this time
		}
	}


	// vertex input state, currently this data does not change so we might as well dump
	// all of this somewhere else and just reference that, but keeping it here for now as
	// I'm not sure if I will do more ways to input vertices to the pipeline in the future
	Vector<vk::VertexInputBindingDescription> vertex_binding_descriptions;
	Vector<vk::VertexInputAttributeDescription> vertex_attribute_descriptions;
	vk::PipelineVertexInputStateCreateInfo vertex_input_state_CI {};
	{
		{
			vertex_binding_descriptions.push_back( {} );
			auto & binding_desc		= vertex_binding_descriptions.back();
			binding_desc.binding	= 0;
			binding_desc.stride		= sizeof( Vertex );
			binding_desc.inputRate	= vk::VertexInputRate::eVertex;
		}
		{
			vertex_attribute_descriptions.push_back( {} );
			auto & attribute_desc	= vertex_attribute_descriptions.back();
			attribute_desc.location	= 0;
			attribute_desc.binding	= 0;
			attribute_desc.format	= vk::Format::eR32G32B32Sfloat;
			attribute_desc.offset	= offsetof( Vertex, position );
		}
		{
			vertex_attribute_descriptions.push_back( {} );
			auto & attribute_desc	= vertex_attribute_descriptions.back();
			attribute_desc.location	= 1;
			attribute_desc.binding	= 0;
			attribute_desc.format	= vk::Format::eR32G32B32Sfloat;
			attribute_desc.offset	= offsetof( Vertex, normal );
		}
		{
			vertex_attribute_descriptions.push_back( {} );
			auto & attribute_desc	= vertex_attribute_descriptions.back();
			attribute_desc.location	= 2;
			attribute_desc.binding	= 0;
			attribute_desc.format	= vk::Format::eR32G32Sfloat;
			attribute_desc.offset	= offsetof( Vertex, uv );
		}
		{
			vertex_attribute_descriptions.push_back( {} );
			auto & attribute_desc	= vertex_attribute_descriptions.back();
			attribute_desc.location	= 3;
			attribute_desc.binding	= 0;
			attribute_desc.format	= vk::Format::eR32Sint;
			attribute_desc.offset	= offsetof( Vertex, material );
		}
		vertex_input_state_CI.vertexBindingDescriptionCount		= uint32_t( vertex_binding_descriptions.size() );
		vertex_input_state_CI.pVertexBindingDescriptions		= vertex_binding_descriptions.data();
		vertex_input_state_CI.vertexAttributeDescriptionCount	= uint32_t( vertex_attribute_descriptions.size() );
		vertex_input_state_CI.pVertexAttributeDescriptions		= vertex_attribute_descriptions.data();
	}


	// input assembly state
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_CI {};
	{
		vk::PrimitiveTopology topology		= vk::PrimitiveTopology::eTriangleList;

		auto xml_input_assembly				= xml_root->FirstChildElement( "INPUT_ASSEMBLY" );
		if( !xml_input_assembly ) {
			res->p_logger->LogWarning( String( "Graphics pipeline xml file: " ) + xml_file->GetPath().string().c_str() + " missing <INPUT_ASSEMBLY> element, fallback to using default values" );
		}
		auto topology_str				= xml_file->GetFieldValue_Text( xml_input_assembly, "topology", "" );
		if( topology_str.size() ) {
			if( topology_str == "POINT_LIST" )				topology = vk::PrimitiveTopology::ePointList;
			else if( topology_str == "LINE_LIST" )			topology = vk::PrimitiveTopology::eLineList;
			else if( topology_str == "LINE_STRIP" )			topology = vk::PrimitiveTopology::eLineStrip;
			else if( topology_str == "TRIANGLE_LIST" )		topology = vk::PrimitiveTopology::eTriangleList;
			else if( topology_str == "TRIANGLE_STRIP" )		topology = vk::PrimitiveTopology::eTriangleStrip;
			else if( topology_str == "TRIANGLE_FAN" )		topology = vk::PrimitiveTopology::eTriangleFan;
			else if( topology_str == "LINE_LIST_WITH_ADJACENCY" )			topology = vk::PrimitiveTopology::eLineListWithAdjacency;
			else if( topology_str == "LINE_STRIP_WITH_ADJACENCY" )			topology = vk::PrimitiveTopology::eLineStripWithAdjacency;
			else if( topology_str == "TRIANGLE_LIST_WITH_ADJACENCY" )		topology = vk::PrimitiveTopology::eTriangleListWithAdjacency;
			else if( topology_str == "TRIANGLE_STRIP_WITH_ADJACENCY" )		topology = vk::PrimitiveTopology::eTriangleStripWithAdjacency;
		}
		
		input_assembly_state_CI.topology				= topology;
		input_assembly_state_CI.primitiveRestartEnable	= VkBool32( xml_file->GetFieldValue_Bool( xml_input_assembly, "primitive_restart_enable", false ) );
	}


	// tessellation state
	vk::PipelineTessellationStateCreateInfo tessellation_state_CI {};
	{
		auto xml_tessellation							= xml_root->FirstChildElement( "TESSELLATION" );
		if( !xml_tessellation && ( res->vk_tessellation_control_shader_module && res->vk_tessellation_evaluation_shader_module ) ) {
			res->p_logger->LogWarning( String( "Graphics pipeline xml file: " ) + xml_file->GetPath().string().c_str() + " missing <TESSELLATION> element, fallback to using default values" );
		}
		auto patch_control_points						= uint32_t( xml_file->GetFieldValue_Int64( xml_tessellation, "patch_control_points", 3 ) );
		tessellation_state_CI.patchControlPoints		= std::min( patch_control_points, res->p_renderer->GetPhysicalDeviceLimits().maxTessellationPatchSize );
	}


	// viewport and scissor state
	vk::PipelineViewportStateCreateInfo		viewport_state_CI {};
	vk::Viewport							viewport {};
	vk::Rect2D								scissor {};
	{
		// Viewport and scissors are dynamic states so these are just dummy values,
		// pipelines are automatically adjusted to viewport
		viewport.x			= 0;
		viewport.y			= 0;
		viewport.width		= 512;
		viewport.height		= 512;
		viewport.minDepth	= 0.0f;
		viewport.maxDepth	= 1.0f;

		scissor.offset		= { 0, 0 };
		scissor.extent		= { 512, 512 };

		viewport_state_CI.viewportCount	= 1;
		viewport_state_CI.pViewports	= &viewport;
		viewport_state_CI.scissorCount	= 1;
		viewport_state_CI.pScissors		= &scissor;
	}


	// rasterization state
	vk::PipelineRasterizationStateCreateInfo rasterization_state_CI {};
	{
		auto xml_rasterization							= xml_root->FirstChildElement( "RASTERIZATION" );
		if( !xml_rasterization ) {
			res->p_logger->LogWarning( String( "Graphics pipeline xml file: " ) + xml_file->GetPath().string().c_str() + " missing <RASTERIZATION> element, fallback to using default values" );
		}

		vk::PolygonMode				polygon_mode		= vk::PolygonMode::eFill;
		auto polygon_mode_str							= xml_file->GetFieldValue_Text( xml_rasterization, "polygon_mode", "FILL" );
		if( polygon_mode_str == "FILL" )				polygon_mode = vk::PolygonMode::eFill;
		else if( polygon_mode_str == "LINE" )			polygon_mode = vk::PolygonMode::eLine;
		else if( polygon_mode_str == "POINT" )			polygon_mode = vk::PolygonMode::ePoint;

		vk::CullModeFlags			cull_mode			= vk::CullModeFlagBits::eNone;
		auto cull_mode_str								= xml_file->GetFieldValue_Text( xml_rasterization, "cull_mode", "NONE" );
		if( cull_mode_str == "NONE" )					cull_mode = vk::CullModeFlagBits::eNone;
		else if( cull_mode_str == "FRONT" )				cull_mode = vk::CullModeFlagBits::eFront;
		else if( cull_mode_str == "BACK" )				cull_mode = vk::CullModeFlagBits::eBack;
		else if( cull_mode_str == "FRONT_AND_BACK" )	cull_mode = vk::CullModeFlagBits::eFrontAndBack;

		vk::FrontFace				front_face			= vk::FrontFace::eCounterClockwise;
		auto front_face_str								= xml_file->GetFieldValue_Text( xml_rasterization, "front_face", "COUNTER_CLOCKWISE" );
		if( front_face_str == "COUNTER_CLOCKWISE" )		front_face = vk::FrontFace::eCounterClockwise;
		else if( front_face_str == "CLOCKWISE" )		front_face = vk::FrontFace::eClockwise;


		rasterization_state_CI.depthClampEnable			= VkBool32( xml_file->GetFieldValue_Bool( xml_rasterization, "depth_clamp_enable", false ) );
		rasterization_state_CI.rasterizerDiscardEnable	= VkBool32( xml_file->GetFieldValue_Bool( xml_rasterization, "rasterizer_discard_enable", false ) );
		rasterization_state_CI.polygonMode				= polygon_mode;
		rasterization_state_CI.cullMode					= cull_mode;
		rasterization_state_CI.frontFace				= front_face;
		rasterization_state_CI.depthBiasEnable			= VkBool32( xml_file->GetFieldValue_Bool( xml_rasterization, "depth_bias_enable", false ) );
		rasterization_state_CI.depthBiasConstantFactor	= float( xml_file->GetFieldValue_Double( xml_rasterization, "depth_bias_constant_factor", 0.0 ) );
		rasterization_state_CI.depthBiasClamp			= float( xml_file->GetFieldValue_Double( xml_rasterization, "depth_bias_clamp", 0.0 ) );
		rasterization_state_CI.depthBiasSlopeFactor		= float( xml_file->GetFieldValue_Double( xml_rasterization, "depth_bias_slope_factor", 0.0 ) );
		rasterization_state_CI.lineWidth				= float( xml_file->GetFieldValue_Double( xml_rasterization, "line_width", 1.0 ) );
	}


	// multisampling state, this engine uses deferred rendering so multisampling is disabled by design
	vk::PipelineMultisampleStateCreateInfo multisample_state_CI {};
	{
		multisample_state_CI.rasterizationSamples		= vk::SampleCountFlagBits::e1;
		multisample_state_CI.sampleShadingEnable		= VK_FALSE;
		multisample_state_CI.minSampleShading			= 1.0f;
		multisample_state_CI.pSampleMask				= nullptr;
		multisample_state_CI.alphaToCoverageEnable		= VK_FALSE;
		multisample_state_CI.alphaToOneEnable			= VK_FALSE;
	}


	// depth stencil state
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_CI {};
	vk::StencilOpState stencil_op_state_front {};
	vk::StencilOpState stencil_op_state_back {};
	{
		auto xml_depth_stencil							= xml_root->FirstChildElement( "DEPTH_STENCIL" );
		if( !xml_depth_stencil ) {
			res->p_logger->LogWarning( String( "Graphics pipeline xml file: " ) + xml_file->GetPath().string().c_str() + " missing <DEPTH_STENCIL> element, fallback to using default values" );
		}

		auto depth_compare_op					= vk::CompareOp::eLess;
		auto depth_compare_op_str				= xml_file->GetFieldValue_Text( xml_depth_stencil, "compare_op", "LESS" );
		if( depth_compare_op_str.size() ) {
			if( depth_compare_op_str == "NEVER" )					depth_compare_op = vk::CompareOp::eNever;
			else if( depth_compare_op_str == "LESS" )				depth_compare_op = vk::CompareOp::eLess;
			else if( depth_compare_op_str == "EQUAL" )				depth_compare_op = vk::CompareOp::eEqual;
			else if( depth_compare_op_str == "LESS_OR_EQUAL" )		depth_compare_op = vk::CompareOp::eLessOrEqual;
			else if( depth_compare_op_str == "GREATER" )			depth_compare_op = vk::CompareOp::eGreater;
			else if( depth_compare_op_str == "NOT_EQUAL" )			depth_compare_op = vk::CompareOp::eNotEqual;
			else if( depth_compare_op_str == "GREATER_OR_EQUAL" )	depth_compare_op = vk::CompareOp::eGreaterOrEqual;
			else if( depth_compare_op_str == "ALWAYS" )				depth_compare_op = vk::CompareOp::eAlways;
		}

		TODO( "Stencil operations are currently disabled, might consider enabling stencils in the future" );
		stencil_op_state_front.failOp		= vk::StencilOp::eKeep;
		stencil_op_state_front.passOp		= vk::StencilOp::eKeep;
		stencil_op_state_front.depthFailOp	= vk::StencilOp::eKeep;
		stencil_op_state_front.compareOp	= vk::CompareOp::eAlways;
		stencil_op_state_front.compareMask	= 0b00000000;
		stencil_op_state_front.writeMask	= 0b00000000;
		stencil_op_state_front.reference	= 0b00000000;
		stencil_op_state_back				= stencil_op_state_front;

		depth_stencil_state_CI.depthTestEnable			= VkBool32( xml_file->GetFieldValue_Bool( xml_depth_stencil, "depth_test_enable", true ) );
		depth_stencil_state_CI.depthWriteEnable			= VkBool32( xml_file->GetFieldValue_Bool( xml_depth_stencil, "depth_write_enable", true ) );
		depth_stencil_state_CI.depthCompareOp			= depth_compare_op;
		depth_stencil_state_CI.stencilTestEnable		= VK_FALSE;
		depth_stencil_state_CI.front					= stencil_op_state_front;
		depth_stencil_state_CI.back						= stencil_op_state_back;
		depth_stencil_state_CI.depthBoundsTestEnable	= VkBool32( xml_file->GetFieldValue_Bool( xml_depth_stencil, "depth_bounds_test_enable", false ) );
		depth_stencil_state_CI.minDepthBounds			= float( xml_file->GetFieldValue_Double( xml_depth_stencil, "min_depth_bounds", 0.0 ) );
		depth_stencil_state_CI.maxDepthBounds			= float( xml_file->GetFieldValue_Double( xml_depth_stencil, "max_depth_bounds", 1.0 ) );
	}


	// color blend state
	vk::PipelineColorBlendStateCreateInfo color_blend_state_CI {};
	Vector<vk::PipelineColorBlendAttachmentState> color_blend_attachment_states;
	{
		auto xml_color_blend							= xml_root->FirstChildElement( "COLOR_BLEND" );
		if( !xml_color_blend ) {
			res->p_logger->LogWarning( String( "Graphics pipeline xml file: " ) + xml_file->GetPath().string().c_str() + " missing <COLOR_BLEND> element, fallback to using default values" );
		}

		auto logic_op							= vk::LogicOp::eCopy;
		auto logic_op_str						= xml_file->GetFieldValue_Text( xml_color_blend, "logic_op", "COPY" );
		if( logic_op_str.size() ) {
			if( logic_op_str == "CLEAR" )				logic_op = vk::LogicOp::eClear;
			else if( logic_op_str == "AND" )			logic_op = vk::LogicOp::eAnd;
			else if( logic_op_str == "AND_REVERSE" )	logic_op = vk::LogicOp::eAndReverse;
			else if( logic_op_str == "COPY" )			logic_op = vk::LogicOp::eCopy;
			else if( logic_op_str == "AND_INVERTED" )	logic_op = vk::LogicOp::eAndInverted;
			else if( logic_op_str == "NO_OP" )			logic_op = vk::LogicOp::eNoOp;
			else if( logic_op_str == "XOR" )			logic_op = vk::LogicOp::eXor;
			else if( logic_op_str == "OR" )				logic_op = vk::LogicOp::eOr;
			else if( logic_op_str == "NOR" )			logic_op = vk::LogicOp::eNor;
			else if( logic_op_str == "EQUIVALENT" )		logic_op = vk::LogicOp::eEquivalent;
			else if( logic_op_str == "INVERT" )			logic_op = vk::LogicOp::eInvert;
			else if( logic_op_str == "OR_REVERSE" )		logic_op = vk::LogicOp::eOrReverse;
			else if( logic_op_str == "COPY_INVERTED" )	logic_op = vk::LogicOp::eCopyInverted;
			else if( logic_op_str == "INVERTED" )		logic_op = vk::LogicOp::eOrInverted;
			else if( logic_op_str == "NAND" )			logic_op = vk::LogicOp::eNand;
			else if( logic_op_str == "SET" )			logic_op = vk::LogicOp::eSet;
		}

		for( auto xml_as		= xml_color_blend->FirstChildElement( "BLEND_ATTACHMENT" ); xml_as; xml_as = xml_as->NextSiblingElement( "BLEND_ATTACHMENT" ) ) {
			color_blend_attachment_states.push_back( {} );
			auto & attachment	= color_blend_attachment_states.back();

			auto lambda_get_blend_factor = [ xml_file, xml_as ]( String name, String default_value ) {
				auto value								= vk::BlendFactor::eSrcColor;
				auto value_str							= xml_file->GetFieldValue_Text( xml_as, name, default_value );
				if( value_str.size() ) {
					if( value_str == "ZERO" )							value = vk::BlendFactor::eZero;
					else if( value_str == "ONE" )						value = vk::BlendFactor::eOne;
					else if( value_str == "SRC_COLOR" )					value = vk::BlendFactor::eSrcColor;
					else if( value_str == "ONE_MINUS_SRC_COLOR" )		value = vk::BlendFactor::eOneMinusSrcColor;
					else if( value_str == "DST_COLOR" )					value = vk::BlendFactor::eDstColor;
					else if( value_str == "ONE_MINUS_DST_COLOR" )		value = vk::BlendFactor::eOneMinusDstColor;
					else if( value_str == "SRC_ALPHA" )					value = vk::BlendFactor::eSrcAlpha;
					else if( value_str == "ONE_MINUS_SRC_ALPHA" )		value = vk::BlendFactor::eOneMinusSrcAlpha;
					else if( value_str == "DST_ALPHA" )					value = vk::BlendFactor::eDstAlpha;
					else if( value_str == "ONE_MINUS_DST_ALPHA" )		value = vk::BlendFactor::eOneMinusDstAlpha;
					else if( value_str == "CONSTANT_COLOR" )			value = vk::BlendFactor::eConstantColor;
					else if( value_str == "ONE_MINUS_CONSTANT_COLOR" )	value = vk::BlendFactor::eOneMinusConstantColor;
					else if( value_str == "CONSTANT_ALPHA" )			value = vk::BlendFactor::eConstantAlpha;
					else if( value_str == "ONE_MINUS_CONSTANT_ALPHA" )	value = vk::BlendFactor::eOneMinusConstantAlpha;
					else if( value_str == "SRC_ALPHA_SATURATE" )		value = vk::BlendFactor::eSrcAlphaSaturate;
					else if( value_str == "SRC1_COLOR" )				value = vk::BlendFactor::eSrc1Color;
					else if( value_str == "ONE_MINUS_SRC1_COLOR" )		value = vk::BlendFactor::eOneMinusSrc1Color;
					else if( value_str == "SRC1_ALPHA" )				value = vk::BlendFactor::eSrc1Alpha;
					else if( value_str == "ONE_MINUS_SRC1_ALPHA" )		value = vk::BlendFactor::eOneMinusSrc1Alpha;
				}
				return value;
			};

			auto lambda_get_blend_op		= [ xml_file, xml_as ]( String name, String default_value ) {
				auto value								= vk::BlendOp::eAdd;
				auto value_str							= xml_file->GetFieldValue_Text( xml_as, name, default_value );
				if( value_str.size() ) {
					if( value_str == "ADD" )					value = vk::BlendOp::eAdd;
					else if( value_str == "SUBTRACT" )			value = vk::BlendOp::eSubtract;
					else if( value_str == "REVERSE_SUBTRACT" )	value = vk::BlendOp::eReverseSubtract;
					else if( value_str == "MIN" )				value = vk::BlendOp::eMin;
					else if( value_str == "MAX" )				value = vk::BlendOp::eMax;
				}
				return value;
			};

			vk::ColorComponentFlags									color_component_flags = vk::ColorComponentFlagBits( 0 );
			{
				auto color_component_flags_str						= xml_file->GetMultiFieldValues_Text( xml_as, "color_component_flags" );
				for( auto & f : color_component_flags_str ) {
					if( f.second == "R" )			color_component_flags |= vk::ColorComponentFlagBits::eR;
					else if( f.second == "G" )		color_component_flags |= vk::ColorComponentFlagBits::eG;
					else if( f.second == "B" )		color_component_flags |= vk::ColorComponentFlagBits::eB;
					else if( f.second == "A" )		color_component_flags |= vk::ColorComponentFlagBits::eA;
				}
			}

			attachment.blendEnable			= VkBool32( xml_file->GetFieldValue_Bool( xml_as, "blend_enable", false ) );;
			attachment.srcColorBlendFactor	= lambda_get_blend_factor( "src_color_blend_factor", "SRC_COLOR" );
			attachment.dstColorBlendFactor	= lambda_get_blend_factor( "dst_color_blend_factor", "ZERO" );
			attachment.colorBlendOp			= lambda_get_blend_op( "color_blend_op", "ADD" );
			attachment.srcAlphaBlendFactor	= lambda_get_blend_factor( "src_alpha_blend_factor", "SRC_COLOR" );
			attachment.dstAlphaBlendFactor	= lambda_get_blend_factor( "dst_alpha_blend_factor", "ZERO" );
			attachment.alphaBlendOp			= lambda_get_blend_op( "color_blend_op", "ADD" );
			attachment.colorWriteMask		= color_component_flags;
		}

		color_blend_state_CI.logicOpEnable			= VkBool32( xml_file->GetFieldValue_Bool( xml_color_blend, "logic_op_enable", false ) );
		color_blend_state_CI.logicOp				= logic_op;
		color_blend_state_CI.attachmentCount		= uint32_t( color_blend_attachment_states.size() );
		color_blend_state_CI.pAttachments			= color_blend_attachment_states.data();
		color_blend_state_CI.blendConstants[ 0 ]	= float( xml_file->GetFieldValue_Double( xml_color_blend, "blend_constant_r", 1.0 ) );
		color_blend_state_CI.blendConstants[ 1 ]	= float( xml_file->GetFieldValue_Double( xml_color_blend, "blend_constant_g", 1.0 ) );
		color_blend_state_CI.blendConstants[ 2 ]	= float( xml_file->GetFieldValue_Double( xml_color_blend, "blend_constant_b", 1.0 ) );
		color_blend_state_CI.blendConstants[ 3 ]	= float( xml_file->GetFieldValue_Double( xml_color_blend, "blend_constant_a", 1.0 ) );
	}


	vk::PipelineDynamicStateCreateInfo dynamic_state_CI {};
	{
		auto xml_dynamic_states						= xml_root->FirstChildElement( "DYNAMIC_STATES" );
		if( !xml_dynamic_states ) {
			res->p_logger->LogWarning( String( "Graphics pipeline xml file: " ) + xml_file->GetPath().string().c_str() + " missing <DYNAMIC_STATES> element, fallback to using default values" );
		}
		// viewport and scissor are always dynamic states to make it easier to migrate pipelines from one output size to another
		res->dynamic_states.push_back( vk::DynamicState::eViewport );
		res->dynamic_states.push_back( vk::DynamicState::eScissor );

		auto dynamic_state_str							= xml_file->GetMultiFieldValues_Text( xml_dynamic_states, "states" );
		for( auto & f : dynamic_state_str ) {
			if( f.second == "LINE_WIDTH" )				res->dynamic_states.push_back( vk::DynamicState::eLineWidth );
			else if( f.second == "DEPTH_BIAS" )			res->dynamic_states.push_back( vk::DynamicState::eDepthBias );
			else if( f.second == "BLEND_CONSTANTS" )	res->dynamic_states.push_back( vk::DynamicState::eBlendConstants );
			else if( f.second == "DEPTH_BOUNDS" )		res->dynamic_states.push_back( vk::DynamicState::eDepthBounds );
		}

		dynamic_state_CI.dynamicStateCount	= uint32_t( res->dynamic_states.size() );
		dynamic_state_CI.pDynamicStates		= res->dynamic_states.data();
	}


	res->image_count					= uint32_t( xml_file->GetFieldValue_Int64( xml_root, "image_count", 0 ) );
	if( res->image_count > BUILD_MAX_PER_SHADER_SAMPLED_IMAGE_COUNT ) {
		std::stringstream ss;
		ss << "Graphics pipeline xml file: "
			<< xml_file->GetPath().string()
			<< " <image_count> exceeds maximum of "
			<< BUILD_MAX_PER_SHADER_SAMPLED_IMAGE_COUNT
			<< " images past maximum are ignored and may result to undefined behaviour in the shader\n";
		res->p_logger->LogWarning( ss.str() );
		assert( 0 && "Image count exceeded" );
		res->image_count = BUILD_MAX_PER_SHADER_SAMPLED_IMAGE_COUNT - 1;
	}

	vk::GraphicsPipelineCreateInfo pipeline_CI {};
	pipeline_CI.flags					= vulkan_flags;
	pipeline_CI.stageCount				= uint32_t( shader_stages.size() );
	pipeline_CI.pStages					= shader_stages.data();
	pipeline_CI.pVertexInputState		= &vertex_input_state_CI;
	pipeline_CI.pInputAssemblyState		= &input_assembly_state_CI;
	pipeline_CI.pTessellationState		= ( res->vk_tessellation_control_shader_module && res->vk_tessellation_evaluation_shader_module ) ? &tessellation_state_CI : nullptr;
	pipeline_CI.pViewportState			= &viewport_state_CI;
	pipeline_CI.pRasterizationState		= &rasterization_state_CI;
	pipeline_CI.pMultisampleState		= &multisample_state_CI;
	pipeline_CI.pDepthStencilState		= &depth_stencil_state_CI;
	pipeline_CI.pColorBlendState		= &color_blend_state_CI;
	pipeline_CI.pDynamicState			= &dynamic_state_CI;
	pipeline_CI.layout					= res->p_renderer->GetVulkanGraphicsPipelineLayout( res->image_count );
	pipeline_CI.renderPass				= res->p_renderer->GetVulkanRenderPass();
	pipeline_CI.subpass					= 0; TODO( "G-buffers, pipeline working either with G-buffers or final render" );
	pipeline_CI.basePipelineHandle		= nullptr;
	pipeline_CI.basePipelineIndex		= 0;
	{
		LOCK_GUARD( *res->ref_vk_device.mutex );
		res->vk_pipeline	= res->ref_vk_device.object.createGraphicsPipeline( nullptr, pipeline_CI );
		TODO( "Implement pipeline cache" );
	}
	if( res->vk_pipeline ) {
		return DeviceResource::LoadingState::LOADED;
	}
	return DeviceResource::LoadingState::UNABLE_TO_LOAD;
}

DeviceResource::LoadingState DeviceResource_GraphicsPipeline::Load()
{
	// test all the usual, if we can load the file
	if( file_resources.size() != 1 ) {
		assert( 0 && "Can't load mesh, file resource count doesn't match requirements" );
		return DeviceResource::LoadingState::UNABLE_TO_LOAD;
	}

	if( file_resources[ 0 ]->GetResourceType() != FileResource::Type::XML ) {
		assert( 0 && "Can't load mesh, file resource type is not XML" );
		return DeviceResource::LoadingState::UNABLE_TO_LOAD;
	}

	auto xml_resource			= dynamic_cast<FileResource_XML*>( file_resources[ 0 ].Get() );
	if( !xml_resource ) {
		assert( 0 && "Can't load xml, file resource dynamic_cast failed" );
		return DeviceResource::LoadingState::UNABLE_TO_LOAD;
	}
	// We do not need to check if the resource has been fully loaded, this function
	// is only called if and when all file resources have been fully loaded in memory

	auto xml_root				= xml_resource->GetRootElement();
	if( !xml_root ) return LoadingState::UNABLE_TO_LOAD;
	if( String( xml_root->Name() ) != "GRAPHICS_PIPELINE" ) return LoadingState::UNABLE_TO_LOAD;

	auto xml_shaders						= xml_root->FirstChildElement( "SHADERS" );
	if( !xml_shaders ) return DeviceResource::LoadingState::UNABLE_TO_LOAD;

	auto shader_paths						= xml_resource->GetMultiFieldValues_Text( xml_shaders, "paths" );
	auto vert_iter							= shader_paths.find( "vertex" );
	auto tesc_iter							= shader_paths.find( "tessellation_control" );
	auto tese_iter							= shader_paths.find( "tessellation_evaluation" );
	auto geom_iter							= shader_paths.find( "geometry" );
	auto frag_iter							= shader_paths.find( "fragment" );
	vertex_shader_resource					= ( vert_iter != shader_paths.end() ) ? p_file_resource_manager->RequestResource( vert_iter->second ) : nullptr;
	tessellation_control_shader_resource	= ( tesc_iter != shader_paths.end() ) ? p_file_resource_manager->RequestResource( tesc_iter->second ) : nullptr;
	tessellation_evaluation_shader_resource	= ( tese_iter != shader_paths.end() ) ? p_file_resource_manager->RequestResource( tese_iter->second ) : nullptr;
	geometry_shader_resource				= ( geom_iter != shader_paths.end() ) ? p_file_resource_manager->RequestResource( geom_iter->second ) : nullptr;
	fragment_shader_resource				= ( frag_iter != shader_paths.end() ) ? p_file_resource_manager->RequestResource( frag_iter->second ) : nullptr;

	// at least vertex and fragment shaders are required
	if( !( vertex_shader_resource && fragment_shader_resource ) ) {
		return LoadingState::UNABLE_TO_LOAD;
	}

	// We need to wait for more resources to load up, so postpone resource loading for now
	SetNextLoadOperation( ContinueGraphicsPipelineLoadTest_1, ContinueGraphicsPipelineLoad_1 );
	return LoadingState::CONTINUE_LOADING;
}

DeviceResource::UnloadingState DeviceResource_GraphicsPipeline::Unload()
{
	{
		LOCK_GUARD( *ref_vk_device.mutex );
		if( vk_pipeline )								ref_vk_device.object.destroyPipeline( vk_pipeline );

		if( vk_vertex_shader_module )					ref_vk_device.object.destroyShaderModule( vk_vertex_shader_module );
		if( vk_tessellation_control_shader_module )		ref_vk_device.object.destroyShaderModule( vk_tessellation_control_shader_module );
		if( vk_tessellation_evaluation_shader_module )	ref_vk_device.object.destroyShaderModule( vk_tessellation_evaluation_shader_module );
		if( vk_geometry_shader_module )					ref_vk_device.object.destroyShaderModule( vk_geometry_shader_module );
		if( vk_fragment_shader_module )					ref_vk_device.object.destroyShaderModule( vk_fragment_shader_module );
	}
	vk_pipeline									= nullptr;

	vk_vertex_shader_module						= nullptr;
	vk_tessellation_control_shader_module		= nullptr;
	vk_tessellation_evaluation_shader_module	= nullptr;
	vk_geometry_shader_module					= nullptr;
	vk_fragment_shader_module					= nullptr;

	vertex_shader_resource						= nullptr;
	tessellation_control_shader_resource		= nullptr;
	tessellation_evaluation_shader_resource		= nullptr;
	geometry_shader_resource					= nullptr;
	fragment_shader_resource					= nullptr;

	image_count									= 0;
	dynamic_states.clear();
	return UnloadingState::UNLOADED;
}

}
