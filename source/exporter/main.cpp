#include "final_export.hpp"
#include "../common/memory_mapped_file.hpp"

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
