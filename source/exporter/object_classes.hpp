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
};

enum class ArealObjectClass : uint8_t
{
	None,
	Building,
	Water,
	Wood,
	Grassland,
	Cemetery,
};

} // namespace PanzerMaps
