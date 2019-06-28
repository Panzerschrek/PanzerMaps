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

	using PointObjectStyles= std::unordered_map<PointObjectClass, PointObjectStyle>;
	using LinearObjectStyles= std::unordered_map<LinearObjectClass, LinearObjectStyle>;
	using ArealObjectStyles= std::unordered_map<ArealObjectClass, ArealObjectStyle>;

	struct ZoomLevel
	{
		size_t scale_to_prev_log2= 1u; // For first zoom level - initial scale.
		std::vector<ArealObjectPhase> areal_object_phases;
		std::vector<PointObjectClass> point_classes_ordered;
		std::vector<LinearObjectClass> linear_classes_ordered;

		PointObjectStyles point_object_styles_override;
		LinearObjectStyles linear_object_styles_override;
		ArealObjectStyles areal_object_styles_override;
	};

	ColorRGBA background_color= {0};

	PointObjectStyles point_object_styles;
	LinearObjectStyles linear_object_styles;
	ArealObjectStyles areal_object_styles;
	std::vector<ZoomLevel> zoom_levels;
};

Styles LoadStyles( const char* const file_name );

} // namespace PanzerMaps
