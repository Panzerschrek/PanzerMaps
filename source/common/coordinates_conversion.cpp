#include "coordinates_conversion.hpp"

namespace PanzerMaps
{

bool operator==( const GeoPoint& l, const GeoPoint& r )
{
	return l.x == r.x && l.y == r.y;
}

bool operator!=( const GeoPoint& l, const GeoPoint& r )
{
	return !( l == r );
}

bool operator==( const ProjectionPoint& l, const ProjectionPoint& r )
{
	return l.x == r.x && l.y == r.y;
}

bool operator!=( const ProjectionPoint& l, const ProjectionPoint& r )
{
	return !( l == r );
}

ProjectionPoint GeoPointToMercatorPoint( const GeoPoint& point )
{
	ProjectionPoint result;

	result.x=
		static_cast<int32_t>( ( Constants::two_pow_31 / 180.0 ) * point.x );
	result.y=
		static_cast<int32_t>(
			( Constants::two_pow_31 / Constants::pi ) *
			std::log( std::tan( Constants::pi * 0.25 + point.y * ( 0.5 * Constants::deg_to_rad ) ) ) );
	return result;
}

GeoPoint MercatorPointToGeoPoint( const ProjectionPoint& point )
{
	GeoPoint result;

	result.x= static_cast<double>(point.x) * ( 180.0 / Constants::two_pow_31 );
	result.y=
		Constants::rad_to_deg *
		( 2.0 * std::atan( std::exp( static_cast<double>(point.y) * ( Constants::pi / Constants::two_pow_31 ) ) ) - Constants::pi * 0.5 );
	return result;
}

ProjectionPoint MercatorProjection::Project( const GeoPoint& geo_point ) const
{
	return GeoPointToMercatorPoint( geo_point );
}

StereographicProjection::StereographicProjection( const GeoPoint& min_point, const GeoPoint& max_point )
{
	const GeoPoint center{ ( min_point.x + max_point.x ) * 0.5, ( min_point.y + max_point.y ) * 0.5 };
	center_lon_rad_= center.x * Constants::deg_to_rad;
	const double center_lat_rad= center.y * Constants::deg_to_rad;

	center_lat_sin_= std::sin(center_lat_rad);
	center_lat_cos_= std::cos(center_lat_rad);
}

ProjectionPoint StereographicProjection::Project( const GeoPoint& geo_point ) const
{
	// http://mathworld.wolfram.com/StereographicProjection.html
	const double lon_rad= geo_point.x * Constants::deg_to_rad;
	const double lat_rad= geo_point.y * Constants::deg_to_rad;

	const double lon_delta= lon_rad - center_lon_rad_;
	const double lon_dalta_sin= std::sin(lon_delta);
	const double lon_delta_cos= std::cos(lon_delta);

	const double lat_sin= std::sin(lat_rad);
	const double lat_cos= std::cos(lat_rad);

	// Project sphere with radius 1, maximum projection will be 2. Scale it to fit integer range.
	const double k= double( 1 << 30 ) * 2.0 / ( 1.0 + center_lat_sin_ * lat_sin + center_lat_cos_ * lat_cos * lon_delta_cos );
	const double x= k * ( lat_cos * lon_dalta_sin );
	const double y= k * ( center_lat_cos_ * lat_sin - center_lat_sin_ * lat_cos * lon_delta_cos );

	return ProjectionPoint{ int32_t(x), int32_t(y) };
}

AlbersProjection::AlbersProjection( const GeoPoint& min_point, const GeoPoint& max_point )
{
	const double latitude_diff_div_6= ( max_point.y - min_point.y ) / 6.0;
	const double zero_latitude= ( min_point.y + max_point.y ) * 0.5 * Constants::deg_to_rad;
	const double base_latitude1= min_point.y + latitude_diff_div_6;
	const double base_latitude2= max_point.y - latitude_diff_div_6;

	const double base_latitude1_sin= std::sin( base_latitude1 * Constants::deg_to_rad );
	const double base_latitude1_cos= std::cos( base_latitude1 * Constants::deg_to_rad );
	const double base_latitude2_sin= std::sin( base_latitude2 * Constants::deg_to_rad );

	const double zero_latitude_deg= ( min_point.x + max_point.x ) * 0.5;
	zero_longitude_rad_= zero_latitude_deg * Constants::deg_to_rad;
	latitude_avg_sin_= ( base_latitude1_sin + base_latitude2_sin ) * 0.5;
	c_= base_latitude1_cos * base_latitude1_cos + 2.0 * latitude_avg_sin_ * base_latitude1_sin;
	p0_= std::sqrt( c_ - 2.0 * latitude_avg_sin_ * std::sin(zero_latitude) ) / latitude_avg_sin_;

	// Calculate scale factor.
	scale_factor_= double( 1 << 28 );
	ProjectionPoint special_points[10];
	special_points[0]= Project( GeoPoint{ zero_latitude_deg -90.0, -90.0 } );
	special_points[1]= Project( GeoPoint{ zero_latitude_deg +90.0, -90.0 } );
	special_points[2]= Project( GeoPoint{ zero_latitude_deg -90.0, +90.0 } );
	special_points[3]= Project( GeoPoint{ zero_latitude_deg +90.0, +90.0 } );
	special_points[4]= Project( GeoPoint{ zero_latitude_deg  +0.0, -90.0 } );
	special_points[5]= Project( GeoPoint{ zero_latitude_deg  +0.0, +90.0 } );
	special_points[6]= Project( GeoPoint{ zero_latitude_deg -90.0 / latitude_avg_sin_, -90.0 } );
	special_points[7]= Project( GeoPoint{ zero_latitude_deg +90.0 / latitude_avg_sin_, -90.0 } );
	special_points[8]= Project( GeoPoint{ zero_latitude_deg -90.0 / latitude_avg_sin_, +90.0 } );
	special_points[9]= Project( GeoPoint{ zero_latitude_deg +90.0 / latitude_avg_sin_, +90.0 } );

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

ProjectionPoint AlbersProjection::Project( const GeoPoint& geo_point ) const
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

LinearProjectionTransformation::LinearProjectionTransformation(
	IProjectionPtr projection,
	const GeoPoint& min_point,
	const GeoPoint& max_point,
	const int32_t unit_size )
	: projection_(std::move(projection)), unit_size_(unit_size)
{
	ProjectionPoint special_points[8];
	special_points[0]= projection_->Project( GeoPoint{ min_point.x, min_point.y } );
	special_points[1]= projection_->Project( GeoPoint{ max_point.x, min_point.y } );
	special_points[2]= projection_->Project( GeoPoint{ min_point.x, max_point.y } );
	special_points[3]= projection_->Project( GeoPoint{ max_point.x, max_point.y } );
	special_points[4]= projection_->Project( GeoPoint{ ( min_point.x + max_point.x ) * 0.5, min_point.y } );
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

ProjectionPoint LinearProjectionTransformation::Project( const GeoPoint& geo_point ) const
{
	const ProjectionPoint p= projection_->Project( geo_point );
	return ProjectionPoint{ ( p.x - min_point_.x ) / unit_size_, ( p.y - min_point_.y ) / unit_size_ };
}

} // namespace PanzerMaps
