#pragma once
#include <cstddef>
#include "../common/coordinates_conversion.hpp"

namespace PanzerMaps
{

// Returns positive value for clockwise polygon, negative - for anticlockwise.
int64_t CalculatePolygonDoubleSignedArea( const ProjectionPoint* const vertices, size_t vertex_count );

} // namespace PanzerMaps
