#pragma once
#include "coordinates_transformation_pass.hpp"

namespace PanzerMaps
{

// Split polygons into convex parts, make all polygons clockwise, etc.
// TODO - fix also self-intersecting polygons.
void NormalizePolygons( ObjectsData& data );

} // namespace PanzerMaps
