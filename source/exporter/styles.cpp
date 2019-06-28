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

	for( const auto& point_style_json : json_parse_result->root["point_styles"].object_elements() )
	{
		const PointObjectClass object_class= StringToPointObjectClass( point_style_json.first );
		if( object_class == PointObjectClass::None )
			continue;
		if( result.point_object_styles.count( object_class ) > 0 )
			continue;

		Styles::PointObjectStyle& out_style= result.point_object_styles[ object_class ];
		(void)out_style;
	}

	for( const auto& linear_style_json : json_parse_result->root["linear_styles"].object_elements() )
	{
		const LinearObjectClass object_class= StringToLinearObjectClass( linear_style_json.first );
		if( object_class == LinearObjectClass::None )
			continue;
		if( result.linear_object_styles.count( object_class ) > 0 )
			continue;

		Styles::LinearObjectStyle& out_style= result.linear_object_styles[ object_class ];

		if( linear_style_json.second.IsMember( "color" ) )
			ParseColor( linear_style_json.second["color"].AsString(), out_style.color );

		const PanzerJson::Value& width_m_json= linear_style_json.second["width_m"];
		if( width_m_json.IsNumber() )
			out_style.width_m= width_m_json.AsFloat();
	}

	for( const auto& areal_style_json : json_parse_result->root["areal_styles"].object_elements() )
	{
		const ArealObjectClass object_class= StringToArealObjectClass( areal_style_json.first );
		if( object_class == ArealObjectClass::None )
			continue;
		if( result.areal_object_styles.count( object_class ) > 0 )
			continue;

		Styles::ArealObjectStyle& out_style= result.areal_object_styles[ object_class ];

		if( areal_style_json.second.IsMember( "color" ) )
			ParseColor( areal_style_json.second["color"].AsString(), out_style.color );
	}

	for( const PanzerJson::Value& areal_object_phase : json_parse_result->root["areal_phases"].array_elements() )
	{
		Styles::ArealObjectPhase result_phase;
		for( const PanzerJson::Value& class_json : areal_object_phase["classes"].array_elements() )
		{
			const ArealObjectClass object_class= StringToArealObjectClass( class_json.AsString() );
			if( object_class != ArealObjectClass::None )
			{
				result_phase.classes.insert( object_class );
			}
		}
		result.areal_object_phases.push_back( std::move(result_phase) );
	}

	return result;
}

} // namespace PanzerMaps
