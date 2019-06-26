#include "final_export.hpp"
#include "../common/memory_mapped_file.hpp"

int main()
{
	using namespace PanzerMaps;

	OSMParseResult osm_parse_result= ParseOSM( "maps_src/Щ.osm" );

	CoordinatesTransformationPassResult coordinates_transform_result= TransformCoordinates( osm_parse_result );
	osm_parse_result= OSMParseResult();

	PolygonsNormalizationPassResult normalize_polygons_result= NormalizePolygons( coordinates_transform_result );
	coordinates_transform_result= CoordinatesTransformationPassResult();

	CreateDataFile(
		normalize_polygons_result,
		LoadStyles( "source/styles.json" ),
		"map.pm" );
}
