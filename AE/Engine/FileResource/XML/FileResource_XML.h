#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include <tinyxml2.h>

#include "../FileResource.h"

namespace AE
{

class FileResource_XML : public FileResource
{
public:
	FileResource_XML( Engine * engine, FileResourceManager * file_resource_manager );
	~FileResource_XML();

	tinyxml2::XMLDocument	*	GetRawXML();

private:
	bool						Load( FileStream * stream, const Path & path );
	bool						Unload();

	tinyxml2::XMLDocument		xml;
};

}
