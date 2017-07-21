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
	struct XMLRootStruct_SceneNode
	{
		tinyxml2::XMLElement	*	element					= nullptr;
		String						type;
		String						name;
		bool						visible					= true;
		bool						is_ok					= false;
	};

	struct XMLStruct_Mesh
	{
		tinyxml2::XMLElement	*	element					= nullptr;
		String						name;
		Path						path;
		bool						visible					= true;
		Vec3						position				= Vec3( 0, 0, 0 );
		Quat						rotation				= Quat( 1, 0, 0, 0 );
		Vec3						scale					= Vec3( 1, 1, 1 );
		bool						is_ok					= false;
	};

	struct XMLStruct_RenderInfo
	{
		tinyxml2::XMLElement	*	element					= nullptr;
		Path						pipeline_path;
		bool						is_ok					= false;
	};

	struct XMLStruct_Images
	{
		tinyxml2::XMLElement	*	element					= nullptr;
		Array<Path, BUILD_MAX_PER_SHADER_SAMPLED_IMAGE_COUNT>			texture_paths			= {};
		bool						is_ok					= false;
	};

public:
	FileResource_XML( Engine * engine, FileResourceManager * file_resource_manager );
	~FileResource_XML();

	tinyxml2::XMLElement			*	GetRootElement();
	tinyxml2::XMLElement			*	GetChildElement( tinyxml2::XMLElement * parent, const String & child_name );

	tinyxml2::XMLDocument			*	GetRawXML();

	String								GetFieldValue_Text( tinyxml2::XMLElement * parent, const String & field_name );
	int64_t								GetFieldValue_Int64( tinyxml2::XMLElement * parent, const String & field_name );
	double								GetFieldValue_Double( tinyxml2::XMLElement * parent, const String & field_name );
	bool								GetFieldValue_Bool( tinyxml2::XMLElement * parent, const String & field_name );

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
