#pragma once
#include <unordered_map>
#include <vector>
#include "object_classes.hpp"

namespace PanzerMaps
{

struct Styles
{
	using ColorRGBA= unsigned char[4];

	struct PointObjectStyle
	{
	};

	struct LinearObjectStyle
	{
		ColorRGBA color= {0};
		float width_m= 0.0f;
	};

	struct ArealObjectStyle
	{
		ColorRGBA color= {0};
	};

	ColorRGBA background_color= {0};

	std::unordered_map<PointObjectClass, PointObjectStyle> point_object_styles;
	std::unordered_map<LinearObjectClass, LinearObjectStyle> linear_object_styles;
	std::unordered_map<ArealObjectClass, ArealObjectStyle> areal_object_styles;
};

Styles LoadStyles( const char* const file_name );

} // namespace PanzerMaps
