#pragma once

#ifdef __ANDROID__
#include "gps_service_android.hpp"
#else
#include "gps_service_stub.hpp"
#endif
