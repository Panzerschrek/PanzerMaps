#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>

#include "../common/data_file.hpp"
#include "final_export.hpp"

namespace PanzerMaps
{

static std::vector<unsigned char> DumpDataChunk( const OSMParseResult& prepared_data )
{
	using namespace DataFileDescription;

	std::vector<unsigned char> result;
	result.resize( sizeof(Chunk), 0 );

	const auto get_chunk= [&]() -> Chunk& { return *reinterpret_cast<Chunk*>( result.data() ); };
	get_chunk().point_object_groups_count= get_chunk().linear_object_groups_count= get_chunk().areal_object_groups_count= 0;

	MercatorPoint min_point;
	MercatorPoint max_point;
	if( !prepared_data.vertices.empty() )
		min_point= max_point= GeoPointToWebMercatorPoint( prepared_data.vertices.front() );
	else
		min_point.x= max_point.x= min_point.y= max_point.y= 0;

	// TODO - asssert, if data chunk contains >= 65535 vertices.

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
		get_chunk().point_object_groups_offset= static_cast<uint32_t>(result.size());

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
			if( object.class_ != prev_class )
			{
				if( prev_class != PointObjectClass::None )
				{
					group.vertex_count= static_cast<uint16_t>( vertices.size() - group.first_vertex );
					result.insert(
						result.end(),
						reinterpret_cast<const unsigned char*>(&group),
						reinterpret_cast<const unsigned char*>(&group) + sizeof(group) );
					++get_chunk().point_object_groups_count;
				}

				group.first_vertex= static_cast<uint16_t>( vertices.size() );
				group.style_index= static_cast<Chunk::StyleIndex>( object.class_ );

				prev_class= object.class_;
			}

			const MercatorPoint& mercator_point= prepared_data_vetices_converted[object.vertex_index];
			const int32_t vertex_x= ( mercator_point.x - min_point.x ) >> coordinates_scale_log2;
			const int32_t vertex_y= ( mercator_point.y - min_point.y ) >> coordinates_scale_log2;
			vertices.push_back( ChunkVertex{ static_cast<ChunkCoordType>(vertex_x), static_cast<ChunkCoordType>(vertex_y) } );
		}
		if( prev_class != PointObjectClass::None )
		{
			group.vertex_count= static_cast<uint16_t>( vertices.size() - group.first_vertex );
			result.insert(
				result.end(),
				reinterpret_cast<const unsigned char*>(&group),
				reinterpret_cast<const unsigned char*>(&group) + sizeof(group) );
			++get_chunk().point_object_groups_count;
		}
	}
	{
		get_chunk().linear_object_groups_offset= static_cast<uint32_t>(result.size());

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
			if( object.class_ != prev_class )
			{
				if( prev_class != LinearObjectClass::None )
				{
					group.vertex_count= static_cast<uint16_t>( vertices.size() - group.first_vertex );
					result.insert(
						result.end(),
						reinterpret_cast<const unsigned char*>(&group),
						reinterpret_cast<const unsigned char*>(&group) + sizeof(group) );
					++get_chunk().linear_object_groups_count;
				}

				group.first_vertex= static_cast<uint16_t>( vertices.size() );
				group.style_index= static_cast<Chunk::StyleIndex>( object.class_ );

				prev_class= object.class_;
			}

			for( size_t v= object.first_vertex_index; v < object.first_vertex_index + object.vertex_count; ++v )
			{
				const MercatorPoint& mercator_point= prepared_data_vetices_converted[v];
				const int32_t vertex_x= ( mercator_point.x - min_point.x ) >> coordinates_scale_log2;
				const int32_t vertex_y= ( mercator_point.y - min_point.y ) >> coordinates_scale_log2;
				vertices.push_back( ChunkVertex{ static_cast<ChunkCoordType>(vertex_x), static_cast<ChunkCoordType>(vertex_y) } );
			}
			vertices.push_back(break_primitive_vertex);
		}
		if( prev_class != LinearObjectClass::None )
		{
			group.vertex_count= static_cast<uint16_t>( vertices.size() - group.first_vertex );
			result.insert(
				result.end(),
				reinterpret_cast<const unsigned char*>(&group),
				reinterpret_cast<const unsigned char*>(&group) + sizeof(group) );
			++get_chunk().linear_object_groups_count;
		}
	}
	{
		get_chunk().areal_object_groups_offset= static_cast<uint32_t>(result.size());

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
			if( object.class_ != prev_class )
			{
				if( prev_class != ArealObjectClass::None )
				{
					group.vertex_count= static_cast<uint16_t>( vertices.size() - group.first_vertex );
					result.insert(
						result.end(),
						reinterpret_cast<const unsigned char*>(&group),
						reinterpret_cast<const unsigned char*>(&group) + sizeof(group) );
					++get_chunk().areal_object_groups_count;
				}

				group.first_vertex= static_cast<uint16_t>( vertices.size() );
				group.style_index= static_cast<Chunk::StyleIndex>( object.class_ );

				prev_class= object.class_;
			}

			for( size_t v= object.first_vertex_index; v < object.first_vertex_index + object.vertex_count; ++v )
			{
				const MercatorPoint& mercator_point= prepared_data_vetices_converted[v];
				const int32_t vertex_x= ( mercator_point.x - min_point.x ) >> coordinates_scale_log2;
				const int32_t vertex_y= ( mercator_point.y - min_point.y ) >> coordinates_scale_log2;
				vertices.push_back( ChunkVertex{ static_cast<ChunkCoordType>(vertex_x), static_cast<ChunkCoordType>(vertex_y) } );
			}

			// Drop duplicated vertex.
			if( prepared_data_vetices_converted[ object.first_vertex_index ] ==
				prepared_data_vetices_converted[object.first_vertex_index + object.vertex_count - 1u ] )
				vertices.pop_back();

			vertices.push_back(break_primitive_vertex);
		}
		if( prev_class != ArealObjectClass::None )
		{
			group.vertex_count= static_cast<uint16_t>( vertices.size() - group.first_vertex );
			result.insert(
				result.end(),
				reinterpret_cast<const unsigned char*>(&group),
				reinterpret_cast<const unsigned char*>(&group) + sizeof(group) );
			++get_chunk().areal_object_groups_count;
		}
	}

	get_chunk().vertices_offset= static_cast<uint32_t>( result.size() );
	get_chunk().vertex_count= static_cast<uint16_t>( vertices.size() );

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

	data_file.chunks_offset= static_cast<uint32_t>( result.size() );
	data_file.chunk_count= 1u;
	result.insert( result.end(), chunk_data.begin(), chunk_data.end() );

	return result;
}

static void WriteFile( const std::vector<unsigned char>& content, const char* file_name )
{
	std::FILE* const f= std::fopen( file_name, "wb" );
	if( f == nullptr )
		return;

	size_t write_total= 0u;
	do
	{
		const size_t write= std::fwrite( content.data() + write_total, 1, content.size() - write_total, f );
		if( write == 0 )
			break;

		write_total+= write;
	} while( write_total < content.size() );
}

void CreateDataFile( const OSMParseResult& prepared_data, const char* const file_name )
{
	WriteFile( DumpDataFile( prepared_data ), file_name );
}

} // namespace PanzerMaps
