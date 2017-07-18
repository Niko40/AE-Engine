
#include <fstream>

#include <assert.h>

#include "FileSystem.h"

#include "../Logger/Logger.h"

namespace AE
{

FileSystem::FileSystem( Engine * engine )
	: SubSystem( engine, "FileSystem" )
{
}

FileSystem::~FileSystem()
{
}

FileStream * FileSystem::OpenFileStream( Path path )
{
	TODO( "File archive support" );

	FileStream		*	ret		= nullptr;
	if( fsys::is_regular_file( path ) ) {
		std::ifstream file( path, std::fstream::out | std::fstream::binary | std::fstream::ate );
		if( file.good() && file.is_open() ) {
			int64_t		file_size	= int64_t( file.tellg() );
			file.seekg( 0 );
			auto filestream			= FileStream( this );
			filestream.raw_stream.resize( file_size );
			file.read( filestream.raw_stream.data(), file_size );

			std::lock_guard<std::mutex> opened_streams_guard( mutex_opened_streams );
			opened_streams.push_back( std::move( filestream ) );
			ret		= &opened_streams.back();
		}
	}
	return ret;
}

void FileSystem::CloseFileStream( FileStream * stream )
{
	if( nullptr		!= stream ) {
		opened_streams.remove_if([stream](FileStream & i){
			return stream == &i;
		} );
	}
}

}
