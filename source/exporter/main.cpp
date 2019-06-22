#include <iostream>
#include  <vector>
#include <unordered_map>

#include <tinyxml2.h>

struct Node
{
	double lon;
	double lat;
};

using Id= uint64_t; // Valid Ids starts with '1'.
using NodesMap= std::unordered_map<Id, Node>;

enum class PointObjectClass : uint8_t
{
	None,
	StationPlatform,
	SubwayEntrance,
};

enum class LinearObjectClass : uint8_t
{
	None,
	Road,
	Waterway,
};

enum class ArealObjectClass : uint8_t
{
	None,
	Building,
	Water,
	Wood,
	Grassland,
};

struct OSMParseResult
{
	struct PointObject
	{
		PointObjectClass class_= PointObjectClass::None;
		uint64_t vertex_index;
	};

	struct LinearObject
	{
		LinearObjectClass class_= LinearObjectClass::None;
		uint64_t first_vertex_index;
		uint64_t vertex_count;
	};

	struct ArealObject
	{
		ArealObjectClass class_= ArealObjectClass::None;
		uint64_t first_vertex_index;
		uint64_t vertex_count;
	};

	std::vector<PointObject> point_objects;
	std::vector<LinearObject> linear_objects;
	std::vector<ArealObject> areal_objects;
	std::vector<Node> vertices;
};

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
			Id id;
			double lon;
			double lat;

			if( std::sscanf( id_str, "%lu", &id ) == 1 &&
				std::sscanf( lon_str, "%lf", &lon ) == 1 &&
				std::sscanf( lat_str, "%lf", &lat ) == 1 )
				result.insert( std::make_pair( id, Node{ lon, lat } ) );
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

static void ExtractVertices( const tinyxml2::XMLElement* const way_element, const NodesMap& nodes, std::vector<Node>& out_vertices )
{
	for( const tinyxml2::XMLElement* nd_element= way_element->FirstChildElement( "nd" ); nd_element != nullptr; nd_element= nd_element->NextSiblingElement( "nd" ) )
	{
		if( const char* const ref_attrib= nd_element->Attribute( "ref" ) )
		{
			Id id;
			if( std::sscanf( ref_attrib, "%lu", &id ) == 1 )
			{
				const auto node_it= nodes.find(id);
				if( node_it != nodes.end() )
					out_vertices.push_back( node_it->second );
			}
		}

	}
}

OSMParseResult ParseOSM( const tinyxml2::XMLDocument& doc )
{
	const NodesMap nodes= ExtractNodes(doc);
	OSMParseResult result;

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
				std::strcmp( highway, "tertiary_link" ) == 0 )
				obj.class_= LinearObjectClass::Road;

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
			else if( std::strcmp( natural, "grassland" ) == 0 )
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
	}

	return result;
}

int main()
{
	tinyxml2::XMLDocument doc;
	doc.LoadFile( "maps_src/small_place.osm" );

	ParseOSM(doc);
}
