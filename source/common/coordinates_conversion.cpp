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

} // namespace PanzerMaps
