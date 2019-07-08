#pragma once
#include "../panzer_ogl_lib/texture.hpp"

namespace PanzerMaps
{

r_Texture GenTexture_ZoomPlus( size_t tex_size );
r_Texture GenTexture_ZoomMinus( const size_t tex_size );

r_Texture GenTexture_ZoomPlusPressed( size_t tex_size );
r_Texture GenTexture_ZoomMinusPressed( const size_t tex_size );

} // namespace PanzerMaps
