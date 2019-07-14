#pragma once
#include <cmath>
#include <cstdint>
#include <memory>

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
	virtual ProjectionPoint Project( const GeoPoint& geo_point ) const override;
};

class StereographicProjection final : public IProjection
{
public:
	StereographicProjection( const GeoPoint& min_point, const GeoPoint& max_point );

	virtual ProjectionPoint Project( const GeoPoint& geo_point ) const override;
private:
	double center_lon_rad_, center_lat_sin_, center_lat_cos_;
};

class AlbersProjection final : public IProjection
{
public:
	AlbersProjection( const GeoPoint& min_point, const GeoPoint& max_point );

	virtual ProjectionPoint Project( const GeoPoint& geo_point ) const override;

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
	LinearProjectionTransformation( IProjectionPtr projection, const GeoPoint& min_point, const GeoPoint& max_point, const int32_t unit_size );

	virtual ProjectionPoint Project( const GeoPoint& geo_point ) const;

	const ProjectionPoint GetMinPoint() const { return min_point_; }
	const ProjectionPoint GetMaxPoint() const { return max_point_; }

private:
	const IProjectionPtr projection_;
	ProjectionPoint min_point_;
	ProjectionPoint max_point_;
	int32_t unit_size_;
};

} // namespace PanzerMaps
