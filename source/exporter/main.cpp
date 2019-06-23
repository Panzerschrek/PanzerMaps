#include "final_export.hpp"

int main()
{
	PanzerMaps::CreateDataFile( PanzerMaps::ParseOSM( "maps_src/small_place.osm" ), "map.pm" );
}
