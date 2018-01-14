
#include "Math.h"

namespace AE
{

size_t RoundToAlignment( size_t src_value, size_t alignment_value )
{
	return ( ( src_value / alignment_value + !!( src_value % alignment_value ) ) * alignment_value );
}

Mat4 CalculateTransformationMatrixFromPosScaleRot( const Vec3 & position, const Quat & rotation, const Vec3 & scale )
{
	auto transformation_matrix = Mat4( 1 );
	transformation_matrix = glm::translate( transformation_matrix, position );
	transformation_matrix *= glm::mat4_cast( rotation );
	transformation_matrix = glm::scale( transformation_matrix, scale );
	return transformation_matrix;
}

TransformationComponents CalculateComponentsFromTransformationMatrix( const Mat4 & transformations )
{
	TransformationComponents ret {};
	glm::decompose( transformations, ret.scale, ret.rotation, ret.position, ret.skew, ret.perspective );
	ret.rotation	= glm::conjugate( ret.rotation );		// returned rotation is a conjugate so we need to flip it
	return ret;
}

}
