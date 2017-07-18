
#include "FileResource_Lua.h"

namespace AE
{

FileResource_Lua::FileResource_Lua( Engine * engine, FileResourceManager * file_resource_manager )
	: FileResource( engine, file_resource_manager, FileResource::Type::LUA )
{
}

FileResource_Lua::~FileResource_Lua()
{
}

const Vector<char>& FileResource_Lua::GetScript() const
{
	return script;
}

bool FileResource_Lua::Load( FileStream * stream, const Path & path )
{
	script = stream->GetRawStream();
	return true;
}

bool FileResource_Lua::Unload()
{
	script.clear();
	return true;
}

}
