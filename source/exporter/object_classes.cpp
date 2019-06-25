#include "object_classes.hpp"
#include <cstring>

namespace PanzerMaps
{

PointObjectClass StringToPointObjectClass( const char* const str )
{
	#define PROCESS_OBJECT_CLASS(X) if( std::strcmp( str, #X ) == 0 ) return PointObjectClass::X;
	#include "point_objects_classes.hpp"
	#undef PROCESS_OBJECT_CLASS
	return PointObjectClass::None;
}

LinearObjectClass StringToLinearObjectClass( const char* const str )
{
	#define PROCESS_OBJECT_CLASS(X) if( std::strcmp( str, #X ) == 0 ) return LinearObjectClass::X;
	#include "linear_objects_classes.hpp"
	#undef PROCESS_OBJECT_CLASS
	return LinearObjectClass::None;
}

ArealObjectClass StringToArealObjectClass( const char* const str )
{
	#define PROCESS_OBJECT_CLASS(X) if( std::strcmp( str, #X ) == 0 ) return ArealObjectClass::X;
	#include "areal_objects_classes.hpp"
	#undef PROCESS_OBJECT_CLASS
	return ArealObjectClass::None;
}

} // namespace PanzerMaps
