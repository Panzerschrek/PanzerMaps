#pragma once
#include "../common/coordinates_conversion.hpp"

namespace PanzerMaps
{

class GPSService final
{
public:
	void SetEnabled( const bool enabled ){ enabled_= enabled; }
	bool GetEnabled() const{ return enabled_; }

	void Update(){}
	GeoPoint GetGPSPosition() const { return gps_position_; }

private:
	bool enabled_= false;
	GeoPoint gps_position_{ 1000.0, 1000.0 };
};

} // namespace PanzerMaps
