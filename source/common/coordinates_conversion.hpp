#pragma once
#include <cmath>

namespace PanzerMaps
{

namespace Constants
{

const double pi = 3.1415926535;
const double earth_radius_m = 6371000.0;
const double earth_equator_length_m= earth_radius_m * 2.0 * pi;

const double deg_to_rad = pi / 180.0;
const double rad_to_deg = 180.0 / pi;

const double two_pow_31 = 2147483648.0;
const double two_pow_32 = 4294967296.0;

// Max latitude for used projection.
const double max_latitude = 2.0 * std::atan(std::exp(Constants::pi)) - Constants::pi * 0.5;

}

struct GeoPoint
{
	double x; // logitude, [ -180; 180 )
	double y; // latitude, ( -90; 90 )
};

struct MercatorPoint
{
	int32_t x; // map [ -180; 180 ) to [ int_min, int_max ]
	int32_t y; // map [ -max_latitude, max_latitude ) to [ int_min, int_max ]
};

bool operator==( const MercatorPoint& l, const MercatorPoint& r );
bool operator!=( const MercatorPoint& l, const MercatorPoint& r );

MercatorPoint GeoPointToMercatorPoint( const GeoPoint& point );
GeoPoint MercatorPointToGeoPoint( const MercatorPoint& point );

} // namespace PanzerMaps
