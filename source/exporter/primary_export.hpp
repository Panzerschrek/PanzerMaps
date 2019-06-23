#pragma once
#include <vector>

#include "../common/coordinates_conversion.hpp"
#include "object_classes.hpp"

namespace PanzerMaps
{

struct OSMParseResult
{
	struct PointObject
	{
		PointObjectClass class_= PointObjectClass::None;
		size_t vertex_index;
	};

	struct LinearObject
	{
		LinearObjectClass class_= LinearObjectClass::None;
		size_t first_vertex_index;
		size_t vertex_count;
	};

	struct ArealObject
	{
		ArealObjectClass class_= ArealObjectClass::None;
		size_t first_vertex_index;
		size_t vertex_count;
	};

	std::vector<PointObject> point_objects;
	std::vector<LinearObject> linear_objects;
	std::vector<ArealObject> areal_objects;
	std::vector<GeoPoint> vertices;
};

OSMParseResult ParseOSM( const char* file_name );

} // namespace PanzerMaps
