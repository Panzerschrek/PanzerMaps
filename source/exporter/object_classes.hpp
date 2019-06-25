#pragma once
#include <cstdint>

namespace PanzerMaps
{

enum class PointObjectClass : uint8_t
{
	None,
	#define PROCESS_OBJECT_CLASS(X) X,
	#include "point_objects_classes.hpp"
	#undef PROCESS_OBJECT_CLASS
	Last
};

enum class LinearObjectClass : uint8_t
{
	None,
	#define PROCESS_OBJECT_CLASS(X) X,
	#include "linear_objects_classes.hpp"
	#undef PROCESS_OBJECT_CLASS
	Last
};

enum class ArealObjectClass : uint8_t
{
	None,
	#define PROCESS_OBJECT_CLASS(X) X,
	#include "areal_objects_classes.hpp"
	#undef PROCESS_OBJECT_CLASS
	Last
};

PointObjectClass StringToPointObjectClass( const char* str );
LinearObjectClass StringToLinearObjectClass( const char* str );
ArealObjectClass StringToArealObjectClass( const char* str );

} // namespace PanzerMaps
