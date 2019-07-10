#pragma once
#include <memory>
#include <vector>

#include "../common/coordinates_conversion.hpp"
#include "object_classes.hpp"

namespace PanzerMaps
{

const size_t g_zero_z_level= 5u;
const size_t g_max_z_level= 10u;

struct BaseDataRepresentation
{
	struct PointObject
	{
		PointObjectClass class_= PointObjectClass::None;
	};

	struct LinearObject
	{
		LinearObjectClass class_= LinearObjectClass::None;
		size_t first_vertex_index;
		size_t vertex_count; // 1 or more.
		size_t z_level= g_zero_z_level;
	};

	struct Multipolygon
	{
		struct Part
		{
			size_t first_vertex_index;
			size_t vertex_count;
		};

		std::vector<Part> outer_rings;
		std::vector<Part> inner_rings;
	};

	struct ArealObject
	{
		ArealObjectClass class_= ArealObjectClass::None;
		size_t first_vertex_index;
		size_t vertex_count;
		size_t z_level= g_zero_z_level;

		std::unique_ptr<Multipolygon> multipolygon;
	};

	std::vector<PointObject> point_objects;
	std::vector<LinearObject> linear_objects;
	std::vector<ArealObject> areal_objects;
};

struct OSMParseResult : public BaseDataRepresentation
{
	std::vector<GeoPoint> point_objects_vertices;
	std::vector<GeoPoint> linear_objects_vertices;
	std::vector<GeoPoint> areal_objects_vertices;
};

OSMParseResult ParseOSM( const char* file_name );

} // namespace PanzerMaps
