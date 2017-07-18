#pragma once

#include <filesystem>

#include "../BUILD_OPTIONS.h"
#include "../Platform.h"

#include "../Memory/Memory.h"

namespace AE
{

namespace fsys		= std::tr2::sys;

class FileSystem;

class FileStream
{
	friend class FileSystem;

public:
	FileStream();
	FileStream( FileSystem * file_system );
	FileStream( FileStream & other ) = delete;
	FileStream( FileStream && other );
	~FileStream();

	FileStream			&	operator=( FileStream & other ) = delete;
	FileStream			&	operator=( FileStream && other );

	void					Seek( size_t cursor_position );
	size_t					Tell() const;
	size_t					Size() const;
	bool					EndOfStream();
	const Vector<char>	&	GetRawStream() const;
	Vector<char>		&	GetEditableRawStream();

	// read bytes similarly to fstream, returns the actual amount of bytes read
	template<typename D>
	size_t Read( D * data, size_t amount )
	{
		assert( ( cursor + amount * sizeof( D ) ) <= raw_stream.size() );

		size_t read_amount_bytes = std::min( amount * sizeof( D ), raw_stream.size() - cursor );
		std::memcpy( data, raw_stream.data() + cursor, read_amount_bytes );
		cursor += read_amount_bytes;
		return read_amount_bytes;
	}

	template<typename T>
	const T & Read()
	{
		assert( ( cursor + sizeof( T ) ) < raw_stream.size() );
		return *( (T*)( raw_stream.data() + cursor ) );
		cursor += sizeof( T );
	}

	String					ReadLine();

private:
	FileSystem			*	p_file_system		= nullptr;
	Vector<char>			raw_stream;
	size_t					cursor				= 0;
};

}
