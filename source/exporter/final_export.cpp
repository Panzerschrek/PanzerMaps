#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>

#include "../common/data_file.hpp"
#include "final_export.hpp"

namespace PanzerMaps
{

static const int32_t c_max_chunk_size= 64000; // Near to 65536
static const int32_t c_min_chunk_size= c_max_chunk_size / 512;

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

static std::vector< std::vector<MercatorPoint> > SplitPolyline(
	const std::vector<MercatorPoint> &polyline,
	const int32_t distance,
	const int32_t normal_x,
	const int32_t normal_y )
{
	std::vector< std::vector<MercatorPoint> > polylines;

	const auto vertex_pos=
	[&]( const MercatorPoint& vertex ) -> int
	{
		const int32_t signed_plane_distance= vertex.x * normal_x + vertex.y * normal_y - distance;
		if( signed_plane_distance > 0 ) return +1;
		if( signed_plane_distance < 0 ) return -1;
		return 0;
	};

	const auto split_segment=
	[&]( const size_t segment_index ) -> MercatorPoint
	{
		const MercatorPoint& v0= polyline[ segment_index ];
		const MercatorPoint& v1= polyline[ segment_index + 1u ];
		const int64_t dist0= int64_t( std::abs( v0.x * normal_x + v0.y * normal_y - distance ) );
		const int64_t dist1= int64_t( std::abs( v1.x * normal_x + v1.y * normal_y - distance ) );
		const int64_t dist_sum= dist0 + dist1;
		MercatorPoint result;
		if( dist_sum > 0 )
		{
			result.x= int32_t( ( int64_t(v0.x) * dist1 + int64_t(v1.x) * dist0 ) / dist_sum );
			result.y= int32_t( ( int64_t(v0.y) * dist1 + int64_t(v1.y) * dist0 ) / dist_sum );
		}
		else
			result= v0;

		return result;
	};

	int prev_vertex_pos= vertex_pos( polyline.front() );
	std::vector<MercatorPoint> result_polyline;
	if( prev_vertex_pos >= 0 )
		result_polyline.push_back( polyline.front() );

	for( size_t i= 1u; i < polyline.size(); ++i )
	{
		const int cur_vertex_pos= vertex_pos( polyline[i] );
			 if( prev_vertex_pos >= 0 && cur_vertex_pos >= 0 )
			result_polyline.push_back( polyline[i] );
		else if( prev_vertex_pos >= 0 && cur_vertex_pos < 0 )
		{
			result_polyline.push_back( split_segment(i-1u) );
			polylines.emplace_back();
			polylines.back().swap( result_polyline );
		}
		else if( prev_vertex_pos < 0 && cur_vertex_pos >= 0 )
		{
			result_polyline.push_back( split_segment(i-1u) );
			result_polyline.push_back( polyline[i] );
		}
		else if( prev_vertex_pos < 0 && cur_vertex_pos < 0 )
		{}
		prev_vertex_pos= cur_vertex_pos;
	}

	if( result_polyline.size() >= 2u )
		polylines.push_back( std::move( result_polyline ) );

	return polylines;
}

static std::vector< std::vector<MercatorPoint> > SplitPolyline(
	const std::vector<MercatorPoint>& polyline,
	const int32_t bb_min_x,
	const int32_t bb_min_y,
	const int32_t bb_max_x,
	const int32_t bb_max_y )
{
	std::vector< std::vector<MercatorPoint> > polylines;

	polylines.push_back( polyline );

	const int32_t normals[4][2]{ { +1, 0 }, { -1, 0 }, { 0, +1 }, { 0, -1 }, };
	const int32_t distances[4]{ +bb_min_x, -bb_max_x, +bb_min_y, -bb_max_y };
	for( size_t i= 0u; i < 4u; ++i )
	{
		std::vector< std::vector<MercatorPoint> > new_polylines;
		for( const std::vector<MercatorPoint>& polyline : polylines )
		{
			std::vector< std::vector<MercatorPoint> > polyline_splitted= SplitPolyline( polyline, distances[i], normals[i][0], normals[i][1] );
			for( std::vector<MercatorPoint>& polyline_part : polyline_splitted )
				new_polylines.push_back( std::move( polyline_part ) );
		}
		polylines= std::move( new_polylines );
	}

	return polylines;
}

using ChunkData= std::vector<unsigned char>;
using ChunksData= std::vector<ChunkData>;

static ChunksData DumpDataChunk(
	const CoordinatesTransformationPassResult& prepared_data,
	const int32_t chunk_offset_x,
	const int32_t chunk_offset_y,
	const int32_t chunk_size ) // Offset and size - in scaled coordinates.
{
	using namespace DataFileDescription;

	std::vector<unsigned char> result;
	result.resize( sizeof(Chunk), 0 );

	const auto get_chunk= [&]() -> Chunk& { return *reinterpret_cast<Chunk*>( result.data() ); };
	get_chunk().point_object_groups_count= get_chunk().linear_object_groups_count= get_chunk().areal_object_groups_count= 0;

	// TODO - asssert, if data chunk contains >= 65535 vertices.

	const CoordinatesTransformationPassResult::VertexTranspormed min_point{
		chunk_offset_x - ( 65535 - c_max_chunk_size ) / 2,
		chunk_offset_y - ( 65535 - c_max_chunk_size ) / 2 };

	get_chunk().coord_start_x= min_point.x;
	get_chunk().coord_start_y= min_point.y;

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
			if( mercator_point.x >= chunk_offset_x && mercator_point.y >= chunk_offset_y &&
				mercator_point.x < chunk_offset_x + chunk_size && mercator_point.y < chunk_offset_y + chunk_size )
			{
				const int32_t vertex_x= mercator_point.x - min_point.x;
				const int32_t vertex_y= mercator_point.y - min_point.y;
				vertices.push_back( ChunkVertex{ static_cast<ChunkCoordType>(vertex_x), static_cast<ChunkCoordType>(vertex_y) } );
			}
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

			std::vector<MercatorPoint> polyline_vertices;
			polyline_vertices.reserve( object.vertex_count );
			for( size_t v= object.first_vertex_index; v < object.first_vertex_index + object.vertex_count; ++v )
				polyline_vertices.push_back( prepared_data.vertices[v] );

			for( const std::vector<MercatorPoint>& polyline_part : SplitPolyline( polyline_vertices, chunk_offset_x, chunk_offset_y, chunk_offset_x + chunk_size, chunk_offset_y + chunk_size ) )
			{
				for( const MercatorPoint& polyline_part_vertex : polyline_part )
				{
					const int32_t vertex_x= polyline_part_vertex.x - min_point.x;
					const int32_t vertex_y= polyline_part_vertex.y - min_point.y;
					vertices.push_back( ChunkVertex{ static_cast<ChunkCoordType>(vertex_x), static_cast<ChunkCoordType>(vertex_y) } );

				}
				vertices.push_back(break_primitive_vertex);
			}
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

	// If result chunk contains more vertices, then limit, split it into subchunks.
	if( vertices.size() >= 32768u )
	{
		ChunksData result;
		result.reserve(4u);

		const int32_t half_chunk_size= chunk_size / 2;
		for( int32_t x= 0; x < 2; ++x )
		for( int32_t y= 0; y < 2; ++y )
		{
			ChunksData sub_chunks=
				DumpDataChunk(
					prepared_data,
					chunk_offset_x + x * half_chunk_size,
					chunk_offset_y + y * half_chunk_size,
					half_chunk_size );
			for( ChunkData& sub_chunk : sub_chunks )
				result.push_back( std::move( sub_chunk ) );
		}
		return result;
	}

	get_chunk().vertices_offset= static_cast<uint32_t>( result.size() );
	get_chunk().vertex_count= static_cast<uint16_t>( vertices.size() );

	result.insert(
		result.end(),
		reinterpret_cast<const unsigned char*>( vertices.data() ),
		reinterpret_cast<const unsigned char*>( vertices.data() + vertices.size() ) );

	return { result };
}

static std::vector<unsigned char> DumpDataFile( const CoordinatesTransformationPassResult& prepared_data )
{
	using namespace DataFileDescription;

	std::vector<unsigned char> result;

	result.resize( sizeof(DataFile), static_cast<unsigned char>(0) );
	const auto get_data_file= [&]() -> DataFile& { return *reinterpret_cast<DataFile*>( result.data() ); };

	std::memcpy( get_data_file().header, DataFile::c_expected_header, sizeof(get_data_file().header) );
	get_data_file().version= DataFile::c_expected_version;

	const int32_t used_chunk_size= c_max_chunk_size;
	const int32_t chunks_x= ( prepared_data.max_point.x / prepared_data.coordinates_scale - prepared_data.start_point.x / prepared_data.coordinates_scale + (used_chunk_size-1) ) / used_chunk_size;
	const int32_t chunks_y= ( prepared_data.max_point.y / prepared_data.coordinates_scale - prepared_data.start_point.y / prepared_data.coordinates_scale + (used_chunk_size-1) ) / used_chunk_size;

	ChunksData final_chunks_data;
	for( int32_t x= 0; x < chunks_x; ++x )
	for( int32_t y= 0; y < chunks_y; ++y )
	{
		ChunksData chunks_data=
			DumpDataChunk(
				prepared_data,
				x * used_chunk_size,
				y * used_chunk_size,
				used_chunk_size );
		for( ChunkData& chunk_data : chunks_data )
			final_chunks_data.push_back( std::move( chunk_data ) );
	}

	get_data_file().chunk_count= static_cast<uint32_t>(final_chunks_data.size());
	get_data_file().chunks_description_offset= static_cast<uint32_t>(result.size());
	const auto get_chunks_description= [&]() -> DataFile::ChunkDescription*
	{
		return reinterpret_cast<DataFile::ChunkDescription*>( result.data() + get_data_file().chunks_description_offset );
	};
	result.resize( result.size() + sizeof(DataFile::ChunkDescription) * final_chunks_data.size() );

	for( size_t i= 0u; i < final_chunks_data.size(); ++i )
	{
		get_chunks_description()[i].offset= static_cast<uint32_t>(result.size());
		get_chunks_description()[i].size= static_cast<uint32_t>(final_chunks_data[i].size());
		result.insert( result.end(), final_chunks_data[i].begin(), final_chunks_data[i].end() );
	}

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
