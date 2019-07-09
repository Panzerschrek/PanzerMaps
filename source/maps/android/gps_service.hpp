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

	void SetEnabled( bool enabled );

	void Update();
	GeoPoint GetGPSPosition() const { return gps_position_; }

private:
	jclass gps_source_class_= nullptr;
	jmethodID enable_method_ = nullptr;
	jmethodID disable_method_= nullptr;
	jmethodID get_longitude_method_= nullptr;
	jmethodID get_latitude_method_ = nullptr;
	bool initialized_ok_= false;
	bool enabled_= false;
	GeoPoint gps_position_{ 1000.0, 1000.0 };
};

} // namespace PanzerMaps
