#pragma once
#include "system_window.hpp"

namespace PanzerMaps
{

namespace UiConstants
{

static int border_size= 4;

}

inline int GetButtonSize( const ViewportSize& viewport_size )
{
	return int( std::min( viewport_size.width, viewport_size.height ) ) / 6;
}

inline int GetNorthArrowSize( const ViewportSize& viewport_size )
{
	return int( std::min( viewport_size.width, viewport_size.height ) ) / 10;
}

inline int GetButtonsTop( const ViewportSize& viewport_size )
{
	return int(viewport_size.height) * 5 / 8;
}

} // namespace PanzerMaps
