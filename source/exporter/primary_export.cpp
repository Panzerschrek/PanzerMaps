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
			if( const OsmId id= ParseOsmId( ref_attrib ) )
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
	// All may be non-None.
	PointObjectClass point_object_class= PointObjectClass::None;
	LinearObjectClass linear_object_class= LinearObjectClass::None;
	ArealObjectClass areal_object_class= ArealObjectClass::None;
	size_t z_level= g_zero_z_level;
};

WayClassifyResult ClassifyWay( const tinyxml2::XMLElement& way_element, const bool is_multipolygon )
{
	WayClassifyResult result;

	if( const char* const layer= GetTagValue( way_element, "layer" ) )
	{
		const int layer_value= std::atoi(layer);
		result.z_level= size_t( std::max( 0, std::min( layer_value + int(g_zero_z_level), int(g_max_z_level) ) ) );
	}

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
			else if( lane_count <= 3u )
				result.linear_object_class= LinearObjectClass::RoadSignificance1Lanes3;
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
			else if( lane_count <= 3u )
				result.linear_object_class= LinearObjectClass::RoadSignificance2Lanes3;
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
			else if( lane_count <= 3u )
				result.linear_object_class= LinearObjectClass::RoadSignificance3Lanes3;
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

		if( result.z_level < g_zero_z_level )
		{
			if( result.linear_object_class == LinearObjectClass::Pedestrian )
				result.linear_object_class= LinearObjectClass::PedestrianUnderground;
			else if( result.linear_object_class == LinearObjectClass::RoadSignificance0 )
				result.linear_object_class= LinearObjectClass::RoadUndergroundLanes1;
			else if( lane_count == 0u )
				result.linear_object_class= LinearObjectClass::RoadUndergroundLanes2;
			else if( lane_count <= 1u )
				result.linear_object_class= LinearObjectClass::RoadUndergroundLanes1;
			else if( lane_count <= 2u )
				result.linear_object_class= LinearObjectClass::RoadUndergroundLanes2;
			else if( lane_count <= 3u )
				result.linear_object_class= LinearObjectClass::RoadUndergroundLanes3;
			else if( lane_count <= 4u )
				result.linear_object_class= LinearObjectClass::RoadUndergroundLanes4;
			else if( lane_count <= 6u )
				result.linear_object_class= LinearObjectClass::RoadUndergroundLanes6;
			else
				result.linear_object_class= LinearObjectClass::RoadUndergroundLanes8More;
		}
	}
	else if( const char* const waterway= GetTagValue( way_element, "waterway" ) )
	{
		// Create linear object for any "waterway"="river".
		// Large rivers also have areal objects, like "waterway"="riverbank" or "natural"="water", so, linear object will be drawn atop of areal.

		if( std::strcmp( waterway, "stream" ) == 0 ||
			std::strcmp( waterway, "river" ) == 0 )
			result.linear_object_class= LinearObjectClass::Waterway;
		else if( std::strcmp( waterway, "riverbank" ) == 0 )
			result.areal_object_class= ArealObjectClass::Water;
	}
	else if( const char* const railway= GetTagValue( way_element, "railway" ) )
	{
		bool is_secondary= false;
		if( const char* const service= GetTagValue( way_element, "service" ) )
			is_secondary=
				std::strcmp( service, "yard" ) == 0 ||
				std::strcmp( service, "siding" ) == 0 ||
				std::strcmp( service, "spur" ) == 0;

		if( std::strcmp( railway, "rail" ) == 0 )
		{
			if( const char* const usage= GetTagValue( way_element, "usage" ) )
			{
				if( std::strcmp( usage, "main" ) == 0 )
					result.linear_object_class= LinearObjectClass::Railway;
				else
					result.linear_object_class= is_secondary ? LinearObjectClass::RailwaySecondary : LinearObjectClass::Railway;
			}
			else
				result.linear_object_class= is_secondary ? LinearObjectClass::RailwaySecondary : LinearObjectClass::Railway;
		}
		else if( std::strcmp( railway, "monorail" ) == 0 )
			result.linear_object_class= LinearObjectClass::Monorail;
		else if( std::strcmp( railway, "tram" ) == 0 )
			result.linear_object_class= is_secondary ? LinearObjectClass::TramSecondary : LinearObjectClass::Tram;
	}
	else if( const char* const building= GetTagValue( way_element, "building" ) )
	{
		result.areal_object_class= ArealObjectClass::Building;

		// Create "church" and "mosque" only for specialized religious buildings.
		// Do not create such point objects for religious places in regular buildings.
		if( std::strcmp( building, "church" ) == 0 ||
			std::strcmp( building, "chapel" ) == 0 ||
			std::strcmp( building, "cathedral" ) == 0  )
			result.point_object_class= PointObjectClass::Church;
		else if( std::strcmp( building, "mosque" ) == 0 )
			result.point_object_class= PointObjectClass::Mosque;
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
		else if( std::strcmp( natural, "beach" ) == 0 )
			result.areal_object_class= ArealObjectClass::Sand;
		else if( std::strcmp( natural, "sand" ) == 0 )
			result.areal_object_class= ArealObjectClass::Sand;
		else if( std::strcmp( natural, "wetland" ) == 0 )
			result.areal_object_class= ArealObjectClass::Wetland;
	}
	else if( const char* const landuse= GetTagValue( way_element, "landuse" ) )
	{
			if( std::strcmp( landuse, "basin" ) == 0 )
			result.areal_object_class= ArealObjectClass::Water;
		else if( std::strcmp( landuse, "cemetery" ) == 0 )
			result.areal_object_class= ArealObjectClass::Cemetery;
		else if( std::strcmp( landuse, "forest" ) == 0 ||
				 std::strcmp( landuse, "wood" ) == 0 ||
				 std::strcmp( landuse, "orchard" ) == 0 ||
				 std::strcmp( landuse, "plant_nursery" ) == 0 ||
				 std::strcmp( landuse, "vineyard" ) == 0 )
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
				 std::strcmp( landuse, "construction" ) == 0 ||
				 std::strcmp( landuse, "landfill" ) == 0 )
			result.areal_object_class= ArealObjectClass::Industrial;
		else if( std::strcmp( landuse, "commercial" ) == 0 ||
				 std::strcmp( landuse, "retail" ) == 0 ||
				 std::strcmp( landuse, "religious " ) == 0 )
			result.areal_object_class= ArealObjectClass::PublicArea;
		else if( std::strcmp( landuse, "recreation_ground" ) == 0 ||
				 std::strcmp( landuse, "garden" ) == 0 )
			result.areal_object_class= ArealObjectClass::Park;
		else if( std::strcmp( landuse, "farmland" ) == 0 ||
				 std::strcmp( landuse, "farmyard" ) == 0 ||
				 std::strcmp( landuse, "greenhouse_horticulture" ) == 0 )
			result.areal_object_class= ArealObjectClass::Field;
		else if( std::strcmp( landuse, "allotments" ) == 0 )
			result.areal_object_class= ArealObjectClass::Allotments;
	}
	else if( const char* const amenity= GetTagValue( way_element, "amenity" ) )
	{
		if( std::strcmp( amenity, "grave_yard" ) == 0 )
			result.areal_object_class= ArealObjectClass::Cemetery;
		else if(
			std::strcmp( amenity, "bar" ) == 0 ||
			std::strcmp( amenity, "cafe" ) == 0 ||
			std::strcmp( amenity, "fast_food" ) == 0 ||
			std::strcmp( amenity, "food_court" ) == 0 ||
			std::strcmp( amenity, "pub" ) == 0 ||
			std::strcmp( amenity, "restaurant" ) == 0 ||
			std::strcmp( amenity, "college" ) == 0  ||
			std::strcmp( amenity, "driving_school" ) == 0  ||
			std::strcmp( amenity, "kindergarten" ) == 0 ||
			std::strcmp( amenity, "library" ) == 0 ||
			std::strcmp( amenity, "school" ) == 0 ||
			std::strcmp( amenity, "university" ) == 0 ||
			std::strcmp( amenity, "clinic" ) == 0 ||
			std::strcmp( amenity, "dentist" ) == 0 ||
			std::strcmp( amenity, "doctors" ) == 0 ||
			std::strcmp( amenity, "hospital" ) == 0 ||
			std::strcmp( amenity, "ursing_home" ) == 0 ||
			std::strcmp( amenity, "pharmacy" ) == 0 ||
			std::strcmp( amenity, "social_facility" ) == 0 ||
			std::strcmp( amenity, "veterinary" ) == 0 ||
			std::strcmp( amenity, "bank" ) == 0 ||
			std::strcmp( amenity, "arts_centre" ) == 0 ||
			std::strcmp( amenity, "brothel" ) == 0 ||
			std::strcmp( amenity, "casino" ) == 0 ||
			std::strcmp( amenity, "cinema" ) == 0 ||
			std::strcmp( amenity, "community_centre" ) == 0 ||
			std::strcmp( amenity, "gambling" ) == 0 ||
			std::strcmp( amenity, "nightclub" ) == 0 ||
			std::strcmp( amenity, "planetarium" ) == 0 ||
			std::strcmp( amenity, "social_centre" ) == 0 ||
			std::strcmp( amenity, "theatre" ) == 0 ||
			std::strcmp( amenity, "courthouse" ) == 0 ||
			std::strcmp( amenity, "crematorium" ) == 0 ||
			std::strcmp( amenity, "embassy" ) == 0 ||
			std::strcmp( amenity, "fire_station" ) == 0 ||
			std::strcmp( amenity, "marketplace" ) == 0 ||
			std::strcmp( amenity, "police" ) == 0 ||
			std::strcmp( amenity, "post_depot" ) == 0 ||
			std::strcmp( amenity, "post_office" ) == 0 ||
			std::strcmp( amenity, "public_bath" ) == 0 ||
			std::strcmp( amenity, "townhall" ) == 0 )
			result.areal_object_class= ArealObjectClass::PublicArea;
		else if(std::strcmp( amenity, "parking" ) == 0 )
			result.areal_object_class= ArealObjectClass::Parking;
	}
	else if( const char* const leisure= GetTagValue( way_element, "leisure" ) )
	{
		if( std::strcmp( leisure, "park" ) == 0 )
			result.areal_object_class= ArealObjectClass::Park;
		else if( std::strcmp( leisure, "pitch" ) == 0 )
			result.areal_object_class= ArealObjectClass::SportArea;
		else if( std::strcmp( leisure, "stadium" ) == 0 )
			result.areal_object_class= ArealObjectClass::Park; // Stadium area is like park.
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
	{
		Log::Warning( "Can not close some ways" );
		for( auto& way : ways )
			out_closed_ways.push_back( std::move(way) );
	}
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

	NodesMap nodes;
	std::unordered_map< OsmId, const tinyxml2::XMLElement* > ways_map;

	for( const tinyxml2::XMLElement* node_element= doc.RootElement()->FirstChildElement( "node" ); node_element != nullptr; node_element= node_element->NextSiblingElement( "node" ) )
	{
		const char* const lon_str= node_element->Attribute( "lon" );
		const char* const lat_str= node_element->Attribute( "lat" );
		const char* const id_str= node_element->Attribute( "id" );
		GeoPoint node_geo_point;

		if( !( lon_str != nullptr && lat_str != nullptr && id_str != nullptr ) )
			continue;
		if( !( std::sscanf( lon_str, "%lf", &node_geo_point.x ) == 1 && std::sscanf( lat_str, "%lf", &node_geo_point.y ) == 1 ) )
			continue;
		const OsmId id= ParseOsmId( id_str );
		if( id == 0 )
			continue;

		nodes[id]= node_geo_point;

		OSMParseResult::PointObject obj;

		if( const char* const railway= GetTagValue( *node_element, "railway" ) )
		{
			if( std::strcmp( railway, "subway_entrance" ) == 0 )
				obj.class_= PointObjectClass::SubwayEntrance;
			else if( std::strcmp( railway, "tram_stop" ) == 0 )
				obj.class_= PointObjectClass::TramStop;
			else if( std::strcmp( railway, "station" ) == 0 )
			{
				const char* const station= GetTagValue( *node_element, "station" );
				const char* const subway= GetTagValue( *node_element, "subway" );

				if( !( subway != nullptr || ( station != nullptr && std::strcmp( station, "subway" ) == 0 ) ) )
					obj.class_= PointObjectClass::RailwayStation;
			}
		}
		else if( const char* const public_transport= GetTagValue( *node_element, "public_transport" ) )
		{
			if( std::strcmp( public_transport, "platform" ) == 0 )
				obj.class_= PointObjectClass::BusStop;
		}
		else if( const char* const highway= GetTagValue( *node_element, "highway" ) )
		{
			if( std::strcmp( highway, "bus_stop" ) == 0 )
				obj.class_= PointObjectClass::BusStop;
		}
		else if( const char* const historic= GetTagValue( *node_element, "historic" ) )
		{
			if( std::strcmp( historic, "memorial" ) == 0 )
			{
				const char* const memorial= GetTagValue( *node_element, "memorial" );
				if( memorial != nullptr && std::strcmp( memorial, "statue" ) == 0 )
					obj.class_= PointObjectClass::MemorialStatue;
				else if( memorial != nullptr && std::strcmp( memorial, "stone" ) == 0 )
					obj.class_= PointObjectClass::Stone;
				else
					obj.class_= PointObjectClass::Memorial;
			}
		}
		else if( const char* const power= GetTagValue( *node_element, "power" ) )
		{
			if( std::strcmp( power, "tower" ) == 0 )
				obj.class_= PointObjectClass::PowerTower;
		}
		else if( const char* const natural= GetTagValue( *node_element, "natural" ) )
		{
			if( std::strcmp( natural, "peak" ) == 0 ||
				std::strcmp( natural, "volkano" ) == 0 )
				obj.class_= PointObjectClass::MountainTop;
			else if( std::strcmp( natural, "stone" ) == 0 )
				obj.class_= PointObjectClass::Stone;
		}
		else if( const char* const waterway= GetTagValue( *node_element, "waterway" ) )
		{
			if( std::strcmp( waterway, "waterfall" ) == 0 )
				obj.class_= PointObjectClass::Waterfall;
		}

		if( obj.class_ != PointObjectClass::None )
		{
			obj.vertex_index= result.vertices.size();
			result.vertices.push_back( node_geo_point );
			result.point_objects.push_back(obj);
		}
	}

	std::vector<GeoPoint> tmp_points;
	for( const tinyxml2::XMLElement* way_element= doc.RootElement()->FirstChildElement( "way" ); way_element != nullptr; way_element= way_element->NextSiblingElement( "way" ) )
	{
		if( const char* const id_str= way_element->Attribute("id") )
			if( const OsmId id= ParseOsmId( id_str ) )
				ways_map[id]= way_element;

		const WayClassifyResult classify_result= ClassifyWay( *way_element, false );
		if( classify_result.point_object_class != PointObjectClass::None )
		{
			tmp_points.clear();
			ExtractVertices( way_element, nodes, tmp_points );
			if( !tmp_points.empty() )
			{
				// TODO - calculate centroid of polygon.
				GeoPoint geo_point{ 0.0, 0.0 };
				for( const GeoPoint& way_point : tmp_points )
				{
					geo_point.x+= way_point.x;
					geo_point.y+= way_point.y;
				}
				geo_point.x/= double(tmp_points.size());
				geo_point.y/= double(tmp_points.size());

				OSMParseResult::PointObject obj;
				obj.class_= classify_result.point_object_class;
				obj.vertex_index= result.vertices.size();
				result.vertices.push_back( geo_point );
				result.point_objects.push_back(obj);
			}
		}
		if( classify_result.linear_object_class != LinearObjectClass::None )
		{
			OSMParseResult::LinearObject obj;
			obj.class_= classify_result.linear_object_class;
			obj.first_vertex_index= result.vertices.size();
			obj.z_level= classify_result.z_level;
			ExtractVertices( way_element, nodes, result.vertices );
			obj.vertex_count= result.vertices.size() - obj.first_vertex_index;
			if( obj.vertex_count > 0u )
				result.linear_objects.push_back(obj);
		}
		if( classify_result.areal_object_class != ArealObjectClass::None )
		{
			OSMParseResult::ArealObject obj;
			obj.class_= classify_result.areal_object_class;
			obj.first_vertex_index= result.vertices.size();
			ExtractVertices( way_element, nodes, result.vertices );
			obj.vertex_count= result.vertices.size() - obj.first_vertex_index;
			if( obj.vertex_count > 0u )
				result.areal_objects.push_back( std::move(obj) );
		}
	}

	// Extract multipolygons.
	for( const tinyxml2::XMLElement* relation_element= doc.RootElement()->FirstChildElement( "relation" ); relation_element != nullptr; relation_element= relation_element->NextSiblingElement( "relation" ) )
	{
		const char* const type= GetTagValue( *relation_element, "type" );
		if( type == nullptr || std::strcmp( type, "multipolygon" ) != 0 )
			continue;

		const WayClassifyResult classify_result= ClassifyWay( *relation_element, true );
		if( classify_result.areal_object_class == ArealObjectClass::None && classify_result.linear_object_class == LinearObjectClass::None )
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

			if( classify_result.linear_object_class != LinearObjectClass::None )
			{
				OSMParseResult::LinearObject obj;
				obj.class_= classify_result.linear_object_class;
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

		if( !outer_ways.empty() && classify_result.areal_object_class != ArealObjectClass::None )
		{
			OSMParseResult::ArealObject obj;
			obj.class_= classify_result.areal_object_class;
			obj.first_vertex_index= obj.vertex_count= 0u;

			obj.multipolygon.reset( new OSMParseResult::Multipolygon );
			CreateMultipolygon( *obj.multipolygon, result.vertices, outer_ways, inner_ways );

			if( !obj.multipolygon->outer_rings.empty() )
				result.areal_objects.push_back( std::move(obj) );
		}

		if( !outer_ways.empty() && classify_result.point_object_class != PointObjectClass::None )
		{
			tmp_points.clear();
			for( const std::vector<GeoPoint>& outer_way : outer_ways )
			for( const GeoPoint& geo_point : outer_way )
				tmp_points.push_back(geo_point);

			if( !tmp_points.empty() )
			{
				// TODO - calculate centroid of polygon.
				GeoPoint geo_point{ 0.0, 0.0 };
				for( const GeoPoint& way_point : tmp_points )
				{
					geo_point.x+= way_point.x;
					geo_point.y+= way_point.y;
				}
				geo_point.x/= double(tmp_points.size());
				geo_point.y/= double(tmp_points.size());

				OSMParseResult::PointObject obj;
				obj.class_= classify_result.point_object_class;
				obj.vertex_index= result.vertices.size();
				result.vertices.push_back( geo_point );
				result.point_objects.push_back(obj);
			}
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
