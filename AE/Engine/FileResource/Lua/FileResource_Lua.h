#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../FileResource.h"

namespace AE
{

class FileResource_Lua : public FileResource
{
public:
	FileResource_Lua( Engine * engine, FileResourceManager * file_resource_manager );
	~FileResource_Lua();

	const Vector<char>		&	GetScript() const;

private:
	bool						Load( FileStream * stream, const Path & path );
	bool						Unload();

	Vector<char>				script;
};

}
