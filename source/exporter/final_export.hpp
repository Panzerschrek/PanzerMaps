#pragma once
#include "polygons_normalization_pass.hpp"
#include "styles.hpp"

namespace PanzerMaps
{

void CreateDataFile( const PolygonsNormalizationPassResult& prepared_data, const Styles& styles, const char* const file_name );

} // namespace PanzerMaps
