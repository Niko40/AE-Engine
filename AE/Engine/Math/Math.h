#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace AE
{

using FVec2			= glm::fvec2;
using FVec3			= glm::fvec3;
using FVec4			= glm::fvec4;

using FQuat			= glm::fquat;

using FMat2			= glm::fmat2;
using FMat2x2		= glm::fmat2x2;
using FMat2x3		= glm::fmat2x3;
using FMat2x4		= glm::fmat2x4;

using FMat3			= glm::fmat3;
using FMat3x2		= glm::fmat3x2;
using FMat3x3		= glm::fmat3x3;
using FMat3x4		= glm::fmat3x4;

using FMat4			= glm::fmat4;
using FMat4x2		= glm::fmat4x2;
using FMat4x3		= glm::fmat4x3;
using FMat4x4		= glm::fmat4x4;


using Vec2			= glm::dvec2;
using Vec3			= glm::dvec3;
using Vec4			= glm::dvec4;

using Quat			= glm::dquat;

using Mat2			= glm::dmat2;
using Mat2x2		= glm::dmat2x2;
using Mat2x3		= glm::dmat2x3;
using Mat2x4		= glm::dmat2x4;

using Mat3			= glm::dmat3;
using Mat3x2		= glm::dmat3x2;
using Mat3x3		= glm::dmat3x3;
using Mat3x4		= glm::dmat3x4;

using Mat4			= glm::dmat4;
using Mat4x2		= glm::dmat4x2;
using Mat4x3		= glm::dmat4x3;
using Mat4x4		= glm::dmat4x4;


size_t RoundToAlignment( size_t src_value, size_t alignment_value );

}
