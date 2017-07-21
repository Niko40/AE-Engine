#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Memory/MemoryTypes.h"
#include "../../Math/Math.h"

#include "SceneNodeBase.h"

namespace AE
{

class Engine;

class SceneNode : public SceneNodeBase
{
public:
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

private:
};

}
