#include <cstring>
#include "../common/log.hpp"
#include "final_export.hpp"

int main( int argc, const char* const argv[] )
{
	using namespace PanzerMaps;

	std::vector<std::string> input_files;
	std::string output_file;
	std::string styles_dir= "styles";

	static const char help_message[]=
	R"(
PanzerMaps Exporter. Input file format - .osm
Usage:
	Exporter -i [input_file] -o [output_file] --styles [styles_dir])";

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
		else if( std::strcmp( argv[i], "--styles" ) == 0 )
		{
			EXPECT_ARG_VALUE
			styles_dir= argv[ i + 1 ];
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

	const Styles styles= LoadStyles( styles_dir );
	const OSMParseResult osm_parse_result= ParseOSM( input_files.front().c_str() );

	std::vector<PolygonsNormalizationPassResult> ou_data_by_zoom_level;
	ou_data_by_zoom_level.reserve( styles.zoom_levels.size() );
	size_t zoom_level_scale_log2= 0u;
	for( const Styles::ZoomLevel& zoom_level : styles.zoom_levels )
	{
		Log::Info( "" );
		Log::Info( "-- ZOOM LEVEL ", &zoom_level - styles.zoom_levels.data(), " ---" );
		Log::Info( "" );

		if( &zoom_level != &styles.zoom_levels.front() )
			zoom_level_scale_log2+= zoom_level.scale_to_prev_log2;

		CoordinatesTransformationPassResult coordinates_transform_result= TransformCoordinates( osm_parse_result, zoom_level_scale_log2, zoom_level.simplification_distance );

		PhaseSortResult phase_sort_result= SortByPhase( coordinates_transform_result, zoom_level );
		coordinates_transform_result= CoordinatesTransformationPassResult();

		PolygonsNormalizationPassResult normalize_polygons_result= NormalizePolygons( phase_sort_result );
		ou_data_by_zoom_level.push_back( std::move(normalize_polygons_result) );

		Log::Info( "" );
		Log::Info( "-- ZOOM LEVEL END ---" );
		Log::Info( "" );
	}

	CreateDataFile(
		ou_data_by_zoom_level,
		styles,
		output_file.c_str() );
}
