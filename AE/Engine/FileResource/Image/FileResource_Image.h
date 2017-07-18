#pragma once

#include "../../BUILD_OPTIONS.h"
#include "../../Platform.h"

#include "../../Vulkan/Vulkan.h"

#include "../FileResource.h"
#include "ImageData.h"

namespace AE
{

class FileResource_Image : public FileResource
{
public:
	FileResource_Image( Engine * engine, FileResourceManager * file_resource_manager );
	~FileResource_Image();

	bool					Load( FileStream * stream, const Path & path );
	bool					Unload();

	const ImageData		&	GetImageData() const;

	uint32_t				GetWidth() const;
	uint32_t				GetHeight() const;
	vk::Format				GetFormat() const;
	uint32_t				GetUsedChannels() const;
	uint32_t				GetBitsPerChannel() const;
	uint32_t				GetBytesPerRow() const;
	vk::Bool32				GetHasAlpha() const;

private:
	ImageData				image_data;
};

}
