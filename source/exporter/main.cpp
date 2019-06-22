#include <algorithm>
#include <iostream>
#include <vector>
#include <unordered_map>

#include <tinyxml2.h>

#include "../common/data_file.hpp"
#include "../common/coordinates_conversion.hpp"

namespace PanzerMaps
{


using Id= uint64_t; // Valid Ids starts with '1'.
using NodesMap= std::unordered_map<Id, GeoPoint>;

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
		size_t vertex_index;
	};

	struct LinearObject
	{
		LinearObjectClass class_= LinearObjectClass::None;
		size_t first_vertex_index;
		size_t vertex_count;
	};

	struct ArealObject
	{
		ArealObjectClass class_= ArealObjectClass::None;
		size_t first_vertex_index;
		size_t vertex_count;
	};

	std::vector<PointObject> point_objects;
	std::vector<LinearObject> linear_objects;
	std::vector<ArealObject> areal_objects;
	std::vector<GeoPoint> vertices;
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

static std::vector<unsigned char> DumpDataChunk( const OSMParseResult& prepared_data )
{
	using namespace DataFileDescription;

	std::vector<unsigned char> result;

	result.resize( sizeof(DataFile), 0 );

	Chunk& chunk= *reinterpret_cast<Chunk*>( result.data() );

	MercatorPoint min_point;
	MercatorPoint max_point;
	if( !prepared_data.vertices.empty() )
		min_point= max_point= GeoPointToWebMercatorPoint( prepared_data.vertices.front() );
	else
		min_point.x= max_point.x= min_point.y= max_point.y= 0;

	// Convert input coordinates to web mercator, calculate bounding box.
	std::vector<MercatorPoint> prepared_data_vetices_converted;
	prepared_data_vetices_converted.reserve( prepared_data.vertices.size() );
	for( const GeoPoint& geo_point : prepared_data.vertices )
	{
		const MercatorPoint mercator_point= GeoPointToWebMercatorPoint(geo_point);
		min_point.x= std::min( min_point.x, mercator_point.x );
		min_point.y= std::min( min_point.y, mercator_point.y );
		max_point.x= std::max( max_point.x, mercator_point.x );
		max_point.y= std::max( max_point.y, mercator_point.y );
		prepared_data_vetices_converted.push_back( mercator_point );
	}

	const uint32_t coordinates_scale_log2= 4;

	std::vector<ChunkVertex> vertices;
	const ChunkVertex break_primitive_vertex{ std::numeric_limits<ChunkCoordType>::max(), std::numeric_limits<ChunkCoordType>::max() };
	{
		auto point_objects= prepared_data.point_objects;
		// Sort by class.
		std::sort(
			point_objects.begin(), point_objects.end(),
			[]( const OSMParseResult::PointObject& l, const OSMParseResult::PointObject& r )
			{
				return l.class_ < r.class_;
			} );


		PointObjectClass prev_class= PointObjectClass::None;
		Chunk::PointObjectGroup group;
		for( const OSMParseResult::PointObject& object : point_objects )
		{
			if( object.class_ != prev_class || &object == &point_objects.back() )
			{
				if( prev_class != PointObjectClass::None )
				{
					group.vertex_count= vertices.size() - group.first_vertex;
					result.insert(
						result.end(),
						reinterpret_cast<const unsigned char*>(&group),
						reinterpret_cast<const unsigned char*>(&group) + sizeof(group) );
				}

				group.first_vertex= vertices.size();
				group.style_index= static_cast<Chunk::StyleIndex>( prev_class );

				prev_class= object.class_;
			}

			const MercatorPoint& mercator_point= prepared_data_vetices_converted[object.vertex_index];
			const int32_t vertex_x= ( mercator_point.x - min_point.x ) >> coordinates_scale_log2;
			const int32_t vertex_y= ( mercator_point.x - min_point.x ) >> coordinates_scale_log2;
			vertices.push_back( ChunkVertex{ static_cast<ChunkCoordType>(vertex_x), static_cast<ChunkCoordType>(vertex_y) } );
		}
	}
	{
		auto linear_objects= prepared_data.linear_objects;
		// Sort by class.
		std::sort(
			linear_objects.begin(), linear_objects.end(),
			[]( const OSMParseResult::LinearObject& l, const OSMParseResult::LinearObject& r )
			{
				return l.class_ < r.class_;
			} );

		LinearObjectClass prev_class= LinearObjectClass::None;
		Chunk::LinearObjectGroup group;
		for( const OSMParseResult::LinearObject& object : linear_objects )
		{
			if( object.class_ != prev_class || &object == &linear_objects.back() )
			{
				if( prev_class != LinearObjectClass::None )
				{
					group.vertex_count= vertices.size() - group.first_vertex;
					result.insert(
						result.end(),
						reinterpret_cast<const unsigned char*>(&group),
						reinterpret_cast<const unsigned char*>(&group) + sizeof(group) );
				}

				group.first_vertex= vertices.size();
				group.style_index= static_cast<Chunk::StyleIndex>( prev_class );

				prev_class= object.class_;
			}

			for( size_t v= object.first_vertex_index; v < object.first_vertex_index + object.vertex_count; ++v )
			{
				const MercatorPoint& mercator_point= prepared_data_vetices_converted[v];
				const int32_t vertex_x= ( mercator_point.x - min_point.x ) >> coordinates_scale_log2;
				const int32_t vertex_y= ( mercator_point.x - min_point.x ) >> coordinates_scale_log2;
				vertices.push_back( ChunkVertex{ static_cast<ChunkCoordType>(vertex_x), static_cast<ChunkCoordType>(vertex_y) } );
			}
			vertices.push_back(break_primitive_vertex);
		}
	}
	{
		auto areal_objects= prepared_data.areal_objects;
		// Sort by class.
		std::sort(
			areal_objects.begin(), areal_objects.end(),
			[]( const OSMParseResult::ArealObject& l, const OSMParseResult::ArealObject& r )
			{
				return l.class_ < r.class_;
			} );

		ArealObjectClass prev_class= ArealObjectClass::None;
		Chunk::LinearObjectGroup group;
		for( const OSMParseResult::ArealObject& object : areal_objects )
		{
			if( object.class_ != prev_class || &object == &areal_objects.back() )
			{
				if( prev_class != ArealObjectClass::None )
				{
					group.vertex_count= vertices.size() - group.first_vertex;
					result.insert(
						result.end(),
						reinterpret_cast<const unsigned char*>(&group),
						reinterpret_cast<const unsigned char*>(&group) + sizeof(group) );
				}

				group.first_vertex= vertices.size();
				group.style_index= static_cast<Chunk::StyleIndex>( prev_class );

				prev_class= object.class_;
			}

			for( size_t v= object.first_vertex_index; v < object.first_vertex_index + object.vertex_count; ++v )
			{
				const MercatorPoint& mercator_point= prepared_data_vetices_converted[v];
				const int32_t vertex_x= ( mercator_point.x - min_point.x ) >> coordinates_scale_log2;
				const int32_t vertex_y= ( mercator_point.x - min_point.x ) >> coordinates_scale_log2;
				vertices.push_back( ChunkVertex{ static_cast<ChunkCoordType>(vertex_x), static_cast<ChunkCoordType>(vertex_y) } );
			}
			vertices.push_back(break_primitive_vertex);
		}
	}

	result.insert(
		result.end(),
		reinterpret_cast<const unsigned char*>( vertices.data() ),
		reinterpret_cast<const unsigned char*>( vertices.data() + vertices.size() ) );

	return result;
}

static std::vector<unsigned char> DumpDataFile( const OSMParseResult& prepared_data )
{
	using namespace DataFileDescription;

	std::vector<unsigned char> result;

	result.resize( sizeof(DataFile), static_cast<unsigned char>(0) );
	DataFile& data_file= *reinterpret_cast<DataFile*>( result.data() );

	std::memcpy( data_file.header, DataFile::c_expected_header, sizeof(data_file.header) );
	data_file.version= DataFile::c_expected_version;

	std::vector<unsigned char> chunk_data= DumpDataChunk( prepared_data );

	data_file.chunks_offset= result.size();
	data_file.chunk_count= 1u;
	result.insert( result.end(), chunk_data.begin(), chunk_data.end() );

	return result;
}

} // namespace PanzerMaps

int main()
{
	tinyxml2::XMLDocument doc;
	doc.LoadFile( "maps_src/small_place.osm" );

	PanzerMaps::DumpDataFile( PanzerMaps::ParseOSM(doc) );
}
