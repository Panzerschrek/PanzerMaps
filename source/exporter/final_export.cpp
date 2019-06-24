#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>

#include "../common/data_file.hpp"
#include "final_export.hpp"

namespace PanzerMaps
{

// Returns positive value for clockwise polygon, negative - for anticlockwisi.
static int64_t CalculatePolygonDoubleSignedArea( const MercatorPoint* const vertices, size_t vertex_count )
{
	int64_t result= 0;

	result+= int64_t(vertices[0u].x) * int64_t(vertices[vertex_count-1u].y) - int64_t(vertices[vertex_count-1u].x) * int64_t(vertices[0u].y);
	for( size_t i= 1u; i < vertex_count; ++i )
		result+= int64_t(vertices[i].x) * int64_t(vertices[i-1u].y) - int64_t(vertices[i-1u].x) * int64_t(vertices[i].y);

	return result;
}

// Returns non-negative value for convex vertex of clockwise polygon.
static int64_t PolygonVertexCross( const MercatorPoint& p0, const MercatorPoint& p1, const MercatorPoint& p2 )
{
	const int32_t dx0= p1.x - p0.x;
	const int32_t dy0= p1.y - p0.y;
	const int32_t dx1= p2.x - p1.x;
	const int32_t dy1= p2.y - p1.y;

	return int64_t(dx1) * int64_t(dy0) - int64_t(dx0) * int64_t(dy1);
}

static bool VertexIsInisideClockwiseConvexPolygon( const MercatorPoint* const vertices, size_t vertex_count, const MercatorPoint& test_vertex )
{
	for( size_t i= 0u; i < vertex_count; ++i )
	{
		if(
			PolygonVertexCross(
				vertices[i],
				vertices[ (i+1u) % vertex_count ],
				test_vertex ) < 0 )
			return false;
	}
	return true;
}

static std::vector< std::vector<MercatorPoint> > SplitPolygonIntoConvexParts( std::vector<MercatorPoint> vertices )
{
	std::vector< std::vector<MercatorPoint> > result;

	const int64_t polygon_double_signed_area= CalculatePolygonDoubleSignedArea( vertices.data(), vertices.size() );
	if( polygon_double_signed_area == 0 )
		return { result };

	if( polygon_double_signed_area < 0 )
		std::reverse( vertices.begin(), vertices.end() ); // Make polygon clockwise.

	const auto is_split_vertex=
	[&]( const size_t vertex_index ) -> bool
	{
		const int64_t cross=
		PolygonVertexCross(
			vertices[ ( vertex_index + vertices.size() - 1u ) % vertices.size() ],
			vertices[ vertex_index % vertices.size()  ],
			vertices[ ( vertex_index + 1u ) % vertices.size() ] );

		return cross < 0;
	};

	std::vector<MercatorPoint> triangle;
	while( vertices.size() > 3u )
	{
		bool have_split_vertices= false;
		for( size_t i= 0u; i < vertices.size(); ++i )
			have_split_vertices= have_split_vertices || is_split_vertex(i);
		if( !have_split_vertices )
		{
			// Polygon is convex, stop triangulation
			goto finish_triangulation;
		}

		for( size_t i= 0u; i < vertices.size(); ++i )
		{
			if( is_split_vertex(i) )
				continue;
			// Try create triangle with convex vertex.

			triangle.clear();
			triangle.push_back( vertices[ ( i + vertices.size() - 1u ) % vertices.size() ] );
			triangle.push_back( vertices[ i % vertices.size() ] );
			triangle.push_back( vertices[ ( i + 1u ) % vertices.size() ] );

			for( size_t j = 2u; j < vertices.size() - 1u; ++j )
			{
				if( VertexIsInisideClockwiseConvexPolygon( triangle.data(), triangle.size(), vertices[ ( i + j ) % vertices.size() ] ) )
					goto select_next_vertex_fo_triangulation;
			}

			result.push_back( triangle );
			vertices.erase( vertices.begin() + i );
			break;

			select_next_vertex_fo_triangulation:;
		}
	}
	finish_triangulation:
	result.push_back( vertices );


	// TODO - merge adjusted polygons, if merge result polygon will be convex.

	return result;
}

static std::vector<unsigned char> DumpDataChunk( const CoordinatesTransformationPassResult& prepared_data )
{
	using namespace DataFileDescription;

	std::vector<unsigned char> result;
	result.resize( sizeof(Chunk), 0 );

	const auto get_chunk= [&]() -> Chunk& { return *reinterpret_cast<Chunk*>( result.data() ); };
	get_chunk().point_object_groups_count= get_chunk().linear_object_groups_count= get_chunk().areal_object_groups_count= 0;


	// TODO - asssert, if data chunk contains >= 65535 vertices.

	// TODO - set min_point to chunk offset
	const CoordinatesTransformationPassResult::VertexTranspormed min_point{ 0, 0 };

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

			const MercatorPoint& mercator_point= prepared_data.vertices[object.vertex_index];
			const int32_t vertex_x= mercator_point.x - min_point.x;
			const int32_t vertex_y= mercator_point.y - min_point.y;
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
				const MercatorPoint& mercator_point= prepared_data.vertices[v];
				const int32_t vertex_x= mercator_point.x - min_point.x;
				const int32_t vertex_y= mercator_point.y - min_point.y;
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

			std::vector<MercatorPoint> polygon_vertices;
			polygon_vertices.reserve( object.vertex_count );
			for( size_t v= object.first_vertex_index; v < object.first_vertex_index + object.vertex_count; ++v )
				polygon_vertices.push_back( prepared_data.vertices[v] );

			for( const std::vector<MercatorPoint>& polygon_part : SplitPolygonIntoConvexParts( polygon_vertices ) )
			{
				for( const MercatorPoint& polygon_part_vertex : polygon_part )
				{
					const int32_t vertex_x= polygon_part_vertex.x - min_point.x;
					const int32_t vertex_y= polygon_part_vertex.y - min_point.y;
					vertices.push_back( ChunkVertex{ static_cast<ChunkCoordType>(vertex_x), static_cast<ChunkCoordType>(vertex_y) } );

				}
				vertices.push_back(break_primitive_vertex);
			}
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

static std::vector<unsigned char> DumpDataFile( const CoordinatesTransformationPassResult& prepared_data )
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

void CreateDataFile( const CoordinatesTransformationPassResult& prepared_data, const char* const file_name )
{
	WriteFile( DumpDataFile( prepared_data ), file_name );
}

} // namespace PanzerMaps
