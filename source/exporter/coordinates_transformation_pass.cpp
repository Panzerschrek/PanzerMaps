#include <limits>
#include "../common/assert.hpp"
#include "../common/log.hpp"
#include "coordinates_transformation_pass.hpp"

namespace PanzerMaps
{

CoordinatesTransformationPassResult TransformCoordinates(
	const OSMParseResult& prepared_data,
	const size_t additional_scale_log2,
	const int32_t simplification_distance_units )
{
	CoordinatesTransformationPassResult result;

	if( prepared_data.point_objects_vertices.empty() && prepared_data.linear_objects_vertices.empty() && prepared_data.areal_objects_vertices.empty() )
	{
		result.min_point.x= result.max_point.x= result.min_point.y= result.max_point.y= 0;
		result.start_point.x= result.start_point.y= 0;
		result.coordinates_scale= 1;
		return result;
	}

	result.min_point.x= std::numeric_limits<int32_t>::max();
	result.min_point.y= std::numeric_limits<int32_t>::max();
	result.max_point.x= std::numeric_limits<int32_t>::min();
	result.max_point.y= std::numeric_limits<int32_t>::min();

	// Convert geo points to projection, calculate bounding box.
	std::vector<MercatorPoint> point_objects_vetices_converted, linear_objects_vetices_converted, areal_objects_vetices_converted;

	point_objects_vetices_converted.reserve( prepared_data.point_objects_vertices.size() );
	for( const GeoPoint& geo_point : prepared_data.point_objects_vertices )
	{
		const MercatorPoint mercator_point= GeoPointToMercatorPoint(geo_point);
		result.min_point.x= std::min( result.min_point.x, mercator_point.x );
		result.min_point.y= std::min( result.min_point.y, mercator_point.y );
		result.max_point.x= std::max( result.max_point.x, mercator_point.x );
		result.max_point.y= std::max( result.max_point.y, mercator_point.y );
		point_objects_vetices_converted.push_back( mercator_point );
	}

	linear_objects_vetices_converted.reserve( prepared_data.linear_objects_vertices.size() );
	for( const GeoPoint& geo_point : prepared_data.linear_objects_vertices )
	{
		const MercatorPoint mercator_point= GeoPointToMercatorPoint(geo_point);
		result.min_point.x= std::min( result.min_point.x, mercator_point.x );
		result.min_point.y= std::min( result.min_point.y, mercator_point.y );
		result.max_point.x= std::max( result.max_point.x, mercator_point.x );
		result.max_point.y= std::max( result.max_point.y, mercator_point.y );
		linear_objects_vetices_converted.push_back( mercator_point );
	}

	areal_objects_vetices_converted.reserve( prepared_data.areal_objects_vertices.size() );
	for( const GeoPoint& geo_point : prepared_data.areal_objects_vertices )
	{
		const MercatorPoint mercator_point= GeoPointToMercatorPoint(geo_point);
		result.min_point.x= std::min( result.min_point.x, mercator_point.x );
		result.min_point.y= std::min( result.min_point.y, mercator_point.y );
		result.max_point.x= std::max( result.max_point.x, mercator_point.x );
		result.max_point.y= std::max( result.max_point.y, mercator_point.y );
		areal_objects_vetices_converted.push_back( mercator_point );
	}

	// Calculate unit scale.
	// For closest to equator point we must have accuracy, near to expected.
	const double c_required_accuracy_m= 0.2; // 20 cm
	const int32_t min_abs_y= std::min( std::abs( result.max_point.y ), std::abs( result.min_point.y ) );
	const double max_latitude_scale= std::cos( MercatorPointToGeoPoint( MercatorPoint{ 0, min_abs_y } ).y * Constants::deg_to_rad );
	const double scale_calculated= Constants::two_pow_32 / ( Constants::earth_equator_length_m / c_required_accuracy_m  * max_latitude_scale );
	result.coordinates_scale= std::max( 1, static_cast<int32_t>(scale_calculated) );
	result.coordinates_scale <<= int(additional_scale_log2);
	result.zoom_level= additional_scale_log2;

	const double average_latitude_scale= std::cos( MercatorPointToGeoPoint( MercatorPoint{ 0, result.min_point.y / 2 + result.max_point.y / 2 } ).y * Constants::deg_to_rad );
	result.meters_in_unit= static_cast<float>( double(result.coordinates_scale) * Constants::earth_equator_length_m * average_latitude_scale / Constants::two_pow_32 );

	result.start_point= result.min_point;

	const auto convert_point=
	[&result]( MercatorPoint point ) -> MercatorPoint
	{
		return MercatorPoint{ ( point.x - result.start_point.x ) / result.coordinates_scale, ( point.y - result.start_point.y ) / result.coordinates_scale };
	};

	const int32_t simplification_suqare_distance= simplification_distance_units * simplification_distance_units;
	const auto points_near=
	[&]( const MercatorPoint& p0, const MercatorPoint& p1 ) -> bool
	{
		const int32_t dx= p1.x - p0.x;
		const int32_t dy= p1.y - p0.y;
		return dx * dx + dy * dy <= simplification_suqare_distance;
	};

	result.point_objects.reserve( prepared_data.point_objects.size() );
	result.linear_objects.reserve( prepared_data.linear_objects.size() );
	result.areal_objects.reserve( prepared_data.areal_objects.size() );

	result.point_objects= prepared_data.point_objects;
	result.point_objects_vertices= std::move( point_objects_vetices_converted );

	// Remove equal adjusted vertices of linear objects. Remove too short line objects.
	for( const BaseDataRepresentation::LinearObject& in_object : prepared_data.linear_objects )
	{
		BaseDataRepresentation::LinearObject out_object;
		out_object.class_= in_object.class_;
		out_object.z_level= in_object.z_level;
		out_object.first_vertex_index= result.linear_objects_vertices.size();
		out_object.vertex_count= 1u;
		result.linear_objects_vertices.push_back( convert_point( linear_objects_vetices_converted[ in_object.first_vertex_index ] ) );

		for( size_t v= in_object.first_vertex_index + 1u; v < in_object.first_vertex_index + in_object.vertex_count; ++v )
		{
			const CoordinatesTransformationPassResult::VertexTranspormed vertex_transformed=
				convert_point( linear_objects_vetices_converted[v] );
			if( !points_near( vertex_transformed, result.linear_objects_vertices.back() ) )
			{
				result.linear_objects_vertices.push_back( vertex_transformed );
				++out_object.vertex_count;
			}
			else if( vertex_transformed != result.linear_objects_vertices.back() && v == in_object.first_vertex_index + in_object.vertex_count - 1u )
			{
				result.linear_objects_vertices.push_back( vertex_transformed );
				++out_object.vertex_count;
			}
		}

		result.linear_objects.push_back( out_object );
	} // For linear objects.

	// Remove equal adjusted vertices of areal objects. Remove too small areal objects.
	for( const BaseDataRepresentation::ArealObject& in_object : prepared_data.areal_objects )
	{
		const auto transform_polygon=
		[&]( const size_t in_first_vertex, const size_t in_vertex_count, size_t& out_first_vertex, size_t& out_vertex_count )
		{
			out_first_vertex= result.areal_objects_vertices.size();
			out_vertex_count= 1u;

			result.areal_objects_vertices.push_back( convert_point( areal_objects_vetices_converted[in_first_vertex] ) );

			for( size_t v= in_first_vertex + 1u; v < in_first_vertex + in_vertex_count; ++v )
			{
				const auto vertex_transformed= convert_point( areal_objects_vetices_converted[v] );
				if( !points_near( vertex_transformed, result.areal_objects_vertices.back() ) )
				{
					result.areal_objects_vertices.push_back( vertex_transformed );
					++out_vertex_count;
				}
			}

			if( out_vertex_count >= 3u &&
				points_near( result.areal_objects_vertices[ out_first_vertex ], result.areal_objects_vertices[ out_first_vertex + out_vertex_count - 1u ] ) )
			{
				result.areal_objects_vertices.pop_back(); // Remove duplicated start and end vertex.
				--out_vertex_count;
			}
			if( out_vertex_count < 3u )
			{
				result.areal_objects_vertices.resize( result.areal_objects_vertices.size() - out_vertex_count ); // Polygon is too small.
				out_vertex_count= 0u;
			}
		};

		if( in_object.multipolygon != nullptr )
		{
			BaseDataRepresentation::Multipolygon out_multipolygon;
			for( const BaseDataRepresentation::Multipolygon::Part& inner_ring : in_object.multipolygon->inner_rings )
			{
				BaseDataRepresentation::Multipolygon::Part out_part;
				transform_polygon( inner_ring.first_vertex_index, inner_ring.vertex_count, out_part.first_vertex_index, out_part.vertex_count );
				if( out_part.vertex_count > 0u )
					out_multipolygon.inner_rings.push_back(out_part);
			}
			for( const BaseDataRepresentation::Multipolygon::Part& outer_ring : in_object.multipolygon->outer_rings )
			{
				BaseDataRepresentation::Multipolygon::Part out_part;
				transform_polygon( outer_ring.first_vertex_index, outer_ring.vertex_count, out_part.first_vertex_index, out_part.vertex_count );
				if( out_part.vertex_count > 0u )
					out_multipolygon.outer_rings.push_back(out_part);
			}

			if( !out_multipolygon.outer_rings.empty() )
			{
				BaseDataRepresentation::ArealObject out_object;
				out_object.class_= in_object.class_;
				out_object.z_level= in_object.z_level;
				out_object.first_vertex_index= out_object.vertex_count= 0u;
				out_object.multipolygon.reset( new BaseDataRepresentation::Multipolygon( std::move(out_multipolygon) ) );
				result.areal_objects.push_back( std::move(out_object) );
			}
		}
		else
		{
			BaseDataRepresentation::ArealObject out_object;
			out_object.class_= in_object.class_;
			out_object.z_level= in_object.z_level;

			transform_polygon( in_object.first_vertex_index, in_object.vertex_count, out_object.first_vertex_index, out_object.vertex_count );
			if( out_object.vertex_count > 0u )
				result.areal_objects.push_back( std::move(out_object) );
		}
	} // for areal objects

	PM_ASSERT( result.point_objects.size() == result.point_objects_vertices.size() );

	Log::Info( "Coordinates transformation pass: " );
	Log::Info( "Unit size: ", result.coordinates_scale );
	Log::Info( "Simplification distance: ", result.coordinates_scale * simplification_distance_units );
	Log::Info( result.point_objects.size(), " point objects" );
	Log::Info( result.linear_objects.size(), " linear objects" );
	Log::Info( result.linear_objects_vertices.size(), " linear objects vertices" );
	Log::Info( result.areal_objects.size(), " areal objects" );
	Log::Info( result.areal_objects_vertices.size(), " areal objects vertices" );
	Log::Info( "" );

	return result;
}

} // namespace PanzerMaps
