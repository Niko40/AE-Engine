#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include <tinyxml2.h>

#include "../../Memory/MemoryTypes.h"
#include "../../Math/Math.h"

#include "SceneNodeBase.h"

#include "../../Renderer/DeviceResource/DeviceResource.h"

namespace AE
{

class Engine;
class DeviceResource_Mesh;
class DeviceResource_Image;

class SceneNode : public SceneNodeBase
{
public:
	struct ImageInfo
	{
		Array<DeviceResourceHandle<DeviceResource_Image>, BUILD_MAX_PER_SHADER_SAMPLED_IMAGE_COUNT>		image_resources;
	};

	struct RenderInfo
	{
//		DeviceResourceHandle<DeviceResource_Pipeline>				pipeline_resource;
		ImageInfo							image_info				= {};
	};

	struct MeshInfo
	{
		String								name;
		bool								is_visible				= true;
		Vec3								position				= Vec3( 0, 0, 0 );
		Quat								rotation				= Quat( 1, 0, 0, 0 );
		Vec3								scale					= Vec3( 1, 1, 1 );

		DeviceResourceHandle<DeviceResource_Mesh>					mesh_resource;
		RenderInfo							render_info				= {};
	};

								SceneNode( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path, SceneNodeBase::Type scene_node_type );
	virtual						~SceneNode();

	// Calculates a new transformation matrix from position, scale and rotation
	// Matrix is stored in transformation_matrix variable, it's also returned out of convenience
	const Mat4				&	CalculateTransformationMatrixFromPosScaleRot();

	// inversion of CalculateTransformationMatrixFromPosScaleRot, this one takes in a matrix and
	// calculates a new position, scale and rotation from it
	void						CalculatePosScaleRotFromTransformationMatrix( const Mat4 & new_transformations );

	// 3d position of the object
	Vec3						position						= Vec3( 0, 0, 0 );

	// 3d scale of the object
	Vec3						scale							= Vec3( 1, 1, 1 );

	// Quaternion rotation of the object
	Quat						rotation						= Quat( 1, 0, 0, 0 );

	// transformation matrix of the object to be used with other calculations that need it or with rendering
	// this value is not calculated automatically, use CalculateTransformationMatrixFromPosScaleRot before using this
	Mat4						transformation_matrix			= Mat4( 1 );

protected:
	// because it's easier to call parent functions than branching child object's functions
	// I have inverted the parsing into sections, ParseConfigFile() is responsible for calling it's immediate
	// parent class parse function and the function will parse that section of the XML file as well as any below
	// returns new parent xml element for the next stage or nullptr if error
	tinyxml2::XMLElement	*	ParseConfigFile_SceneNodeSection();

private:
	Vector<MeshInfo>			mesh_info_list;
};

}
