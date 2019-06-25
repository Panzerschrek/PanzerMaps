#pragma once
#include "coordinates_transformation_pass.hpp"
#include "styles.hpp"

namespace PanzerMaps
{

void CreateDataFile( const CoordinatesTransformationPassResult& prepared_data, const Styles& styles, const char* const file_name );

} // namespace PanzerMaps
