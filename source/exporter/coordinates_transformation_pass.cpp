#include <limits>
#include <numeric>
#include "../common/assert.hpp"
#include "../common/log.hpp"
#include "coordinates_transformation_pass.hpp"

namespace PanzerMaps
{

class IProjection
{
public:
	virtual ~IProjection()= default;
	virtual ProjectionPoint Project( const GeoPoint& geo_point ) const= 0;
};

using IProjectionPtr= std::unique_ptr<IProjection>;

class MercatorProjection final : public IProjection
{
public:
	virtual ProjectionPoint Project( const GeoPoint& geo_point ) const override
	{
		return GeoPointToMercatorPoint( geo_point );
	}
};

class AlbersProjection final : public IProjection
{
public:
	AlbersProjection( const GeoPoint& min_point, const GeoPoint& max_point )
	{
		const double latitude_diff_div_6= ( max_point.y - min_point.y ) / 6.0;
		const double zero_latitude= ( min_point.y + max_point.y ) * 0.5 * Constants::deg_to_rad;
		const double base_latitude1= min_point.y + latitude_diff_div_6;
		const double base_latitude2= max_point.y - latitude_diff_div_6;

		const double base_latitude1_sin= std::sin( base_latitude1 * Constants::deg_to_rad );
		const double base_latitude1_cos= std::cos( base_latitude1 * Constants::deg_to_rad );
		const double base_latitude2_sin= std::sin( base_latitude2 * Constants::deg_to_rad );

		zero_longitude_rad_= ( min_point.x + max_point.x ) * 0.5 * Constants::deg_to_rad;
		latitude_avg_sin_= ( base_latitude1_sin + base_latitude2_sin ) * 0.5;
		c_= base_latitude1_cos * base_latitude1_cos + 2.0 * latitude_avg_sin_ * base_latitude1_sin;
		p0_= std::sqrt( c_ - 2.0 * latitude_avg_sin_ * std::sin(zero_latitude) ) / latitude_avg_sin_;

		// Calculate scale factor.
		scale_factor_= double( 1 << 28 );
		ProjectionPoint special_points[10];
		special_points[0]= Project( GeoPoint{ -90.0, -90.0 } );
		special_points[1]= Project( GeoPoint{ +90.0, -90.0 } );
		special_points[2]= Project( GeoPoint{ -90.0, +90.0 } );
		special_points[3]= Project( GeoPoint{ +90.0, +90.0 } );
		special_points[4]= Project( GeoPoint{   0.0, -90.0 } );
		special_points[5]= Project( GeoPoint{   0.0, +90.0 } );
		special_points[6]= Project( GeoPoint{ -90.0 / latitude_avg_sin_, -90.0 } );
		special_points[7]= Project( GeoPoint{ +90.0 / latitude_avg_sin_, -90.0 } );
		special_points[8]= Project( GeoPoint{ -90.0 / latitude_avg_sin_, +90.0 } );
		special_points[9]= Project( GeoPoint{ +90.0 / latitude_avg_sin_, +90.0 } );

		ProjectionPoint min_test_point= special_points[0], max_test_point= special_points[0];
		for( const ProjectionPoint& point : special_points )
		{
			min_test_point.x= std::min( min_test_point.x, point.x );
			min_test_point.y= std::min( min_test_point.y, point.y );
			max_test_point.x= std::max( max_test_point.x, point.x );
			max_test_point.y= std::max( max_test_point.y, point.y );
		}
		const int32_t max_abs_x= std::max( std::abs(min_test_point.x), std::abs(max_test_point.x) );
		const int32_t max_abs_y= std::max( std::abs(min_test_point.y), std::abs(max_test_point.y) );
		const int32_t max_coordinate= std::max( max_abs_x, max_abs_y );
		scale_factor_*= Constants::two_pow_31 / double(max_coordinate);
	}

	virtual ProjectionPoint Project( const GeoPoint& geo_point ) const override
	{
		const double longitude_rad= geo_point.x * Constants::deg_to_rad;
		const double latitude_rad = geo_point.y * Constants::deg_to_rad;

		const double longitude_scaled_diff= latitude_avg_sin_ * ( longitude_rad - zero_longitude_rad_ );
		const double p= std::sqrt( c_ - 2.0 * latitude_avg_sin_ * std::sin(latitude_rad) ) / latitude_avg_sin_;

		ProjectionPoint result;
		result.x= int32_t( scale_factor_ * ( p * std::sin(longitude_scaled_diff) ) );
		result.y= int32_t( scale_factor_ * ( p0_ - p * std::cos(longitude_scaled_diff) ) );
		return result;
	}

private:
	double zero_longitude_rad_;
	double latitude_avg_sin_;
	double c_;
	double p0_;
	double scale_factor_;
	int32_t unit_scale_;
};

class LinearProjectionTransformation final : public IProjection
{
public:
	LinearProjectionTransformation( IProjectionPtr projection, const GeoPoint& min_point, const GeoPoint& max_point, const int32_t unit_size )
		: projection_(std::move(projection)), unit_size_(unit_size)
	{
		ProjectionPoint special_points[8];
		special_points[0]= projection_->Project( GeoPoint{ min_point.x, min_point.y } );
		special_points[1]= projection_->Project( GeoPoint{ max_point.x, min_point.y } );
		special_points[2]= projection_->Project( GeoPoint{ min_point.x, max_point.y } );
		special_points[3]= projection_->Project( GeoPoint{ max_point.x, max_point.y } );
		special_points[4]= projection_->Project( GeoPoint{ ( min_point.x + max_point.x ) * 0.5, max_point.y } );
		special_points[5]= projection_->Project( GeoPoint{ ( min_point.x + max_point.x ) * 0.5, max_point.y } );
		special_points[6]= projection_->Project( GeoPoint{ min_point.x, ( min_point.y + max_point.y ) * 0.5 } );
		special_points[7]= projection_->Project( GeoPoint{ max_point.x, ( min_point.y + max_point.y ) * 0.5 } );

		min_point_= max_point_= special_points[0];
		for( const ProjectionPoint& point : special_points )
		{
			min_point_.x= std::min( min_point_.x, point.x );
			min_point_.y= std::min( min_point_.y, point.y );
			max_point_.x= std::max( max_point_.x, point.x );
			max_point_.y= std::max( max_point_.y, point.y );
		}
	}

	virtual ProjectionPoint Project( const GeoPoint& geo_point ) const override
	{
		const ProjectionPoint p= projection_->Project( geo_point );
		return ProjectionPoint{ ( p.x - min_point_.x ) / unit_size_, ( p.y - min_point_.y ) / unit_size_ };
	}

	const ProjectionPoint GetMinPoint() const
	{
		return min_point_;
	}

	const ProjectionPoint GetMaxPoint() const
	{
		return max_point_;
	}

private:
	const IProjectionPtr projection_;
	ProjectionPoint min_point_;
	ProjectionPoint max_point_;
	int32_t unit_size_;
};

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
	GeoPoint geo_min{ 90.0f, 180.0f }, geo_max{ -90.0f, -180.0f };
	for( const GeoPoint& geo_point : prepared_data.point_objects_vertices )
	{
		geo_min.x= std::min( geo_min.x, geo_point.x );
		geo_min.y= std::min( geo_min.y, geo_point.y );
		geo_max.x= std::max( geo_max.x, geo_point.x );
		geo_max.y= std::max( geo_max.y, geo_point.y );
	}
	for( const GeoPoint& geo_point : prepared_data.linear_objects_vertices )
	{
		geo_min.x= std::min( geo_min.x, geo_point.x );
		geo_min.y= std::min( geo_min.y, geo_point.y );
		geo_max.x= std::max( geo_max.x, geo_point.x );
		geo_max.y= std::max( geo_max.y, geo_point.y );
	}
	for( const GeoPoint& geo_point : prepared_data.areal_objects_vertices )
	{
		geo_min.x= std::min( geo_min.x, geo_point.x );
		geo_min.y= std::min( geo_min.y, geo_point.y );
		geo_max.x= std::max( geo_max.x, geo_point.x );
		geo_max.y= std::max( geo_max.y, geo_point.y );
	}

	// Select projection.
	IProjectionPtr base_projection;
	const bool use_albers_projection= true;
	if( use_albers_projection )
	{
		base_projection.reset( new AlbersProjection( geo_min, geo_max ) );
		Log::Info( "Select projection: Albers" );
	}
	else
	{
		base_projection.reset( new MercatorProjection );
		Log::Info( "Select projection: Mercator" );
	}

	{ // Calculate unit size.
		const double c_try_meters= 1000.0;
		const double c_required_accuracy_m= 0.2; // 20 cm

		const GeoPoint middle_point{ ( geo_min.x + geo_max.x ) * 0.5, ( geo_min.y + geo_max.y ) * 0.5 };
		const GeoPoint try_point_0{ middle_point.x, middle_point.y - 0.5 * c_try_meters * ( 360.0 / Constants::earth_equator_length_m ) };
		const GeoPoint try_point_1{ middle_point.x, middle_point.y + 0.5 * c_try_meters * ( 360.0 / Constants::earth_equator_length_m ) };
		const int32_t y0= base_projection->Project( try_point_0 ).y;
		const int32_t y1= base_projection->Project( try_point_1 ).y;

		const double meters_un_unit_initial= c_try_meters / double( y1 - y0 );
		result.coordinates_scale= std::max( 1, static_cast<int>( c_required_accuracy_m / meters_un_unit_initial ) );
		result.coordinates_scale<<= additional_scale_log2;
		result.meters_in_unit= static_cast<float>( meters_un_unit_initial * double(result.coordinates_scale) );
	}

	const LinearProjectionTransformation projection( std::move(base_projection), geo_min, geo_max, result.coordinates_scale );
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
