#pragma once
#include "polygons_normalization_pass.hpp"
#include "styles.hpp"

namespace PanzerMaps
{

void CreateDataFile(
	const std::vector<PolygonsNormalizationPassResult>& prepared_data,
	const Styles& styles,
	const ImageRGBA& copyright_image,
	const char* const file_name );

} // namespace PanzerMaps
