
#include <assert.h>

#include "FileStream.h"

namespace AE
{

FileStream::FileStream()
{
}

FileStream::FileStream( FileSystem * file_system )
{
	p_file_system		= file_system;
}

FileStream::FileStream( FileStream && other )
{
	std::swap( p_file_system, other.p_file_system );
	std::swap( raw_stream, other.raw_stream );
	std::swap( cursor, other.cursor );
}

FileStream::~FileStream()
{
}

FileStream & FileStream::operator=( FileStream && other )
{
	std::swap( p_file_system, other.p_file_system );
	std::swap( raw_stream, other.raw_stream );
	std::swap( cursor, other.cursor );
	return *this;
}

void FileStream::Seek( size_t cursor_position )
{
	cursor										= cursor_position;
	if( cursor < 0 )					cursor	= 0;
	if( cursor >= raw_stream.size() )	cursor	= raw_stream.size() - 1;
}

const Vector<char>& FileStream::GetRawStream() const
{
	return raw_stream;
}

Vector<char>& FileStream::GetEditableRawStream()
{
	return raw_stream;
}

String FileStream::ReadLine()
{
	String ret;
	auto pos	= cursor;
	while( true ) {
		if( cursor + pos >= raw_stream.size() ) {
			cursor = raw_stream.size();
			break;
		}
		if( raw_stream[ pos ] == 0x0D ) {				// CR
			if( cursor + pos + 1 < raw_stream.size() ) {
				if( raw_stream[ pos + 1 ] == 0x0A ) {	// LF
					cursor	= pos + 2;
					break;
				}
			}
			cursor		= pos + 1;
			break;
		}
		if( raw_stream[ pos ] == 0x0A ) {				// LF
			cursor		= pos + 1;
			break;
		}
		ret.push_back( raw_stream[ pos ] );
		pos++;
	}
	return ret;
}

size_t FileStream::Tell() const
{
	return cursor;
}

size_t FileStream::Size() const
{
	return raw_stream.size();
}

bool FileStream::EndOfStream()
{
	return ( cursor == raw_stream.size() );
}

}
