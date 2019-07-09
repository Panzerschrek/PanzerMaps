#include <cstring>
#include <vector>
#include "textures_generation.hpp"

namespace PanzerMaps
{

namespace TexConstants
{

static const unsigned char circle_color[4]{ 255, 255, 255, 200 };
static const unsigned char circle_color_pressed[4]{ 200, 200, 200, 200 };
static const unsigned char plus_color[4]{ 128, 128, 128, 200 };
static const unsigned char active_gps_symbol_color[4]{ 128, 128, 255, 200 };

} // TexConstants

static void GenCircle( const size_t size, const unsigned char* color_rgba, unsigned char* const out_data )
{
	const int radius= int(size/2u - 2u);
	const int square_radius= radius * radius;
	const int square_radius_plus_one= (radius+1) * (radius+1);
	for( size_t y= 0u; y < size; ++y )
	{
		const int dy= int(y) - int(size/2u);
		for( size_t x= 0u; x < size; ++x )
		{
			const int dx= int(x) - int(size/2u);

			unsigned char* const dst= out_data + 4u * ( x + y * size );
			std::memcpy( dst, color_rgba, sizeof(unsigned char) * 4u );
			const int dist= dx * dx + dy * dy;
			if( dist <= square_radius )
			{}
			else if( dist <= square_radius_plus_one )
				dst[3]= static_cast<unsigned char>( int(color_rgba[3]) * ( square_radius_plus_one - dist ) / ( square_radius_plus_one - square_radius ) );
			else
				dst[3]= 0;
		}
	}
}

static r_Texture CreateTexture( const size_t width, const size_t height, const std::vector<unsigned char>& data )
{
	r_Texture texture( r_Texture::PixelFormat::RGBA8, static_cast<unsigned int>(width), static_cast<unsigned int>(height), data.data() );
	texture.SetFiltration( r_Texture::Filtration::LinearMipmapLinear, r_Texture::Filtration::Linear );
	texture.SetWrapMode( r_Texture::WrapMode::Clamp );
	texture.BuildMips();
	return texture;
}

static r_Texture GenTexture_ZoomPlus_impl( const size_t tex_size, const unsigned char* const circle_color )
{
	std::vector<unsigned char> tex_data( tex_size * tex_size * 4u );

	GenCircle( tex_size, circle_color, tex_data.data() );

	for( size_t y= tex_size * 7u / 16u; y < tex_size * 9u / 16u; ++y )
	for( size_t x= tex_size / 4u; x < tex_size * 3u / 4u; ++x )
		std::memcpy( tex_data.data() + 4u * ( x + y * tex_size ), TexConstants::plus_color, sizeof(unsigned char) * 4u );

	for( size_t y= tex_size / 4u; y < tex_size * 3u / 4u; ++y )
	for( size_t x= tex_size * 7u / 16u; x < tex_size * 9u / 16u; ++x )
		std::memcpy( tex_data.data() + 4u * ( x + y * tex_size ), TexConstants::plus_color, sizeof(unsigned char) * 4u );

	return CreateTexture( tex_size, tex_size, tex_data );
}

static r_Texture GenTexture_ZoomMinus_impl( const size_t tex_size, const unsigned char* const circle_color )
{
	std::vector<unsigned char> tex_data( tex_size * tex_size * 4u );

	GenCircle( tex_size, circle_color, tex_data.data() );

	for( size_t y= tex_size * 7u / 16u; y < tex_size * 9u / 16u; ++y )
	for( size_t x= tex_size / 4u; x < tex_size * 3u / 4u; ++x )
		std::memcpy( tex_data.data() + 4u * ( x + y * tex_size ), TexConstants::plus_color, sizeof(unsigned char) * 4u );

	return CreateTexture( tex_size, tex_size, tex_data );
}

r_Texture GenTexture_ZoomPlus( size_t tex_size )
{
	return GenTexture_ZoomPlus_impl( tex_size, TexConstants::circle_color );
}

r_Texture GenTexture_ZoomMinus( const size_t tex_size )
{
	return GenTexture_ZoomMinus_impl( tex_size, TexConstants::circle_color );
}

r_Texture GenTexture_ZoomPlusPressed( size_t tex_size )
{
	return GenTexture_ZoomPlus_impl( tex_size, TexConstants::circle_color_pressed );
}

r_Texture GenTexture_ZoomMinusPressed( const size_t tex_size )
{
	return GenTexture_ZoomMinus_impl( tex_size, TexConstants::circle_color_pressed );
}

static r_Texture GenTexture_GPSButton_impl( const size_t tex_size, const unsigned char* const symbol_color )
{
	std::vector<unsigned char> tex_data( tex_size * tex_size * 4u );

	GenCircle( tex_size, TexConstants::circle_color, tex_data.data() );

	// Square.
	for( size_t y= tex_size * 4u / 16u; y < tex_size * 5u / 16u; ++y )
	for( size_t x= tex_size / 4u; x < tex_size * 3u / 4u; ++x )
		std::memcpy( tex_data.data() + 4u * ( x + y * tex_size ), symbol_color, sizeof(unsigned char) * 4u );

	for( size_t y= tex_size * 11u / 16u; y < tex_size * 12u / 16u; ++y )
	for( size_t x= tex_size / 4u; x < tex_size * 3u / 4u; ++x )
		std::memcpy( tex_data.data() + 4u * ( x + y * tex_size ), symbol_color, sizeof(unsigned char) * 4u );

	for( size_t y= tex_size / 4u; y < tex_size * 3u / 4u; ++y )
	for( size_t x= tex_size * 4u / 16u; x < tex_size * 5u / 16u; ++x )
		std::memcpy( tex_data.data() + 4u * ( x + y * tex_size ), symbol_color, sizeof(unsigned char) * 4u );

	for( size_t y= tex_size / 4u; y < tex_size * 3u / 4u; ++y )
	for( size_t x= tex_size * 11u / 16u; x < tex_size * 12u / 16u; ++x )
		std::memcpy( tex_data.data() + 4u * ( x + y * tex_size ), symbol_color, sizeof(unsigned char) * 4u );

	// Cross bars
	for( size_t y= tex_size * 15u / 32u; y < tex_size * 17u / 32u; ++y )
	for( size_t x= tex_size * 2u / 16u; x < tex_size * 4u / 16u; ++x )
		std::memcpy( tex_data.data() + 4u * ( x + y * tex_size ), symbol_color, sizeof(unsigned char) * 4u );

	for( size_t y= tex_size * 15u / 32u; y < tex_size * 17u / 32u; ++y )
	for( size_t x= tex_size * 12u / 16u; x < tex_size * 14u / 16u; ++x )
		std::memcpy( tex_data.data() + 4u * ( x + y * tex_size ), symbol_color, sizeof(unsigned char) * 4u );

	for( size_t x= tex_size * 15u / 32u; x < tex_size * 17u / 32u; ++x )
	for( size_t y= tex_size * 2u / 16u; y < tex_size * 4u / 16u; ++y )
		std::memcpy( tex_data.data() + 4u * ( x + y * tex_size ), symbol_color, sizeof(unsigned char) * 4u );

	for( size_t x= tex_size * 15u / 32u; x < tex_size * 17u / 32u; ++x )
	for( size_t y= tex_size * 12u / 16u; y < tex_size * 14u / 16u; ++y )
		std::memcpy( tex_data.data() + 4u * ( x + y * tex_size ), symbol_color, sizeof(unsigned char) * 4u );

	// Dot
	for( size_t x= tex_size * 7u / 16u; x < tex_size * 9u / 16u; ++x )
	for( size_t y= tex_size * 7u / 16u; y < tex_size * 9u / 16u; ++y )
		std::memcpy( tex_data.data() + 4u * ( x + y * tex_size ), symbol_color, sizeof(unsigned char) * 4u );

	return CreateTexture( tex_size, tex_size, tex_data );
}

r_Texture GenTexture_GPSButtonActive( const size_t tex_size )
{
	return GenTexture_GPSButton_impl( tex_size, TexConstants::active_gps_symbol_color );
}

r_Texture GenTexture_GPSButtonUnactive( const size_t tex_size )
{
	return GenTexture_GPSButton_impl( tex_size, TexConstants::plus_color );
}

} // namespace PanzerMaps
