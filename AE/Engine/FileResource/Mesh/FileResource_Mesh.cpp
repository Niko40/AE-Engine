
#include "FileResource_Mesh.h"

#include "ME3DFile.h"

namespace AE
{

FileResource_Mesh::FileResource_Mesh( Engine * engine, FileResourceManager * file_resource_manager )
	: FileResource( engine, file_resource_manager, FileResource::Type::MESH )
{
}

FileResource_Mesh::~FileResource_Mesh()
{
}

bool FileResource_Mesh::Load( FileStream * stream, const Path & path )
{
	ME3D_File me3d;
	me3d.LoadFromFileStream( stream );
	if( me3d.IsLoaded() ) {
		auto me3d_v		= me3d.GetVertices();
		auto me3d_vc	= me3d.GetCopyVertices();
		auto me3d_p		= me3d.GetPolygons();

		vertices.resize( me3d_v.size() );
		copy_vertices.resize( me3d_vc.size() );
		polygons.resize( me3d_p.size() );

		for( size_t i=0; i < me3d_v.size(); ++i ) {
			auto & s = me3d_v[ i ];
			auto & d = vertices[ i ];
			d.position		= glm::vec3( s.position[ 0 ], s.position[ 1 ], s.position[ 2 ] );
			d.normal		= glm::vec3( ME3D_ShortToFloat( s.normals[ 0 ] ), ME3D_ShortToFloat( s.normals[ 1 ] ), ME3D_ShortToFloat( s.normals[ 2 ] ) );
			d.uv			= glm::vec2( ME3D_ShortToFloat( s.uvs[ 0 ] ), ME3D_ShortToFloat( s.uvs[ 1 ] ) );
			d.material		= int32_t( s.material_index );
		}
		for( size_t i=0; i < me3d_vc.size(); ++i ) {
			auto & s = me3d_vc[ i ];
			auto & d = copy_vertices[ i ];
			d.copy_from_index	= int32_t( s.copy_from_index );
		}
		for( size_t i=0; i < me3d_p.size(); ++i ) {
			auto & s = me3d_p[ i ];
			auto & d = polygons[ i ];
			d.indices[ 0 ]	= int32_t( s.indices[ 0 ] );
			d.indices[ 1 ]	= int32_t( s.indices[ 1 ] );
			d.indices[ 2 ]	= int32_t( s.indices[ 2 ] );
		}
		return true;
	}
	return false;
}

bool FileResource_Mesh::Unload()
{
	vertices.clear();
	copy_vertices.clear();
	polygons.clear();
	return true;
}

const Vector<Vertex>& FileResource_Mesh::GetVertices() const
{
	return vertices;
}

const Vector<CopyVertex>& FileResource_Mesh::GetCopyVertices() const
{
	return copy_vertices;
}

const Vector<Polygon>& FileResource_Mesh::GetPolygons() const
{
	return polygons;
}

Vector<Vertex>& FileResource_Mesh::GetEditableVertices()
{
	return vertices;
}

Vector<CopyVertex>& FileResource_Mesh::GetEditableCopyVertices()
{
	return copy_vertices;
}

Vector<Polygon>& FileResource_Mesh::GetEditablePolygons()
{
	return polygons;
}

size_t FileResource_Mesh::GetVerticesByteSize() const
{
	return vertices.size() * sizeof( Vertex );
}

size_t FileResource_Mesh::GetCopyVerticesByteSize() const
{
	return copy_vertices.size() * sizeof( CopyVertex );
}

size_t FileResource_Mesh::GetPolygonsByteSize() const
{
	return polygons.size() * sizeof( Polygon );
}

}
