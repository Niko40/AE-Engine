#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Memory/Memory.h"

// Modified to fit the engine
namespace AE
{

class FileStream;

struct ME3D_Header
{
	int32_t			file_id;
	int32_t			head_size;
	int32_t			file_size;

	int32_t			vert_count;
	int32_t			vert_size;
	int32_t			vert_location;

	int32_t			vert_copy_count;
	int32_t			vert_copy_size;
	int32_t			vert_copy_location;

	int32_t			polygon_count;
	int32_t			polygon_size;
	int32_t			polygon_location;
		
	int32_t			metadata_count;
	int32_t			metadata_size;
	int32_t			metadata_location;

private:
	int32_t			reserved;
};

struct ME3D_MetaData
{
	String			identifier;
	Vector<char>	data;
};

struct ME3D_Vertex
{
	float			position[ 3 ];
	int16_t			normals[ 3 ];
	int16_t			uvs[ 2 ];
	int16_t			material_index;
};

struct ME3D_VertexCopy
{
	int32_t			copy_from_index;
	int16_t			uvs[ 2 ];
	int16_t			material_index;
};

struct ME3D_Polygon
{
	int32_t			indices[ 3 ];
};

uint16_t ME3D_FloatToShort( float value );
float ME3D_ShortToFloat( uint16_t value );

class ME3D_File
{
public:
	ME3D_File();
	ME3D_File( String path );
	~ME3D_File();

	bool										Load( String path );						// returns true if successfully loaded a file
	bool										LoadFromFileStream( FileStream * stream );	// returns true if successfully loaded a file
	const Vector<ME3D_Vertex>				&	GetVertices() const;
	const Vector<ME3D_VertexCopy>			&	GetCopyVertices() const;
	const Vector<ME3D_Polygon>				&	GetPolygons() const;
	const Vector<ME3D_MetaData>				&	GetMetaData() const;

	bool										IsLoaded() const;

private:
	ME3D_Header									head;

	Vector<ME3D_Vertex>							vertices;
	Vector<ME3D_VertexCopy>						vertex_copies;
	Vector<ME3D_Polygon>						polygons;
	Vector<ME3D_MetaData>						meta_data;

	bool										is_loaded					= false;
};

}
