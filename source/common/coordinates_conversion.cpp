#include "coordinates_conversion.hpp"

namespace PanzerMaps
{

static const double c_two_pow_31= 2147483648.0;

bool operator==( const MercatorPoint& l, const MercatorPoint& r )
{
	return l.x == r.x && l.y == r.y;
}

bool operator!=( const MercatorPoint& l, const MercatorPoint& r )
{
	return !( l == r );
}

MercatorPoint GeoPointToMercatorPoint( const GeoPoint& point )
{
	MercatorPoint result;

	result.x=
		static_cast<int32_t>( ( c_two_pow_31 / 180.0 ) * point.x );
	result.y=
		static_cast<int32_t>(
			( c_two_pow_31 / Constants::pi ) *
			std::log( std::tan( Constants::pi * 0.25 + point.y * ( 0.5 * Constants::deg_to_rad ) ) ) );
	return result;
}

GeoPoint MercatorPointToGeoPoint( const MercatorPoint& point )
{
	GeoPoint result;

	result.x= static_cast<double>(point.x) * ( 180.0 / c_two_pow_31 );
	result.y=
		Constants::rad_to_deg *
		( 2.0 * std::atan( std::exp( static_cast<double>(point.y) * ( Constants::pi / c_two_pow_31 ) ) ) - Constants::pi * 0.5 );
	return result;
}

} // namespace PanzerMaps
