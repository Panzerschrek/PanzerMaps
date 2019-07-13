#pragma once
#include <cmath>
#include <cstdint>

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

const double mercator_projection_max_latitude = 2.0 * std::atan(std::exp(Constants::pi)) - Constants::pi * 0.5;
}

struct GeoPoint
{
	double x; // logitude, [ -180; 180 )
	double y; // latitude, ( -90; 90 )
};

struct ProjectionPoint
{
	int32_t x;
	int32_t y;
};

bool operator==( const GeoPoint& l, const GeoPoint& r );
bool operator!=( const GeoPoint& l, const GeoPoint& r );

bool operator==( const ProjectionPoint& l, const ProjectionPoint& r );
bool operator!=( const ProjectionPoint& l, const ProjectionPoint& r );

// Mercator projection.
// Maps longitude in range [ -180; 180 ) to projection x in range [ int_min, int_max ].
// Maps latitude in range [ -max_latitude, max_latitude ) to projection y in range [ int_min, int_max ].
ProjectionPoint GeoPointToMercatorPoint( const GeoPoint& point );
GeoPoint MercatorPointToGeoPoint( const ProjectionPoint& point );


} // namespace PanzerMaps
