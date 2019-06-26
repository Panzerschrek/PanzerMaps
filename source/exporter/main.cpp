#include "final_export.hpp"

int main()
{
	using namespace PanzerMaps;

	CreateDataFile(
		NormalizePolygons(
			TransformCoordinates(
				ParseOSM( "maps_src/Щ.osm" ) ) ),
		LoadStyles( "source/styles.json" ),
		"map.pm" );
}
