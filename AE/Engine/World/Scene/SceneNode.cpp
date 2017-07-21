
#include "SceneNode.h"

#include <glm/gtx/matrix_decompose.hpp>

#include "../../Engine.h"
#include "../../Logger/Logger.h"

namespace AE
{

SceneNode::SceneNode( Engine * engine, SceneManager * scene_manager, const Path & scene_node_path, SceneNodeBase::Type scene_node_type )
	: SceneNodeBase( engine, scene_manager, scene_node_path, scene_node_type )
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

}
