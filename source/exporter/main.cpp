#include <cstring>
#include "final_export.hpp"
#include "../common/log.hpp"

int main( int argc, const char* const argv[] )
{
	using namespace PanzerMaps;

	std::vector<std::string> input_files;
	std::string output_file;
	std::string style_file= "source/styles.json";

	static const char help_message[]=
	R"(
PanzerMaps Exporter. Input file format - .osm
Usage:
	Exporter -i [input_file] -o [output_file] --style [style_file])";

	if( argc <= 1 )
	{
		Log::User( help_message );
		return 0;
	}

	// Parse command line
	#define EXPECT_ARG_VALUE if( i + 1 >= argc ) { Log::Warning( "Expeted name after \"", argv[i], "\"" ); return -1; }
	for( int i = 1; i < argc; )
	{
		if( std::strcmp( argv[i], "-o" ) == 0 )
		{
			EXPECT_ARG_VALUE
			output_file= argv[ i + 1 ];
			i+= 2;
		}
		else if( std::strcmp( argv[i], "--style" ) == 0 )
		{
			EXPECT_ARG_VALUE
			style_file= argv[ i + 1 ];
			i+= 2;
		}
		else if( std::strcmp( argv[i], "-h" ) == 0 || std::strcmp( argv[i], "--help" ) == 0 )
		{
			Log::User( help_message );
			return 0;
		}
		else if( std::strcmp( argv[i], "-i" ) == 0 )
		{
			EXPECT_ARG_VALUE
			input_files.push_back( argv[ i + 1 ] );
			i+= 2;
		}
		else
		{
			Log::Warning( "unknown option: \"", argv[i], "\"" );
			++i;
		}
	}

	if( input_files.empty() )
	{
		Log::FatalError( "No input files" );
		return -1;
	}
	if( input_files.size() > 1u )
	{
		Log::Warning( "Multiple input files not suppored" );
	}
	if( output_file.empty() )
	{
		Log::FatalError( "Output file name not specified" );
		return -1;
	}

	const Styles styles= LoadStyles( style_file.c_str() );
	OSMParseResult osm_parse_result= ParseOSM( input_files.front().c_str() );

	CoordinatesTransformationPassResult coordinates_transform_result= TransformCoordinates( osm_parse_result );
	osm_parse_result= OSMParseResult();

	PhaseSortResult phase_sort_result= SortByPhase( coordinates_transform_result, styles.zoom_levels.front() );
	coordinates_transform_result= CoordinatesTransformationPassResult();

	PolygonsNormalizationPassResult normalize_polygons_result= NormalizePolygons( phase_sort_result );
	phase_sort_result= PhaseSortResult();

	CreateDataFile(
		normalize_polygons_result,
		styles,
		output_file.c_str() );
}
