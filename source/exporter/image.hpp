#pragma once
#include <string>
#include <vector>

namespace PanzerMaps
{

struct ImageRGBA
{
	size_t size[2u] = { 0u, 0u };
	std::vector< unsigned char > data;
};

// Currently, supports only 32bit rgba PNG.
ImageRGBA LoadImage( const std::string& image_file_name );

} // namespace PanzerMaps
