#include <cstring>
#include <png.h>
#include "../common/log.hpp"
#include "../common/memory_mapped_file.hpp"
#include "image.hpp"

namespace PanzerMaps
{

// Currently, supports only PNG rgb and rgba.
ImageRGBA LoadImage( const std::string& image_file_name )
{
	ImageRGBA result;

	MemoryMappedFilePtr file= MemoryMappedFile::Create( image_file_name.c_str() );
	if( file == nullptr )
		return result;

	png_image img;
	std::memset( &img, 0, sizeof(img) );
	img.version= PNG_IMAGE_VERSION;

	png_image_begin_read_from_memory( &img, file->Data(), file->Size() );
	if( PNG_IMAGE_FAILED(img) )
	{
		Log::Warning( "Error, opening png file \"", image_file_name, "\": ", img.message );
		png_image_free( &img );
		return result;
	}
	if( img.format != PNG_FORMAT_RGBA )
	{
		Log::Warning( "Unsupported image format, supported only rgba PNG images" );
		png_image_free( &img );
		return result;
	}

	result.data.resize( 4u * img.width * img.height, 0u );
	result.size[0]= img.width ;
	result.size[1]= img.height;

	png_image_finish_read( &img, nullptr, result.data.data(), img.width * 4u, nullptr );

	return result;
}

} // namespace PanzerMaps
