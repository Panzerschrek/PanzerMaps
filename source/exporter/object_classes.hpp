#pragma once
#include <cstdint>

namespace PanzerMaps
{

enum class PointObjectClass : uint8_t
{
	None,
	StationPlatform,
	SubwayEntrance,
};

enum class LinearObjectClass : uint8_t
{
	None,
	Road,
	Pedestrian,
	Waterway,
	Railway,
	Tram,
	Monorail,
	Barrier,
};

enum class ArealObjectClass : uint8_t
{
	None,
	Water,
	Grassland,
	Cemetery,
	Industrial,
	Residential,
	Administrative,
	Wood,
	Building,
};

} // namespace PanzerMaps
