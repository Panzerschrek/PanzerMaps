#pragma once
#include "phase_sort_pass.hpp"

namespace PanzerMaps
{


// Split polygons into convex parts, make all polygons clockwise, etc.
// TODO - fix also self-intersecting polygons.
void NormalizePolygons( ObjectsData& in_data );

} // namespace PanzerMaps
