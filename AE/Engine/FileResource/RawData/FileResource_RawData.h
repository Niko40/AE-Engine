#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../FileResource.h"

namespace AE
{

class FileResource_RawData : public FileResource
{
public:
	FileResource_RawData( Engine * engine, FileResourceManager * file_resource_manager );
	~FileResource_RawData();

	const Vector<char>	&	GetData();

private:

	bool					Load( FileStream * stream, const Path & path );
	bool					Unload();

	Vector<char>			data;
};

}
