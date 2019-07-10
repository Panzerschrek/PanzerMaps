#pragma once
#include "../common/coordinates_conversion.hpp"
#include "primary_export.hpp"

namespace PanzerMaps
{

struct ObjectsData : BaseDataRepresentation
{
	MercatorPoint min_point;
	MercatorPoint max_point;

	MercatorPoint start_point;
	 // Scale to Mercator unit.
	int coordinates_scale;

	// logarithm of zoom level of this pass.
	size_t zoom_level;

	float meters_in_unit; // Meters in unit for center.

	// Scaled and shifted.
	using VertexTranspormed= MercatorPoint;

	// transformed_vertex= ( GeoToProjection(vertex) - start_point ) / coordinates_scale
	std::vector<VertexTranspormed> point_objects_vertices;
	std::vector<VertexTranspormed> linear_objects_vertices;
	std::vector<VertexTranspormed> areal_objects_vertices;
};

ObjectsData TransformCoordinates(
	const OSMParseResult& prepared_data,
	size_t additional_scale_log2,
	int32_t simplification_distance_units );

} // namespace PanzerMaps
