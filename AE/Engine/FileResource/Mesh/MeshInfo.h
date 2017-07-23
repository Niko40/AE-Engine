#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Math/Math.h"

namespace AE
{

struct Vertex
{
	glm::vec3	position;
	glm::vec3	normal;
	glm::vec2	uv;
	int32_t		material;
};

struct CopyVertex
{
	int32_t		copy_from_index;
};

struct Polygon
{
	int32_t		indices[ 3 ];
};

}
