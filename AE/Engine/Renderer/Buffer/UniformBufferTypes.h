#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Math/Math.h"

namespace AE
{

struct UniformBufferData_Camera
{
	FMat4		view_matrix;
	FMat4		projection_matrix;
	// Todo
};

struct UniformBufferData_Mesh
{
	FMat4		model_matrix;
	// Todo
};

struct UniformBufferData_Pipeline
{
	// Todo
};

}
