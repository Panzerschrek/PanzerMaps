#include <unordered_map>
#include <tinyxml2.h>
#include "../common/assert.hpp"
#include "../common/log.hpp"
#include "../common/memory_mapped_file.hpp"
#include "primary_export.hpp"

namespace PanzerMaps
{

using OsmId= uint64_t; // Valid Ids starts with '1'.
using NodesMap= std::unordered_map<OsmId, GeoPoint>;

static OsmId ParseOsmId( const char* const id_str )
{
	OsmId id;
	if( std::sscanf( id_str, "%lu", &id ) == 1 )
		return id;
	return 0;
}

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
			if( const OsmId id= ParseOsmId( id_str ) )
			{
				double lon;
				double lat;

				if( std::sscanf( lon_str, "%lf", &lon ) == 1 &&
					std::sscanf( lat_str, "%lf", &lat ) == 1 )
					result.insert( std::make_pair( id, GeoPoint{ lon, lat } ) );
			}
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

WayClassifyResult ClassifyWay( const tinyxml2::XMLElement& way_element, const bool is_multipolygon )
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
		{
			const char* const area= GetTagValue( way_element, "area" );
			if( is_multipolygon || ( area != nullptr && std::strcmp( area, "yes" ) == 0 ) )
				result.areal_object_class= ArealObjectClass::PedestrianArea;
			else
				result.linear_object_class= LinearObjectClass::Pedestrian;
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

	if( const char* const barrier= GetTagValue( way_element, "barrier" ) )
	{
		if( std::strcmp( barrier, "cable_barrier" ) == 0 ||
			std::strcmp( barrier, "city_wall" ) == 0 ||
			std::strcmp( barrier, "fence" ) == 0 ||
			std::strcmp( barrier, "hedge" ) == 0 ||
			std::strcmp( barrier, "wall" ) == 0 ||
			std::strcmp( barrier, "hampshire_gate" ) == 0 )
			result.linear_object_class= LinearObjectClass::Barrier;
	}

	return result;
}

static std::vector< std::vector<GeoPoint> > CreateClosedWays(
	std::vector< std::vector<GeoPoint> > ways )
{
	std::vector< std::vector<GeoPoint> > out_closed_ways;

	// Search closed ways.
	for( size_t i= 0u; i < ways.size();  )
	{
		PM_ASSERT( !ways[i].empty() );
		if( ways[i].front() == ways[i].back() )
		{
			// Way already closed.
			ways[i].pop_back();
			out_closed_ways.push_back( std::move( ways[i] ) );
			if( i +1u < ways.size() )
				ways[i]= std::move(ways.back());
			ways.pop_back();
		}
		else
			++i;
	}

	while( true )
	{
		bool ways_combined= false;
		for( size_t w0= 0u; w0 < ways.size(); ++w0 )
		for( size_t w1= w0 + 1u; w1 < ways.size(); ++w1 )
		{
			std::vector<GeoPoint> combined_way;

			if( ways[w0].front() == ways[w1].front() )
			{
				combined_way.reserve( ways[w0].size() + ways[w1].size() );
				combined_way.insert( combined_way.end(), ways[w0].rbegin(), ways[w0].rend() );
				combined_way.pop_back();
				combined_way.insert( combined_way.end(), ways[w1].begin(), ways[w1].end() );

			}
			else if( ways[w0].front() == ways[w1].back() )
			{
				combined_way.reserve( ways[w0].size() + ways[w1].size() );
				combined_way.insert( combined_way.end(), ways[w1].begin(), ways[w1].end() );
				combined_way.pop_back();
				combined_way.insert( combined_way.end(), ways[w0].begin(), ways[w0].end() );

			}
			else if( ways[w0].back() == ways[w1].front() )
			{
				combined_way.reserve( ways[w0].size() + ways[w1].size() );
				combined_way.insert( combined_way.end(), ways[w0].begin(), ways[w0].end() );
				combined_way.pop_back();
				combined_way.insert( combined_way.end(), ways[w1].begin(), ways[w1].end() );

			}
			else if( ways[w0].back() == ways[w1].back() )
			{
				combined_way.reserve( ways[w0].size() + ways[w1].size() );
				combined_way.insert( combined_way.end(), ways[w0].begin(), ways[w0].end() );
				combined_way.pop_back();
				combined_way.insert( combined_way.end(), ways[w1].rbegin(), ways[w1].rend() );
			}

			if( !combined_way.empty() )
			{
				ways_combined= true;
				PM_ASSERT( combined_way.size() == ways[w0].size() + ways[w1].size() - 1u );
				if( combined_way.front() == combined_way.back() )
				{
					combined_way.pop_back();
					out_closed_ways.push_back( std::move(combined_way) );

					if( w0 + 1u < ways.size() )
						ways[w0]= std::move( ways.back() );
					ways.pop_back();
					if( w1 + 1u < ways.size() )
						ways[w1]= std::move( ways.back() );
					ways.pop_back();
				}
				else
				{
					ways[w0]= std::move(combined_way);
					if( w1 + 1u < ways.size() )
						ways[w1]= std::move( ways.back() );
					ways.pop_back();
				}
				goto continue_ways_connecting;
			}
		}

		continue_ways_connecting:
		if( !ways_combined )
			break;
	}

	if( !ways.empty() )
		Log::Warning( "Can not close some ways" );
	return out_closed_ways;
}

static void CreateMultipolygon(
	OSMParseResult::Multipolygon& out_multipolygon,
	std::vector<GeoPoint>& out_vertices,
	const std::vector< std::vector<GeoPoint> >& outer_ways,
	const std::vector< std::vector<GeoPoint> >& inner_ways )
{
	for( const std::vector<GeoPoint>& outer_way : CreateClosedWays( outer_ways ) )
	{
		out_multipolygon.outer_rings.emplace_back();
		OSMParseResult::Multipolygon::Part& part= out_multipolygon.outer_rings.back();
		part.first_vertex_index= out_vertices.size();
		part.vertex_count= outer_way.size();
		out_vertices.insert( out_vertices.end(), outer_way.begin(), outer_way.end() );
	}
	for( const std::vector<GeoPoint>& inner_way : CreateClosedWays( inner_ways ) )
	{
		out_multipolygon.inner_rings.emplace_back();
		OSMParseResult::Multipolygon::Part& part= out_multipolygon.inner_rings.back();
		part.first_vertex_index= out_vertices.size();
		part.vertex_count= inner_way.size();
		out_vertices.insert( out_vertices.end(), inner_way.begin(), inner_way.end() );
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

	std::unordered_map< OsmId, const tinyxml2::XMLElement* > ways_map;

	for( const tinyxml2::XMLElement* way_element= doc.RootElement()->FirstChildElement( "way" ); way_element != nullptr; way_element= way_element->NextSiblingElement( "way" ) )
	{
		if( const char* const id_str= way_element->Attribute("id") )
			if( const OsmId id= ParseOsmId( id_str ) )
				ways_map[id]= way_element;

		const WayClassifyResult way_classes= ClassifyWay( *way_element, false );
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
				result.areal_objects.push_back( std::move(obj) );
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

	// Extract multipolygons.
	for( const tinyxml2::XMLElement* relation_element= doc.RootElement()->FirstChildElement( "relation" ); relation_element != nullptr; relation_element= relation_element->NextSiblingElement( "relation" ) )
	{
		const char* const type= GetTagValue( *relation_element, "type" );
		if( type == nullptr || std::strcmp( type, "multipolygon" ) != 0 )
			continue;

		const WayClassifyResult way_classes= ClassifyWay( *relation_element, true );
		if( way_classes.areal_object_class == ArealObjectClass::None && way_classes.linear_object_class == LinearObjectClass::None )
			continue;

		std::vector< std::vector<GeoPoint> > outer_ways, inner_ways;

		for( const tinyxml2::XMLElement* member_element= relation_element->FirstChildElement("member"); member_element != nullptr; member_element= member_element->NextSiblingElement( "member" ) )
		{
			const char* const member_type= member_element->Attribute( "type" );
			if( member_type == nullptr || std::strcmp( member_type, "way" ) != 0 )
				continue;

			const char* const ref= member_element->Attribute( "ref" );
			const char* const role= member_element->Attribute( "role" );
			if( ref == nullptr || role == nullptr )
				continue;

			const auto it= ways_map.find( ParseOsmId( ref ) );
			if( it == ways_map.end() )
				continue;

			if( way_classes.linear_object_class != LinearObjectClass::None )
			{
				OSMParseResult::LinearObject obj;
				obj.class_= way_classes.linear_object_class;
				obj.first_vertex_index= result.vertices.size();
				ExtractVertices( it->second, nodes, result.vertices );
				obj.vertex_count= result.vertices.size() - obj.first_vertex_index;
				if( obj.vertex_count > 0u )
					result.linear_objects.push_back(obj);
			}

			if( std::strcmp( role, "outer" ) == 0 )
			{
				outer_ways.emplace_back();
				ExtractVertices( it->second, nodes, outer_ways.back() );
			}
			if( std::strcmp( role, "inner" ) == 0 )
			{
				inner_ways.emplace_back();
				ExtractVertices( it->second, nodes, inner_ways.back() );
			}
		} // for multipolygon members.

		if( !outer_ways.empty() && way_classes.areal_object_class != ArealObjectClass::None )
		{
			OSMParseResult::ArealObject obj;
			obj.class_= way_classes.areal_object_class;
			obj.first_vertex_index= obj.vertex_count= 0u;

			obj.multipolygon.reset( new OSMParseResult::Multipolygon );
			CreateMultipolygon( *obj.multipolygon, result.vertices, outer_ways, inner_ways );

			if( !obj.multipolygon->outer_rings.empty() )
				result.areal_objects.push_back( std::move(obj) );
		}
	} // for relations.

	Log::Info( "Primary export: " );
	Log::Info( result.point_objects.size(), " point objects" );
	Log::Info( result.linear_objects.size(), " linear objects" );
	Log::Info( result.areal_objects.size(), " areal objects" );
	Log::Info( result.vertices.size(), " vertices" );
	Log::Info( "" );

	return result;
}

} // namespace PanzerMaps
