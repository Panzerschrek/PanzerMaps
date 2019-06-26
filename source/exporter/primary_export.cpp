#include <unordered_map>
#include <tinyxml2.h>
#include "../common/log.hpp"
#include "../common/memory_mapped_file.hpp"
#include "primary_export.hpp"

namespace PanzerMaps
{

using OsmId= uint64_t; // Valid Ids starts with '1'.
using NodesMap= std::unordered_map<OsmId, GeoPoint>;

static NodesMap ExtractNodes( const tinyxml2::XMLDocument& doc )
{
	NodesMap result;

	const tinyxml2::XMLElement* element= doc.RootElement()->FirstChildElement( "node" );
	while( element != nullptr )
	{
		const char* const id_str= element->Attribute( "id" );
		const char* const lon_str= element->Attribute( "lon" );
		const char* const lat_str= element->Attribute( "lat" );
		if( id_str != nullptr && lon_str != nullptr && lat_str != nullptr )
		{
			OsmId id;
			double lon;
			double lat;

			if( std::sscanf( id_str, "%lu", &id ) == 1 &&
				std::sscanf( lon_str, "%lf", &lon ) == 1 &&
				std::sscanf( lat_str, "%lf", &lat ) == 1 )
				result.insert( std::make_pair( id, GeoPoint{ lon, lat } ) );
		}

		element= element->NextSiblingElement( "node" );
	}

	return result;
}

static const char* GetTagValue( const tinyxml2::XMLElement* const element, const char* const key )
{
	for( const tinyxml2::XMLElement* tag_element= element->FirstChildElement( "tag" ); tag_element != nullptr; tag_element= tag_element->NextSiblingElement( "tag" ) )
	{
		const char* const key_attrib= tag_element->Attribute( "k" );
		const char* const value_attrib= tag_element->Attribute( "v" );
		if( key_attrib == nullptr || value_attrib == nullptr )
			continue;
		if( std::strcmp( key_attrib, key ) == 0 )
			return value_attrib;
	}

	return nullptr;
}

static void ExtractVertices( const tinyxml2::XMLElement* const way_element, const NodesMap& nodes, std::vector<GeoPoint>& out_vertices )
{
	for( const tinyxml2::XMLElement* nd_element= way_element->FirstChildElement( "nd" ); nd_element != nullptr; nd_element= nd_element->NextSiblingElement( "nd" ) )
	{
		if( const char* const ref_attrib= nd_element->Attribute( "ref" ) )
		{
			OsmId id;
			if( std::sscanf( ref_attrib, "%lu", &id ) == 1 )
			{
				const auto node_it= nodes.find(id);
				if( node_it != nodes.end() )
					out_vertices.push_back( node_it->second );
			}
		}

	}
}

OSMParseResult ParseOSM( const char* file_name )
{
	OSMParseResult result;

	MemoryMappedFilePtr file_mapped= MemoryMappedFile::Create( file_name );
	if( file_mapped == nullptr )
		return result;

	tinyxml2::XMLDocument doc;
	const tinyxml2::XMLError load_result= doc.Parse( static_cast<const char*>( file_mapped->Data() ), file_mapped->Size() );
	if( load_result != tinyxml2::XML_SUCCESS )
	{
		Log::FatalError( "XML Parse error: ", doc.ErrorStr() );
		return result;
	}
	file_mapped= nullptr;

	const NodesMap nodes= ExtractNodes(doc);

	for( const tinyxml2::XMLElement* way_element= doc.RootElement()->FirstChildElement( "way" ); way_element != nullptr; way_element= way_element->NextSiblingElement( "way" ) )
	{
		if( const char* const highway= GetTagValue( way_element, "highway" ) )
		{
			OSMParseResult::LinearObject obj;
			if( std::strcmp( highway, "motorway" ) == 0 ||
				std::strcmp( highway, "trunk" ) == 0 ||
				std::strcmp( highway, "primary" ) == 0 ||
				std::strcmp( highway, "secondary" ) == 0 ||
				std::strcmp( highway, "tertiary" ) == 0 ||
				std::strcmp( highway, "unclassified" ) == 0 ||
				std::strcmp( highway, "residential" ) == 0 ||
				std::strcmp( highway, "motorway_link" ) == 0 ||
				std::strcmp( highway, "trunk_link" ) == 0 ||
				std::strcmp( highway, "primary_link" ) == 0 ||
				std::strcmp( highway, "secondary_link" ) == 0 ||
				std::strcmp( highway, "tertiary_link" ) == 0 ||
				std::strcmp( highway, "living_street" ) == 0 ||
				std::strcmp( highway, "service" ) == 0 ||
				std::strcmp( highway, "track" ) == 0 ||
				std::strcmp( highway, "bus_guideway" ) == 0 ||
				std::strcmp( highway, "raceway" ) == 0 ||
				std::strcmp( highway, "road" ) == 0 )
				obj.class_= LinearObjectClass::Road;
			else if( std::strcmp( highway, "pedestrian" ) == 0 ||
				std::strcmp( highway, "footway" ) == 0 ||
				std::strcmp( highway, "path" ) == 0)
				obj.class_= LinearObjectClass::Pedestrian;

			if( obj.class_ != LinearObjectClass::None )
			{
				obj.first_vertex_index= result.vertices.size();
				ExtractVertices( way_element, nodes, result.vertices );
				obj.vertex_count= result.vertices.size() - obj.first_vertex_index;
				if( obj.vertex_count > 0u )
					result.linear_objects.push_back(obj);
			}
		}
		else if( const char* const waterway= GetTagValue( way_element, "waterway" ) )
		{
			OSMParseResult::LinearObject obj;
			if( std::strcmp( waterway, "stream" ) == 0 )
				obj.class_= LinearObjectClass::Waterway;

			if( obj.class_ != LinearObjectClass::None )
			{
				obj.first_vertex_index= result.vertices.size();
				ExtractVertices( way_element, nodes, result.vertices );
				obj.vertex_count= result.vertices.size() - obj.first_vertex_index;
				if( obj.vertex_count > 0u )
					result.linear_objects.push_back(obj);
			}
		}
		else if( const char* const railway= GetTagValue( way_element, "railway" ) )
		{
			OSMParseResult::LinearObject obj;
			if( std::strcmp( railway, "rail" ) == 0 )
				obj.class_= LinearObjectClass::Railway;
			else if( std::strcmp( railway, "monorail" ) == 0 )
				obj.class_= LinearObjectClass::Monorail;
			else if( std::strcmp( railway, "tram" ) == 0 )
				obj.class_= LinearObjectClass::Tram;

			if( obj.class_ != LinearObjectClass::None )
			{
				obj.first_vertex_index= result.vertices.size();
				ExtractVertices( way_element, nodes, result.vertices );
				obj.vertex_count= result.vertices.size() - obj.first_vertex_index;
				if( obj.vertex_count > 0u )
					result.linear_objects.push_back(obj);
			}
		}
		else if( const char* const barrier= GetTagValue( way_element, "barrier" ) )
		{
			OSMParseResult::LinearObject obj;
			if( std::strcmp( barrier, "cable_barrier" ) == 0 ||
				std::strcmp( barrier, "city_wall" ) == 0 ||
				std::strcmp( barrier, "fence" ) == 0 ||
				std::strcmp( barrier, "hedge" ) == 0 ||
				std::strcmp( barrier, "wall" ) == 0 ||
				std::strcmp( barrier, "hampshire_gate" ) == 0 )
				obj.class_= LinearObjectClass::Barrier;

			if( obj.class_ != LinearObjectClass::None )
			{
				obj.first_vertex_index= result.vertices.size();
				ExtractVertices( way_element, nodes, result.vertices );
				obj.vertex_count= result.vertices.size() - obj.first_vertex_index;
				if( obj.vertex_count > 0u )
					result.linear_objects.push_back(obj);
			}
		}
		else if( const char* const building= GetTagValue( way_element, "building" ) )
		{
			(void)building;

			OSMParseResult::ArealObject obj;
			obj.class_= ArealObjectClass::Building;
			obj.first_vertex_index= result.vertices.size();
			ExtractVertices( way_element, nodes, result.vertices );
			obj.vertex_count= result.vertices.size() - obj.first_vertex_index;
			if( obj.vertex_count > 0u )
				result.areal_objects.push_back(obj);
		}
		else if( const char* const natural= GetTagValue( way_element, "natural" ) )
		{
			OSMParseResult::ArealObject obj;
			if( std::strcmp( natural, "water" ) == 0 )
				obj.class_= ArealObjectClass::Water;
			else if( std::strcmp( natural, "wood" ) == 0 )
				obj.class_= ArealObjectClass::Wood;
			else if( std::strcmp( natural, "scrub" ) == 0 )
				obj.class_= ArealObjectClass::Wood;
			else if( std::strcmp( natural, "grassland" ) == 0 )
				obj.class_= ArealObjectClass::Grassland;
			else if( std::strcmp( natural, "heath" ) == 0 )
				obj.class_= ArealObjectClass::Grassland;

			if( obj.class_ != ArealObjectClass::None )
			{
				obj.first_vertex_index= result.vertices.size();
				ExtractVertices( way_element, nodes, result.vertices );
				obj.vertex_count= result.vertices.size() - obj.first_vertex_index;
				if( obj.vertex_count > 0u )
					result.areal_objects.push_back(obj);
			}
		}
		else if( const char* const landuse= GetTagValue( way_element, "landuse" ) )
		{
			OSMParseResult::ArealObject obj;
				if( std::strcmp( landuse, "basin" ) == 0 )
				obj.class_= ArealObjectClass::Water;
			else if( std::strcmp( landuse, "cemetery" ) == 0 )
				obj.class_= ArealObjectClass::Cemetery;
			else if( std::strcmp( landuse, "foreset" ) == 0 )
				obj.class_= ArealObjectClass::Wood;
			else if( std::strcmp( landuse, "wood" ) == 0 )
				obj.class_= ArealObjectClass::Wood;
			else if( std::strcmp( landuse, "grass" ) == 0 )
				obj.class_= ArealObjectClass::Grassland;
			else if( std::strcmp( landuse, "residential" ) == 0 )
				obj.class_= ArealObjectClass::Residential;
			else if( std::strcmp( landuse, "industrial" ) == 0 ||
				std::strcmp( landuse, "garages" ) == 0 ||
				std::strcmp( landuse, "railway" ) == 0 )
				obj.class_= ArealObjectClass::Industrial;
			else if( std::strcmp( landuse, "commercial" ) == 0 ||
				std::strcmp( landuse, "retail" ) == 0 )
				obj.class_= ArealObjectClass::Administrative;

			if( obj.class_ != ArealObjectClass::None )
			{
				obj.first_vertex_index= result.vertices.size();
				ExtractVertices( way_element, nodes, result.vertices );
				obj.vertex_count= result.vertices.size() - obj.first_vertex_index;
				if( obj.vertex_count > 0u )
					result.areal_objects.push_back(obj);
			}
		}
		else if( const char* const amenity= GetTagValue( way_element, "amenity" ) )
		{
			OSMParseResult::ArealObject obj;
			if( std::strcmp( amenity, "grave_yard" ) == 0 )
				obj.class_= ArealObjectClass::Cemetery;
			else if( std::strcmp( amenity, "school" ) == 0 ||
				std::strcmp( amenity, "college" ) == 0  ||
				std::strcmp( amenity, "kindergarten" ) == 0 ||
				std::strcmp( amenity, "library" ) == 0 ||
				std::strcmp( amenity, "university" ) == 0 )
				obj.class_= ArealObjectClass::Administrative;
			else if( std::strcmp( amenity, "clinic" ) == 0 ||
				std::strcmp( amenity, "dentist" ) == 0  ||
				std::strcmp( amenity, "doctors" ) == 0 ||
				std::strcmp( amenity, "hospital" ) == 0 ||
				std::strcmp( amenity, "ursing_home" ) == 0 )
				obj.class_= ArealObjectClass::Administrative;

			if( obj.class_ != ArealObjectClass::None )
			{
				obj.first_vertex_index= result.vertices.size();
				ExtractVertices( way_element, nodes, result.vertices );
				obj.vertex_count= result.vertices.size() - obj.first_vertex_index;
				if( obj.vertex_count > 0u )
					result.areal_objects.push_back(obj);
			}
		}
	}

	for( const tinyxml2::XMLElement* node_element= doc.RootElement()->FirstChildElement( "node" ); node_element != nullptr; node_element= node_element->NextSiblingElement( "node" ) )
	{
		const char* const lon_str= node_element->Attribute( "lon" );
		const char* const lat_str= node_element->Attribute( "lat" );
		GeoPoint node_geo_point;

		if( !( lon_str != nullptr && lat_str != nullptr ) )
			continue;
		if( !( std::sscanf( lon_str, "%lf", &node_geo_point.x ) == 1 && std::sscanf( lat_str, "%lf", &node_geo_point.y ) == 1 ) )
			continue;

		if( const char* const public_transport= GetTagValue( node_element, "public_transport" ) )
		{
			OSMParseResult::PointObject obj;
			if( std::strcmp( public_transport, "platform" ) == 0 )
				obj.class_= PointObjectClass::StationPlatform;

			if( obj.class_ != PointObjectClass::None )
			{
				obj.vertex_index= result.vertices.size();
				result.vertices.push_back( node_geo_point );
				result.point_objects.push_back(obj);
			}
		}
		else if( const char* const highway= GetTagValue( node_element, "highway" ) )
		{
			OSMParseResult::PointObject obj;
			if( std::strcmp( highway, "bus_stop" ) == 0 )
				obj.class_= PointObjectClass::StationPlatform;

			if( obj.class_ != PointObjectClass::None )
			{
				result.vertices.push_back( node_geo_point );
				obj.vertex_index= result.vertices.size();
				result.point_objects.push_back(obj);
			}
		}
		if( const char* const railway= GetTagValue( node_element, "railway" ) )
		{
			OSMParseResult::PointObject obj;
			if( std::strcmp( railway, "subway_entrance" ) == 0 )
				obj.class_= PointObjectClass::SubwayEntrance;

			if( obj.class_ != PointObjectClass::None )
			{
				obj.vertex_index= result.vertices.size();
				result.vertices.push_back( node_geo_point );
				result.point_objects.push_back(obj);
			}
		}
	}

	Log::Info( "Primary export: " );
	Log::Info( result.point_objects.size(), " point objects" );
	Log::Info( result.linear_objects.size(), " linear objects" );
	Log::Info( result.areal_objects.size(), " areal objects" );
	Log::Info( result.vertices.size(), " vertices" );
	Log::Info( "" );

	return result;
}

} // namespace PanzerMaps
