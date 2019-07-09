#pragma once
#include <jni.h>
#include "../../common/coordinates_conversion.hpp"

namespace PanzerMaps
{

class GPSService final
{
public:
	GPSService();
	~GPSService();

	GeoPoint GetGeoPosition() const;

private:
	jclass gps_source_class_= nullptr;
};

} // namespace PanzerMaps
