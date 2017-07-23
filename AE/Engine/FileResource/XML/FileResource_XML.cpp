
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

tinyxml2::XMLElement * FileResource_XML::GetRootElement()
{
	return xml.RootElement();
}

tinyxml2::XMLElement * FileResource_XML::GetChildElement( tinyxml2::XMLElement * parent, const String & child_name )
{
	if( nullptr != parent ) {
		return parent->FirstChildElement( child_name.c_str() );
	}
	return nullptr;
}

tinyxml2::XMLDocument * FileResource_XML::GetRawXML()
{
	return &xml;
}

String FileResource_XML::GetFieldValue_Text( tinyxml2::XMLElement * parent, const String & field_name, const String & default_value )
{
	if( !parent ) return default_value;
	auto type_element	= parent->FirstChildElement( field_name.c_str() );
	if( nullptr == type_element ) return default_value;
	auto attribute		= type_element->FirstAttribute();
	if( nullptr == attribute ) return default_value;
	return attribute->Value();
}

int64_t FileResource_XML::GetFieldValue_Int64( tinyxml2::XMLElement * parent, const String & field_name, int64_t default_value )
{
	if( !parent ) return default_value;
	auto type_element	= parent->FirstChildElement( field_name.c_str() );
	if( nullptr == type_element ) return default_value;
	auto attribute		= type_element->FirstAttribute();
	if( nullptr == attribute ) return default_value;
	return attribute->Int64Value();
}

double FileResource_XML::GetFieldValue_Double( tinyxml2::XMLElement * parent, const String & field_name, double default_value )
{
	if( !parent ) return default_value;
	auto type_element	= parent->FirstChildElement( field_name.c_str() );
	if( nullptr == type_element ) return default_value;
	auto attribute		= type_element->FirstAttribute();
	if( nullptr == attribute ) return default_value;
	return attribute->DoubleValue();
}

bool FileResource_XML::GetFieldValue_Bool( tinyxml2::XMLElement * parent, const String & field_name, bool default_value )
{
	if( !parent ) return default_value;
	auto type_element	= parent->FirstChildElement( field_name.c_str() );
	if( nullptr == type_element ) return default_value;
	auto attribute		= type_element->FirstAttribute();
	if( nullptr == attribute ) return default_value;
	return attribute->BoolValue();
}

Map<String, String> FileResource_XML::GetMultiFieldValues_Text( tinyxml2::XMLElement * parent, const String & field_name )
{
	if( !parent ) return {};
	auto type_element = parent->FirstChildElement( field_name.c_str() );
	if( nullptr == type_element ) return {};
	Map<String, String> ret {};
	for( auto a = type_element->FirstAttribute(); a; a = a->Next() ) {
		ret.insert( std::pair<String, String>( a->Name(), a->Value() ) );
	}
	return ret;
}

Map<String, int64_t> FileResource_XML::GetMultiFieldValues_Int64( tinyxml2::XMLElement * parent, const String & field_name )
{
	if( !parent ) return {};
	auto type_element = parent->FirstChildElement( field_name.c_str() );
	if( nullptr == type_element ) return {};
	Map<String, int64_t> ret {};
	for( auto a = type_element->FirstAttribute(); a; a = a->Next() ) {
		ret.insert( std::pair<String, int64_t>( a->Name(), a->Int64Value() ) );
	}
	return ret;
}

Map<String, double> FileResource_XML::GetMultiFieldValues_Double( tinyxml2::XMLElement * parent, const String & field_name )
{
	if( !parent ) return {};
	auto type_element = parent->FirstChildElement( field_name.c_str() );
	if( nullptr == type_element ) return {};
	Map<String, double> ret {};
	for( auto a = type_element->FirstAttribute(); a; a = a->Next() ) {
		ret.insert( std::pair<String, double>( a->Name(), a->DoubleValue() ) );
	}
	return ret;
}

Map<String, bool> FileResource_XML::GetMultiFieldValues_Bool( tinyxml2::XMLElement * parent, const String & field_name )
{
	if( !parent ) return {};
	auto type_element = parent->FirstChildElement( field_name.c_str() );
	if( nullptr == type_element ) return {};
	Map<String, bool> ret {};
	for( auto a = type_element->FirstAttribute(); a; a = a->Next() ) {
		ret.insert( std::pair<String, bool>( a->Name(), a->BoolValue() ) );
	}
	return ret;
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
