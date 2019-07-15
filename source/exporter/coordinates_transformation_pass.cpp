#include <limits>
#include "../common/assert.hpp"
#include "../common/log.hpp"
#include "coordinates_transformation_pass.hpp"

namespace PanzerMaps
{

ObjectsData TransformCoordinates(
	const OSMParseResult& prepared_data,
	const size_t additional_scale_log2 )
{
	ObjectsData result;

	if( prepared_data.point_objects_vertices.empty() && prepared_data.linear_objects_vertices.empty() && prepared_data.areal_objects_vertices.empty() )
	{
		result.min_point.x= result.max_point.x= result.min_point.y= result.max_point.y= 0;
		result.coordinates_scale= 1;
		return result;
	}

	// Calculate bounding box.
	result.projection_min_point= GeoPoint{ 90.0f, 180.0f };
	result.projection_max_point= GeoPoint{ -90.0f, -180.0f };
	for( const GeoPoint& geo_point : prepared_data.point_objects_vertices )
	{
		result.projection_min_point.x= std::min( result.projection_min_point.x, geo_point.x );
		result.projection_min_point.y= std::min( result.projection_min_point.y, geo_point.y );
		result.projection_max_point.x= std::max( result.projection_max_point.x, geo_point.x );
		result.projection_max_point.y= std::max( result.projection_max_point.y, geo_point.y );
	}
	for( const GeoPoint& geo_point : prepared_data.linear_objects_vertices )
	{
		result.projection_min_point.x= std::min( result.projection_min_point.x, geo_point.x );
		result.projection_min_point.y= std::min( result.projection_min_point.y, geo_point.y );
		result.projection_max_point.x= std::max( result.projection_max_point.x, geo_point.x );
		result.projection_max_point.y= std::max( result.projection_max_point.y, geo_point.y );
	}
	for( const GeoPoint& geo_point : prepared_data.areal_objects_vertices )
	{
		result.projection_min_point.x= std::min( result.projection_min_point.x, geo_point.x );
		result.projection_min_point.y= std::min( result.projection_min_point.y, geo_point.y );
		result.projection_max_point.x= std::max( result.projection_max_point.x, geo_point.x );
		result.projection_max_point.y= std::max( result.projection_max_point.y, geo_point.y );
	}

	// Select projection.
	IProjectionPtr base_projection;
	{
		const double min_abs_lat= std::min( std::abs(result.projection_min_point.y), std::abs(result.projection_max_point.y ) );
		const double max_abs_lat= std::max( std::abs(result.projection_min_point.y), std::abs(result.projection_max_point.y ) );
		const double min_lat_cos= std::cos( min_abs_lat * Constants::deg_to_rad );
		const double max_lat_cos= std::cos( max_abs_lat * Constants::deg_to_rad );

		const double c_max_lat_cos_ratio= 1.05;
		const double max_mercator_projection_latitude= 85.0;
		const double c_max_delta_lat_for_stereographic_projection= 20.0;

		if( min_lat_cos / max_lat_cos <= c_max_lat_cos_ratio && max_abs_lat < max_mercator_projection_latitude )
		{
			// Select Mercator projection for regions, where lengths distortion will be unsignificant.
			result.projection= DataFileDescription::DataFile::Projection::Mercator;
			base_projection.reset( new MercatorProjection );
			Log::Info( "Select projection: Mercator" );
		}
		else
		{
			// Use Albers projection for regions with big latitude delta, but disable it for regions in both north/south hemispheres.
			if( result.projection_max_point.y - result.projection_min_point.y > c_max_delta_lat_for_stereographic_projection &&
				result.projection_min_point.y * result.projection_max_point.y > 0.0 )
			{
				result.projection= DataFileDescription::DataFile::Projection::Albers;
				base_projection.reset( new AlbersProjection( result.projection_min_point, result.projection_max_point ) );
				Log::Info( "Select projection: Albers" );
			}
			else
			{
				result.projection= DataFileDescription::DataFile::Projection::Stereographic;
				base_projection.reset( new StereographicProjection( result.projection_min_point, result.projection_max_point ) );
				Log::Info( "Select projection: Stereographic" );
			}

		}
	}
	{ // Calculate unit size.
		const double c_try_meters= 1000.0;
		const double c_required_accuracy_m= 0.2; // 20 cm

		const GeoPoint middle_point{ ( result.projection_min_point.x + result.projection_max_point.x ) * 0.5, ( result.projection_min_point.y + result.projection_max_point.y ) * 0.5 };
		const GeoPoint try_point_0{ middle_point.x, middle_point.y - 0.5 * c_try_meters * ( 360.0 / Constants::earth_equator_length_m ) };
		const GeoPoint try_point_1{ middle_point.x, middle_point.y + 0.5 * c_try_meters * ( 360.0 / Constants::earth_equator_length_m ) };
		const int32_t y0= base_projection->Project( try_point_0 ).y;
		const int32_t y1= base_projection->Project( try_point_1 ).y;

		const double meters_un_unit_initial= c_try_meters / double( y1 - y0 );
		result.coordinates_scale= std::max( 1, static_cast<int>( c_required_accuracy_m / meters_un_unit_initial ) );
		result.coordinates_scale<<= additional_scale_log2;
		result.meters_in_unit= static_cast<float>( meters_un_unit_initial * double(result.coordinates_scale) );
	}

	const LinearProjectionTransformation projection( std::move(base_projection), result.projection_min_point, result.projection_max_point, result.coordinates_scale );
	result.min_point= projection.GetMinPoint();
	result.max_point= projection.GetMaxPoint();
	result.zoom_level= additional_scale_log2;

	// Start transformation.

	result.point_objects.reserve( prepared_data.point_objects.size() );
	result.linear_objects.reserve( prepared_data.linear_objects.size() );
	result.areal_objects.reserve( prepared_data.areal_objects.size() );

	result.point_objects= prepared_data.point_objects;
	for( const GeoPoint& point_vertex : prepared_data.point_objects_vertices )
		result.point_objects_vertices.push_back( projection.Project( point_vertex ) );

	// Remove equal adjusted vertices of linear objects.
	for( const BaseDataRepresentation::LinearObject& in_object : prepared_data.linear_objects )
	{
		BaseDataRepresentation::LinearObject out_object;
		out_object.class_= in_object.class_;
		out_object.z_level= in_object.z_level;
		out_object.first_vertex_index= result.linear_objects_vertices.size();
		out_object.vertex_count= 1u;
		result.linear_objects_vertices.push_back( projection.Project( prepared_data.linear_objects_vertices[ in_object.first_vertex_index ] ) );

		for( size_t v= in_object.first_vertex_index + 1u; v < in_object.first_vertex_index + in_object.vertex_count; ++v )
		{
			const ObjectsData::VertexTransformed vertex_transformed= projection.Project( prepared_data.linear_objects_vertices[v] );
			if( vertex_transformed != result.linear_objects_vertices.back() )
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

			result.areal_objects_vertices.push_back( projection.Project( prepared_data.areal_objects_vertices[in_first_vertex] ) );

			for( size_t v= in_first_vertex + 1u; v < in_first_vertex + in_vertex_count; ++v )
			{
				const auto vertex_transformed= projection.Project( prepared_data.areal_objects_vertices[v] );
				if( vertex_transformed != result.areal_objects_vertices.back() )
				{
					result.areal_objects_vertices.push_back( vertex_transformed );
					++out_vertex_count;
				}
			}

			if( out_vertex_count >= 3u &&
				result.areal_objects_vertices[ out_first_vertex ] == result.areal_objects_vertices[ out_first_vertex + out_vertex_count - 1u ] )
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
	Log::Info( result.point_objects.size(), " point objects" );
	Log::Info( result.linear_objects.size(), " linear objects" );
	Log::Info( result.linear_objects_vertices.size(), " linear objects vertices" );
	Log::Info( result.areal_objects.size(), " areal objects" );
	Log::Info( result.areal_objects_vertices.size(), " areal objects vertices" );
	Log::Info( "" );

	return result;
}

} // namespace PanzerMaps
