#pragma once
#include "coordinates_transformation_pass.hpp"

namespace PanzerMaps
{

// Simplify lines and areal objects.
void SimplificationPass( ObjectsData& in_data, int32_t simplification_distance_units );

} // namespace PanzerMaps
