#include <cstdio>
#include <png.h>
#include "../common/log.hpp"
#include "image.hpp"

namespace PanzerMaps
{


// Currently, supports only PNG rgb and rgba.
ImageRGBA LoadImage( const std::string& image_file_name )
{
	ImageRGBA result;

	// TODO -use memory mapping.
	FILE* const fp= std::fopen( image_file_name.c_str(), "rb" );
	if( fp == nullptr )
	{
		Log::Warning( "Can not open file \"", image_file_name, "\"" );
		return result;
	}

	png_structp png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );

	png_infop png_info= png_create_info_struct(png_ptr);

	png_init_io(png_ptr, fp );

	png_read_info(png_ptr, png_info);

	const png_uint_32 width = png_get_image_width ( png_ptr, png_info );
	const png_uint_32 height= png_get_image_height( png_ptr, png_info );
	const png_byte color_type= png_get_color_type( png_ptr, png_info );
	const png_byte bit_depth= png_get_bit_depth( png_ptr, png_info );

	if( !( color_type == PNG_COLOR_TYPE_RGB_ALPHA ) && bit_depth != 8 )
	{
		std::fclose( fp );
		return result;
	}

	result.data.resize( 4u * width * height, 0u );
	for( png_uint_32 y=0u; y < height; ++y )
		png_read_row( png_ptr, result.data.data() + y * width * 4u, nullptr );

	png_destroy_read_struct( &png_ptr, &png_info, NULL);

	std::fclose( fp );

	result.size[0]= width ;
	result.size[1]= height;
	return result;
}

} // namespace PanzerMaps
