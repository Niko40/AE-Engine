
#include <assert.h>
#include <array>
//#include <png.h>

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "FileResource_Image.h"

#include "../../Engine.h"
#include "../../Logger/Logger.h"
#include "../../Renderer/Renderer.h"
#include "../../FileSystem/FileStream.h"

namespace AE
{
/*
enum class PNGColorType : png_byte
{
	GRAY			= PNG_COLOR_TYPE_GRAY,
	PALETTE			= PNG_COLOR_TYPE_PALETTE,
	RGB				= PNG_COLOR_TYPE_RGB,
	RGB_ALPHA		= PNG_COLOR_TYPE_RGB_ALPHA,
	GRAY_ALPHA		= PNG_COLOR_TYPE_GRAY_ALPHA,
};

struct PNGImage
{
	int32_t				width;
	int32_t				height;
	PNGColorType		color_type;
	png_byte			bit_depth;
	size_t				image_row_bytes;
	Vector<uint8_t>		image_data;
};



void PNGReadDataFromInputStream( png_structp png_ptr, png_bytep out_bytes, png_size_t byte_count_to_read )
{
	auto stream			= (FileStream*)png_get_io_ptr( png_ptr );
	if( stream == NULL )
		return;		// Error

	size_t bytesRead	= stream->Read( (char*)out_bytes, size_t( byte_count_to_read ) );

	assert( (png_size_t)bytesRead == byte_count_to_read );
}



bool ReadPNGImage( FileStream * stream, PNGImage * return_image )
{
	std::array<png_byte, 8> header;		// 8 is the maximum size that can be checked

	// Test the file to be a png file
	stream->Read( header.data(), header.size() );

	if( png_sig_cmp( header.data(), 0, 8 ) ) {
		return false;
	}


	// initialize stuff
	png_structp png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );

	if( !png_ptr ) {
		return false;
	}

	png_infop info_ptr = png_create_info_struct( png_ptr );
	if( !info_ptr ) {
		png_destroy_read_struct( &png_ptr, nullptr, nullptr );
		return false;
	}

	png_set_read_fn( png_ptr, stream, ReadDataFromInputStream );

	if( setjmp( png_jmpbuf( png_ptr ) ) ) {
		png_destroy_read_struct( &png_ptr, &info_ptr, nullptr );
		return false;
	}

	//	png_init_io( png_ptr, fp );
	png_set_sig_bytes( png_ptr, 8 );

	png_read_info( png_ptr, info_ptr );

	return_image->width			= png_get_image_width( png_ptr, info_ptr );
	return_image->height		= png_get_image_height( png_ptr, info_ptr );
	return_image->color_type	= PNGColorType( png_get_color_type( png_ptr, info_ptr ) );
	return_image->bit_depth		= png_get_bit_depth( png_ptr, info_ptr );

	int32_t number_of_passes	= png_set_interlace_handling( png_ptr );
	png_read_update_info( png_ptr, info_ptr );


	// read file
	if( setjmp( png_jmpbuf( png_ptr ) ) ) {
		png_destroy_read_struct( &png_ptr, &info_ptr, nullptr );
		return false;
	}

	const size_t row_bytes			= png_get_rowbytes( png_ptr, info_ptr );
	return_image->image_row_bytes	= row_bytes;
	png_bytep * buffer		= (png_bytep*)engine_internal::MemoryPool_AllocateRaw( return_image->height * sizeof( png_bytep ) );
	for( size_t i=0; i < return_image->height; ++i ) {
		buffer[ i ]			= (png_bytep)engine_internal::MemoryPool_AllocateRaw( row_bytes * sizeof( png_byte ) );
	}

	png_read_image( png_ptr, buffer );

	return_image->image_data.resize( return_image->height * row_bytes );
	for( size_t i=0; i < return_image->height; ++i ) {
		std::memcpy( &return_image->image_data[ i * row_bytes ], buffer[ i ], row_bytes );
		engine_internal::MemoryPool_FreeRaw( buffer[ i ] );
	}
	engine_internal::MemoryPool_FreeRaw( buffer );

	return true;
}
*/
/*
void write_png_file( char* file_name )
{
	// create file
	FILE *fp = fopen( file_name, "wb" );
	if( !fp )
		abort_( "[write_png_file] File %s could not be opened for writing", file_name );


	// initialize stuff
	png_structp png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );

	if( !png_ptr )
		abort_( "[write_png_file] png_create_write_struct failed" );

	info_ptr = png_create_info_struct( png_ptr );
	if( !info_ptr )
		abort_( "[write_png_file] png_create_info_struct failed" );

	if( setjmp( png_jmpbuf( png_ptr ) ) )
		abort_( "[write_png_file] Error during init_io" );

	png_init_io( png_ptr, fp );


	// write header
	if( setjmp( png_jmpbuf( png_ptr ) ) )
		abort_( "[write_png_file] Error during writing header" );

	png_set_IHDR( png_ptr, info_ptr, width, height,
		bit_depth, color_type, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE );

	png_write_info( png_ptr, info_ptr );


	// write bytes
	if( setjmp( png_jmpbuf( png_ptr ) ) )
		abort_( "[write_png_file] Error during writing bytes" );

	png_write_image( png_ptr, row_pointers );


	// end write
	if( setjmp( png_jmpbuf( png_ptr ) ) )
		abort_( "[write_png_file] Error during end of write" );

	png_write_end( png_ptr, NULL );

	// cleanup heap allocation
	for( y=0; y<height; y++ )
		free( row_pointers[ y ] );
	free( row_pointers );

	fclose( fp );
}
*/


FileResource_Image::FileResource_Image( Engine * engine, FileResourceManager * file_resource_manager )
	: FileResource( engine, file_resource_manager, FileResource::Type::IMAGE )
{
}

FileResource_Image::~FileResource_Image()
{
}

bool FileResource_Image::Load( FileStream * stream, const Path & path )
{
	auto renderer	= p_engine->GetRenderer();
	assert( renderer );

	if( path.extension() == ".png" ) {
		// load png file
		/*
		PNGImage png_image {};

		if( ReadPNGImage( stream, &png_image ) ) {
			if( png_image.bit_depth != 8 ) {
				assert( 0 && "Non 8 bit images not supported" );
				return false;
			}

			image_data.bits_per_channel		= 8;
			image_data.width				= png_image.width;
			image_data.height				= png_image.height;

			switch( png_image.color_type ) {
			case PNGColorType::GRAY:
				image_data.format			= vk::Format::eR8Unorm;
				image_data.used_channels	= 1;
				image_data.bytes_per_pixel	= 1;
				image_data.has_alpha		= VK_FALSE;
				break;
			case PNGColorType::PALETTE:
				assert( 0 && "Unsupported png color type" );
				return false;
				break;
			case PNGColorType::RGB:
				image_data.format			= vk::Format::eR8G8B8Unorm;
				image_data.used_channels	= 3;
				image_data.bytes_per_pixel	= 3;
				image_data.has_alpha		= VK_FALSE;
				break;
			case PNGColorType::RGB_ALPHA:
				image_data.format			= vk::Format::eR8G8B8A8Unorm;
				image_data.used_channels	= 4;
				image_data.bytes_per_pixel	= 4;
				image_data.has_alpha		= VK_TRUE;
				break;
			case PNGColorType::GRAY_ALPHA:
				image_data.format			= vk::Format::eR8G8Unorm;
				image_data.used_channels	= 2;
				image_data.bytes_per_pixel	= 2;
				image_data.has_alpha		= VK_TRUE;
				break;
			default:
				image_data			= {};
				return false;
			}

			image_data.image_bytes	= std::move( png_image.image_data );
			return true;
		}
		image_data	= {};
		return false;
		*/

		// only 8 bit images for now, 16 bits require more work
		int x = 0, y = 0, comp = 0;
		auto image	= stbi_load_from_memory( (stbi_uc*)stream->GetRawStream().data(), int( stream->Size() ), &x, &y, &comp, 0 );
		if( nullptr == image ) {
			p_engine->GetLogger()->LogError( String( "ImageLoad failed with message: " ) + stbi_failure_reason() );
			image_data					= {};
			return false;
		}
		image_data.format				= VK_FORMAT_UNDEFINED;		// determine this
		image_data.used_channels		= uint32_t( comp );
		image_data.bits_per_channel		= 8;
		image_data.bytes_per_pixel		= uint32_t( comp * sizeof( stbi_uc ) );
		image_data.width				= uint32_t( x );
		image_data.height				= uint32_t( y );
		image_data.has_alpha			= 0;						// determine this
		image_data.image_bytes.resize( image_data.bytes_per_pixel * image_data.width * image_data.height );
		std::memcpy( image_data.image_bytes.data(), image, image_data.bytes_per_pixel * image_data.width * image_data.height );
		stbi_image_free( image );

		// following stbi rules, there are 1 to 4 channels per image,
		// if image has 2 or 4 channels, it has alpha
		switch( image_data.used_channels ) {
		case 1:
		{
			image_data.format			= VK_FORMAT_R8_UNORM;
			image_data.has_alpha		= 0;
			break;
		}
		case 2:
		{
			image_data.format			= VK_FORMAT_R8G8_UNORM;
			image_data.has_alpha		= 1;
			break;
		}
		case 3:
		{
			image_data.format			= VK_FORMAT_R8G8B8_UNORM;
			image_data.has_alpha		= 0;
			break;
		}
		case 4:
		{
			image_data.format			= VK_FORMAT_R8G8B8A8_UNORM;
			image_data.has_alpha		= 1;
			break;
		}
		default:
			assert( 0 && "The channel count for loaded image wasn't in range of 1-4" );
			image_data	= {};
			return false;
		}
		return true;
	}
	image_data		= {};
	return false;
}

bool FileResource_Image::Unload()
{
	image_data		= {};
	return true;
}

const ImageData & FileResource_Image::GetImageData() const
{
	return image_data;
}

uint32_t FileResource_Image::GetWidth() const
{
	return image_data.width;
}

uint32_t FileResource_Image::GetHeight() const
{
	return image_data.height;
}

VkFormat FileResource_Image::GetFormat() const
{
	return image_data.format;
}

uint32_t FileResource_Image::GetUsedChannels() const
{
	return image_data.used_channels;
}

uint32_t FileResource_Image::GetBitsPerChannel() const
{
	return image_data.bits_per_channel;
}

uint32_t FileResource_Image::GetBytesPerRow() const
{
	return image_data.bytes_per_pixel;
}

VkBool32 FileResource_Image::GetHasAlpha() const
{
	return image_data.has_alpha;
}

}
