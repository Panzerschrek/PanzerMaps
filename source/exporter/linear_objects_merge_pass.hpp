#pragma once
#include "coordinates_transformation_pass.hpp"

namespace PanzerMaps
{

// Merge linear objects with same start/finsih points and same class and z_level.
// Objects order not preserved.
void MergeLinearObjects( ObjectsData& data );

} // namespace PanzerMaps
