#pragma once
#include "coordinates_transformation_pass.hpp"

namespace PanzerMaps
{

using PolygonsNormalizationPassResult= CoordinatesTransformationPassResult;

// Split polygons into convex parts, make all polygons clockwise, etc.
// TODO - fix also self-intersecting polygons.
PolygonsNormalizationPassResult NormalizePolygons( const PolygonsNormalizationPassResult& in_data );

} // namespace PanzerMaps
