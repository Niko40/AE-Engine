#include "ME3DFile.h"

#include <fstream>
#include <assert.h>

#include "../../FileSystem/FileSystem.h"

// Modified to fit the engine
namespace AE
{

constexpr uint32_t ME3D_FILE_ID						= 1144210765;

constexpr float ME3D_FLOAT_TO_SHORT					= 32767.0f;
constexpr float ME3D_SHORT_TO_FLOAT					= 1 / ME3D_FLOAT_TO_SHORT;

typedef		uint16_t		me3d_str_size_t;		// 16 bits is enough to handle character strings used by me3d

uint16_t ME3D_FloatToShort( float value )
{
	return uint16_t( ME3D_FLOAT_TO_SHORT * value );
}

float ME3D_ShortToFloat( uint16_t value )
{
	return float( ME3D_SHORT_TO_FLOAT * value );
}

String ME3D_ReadString( std::ifstream * file )
{
	assert( nullptr != file );
	assert( file->is_open() );

	me3d_str_size_t		str_size		= 0;
	String				str;
	file->read( (char*)&str_size, sizeof( me3d_str_size_t ) );
	str.resize( str_size );
	file->read( &str[ 0 ], str_size );

	return str;
}

void ME3D_WriteString( std::ofstream * file, String str )
{
	assert( nullptr != file );
	assert( file->is_open() );

	me3d_str_size_t		str_size		= me3d_str_size_t( str.size() );
	file->write( (char*)&str_size, sizeof( me3d_str_size_t ) );
	file->write( &str[ 0 ], str_size );
}

String ME3D_ReadStringFromStream( FileStream * stream )
{
	assert( nullptr != stream );

	me3d_str_size_t		str_size		= stream->Read<me3d_str_size_t>();
	String				str;
	str.resize( str_size );
	stream->Read( &str[ 0 ], str_size );
	return str;
}

void ME3D_WriteStringFromStream( FileStream * stream, String str )
{
	assert( nullptr != stream );
	assert( 0 && "Not supported by FileStream" );
}


ME3D_File::ME3D_File()
{
}

ME3D_File::ME3D_File( String path )
{
	Load( path );
}


ME3D_File::~ME3D_File()
{
}

bool ME3D_File::Load( String path )
{
	is_loaded		= false;
	vertices.clear();
	vertex_copies.clear();
	polygons.clear();

	std::ifstream file( path.c_str(), std::ifstream::binary | std::ifstream::ate );
	if( !file.is_open() ) return false;

	size_t file_size = file.tellg();
	if( file_size < sizeof( ME3D_Header ) ) return false;

	file.seekg( 0 );
	file.read( (char*)&head, sizeof( head ) );

	if( ME3D_FILE_ID != head.file_id )						return false;
	if( head.file_size != file_size )						return false;
	if( head.head_size != sizeof( ME3D_Header ) )			return false;
	if( head.vert_size > sizeof( ME3D_Vertex ) )			return false;
	if( head.vert_copy_size > sizeof( ME3D_VertexCopy ) )	return false;
	if( head.polygon_size > sizeof( ME3D_Polygon ) )		return false;
	// if we got here we can be pretty sure we are reading a compatible file

	// we can check the padding of the data
	bool aligned_vertices		= false;
	bool aligned_vertex_copies	= false;
	bool aligned_polygons		= false;
	if( head.vert_size == sizeof( ME3D_Vertex ) )			aligned_vertices		= true;
	if( head.vert_copy_size == sizeof( ME3D_VertexCopy ) )	aligned_vertex_copies	= true;
	if( head.polygon_size == sizeof( ME3D_Polygon ) )		aligned_polygons		= true;

	vertices.resize( head.vert_count + head.vert_copy_count );
	vertex_copies.resize( head.vert_copy_count );
	polygons.resize( head.polygon_count );

	// based on the way that the data is padded we can either copy it all at once or one at a time
	// not sure if this does anything to the loading speed but lets do this because why not
	file.seekg( head.vert_location );
	if( aligned_vertices ) {
		file.read( (char*)vertices.data(), head.vert_count * head.vert_size );
	} else {
		for( int32_t i=0; i < head.vert_count; ++i ) {
			file.read( (char*)&vertices[ i ], head.vert_size );
		}
	}
	file.seekg( head.vert_copy_location );
	if( aligned_vertex_copies ) {
		file.read( (char*)vertex_copies.data(), head.vert_copy_count * head.vert_copy_size );
	} else {
		for( int32_t i=0; i < head.vert_copy_count; ++i ) {
			file.read( (char*)&vertex_copies[ i ], head.vert_copy_size );
		}
	}
	file.seekg( head.polygon_location );
	if( aligned_polygons ) {
		file.read( (char*)polygons.data(), head.polygon_count * head.polygon_size );
	} else {
		for( int32_t i=0; i < head.polygon_count; ++i ) {
			file.read( (char*)&polygons[ i ], head.polygon_size );
		}
	}
	meta_data.resize( head.metadata_count );
	file.seekg( head.metadata_location );
	for( int32_t i=0; i < head.metadata_count; ++i ) {
		auto & m = meta_data[ i ];
		m.identifier		= ME3D_ReadString( &file );
		int32_t data_lenght	= 0;
		file.read( (char*)&data_lenght, sizeof( data_lenght ) );
		file.read( m.data.data(), m.data.size() );
	}

	// we also need to calculate the copy vertices, but those
	// take the coordinates of the the vertext it's copying
	for( int32_t i=0; i < head.vert_copy_count; ++i ) {
		int32_t dst_index = head.vert_count + i;
		vertices[ dst_index ].position[ 0 ]			= vertices[ vertex_copies[ i ].copy_from_index ].position[ 0 ];
		vertices[ dst_index ].position[ 1 ]			= vertices[ vertex_copies[ i ].copy_from_index ].position[ 1 ];
		vertices[ dst_index ].position[ 2 ]			= vertices[ vertex_copies[ i ].copy_from_index ].position[ 2 ];
		vertices[ dst_index ].normals[ 0 ]			= vertices[ vertex_copies[ i ].copy_from_index ].normals[ 0 ];
		vertices[ dst_index ].normals[ 1 ]			= vertices[ vertex_copies[ i ].copy_from_index ].normals[ 1 ];
		vertices[ dst_index ].normals[ 2 ]			= vertices[ vertex_copies[ i ].copy_from_index ].normals[ 2 ];
		vertices[ dst_index ].uvs[ 0 ]				= vertex_copies[ i ].uvs[ 0 ];
		vertices[ dst_index ].uvs[ 1 ]				= vertex_copies[ i ].uvs[ 1 ];
		vertices[ dst_index ].material_index		= vertex_copies[ i ].material_index;
	}

	is_loaded	= true;
	return		true;
}

bool ME3D_File::LoadFromFileStream( FileStream * stream )
{
	is_loaded		= false;
	vertices.clear();
	vertex_copies.clear();
	polygons.clear();

	size_t stream_size = stream->Size();
	if( stream_size < sizeof( ME3D_Header ) ) return false;

	stream->Seek( 0 );
	head = stream->Read<ME3D_Header>();

	if( ME3D_FILE_ID != head.file_id )						return false;
	if( head.file_size != stream_size )						return false;
	if( head.head_size != sizeof( ME3D_Header ) )			return false;
	if( head.vert_size > sizeof( ME3D_Vertex ) )			return false;
	if( head.vert_copy_size > sizeof( ME3D_VertexCopy ) )	return false;
	if( head.polygon_size > sizeof( ME3D_Polygon ) )		return false;
	// if we got here we can be pretty sure we are reading a compatible file

	// we can check the padding of the data
	bool aligned_vertices		= false;
	bool aligned_vertex_copies	= false;
	bool aligned_polygons		= false;
	if( head.vert_size			== sizeof( ME3D_Vertex ) )		aligned_vertices		= true;
	if( head.vert_copy_size		== sizeof( ME3D_VertexCopy ) )	aligned_vertex_copies	= true;
	if( head.polygon_size		== sizeof( ME3D_Polygon ) )		aligned_polygons		= true;

	vertices.resize( head.vert_count + head.vert_copy_count );
	vertex_copies.resize( head.vert_copy_count );
	polygons.resize( head.polygon_count );

	// based on the way that the data is padded we can either copy it all at once or one at a time
	// not sure if this does anything to the loading speed but lets do this because why not
	stream->Seek( head.vert_location );
	{
		for( int32_t i=0; i < head.vert_count; ++i ) {
			vertices[ i ] = stream->Read<ME3D_Vertex>();
		}
	}
	stream->Seek( head.vert_copy_location );
	{
		for( int32_t i=0; i < head.vert_copy_count; ++i ) {
			vertex_copies[ i ] = stream->Read<ME3D_VertexCopy>();
		}
	}
	stream->Seek( head.polygon_location );
	{
		for( int32_t i=0; i < head.polygon_count; ++i ) {
			polygons[ i ] = stream->Read<ME3D_Polygon>();
		}
	}
	meta_data.resize( head.metadata_count );
	stream->Seek( head.metadata_location );
	for( int32_t i=0; i < head.metadata_count; ++i ) {
		auto & m = meta_data[ i ];
		m.identifier		= ME3D_ReadStringFromStream( stream );
		m.data.resize( stream->Read<int32_t>() );
		stream->Read( m.data.data(), m.data.size() );
	}

	// we also need to calculate the copy vertices, but those
	// take the coordinates of the the vertext it's copying
	for( int32_t i=0; i < head.vert_copy_count; ++i ) {
		int32_t dst_index = head.vert_count + i;
		vertices[ dst_index ].position[ 0 ]			= vertices[ vertex_copies[ i ].copy_from_index ].position[ 0 ];
		vertices[ dst_index ].position[ 1 ]			= vertices[ vertex_copies[ i ].copy_from_index ].position[ 1 ];
		vertices[ dst_index ].position[ 2 ]			= vertices[ vertex_copies[ i ].copy_from_index ].position[ 2 ];
		vertices[ dst_index ].normals[ 0 ]			= vertices[ vertex_copies[ i ].copy_from_index ].normals[ 0 ];
		vertices[ dst_index ].normals[ 1 ]			= vertices[ vertex_copies[ i ].copy_from_index ].normals[ 1 ];
		vertices[ dst_index ].normals[ 2 ]			= vertices[ vertex_copies[ i ].copy_from_index ].normals[ 2 ];
		vertices[ dst_index ].uvs[ 0 ]				= vertex_copies[ i ].uvs[ 0 ];
		vertices[ dst_index ].uvs[ 1 ]				= vertex_copies[ i ].uvs[ 1 ];
		vertices[ dst_index ].material_index		= vertex_copies[ i ].material_index;
	}

	is_loaded	= true;
	return		true;
}

const Vector<ME3D_Vertex>& ME3D_File::GetVertices() const
{
	return vertices;
}

const Vector<ME3D_VertexCopy>& ME3D_File::GetCopyVertices() const
{
	return vertex_copies;
}

const Vector<ME3D_Polygon>& ME3D_File::GetPolygons() const
{
	return polygons;
}

const Vector<ME3D_MetaData>& ME3D_File::GetMetaData() const
{
	return meta_data;
}

bool ME3D_File::IsLoaded() const
{
	return is_loaded;
}

}
