#include "coordinates_transformation_pass.hpp"

namespace PanzerMaps
{

CoordinatesTransformationPassResult TransformCoordinates( const OSMParseResult& prepared_data )
{
	CoordinatesTransformationPassResult result;

	if( prepared_data.vertices.empty() )
	{
		result.min_point.x= result.max_point.x= result.min_point.y= result.max_point.y= 0;
		result.start_point.x= result.start_point.y= 0;
		result.coordinates_scale= 1;
		return result;
	}

	result.min_point= result.max_point= GeoPointToWebMercatorPoint( prepared_data.vertices.front() );

	// Convert geo points to projection, calculate bounding box.
	std::vector<MercatorPoint> src_vetices_converted;
	src_vetices_converted.reserve( prepared_data.vertices.size() );
	for( const GeoPoint& geo_point : prepared_data.vertices )
	{
		const MercatorPoint mercator_point= GeoPointToWebMercatorPoint(geo_point);
		result.min_point.x= std::min( result.min_point.x, mercator_point.x );
		result.min_point.y= std::min( result.min_point.y, mercator_point.y );
		result.max_point.x= std::max( result.max_point.x, mercator_point.x );
		result.max_point.y= std::max( result.max_point.y, mercator_point.y );
		src_vetices_converted.push_back( mercator_point );
	}

	result.coordinates_scale= 20; // TODO - make data-dependent
	result.start_point.x= result.min_point.x / result.coordinates_scale * result.coordinates_scale;
	result.start_point.y= result.min_point.y / result.coordinates_scale * result.coordinates_scale;

	const auto convert_point=
	[&result]( MercatorPoint point ) -> MercatorPoint
	{
		return MercatorPoint{ ( point.x - result.start_point.x ) / result.coordinates_scale, ( point.y - result.start_point.y ) / result.coordinates_scale };
	};

	result.point_objects.reserve( prepared_data.point_objects.size() );
	result.linear_objects.reserve( prepared_data.linear_objects.size() );
	result.areal_objects.reserve( prepared_data.areal_objects.size() );

	for( const BaseDataRepresentation::PointObject& in_object : prepared_data.point_objects )
	{
		BaseDataRepresentation::PointObject out_object;
		out_object.class_= in_object.class_;
		out_object.vertex_index= result.vertices.size();
		result.vertices.push_back( convert_point( src_vetices_converted[ in_object.vertex_index ] ) );
		result.point_objects.push_back( out_object );
	} // For point objects.

	// Remove equal adjusted vertices of linear objects. Remove too short line objects.
	for( const BaseDataRepresentation::LinearObject& in_object : prepared_data.linear_objects )
	{
		BaseDataRepresentation::LinearObject out_object;
		out_object.class_= in_object.class_;
		out_object.first_vertex_index= result.vertices.size();
		out_object.vertex_count= 1u;
		result.vertices.push_back( convert_point( src_vetices_converted[ in_object.first_vertex_index ] ) );

		for( size_t v= in_object.first_vertex_index + 1u; v < in_object.first_vertex_index + in_object.vertex_count; ++v )
		{
			const CoordinatesTransformationPassResult::VertexTranspormed vertex_transformed=
				convert_point( src_vetices_converted[v] );
			if( vertex_transformed != result.vertices.back() )
			{
				result.vertices.push_back( vertex_transformed );
				++out_object.vertex_count;
			}
		}

		if( out_object.vertex_count < 2u )
		{
			result.vertices.pop_back(); // Line too small, do not transform it.
			continue;
		}

		result.linear_objects.push_back( out_object );
	} // For linear objects.

	// Remove equal adjusted vertices of areal objects. Remove too small areal objects.
	for( const BaseDataRepresentation::ArealObject& in_object : prepared_data.areal_objects )
	{
		BaseDataRepresentation::ArealObject out_object;
		out_object.class_= in_object.class_;
		out_object.first_vertex_index= result.vertices.size();
		out_object.vertex_count= 1u;
		result.vertices.push_back( convert_point( src_vetices_converted[ in_object.first_vertex_index ] ) );

		for( size_t v= in_object.first_vertex_index + 1u; v < in_object.first_vertex_index + in_object.vertex_count; ++v )
		{
			const CoordinatesTransformationPassResult::VertexTranspormed vertex_transformed=
				convert_point( src_vetices_converted[v] );
			if( vertex_transformed != result.vertices.back() )
			{
				result.vertices.push_back( vertex_transformed );
				++out_object.vertex_count;
			}
		}

		if( out_object.vertex_count >= 3u &&
			result.vertices[ out_object.first_vertex_index ] == result.vertices[ out_object.first_vertex_index + out_object.vertex_count - 1u ] )
		{
			result.vertices.pop_back(); // Remove duplicated start and end vertex.
			--out_object.vertex_count;
		}

		if( out_object.vertex_count < 3u )
		{
			result.vertices.resize( result.vertices.size() - out_object.vertex_count ); // Polygon is too small.
			continue;
		}

		result.areal_objects.push_back( out_object );
	}

	return result;
}

} // namespace PanzerMaps