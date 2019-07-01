#include <algorithm>
#include <string>
#include <PanzerJson/parser.hpp>
#include "../common/log.hpp"
#include "../common/memory_mapped_file.hpp"
#include "styles.hpp"

namespace PanzerMaps
{

static void ParseColor( const char* color_str, Styles::ColorRGBA& out_color )
{
	if( color_str[0u] == '#' )
		++color_str;
	else
		return;

	const size_t len= std::min( std::strlen( color_str ), size_t(8u) );
	if( len < 6u )
		return;

	for( size_t i= 0u; i < len; ++i )
		if( !std::isxdigit(color_str[i]) )
			return;

	out_color[0]= out_color[1]= out_color[2]= 0;
	out_color[3]= 0xFFu;

	for( size_t i= 0u; i * 2 < len; ++i )
	{
		size_t num= 0u;
		for( size_t j= 0u; j < 2; ++j )
		{
			const char c= color_str[ i * 2u + j ];
			size_t digit_num= 0u;
			if( c >= '0' && c <= '9' ) digit_num= c - '0';
			if( c >= 'a' && c <= 'f' ) digit_num= c - 'a' + 10;
			if( c >= 'A' && c <= 'F' ) digit_num= c - 'A' + 10;
			num|= digit_num << ((1u-j)*4u);
		}
		out_color[i]= static_cast<unsigned char>(num);
	}
}

static void ParsePointObjectStyles( const PanzerJson::Value& point_styles_json, Styles::PointObjectStyles& point_styles )
{
	for( const auto& point_style_json : point_styles_json.object_elements() )
	{
		const PointObjectClass object_class= StringToPointObjectClass( point_style_json.first );
		if( object_class == PointObjectClass::None )
		{
			Log::Warning( "Unknown point object class: ", point_style_json.first );
			continue;
		}

		Styles::PointObjectStyle& out_style= point_styles[ object_class ];
		(void)out_style;
	}
}

static void ParseLinearObjectStyles( const PanzerJson::Value& linear_styles_json, Styles::LinearObjectStyles& linear_styles )
{
	for( const auto& linear_style_json : linear_styles_json.object_elements() )
	{
		const LinearObjectClass object_class= StringToLinearObjectClass( linear_style_json.first );
		if( object_class == LinearObjectClass::None )
		{
			Log::Warning( "Unknown linear object class: ", linear_style_json.first );
			continue;
		}

		Styles::LinearObjectStyle& out_style= linear_styles[ object_class ];

		if( linear_style_json.second.IsMember( "color" ) )
		{
			ParseColor( linear_style_json.second["color"].AsString(), out_style.color );
			std::memcpy( out_style.color2, out_style.color, sizeof(Styles::ColorRGBA) );
		}
		if( linear_style_json.second.IsMember( "color2" ) )
			ParseColor( linear_style_json.second["color2"].AsString(), out_style.color2 );

		if( linear_style_json.second.IsMember( "width_m" ) )
		{
			const PanzerJson::Value& width_m_json= linear_style_json.second["width_m"];
			if( width_m_json.IsNumber() )
			out_style.width_m= std::max( 0.0f, width_m_json.AsFloat() );
		}
		if( linear_style_json.second.IsMember( "dash_size_m" ) )
		{
			const PanzerJson::Value& dash_size_m_json= linear_style_json.second["dash_size_m"];
			if( dash_size_m_json.IsNumber() )
			out_style.dash_size_m= std::max( 0.25f, dash_size_m_json.AsFloat() );
		}
	}
}

static void ParseArealObjectStyles( const PanzerJson::Value& areal_styles_json, Styles::ArealObjectStyles& areal_styles )
{
	for( const auto& areal_style_json : areal_styles_json.object_elements() )
	{
		const ArealObjectClass object_class= StringToArealObjectClass( areal_style_json.first );
		if( object_class == ArealObjectClass::None )
		{
			Log::Warning( "Unknown areal object class: ", areal_style_json.first );
			continue;
		}

		Styles::ArealObjectStyle& out_style= areal_styles[ object_class ];

		if( areal_style_json.second.IsMember( "color" ) )
			ParseColor( areal_style_json.second["color"].AsString(), out_style.color );
	}

}

static Styles::ZoomLevel ParseZoomLevel(
	const PanzerJson::Value& zoom_level_json,
	const Styles::PointObjectStyles& default_point_styles,
	const Styles::LinearObjectStyles& default_linear_styles,
	const Styles::ArealObjectStyles& default_areal_styles )
{
	Styles::ZoomLevel zoom_level;
	zoom_level.point_object_styles= default_point_styles;
	zoom_level.linear_object_styles= default_linear_styles;
	zoom_level.areal_object_styles= default_areal_styles;

	zoom_level.scale_to_prev_log2= static_cast<size_t>( std::max( 1, std::min( zoom_level_json[ "scale_to_prev_log2" ].AsInt(), 4 ) ) );

	ParsePointObjectStyles( zoom_level_json["point_styles"], zoom_level.point_object_styles );
	ParseLinearObjectStyles( zoom_level_json["linear_styles"], zoom_level.linear_object_styles );
	ParseArealObjectStyles( zoom_level_json["areal_styles"], zoom_level.areal_object_styles );

	for( const PanzerJson::Value& areal_object_phase : zoom_level_json["areal_phases"].array_elements() )
	{
		Styles::ArealObjectPhase result_phase;
		for( const PanzerJson::Value& class_json : areal_object_phase["classes"].array_elements() )
		{
			const ArealObjectClass object_class= StringToArealObjectClass( class_json.AsString() );
			if( object_class == ArealObjectClass::None )
			{
				Log::Warning( "Unknown areal object class: ", class_json.AsString() );
				continue;
			}
			result_phase.classes.insert( object_class );
		}
		zoom_level.areal_object_phases.push_back( std::move(result_phase) );
	}

	for( const PanzerJson::Value& point_object_class_json : zoom_level_json["point_classes_ordered"].array_elements() )
	{
		const PointObjectClass object_class= StringToPointObjectClass( point_object_class_json.AsString() );
		if( object_class == PointObjectClass::None )
		{
			Log::Warning( "Unknown point object class: ", point_object_class_json.AsString() );
			continue;
		}
		if( std::find( zoom_level.point_classes_ordered.begin(), zoom_level.point_classes_ordered.end(), object_class ) != zoom_level.point_classes_ordered.end() )
		{
			Log::Warning( "Duplicated point class: ", point_object_class_json.AsString() );
			continue;
		}
		zoom_level.point_classes_ordered.push_back( object_class );
	}

	for( const PanzerJson::Value& linear_object_class_json : zoom_level_json["linear_classes_ordered"].array_elements() )
	{
		const LinearObjectClass object_class= StringToLinearObjectClass( linear_object_class_json.AsString() );
		if( object_class == LinearObjectClass::None )
		{
			Log::Warning( "Unknown linear object class: ", linear_object_class_json.AsString() );
			continue;
		}
		if( std::find( zoom_level.linear_classes_ordered.begin(), zoom_level.linear_classes_ordered.end(), object_class ) != zoom_level.linear_classes_ordered.end() )
		{
			Log::Warning( "Duplicated linear class: ", linear_object_class_json.AsString() );
			continue;
		}
		zoom_level.linear_classes_ordered.push_back( object_class );
	}

	return zoom_level;
}

Styles LoadStyles( const char* const file_name )
{
	Styles result;

	const MemoryMappedFilePtr file= MemoryMappedFile::Create( file_name );
	if( file == nullptr )
		return result;

	const PanzerJson::Parser::ResultPtr json_parse_result= PanzerJson::Parser().Parse( static_cast<const char*>(file->Data()), file->Size() );
	if( json_parse_result->error != PanzerJson::Parser::Result::Error::NoError )
	{
		Log::FatalError( "styles json parse error. Error code: ", int(json_parse_result->error) );
		return result;
	}

	if( !json_parse_result->root.IsObject() )
	{
		Log::FatalError( "styles json root expected to be object" );
		return result;
	}

	if( json_parse_result->root.IsMember( "background_color" ) )
		ParseColor( json_parse_result->root["background_color"].AsString(), result.background_color );

	Styles::PointObjectStyles point_object_styles;
	Styles::LinearObjectStyles linear_object_styles;
	Styles::ArealObjectStyles areal_object_styles;

	ParsePointObjectStyles( json_parse_result->root["point_styles"], point_object_styles );
	ParseLinearObjectStyles( json_parse_result->root["linear_styles"], linear_object_styles );
	ParseArealObjectStyles( json_parse_result->root["areal_styles"], areal_object_styles );

	for( const PanzerJson::Value& zoom_json : json_parse_result->root["zoom_levels" ] )
		result.zoom_levels.push_back( ParseZoomLevel( zoom_json, point_object_styles, linear_object_styles, areal_object_styles ) );

	if( result.zoom_levels.empty() )
		Log::FatalError( "No zoom levels in styles. Required at least one zoom level" );

	return result;
}

} // namespace PanzerMaps
