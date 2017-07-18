
#include "FileResource_RawData.h"

namespace AE
{

FileResource_RawData::FileResource_RawData( Engine * engine, FileResourceManager * file_resource_manager )
	: FileResource( engine, file_resource_manager, FileResource::Type::RAW_DATA )
{
}

FileResource_RawData::~FileResource_RawData()
{
}

const Vector<char>& FileResource_RawData::GetData()
{
	return data;
}

bool FileResource_RawData::Load( FileStream * stream, const Path & path )
{
	data.resize( stream->Size() );
	stream->Read( data.data(), data.size() );
	return true;
}

bool FileResource_RawData::Unload()
{
	data.clear();
	return true;
}

}
