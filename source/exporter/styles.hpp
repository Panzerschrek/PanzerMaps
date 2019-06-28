#pragma once
#include <unordered_map>
#include <unordered_set>
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

	struct ArealObjectPhase
	{
		std::unordered_set<ArealObjectClass> classes;
	};

	ColorRGBA background_color= {0};

	std::unordered_map<PointObjectClass, PointObjectStyle> point_object_styles;
	std::unordered_map<LinearObjectClass, LinearObjectStyle> linear_object_styles;
	std::unordered_map<ArealObjectClass, ArealObjectStyle> areal_object_styles;

	std::vector<ArealObjectPhase> areal_object_phases;

	std::vector<PointObjectClass> point_classes_ordered;
	std::vector<LinearObjectClass> linear_classes_ordered;
};

Styles LoadStyles( const char* const file_name );

} // namespace PanzerMaps
