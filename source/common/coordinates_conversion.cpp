#include "coordinates_conversion.hpp"

namespace PanzerMaps
{

MercatorPoint GeoPointToWebMercatorPoint( const GeoPoint& point )
{
	MercatorPoint result;

	const double two_pow_31= 2147483648.0;

	result.x=
		static_cast<int32_t>( ( two_pow_31 / 180.0 ) * point.x );
	result.y=
		static_cast<int32_t>(
			( two_pow_31 / Constants::pi ) *
			std::log( std::tan( Constants::pi * 0.25 + point.y * ( 0.5 * Constants::deg_to_rad ) ) ) );
	return result;
}

} // namespace PanzerMaps
