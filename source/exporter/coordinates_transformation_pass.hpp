#pragma once
#include "../common/coordinates_conversion.hpp"
#include "primary_export.hpp"

namespace PanzerMaps
{

struct CoordinatesTransformationPassResult : BaseDataRepresentation
{
	MercatorPoint min_point;
	MercatorPoint max_point;

	MercatorPoint start_point;
	int coordinates_scale;

	// Scaled and shifted.
	using VertexTranspormed= MercatorPoint;

	// transformed_vertex= ( GeoToProjection(vertex) - start_point ) / coordinates_scale
	std::vector<VertexTranspormed> vertices;
};

CoordinatesTransformationPassResult TransformCoordinates( const OSMParseResult& prepared_data );

} // namespace PanzerMaps
