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

static const char* GetTagValue( const tinyxml2::XMLElement& element, const char* const key )
{
	for( const tinyxml2::XMLElement* tag_element= element.FirstChildElement( "tag" ); tag_element != nullptr; tag_element= tag_element->NextSiblingElement( "tag" ) )
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

// Returns "0" if unknown.
static size_t GetLaneCount( const tinyxml2::XMLElement& way_element )
{
	size_t lanes= 0u;
	if( const char* const lanes_str= GetTagValue( way_element, "lanes" ) )
	{
		const char* lane_num= lanes_str;
		while( std::isdigit( *lane_num ) )
			++lane_num;

		if( lane_num != lanes_str )
			lanes= std::atoi( lanes_str );

		if( *lane_num == ';' )
		{
			++lane_num;
			while( std::isdigit( *lane_num ) )
				++lane_num;
			lanes+= std::atoi( lane_num );
		}
	}
	else if( const char* const width_str= GetTagValue( way_element, "width" ) )
		lanes= std::max( size_t(1u), size_t( std::atof( width_str ) / 3.5 ) );
	else
	{
		if( const char* const forward_str= GetTagValue( way_element, "lanes:forward" ) )
			lanes+= std::atoi(forward_str);
		if( const char* const backward_str= GetTagValue( way_element, "lanes:backward" ) )
			lanes+= std::atoi(backward_str);
	}

	return lanes;
}

struct WayClassifyResult
{
	// Both may be non-None.
	LinearObjectClass linear_object_class= LinearObjectClass::None;
	ArealObjectClass areal_object_class= ArealObjectClass::None;
};

WayClassifyResult ClassifyWay( const tinyxml2::XMLElement& way_element )
{
	WayClassifyResult result;

	if( const char* const highway= GetTagValue( way_element, "highway" ) )
	{
		const size_t lane_count= GetLaneCount( way_element );
		if( std::strcmp( highway, "living_street" ) == 0 ||
			std::strcmp( highway, "residential" ) == 0 )
		{
			if( lane_count <= 1u )
				result.linear_object_class= LinearObjectClass::RoadSignificance1Lanes1;
			else
				result.linear_object_class= LinearObjectClass::RoadSignificance1Lanes2;
		}
		else if( std::strcmp( highway, "service" ) == 0 )
		{
			if( lane_count <= 2u )
				result.linear_object_class= LinearObjectClass::RoadSignificance0;
			else
				result.linear_object_class= LinearObjectClass::RoadSignificance1Lanes2;
		}
		else if(
			std::strcmp( highway, "unclassified" ) == 0 ||
			std::strcmp( highway, "tertiary" ) == 0 ||
			std::strcmp( highway, "tertiary_link" ) == 0 ||
			std::strcmp( highway, "track" ) == 0 ||
			std::strcmp( highway, "bus_guideway" ) == 0 ||
			std::strcmp( highway, "road" ) == 0 )
		{
				if( lane_count == 0u )
				result.linear_object_class= LinearObjectClass::RoadSignificance1Lanes2;
			else if( lane_count <= 1u )
				result.linear_object_class= LinearObjectClass::RoadSignificance1Lanes1;
			else if( lane_count <= 2u )
				result.linear_object_class= LinearObjectClass::RoadSignificance1Lanes2;
			else if( lane_count <= 4u )
				result.linear_object_class= LinearObjectClass::RoadSignificance1Lanes4;
			else if( lane_count <= 6u )
				result.linear_object_class= LinearObjectClass::RoadSignificance1Lanes6;
			else
				result.linear_object_class= LinearObjectClass::RoadSignificance1Lanes8More;
		}
		else if(
			std::strcmp( highway, "secondary" ) == 0 ||
			std::strcmp( highway, "secondary_link" )  == 0 )
		{
				if( lane_count == 0u )
				result.linear_object_class= LinearObjectClass::RoadSignificance2Lanes2;
			else if( lane_count <= 1u )
				result.linear_object_class= LinearObjectClass::RoadSignificance2Lanes1;
			else if( lane_count <= 2u )
				result.linear_object_class= LinearObjectClass::RoadSignificance2Lanes2;
			else if( lane_count <= 4u )
				result.linear_object_class= LinearObjectClass::RoadSignificance2Lanes4;
			else if( lane_count <= 6u )
				result.linear_object_class= LinearObjectClass::RoadSignificance2Lanes6;
			else
				result.linear_object_class= LinearObjectClass::RoadSignificance2Lanes8More;
		}
		else if(
			std::strcmp( highway, "motorway" ) == 0 ||
			std::strcmp( highway, "motorway_link" ) == 0 ||
			std::strcmp( highway, "trunk" ) == 0 ||
			std::strcmp( highway, "trunk_link" ) == 0 ||
			std::strcmp( highway, "primary" ) == 0 ||
			std::strcmp( highway, "primary_link" ) == 0 )
		{
				 if( lane_count == 0u )
				result.linear_object_class= LinearObjectClass::RoadSignificance3Lanes2;
			else if( lane_count <= 1u )
				result.linear_object_class= LinearObjectClass::RoadSignificance3Lanes1;
			else if( lane_count <= 2u )
				result.linear_object_class= LinearObjectClass::RoadSignificance3Lanes2;
			else if( lane_count <= 4u )
				result.linear_object_class= LinearObjectClass::RoadSignificance3Lanes4;
			else if( lane_count <= 6u )
				result.linear_object_class= LinearObjectClass::RoadSignificance3Lanes6;
			else if( lane_count <= 8u )
				result.linear_object_class= LinearObjectClass::RoadSignificance3Lanes8;
			else
				result.linear_object_class= LinearObjectClass::RoadSignificance3Lanes10More;
		}
		else if(
			std::strcmp( highway, "pedestrian" ) == 0 ||
			std::strcmp( highway, "footway" ) == 0 ||
			std::strcmp( highway, "path" ) == 0 ||
			std::strcmp( highway, "steps" ) == 0 ) // TODO - make spearate class for stairs.
			result.linear_object_class= LinearObjectClass::Pedestrian;

		if( std::strcmp( highway, "pedestrian" ) == 0 )
		{
			if( const char* const area= GetTagValue( way_element, "area" ) )
			{
				if( std::strcmp( area, "yes" ) == 0 )
				{
					result.areal_object_class= ArealObjectClass::PedestrianArea;
					result.linear_object_class= LinearObjectClass::None;
				}
			}
		}
	}
	else if( const char* const waterway= GetTagValue( way_element, "waterway" ) )
	{
		if( std::strcmp( waterway, "stream" ) == 0 )
			result.linear_object_class= LinearObjectClass::Waterway;
	}
	else if( const char* const railway= GetTagValue( way_element, "railway" ) )
	{
		if( std::strcmp( railway, "rail" ) == 0 )
		{
			if( const char* const usage= GetTagValue( way_element, "usage" ) )
			{
				if( std::strcmp( usage, "main" ) == 0 )
					result.linear_object_class= LinearObjectClass::Railway;
				else
					result.linear_object_class= LinearObjectClass::RailwaySecondary;
			}
			else
				result.linear_object_class= LinearObjectClass::RailwaySecondary;
		}
		else if( std::strcmp( railway, "monorail" ) == 0 )
			result.linear_object_class= LinearObjectClass::Monorail;
		else if( std::strcmp( railway, "tram" ) == 0 )
		{
			if( const char* const service= GetTagValue( way_element, "service" ) )
			{
				if(
					std::strcmp( service, "yard" ) == 0 ||
					std::strcmp( service, "siding" ) == 0 ||
					std::strcmp( service, "spur" ) == 0 )
					result.linear_object_class= LinearObjectClass::TramSecondary;
				else
					result.linear_object_class= LinearObjectClass::Tram;
			}
			else
				result.linear_object_class= LinearObjectClass::Tram;
		}
	}
	else if( const char* const barrier= GetTagValue( way_element, "barrier" ) )
	{
		if( std::strcmp( barrier, "cable_barrier" ) == 0 ||
			std::strcmp( barrier, "city_wall" ) == 0 ||
			std::strcmp( barrier, "fence" ) == 0 ||
			std::strcmp( barrier, "hedge" ) == 0 ||
			std::strcmp( barrier, "wall" ) == 0 ||
			std::strcmp( barrier, "hampshire_gate" ) == 0 )
			result.linear_object_class= LinearObjectClass::Barrier;
	}
	else if( const char* const building= GetTagValue( way_element, "building" ) )
	{
		(void)building;
		result.areal_object_class= ArealObjectClass::Building;
	}
	else if( const char* const natural= GetTagValue( way_element, "natural" ) )
	{
		if( std::strcmp( natural, "water" ) == 0 )
			result.areal_object_class= ArealObjectClass::Water;
		else if( std::strcmp( natural, "wood" ) == 0 )
			result.areal_object_class= ArealObjectClass::Wood;
		else if( std::strcmp( natural, "scrub" ) == 0 )
			result.areal_object_class= ArealObjectClass::Wood;
		else if( std::strcmp( natural, "grassland" ) == 0 )
			result.areal_object_class= ArealObjectClass::Grassland;
		else if( std::strcmp( natural, "heath" ) == 0 )
			result.areal_object_class= ArealObjectClass::Grassland;
	}
	else if( const char* const landuse= GetTagValue( way_element, "landuse" ) )
	{
			if( std::strcmp( landuse, "basin" ) == 0 )
			result.areal_object_class= ArealObjectClass::Water;
		else if( std::strcmp( landuse, "cemetery" ) == 0 )
			result.areal_object_class= ArealObjectClass::Cemetery;
		else if( std::strcmp( landuse, "foreset" ) == 0 )
			result.areal_object_class= ArealObjectClass::Wood;
		else if( std::strcmp( landuse, "wood" ) == 0 ||
				 std::strcmp( landuse, "orchard" ) == 0 )
			result.areal_object_class= ArealObjectClass::Wood;
		else if( std::strcmp( landuse, "grass" ) == 0 ||
				 std::strcmp( landuse, "meadow" ) == 0 ||
				 std::strcmp( landuse, "village_green" ) == 0 )
			result.areal_object_class= ArealObjectClass::Grassland;
		else if( std::strcmp( landuse, "residential" ) == 0 )
			result.areal_object_class= ArealObjectClass::Residential;
		else if( std::strcmp( landuse, "industrial" ) == 0 ||
				 std::strcmp( landuse, "garages" ) == 0 ||
				 std::strcmp( landuse, "railway" ) == 0 ||
				 std::strcmp( landuse, "construction" ) == 0 )
			result.areal_object_class= ArealObjectClass::Industrial;
		else if( std::strcmp( landuse, "commercial" ) == 0 ||
				 std::strcmp( landuse, "retail" ) == 0 )
			result.areal_object_class= ArealObjectClass::Administrative;
		else if( std::strcmp( landuse, "recreation_ground" ) == 0 ||
				 std::strcmp( landuse, "garden" ) == 0 )
			result.areal_object_class= ArealObjectClass::Park;
		else if( std::strcmp( landuse, "farmland" ) == 0 ||
				 std::strcmp( landuse, "farmyard" ) == 0 ||
				 std::strcmp( landuse, "greenhouse_horticulture" ) == 0 )
			result.areal_object_class= ArealObjectClass::Field;
	}
	else if( const char* const amenity= GetTagValue( way_element, "amenity" ) )
	{
		if( std::strcmp( amenity, "grave_yard" ) == 0 )
			result.areal_object_class= ArealObjectClass::Cemetery;
		else if(std::strcmp( amenity, "school" ) == 0 ||
			std::strcmp( amenity, "college" ) == 0  ||
			std::strcmp( amenity, "kindergarten" ) == 0 ||
			std::strcmp( amenity, "library" ) == 0 ||
			std::strcmp( amenity, "university" ) == 0 )
			result.areal_object_class= ArealObjectClass::Administrative;
		else if( std::strcmp( amenity, "clinic" ) == 0 ||
			std::strcmp( amenity, "dentist" ) == 0 ||
			std::strcmp( amenity, "doctors" ) == 0 ||
			std::strcmp( amenity, "hospital" ) == 0 ||
			std::strcmp( amenity, "ursing_home" ) == 0 )
			result.areal_object_class= ArealObjectClass::Administrative;
	}
	else if( const char* const leisure= GetTagValue( way_element, "leisure" ) )
	{
		if( std::strcmp( leisure, "park" ) == 0 )
			result.areal_object_class= ArealObjectClass::Park;
		else if( std::strcmp( leisure, "pitch" ) == 0 )
			result.areal_object_class= ArealObjectClass::SportArea;
	}

	return result;
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
		const WayClassifyResult way_classes= ClassifyWay( *way_element );
		if( way_classes.linear_object_class != LinearObjectClass::None )
		{
			OSMParseResult::LinearObject obj;
			obj.class_= way_classes.linear_object_class;
			obj.first_vertex_index= result.vertices.size();
			ExtractVertices( way_element, nodes, result.vertices );
			obj.vertex_count= result.vertices.size() - obj.first_vertex_index;
			if( obj.vertex_count > 0u )
				result.linear_objects.push_back(obj);
		}
		if( way_classes.areal_object_class != ArealObjectClass::None )
		{
			OSMParseResult::ArealObject obj;
			obj.class_= way_classes.areal_object_class;
			obj.first_vertex_index= result.vertices.size();
			ExtractVertices( way_element, nodes, result.vertices );
			obj.vertex_count= result.vertices.size() - obj.first_vertex_index;
			if( obj.vertex_count > 0u )
				result.areal_objects.push_back(obj);
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

		if( const char* const public_transport= GetTagValue( *node_element, "public_transport" ) )
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
		else if( const char* const highway= GetTagValue( *node_element, "highway" ) )
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
		if( const char* const railway= GetTagValue( *node_element, "railway" ) )
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
