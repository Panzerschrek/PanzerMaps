#pragma once
#include "phase_sort_pass.hpp"

namespace PanzerMaps
{

using PolygonsNormalizationPassResult= PhaseSortResult;

// Split polygons into convex parts, make all polygons clockwise, etc.
// TODO - fix also self-intersecting polygons.
PolygonsNormalizationPassResult NormalizePolygons( const PhaseSortResult& in_data );

} // namespace PanzerMaps
