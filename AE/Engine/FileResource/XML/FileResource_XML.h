#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include <tinyxml2.h>

#include "../FileResource.h"
#include "../../Math/Math.h"

namespace AE
{

class FileResource_XML : public FileResource
{
public:
	FileResource_XML( Engine * engine, FileResourceManager * file_resource_manager );
	~FileResource_XML();

	tinyxml2::XMLElement			*	GetRootElement();
	tinyxml2::XMLElement			*	GetChildElement( tinyxml2::XMLElement * parent, const String & child_name );

	tinyxml2::XMLDocument			*	GetRawXML();

	String								GetFieldValue_Text( tinyxml2::XMLElement * parent, const String & field_name, const String & default_value = "" );
	int64_t								GetFieldValue_Int64( tinyxml2::XMLElement * parent, const String & field_name, int64_t default_value = 0 );
	double								GetFieldValue_Double( tinyxml2::XMLElement * parent, const String & field_name, double default_value = 0.0 );
	bool								GetFieldValue_Bool( tinyxml2::XMLElement * parent, const String & field_name, bool default_value = false );

	Map<String, String>					GetMultiFieldValues_Text( tinyxml2::XMLElement * parent, const String & field_name );
	Map<String, int64_t>				GetMultiFieldValues_Int64( tinyxml2::XMLElement * parent, const String & field_name );
	Map<String, double>					GetMultiFieldValues_Double( tinyxml2::XMLElement * parent, const String & field_name );
	Map<String, bool>					GetMultiFieldValues_Bool( tinyxml2::XMLElement * parent, const String & field_name );

private:
	bool								Load( FileStream * stream, const Path & path );
	bool								Unload();

	tinyxml2::XMLDocument				xml;
};

}
