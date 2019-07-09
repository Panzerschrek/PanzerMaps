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

	void Update();
	GeoPoint GetGPSPosition() const { return gps_position_; }

private:
	jclass gps_source_class_= nullptr;
	jmethodID get_longitude_method_= nullptr;
	jmethodID get_latitude_method_ = nullptr;
	GeoPoint gps_position_{ 1000.0, 1000.0 };
};

} // namespace PanzerMaps
