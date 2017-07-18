#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Math/Math.h"

#include "../FileResource.h"

namespace AE
{

struct Vertex
{
	glm::vec3	position;
	glm::vec3	normal;
	glm::vec2	uv;
	int			material;
};

struct CopyVertex
{
	int32_t		copy_from_index;
};

struct Polygon
{
	int32_t		indices[ 3 ];
};

class FileResource_Mesh : public FileResource
{
public:
	FileResource_Mesh( Engine * engine, FileResourceManager * file_resource_manager );
	~FileResource_Mesh();

	bool								Load( FileStream * stream, const Path & path );
	bool								Unload();

	const Vector<Vertex>			&	GetVertices() const;
	const Vector<CopyVertex>		&	GetCopyVertices() const;
	const Vector<Polygon>			&	GetPolygons() const;

	size_t								GetVerticesByteSize() const;
	size_t								GetCopyVerticesByteSize() const;
	size_t								GetPolygonsByteSize() const;

private:
	Vector<Vertex>						vertices;
	Vector<CopyVertex>					copy_vertices;
	Vector<Polygon>						polygons;
};

}
