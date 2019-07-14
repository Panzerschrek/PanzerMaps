#pragma once
#include "../common/coordinates_conversion.hpp"
#include  "../common/data_file.hpp"
#include "primary_export.hpp"

namespace PanzerMaps
{

struct ObjectsData : BaseDataRepresentation
{
	DataFileDescription::DataFile::Projection projection;
	GeoPoint projection_min_point;
	GeoPoint projection_max_point;
	ProjectionPoint min_point;
	ProjectionPoint max_point;

	 // Scale to Mercator unit.
	int coordinates_scale;

	// logarithm of zoom level of this pass.
	size_t zoom_level;

	float meters_in_unit; // Meters in unit for center.

	// Scaled and shifted.
	using VertexTransformed= ProjectionPoint;

	// transformed_vertex= ( GeoToProjection(vertex) - start_point ) / coordinates_scale
	std::vector<VertexTransformed> point_objects_vertices;
	std::vector<VertexTransformed> linear_objects_vertices;
	std::vector<VertexTransformed> areal_objects_vertices;
};

ObjectsData TransformCoordinates(
	const OSMParseResult& prepared_data,
	size_t additional_scale_log2 );

} // namespace PanzerMaps
