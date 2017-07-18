
#include "FileResource_XML.h"

namespace AE
{

FileResource_XML::FileResource_XML( Engine * engine, FileResourceManager * file_resource_manager )
	: FileResource( engine, file_resource_manager, FileResource::Type::XML )
{
}

FileResource_XML::~FileResource_XML()
{
}

tinyxml2::XMLDocument * FileResource_XML::GetRawXML()
{
	return &xml;
}

bool FileResource_XML::Load( FileStream * stream, const Path & path )
{
	assert( stream );

	auto result = xml.Parse( stream->GetRawStream().data(), stream->Size() );
	if( result == tinyxml2::XMLError::XML_SUCCESS ) {
		return true;
	}
	return false;
}

bool FileResource_XML::Unload()
{
	xml.Clear();
	return true;
}

}
