#pragma once

#include <filesystem>
#include <mutex>

#include "../BUILD_OPTIONS.h"
#include "../Platform.h"

#include "../SubSystem.h"
#include "../Memory/Memory.h"
#include "FileStream.h"

namespace AE
{

class Logger;

class FileSystem : public SubSystem
{
public:
	FileSystem( Engine * engine );
	~FileSystem();

	FileStream						*	OpenFileStream( Path path );
	void								CloseFileStream( FileStream * stream );

private:
	std::mutex							mutex_opened_streams;
	List<FileStream>					opened_streams;
};

}
