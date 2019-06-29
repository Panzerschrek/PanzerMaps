#pragma once
#include "coordinates_transformation_pass.hpp"
#include "styles.hpp"

namespace PanzerMaps
{

// Point and linear objects ordered by class in specified in styles order.
// Areal objects in output sorted by phase, inside phase it's sorted by area in descent order.
using PhaseSortResult= CoordinatesTransformationPassResult;

PhaseSortResult SortByPhase( const CoordinatesTransformationPassResult& in_data, const Styles::ZoomLevel& zoom_level );

} // namespace PanzerMaps