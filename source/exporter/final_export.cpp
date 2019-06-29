#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>

#include "../common/assert.hpp"
#include "../common/data_file.hpp"
#include "../common/log.hpp"
#include "final_export.hpp"

namespace PanzerMaps
{

static const int32_t c_max_chunk_size= 64000; // Near to 65536
static const int32_t c_min_chunk_size= c_max_chunk_size / 512;

static std::vector< std::vector<MercatorPoint> > SplitPolyline(
	const std::vector<MercatorPoint> &polyline,
	const int32_t distance,
	const int32_t normal_x,
	const int32_t normal_y )
{
	PM_ASSERT( polyline.size() >= 2u );
	std::vector< std::vector<MercatorPoint> > polylines;

	const auto vertex_signed_plane_distance=
	[&]( const MercatorPoint& vertex ) -> int32_t
	{
		return vertex.x * normal_x + vertex.y * normal_y - distance;
	};

	const auto split_segment=
	[&]( const size_t segment_index ) -> MercatorPoint
	{
		const MercatorPoint& v0= polyline[ segment_index ];
		const MercatorPoint& v1= polyline[ segment_index + 1u ];
		const int64_t dist0= int64_t( std::abs( vertex_signed_plane_distance(v0) ) );
		const int64_t dist1= int64_t( std::abs( vertex_signed_plane_distance(v1) ) );
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

	int32_t prev_vertex_pos= vertex_signed_plane_distance( polyline.front() );
	std::vector<MercatorPoint> result_polyline;
	if( prev_vertex_pos >= 0 )
		result_polyline.push_back( polyline.front() );

	for( size_t i= 1u; i < polyline.size(); ++i )
	{
		const int32_t cur_vertex_pos= vertex_signed_plane_distance( polyline[i] );
			 if( prev_vertex_pos >= 0 && cur_vertex_pos >= 0 )
			result_polyline.push_back( polyline[i] );
		else if( prev_vertex_pos >= 0 && cur_vertex_pos < 0 )
		{
			result_polyline.push_back( split_segment(i-1u) );
			PM_ASSERT( result_polyline.size() >= 2u );
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
	{
		PM_ASSERT( result_polyline.size() >= 2u );
		polylines.push_back( std::move( result_polyline ) );
	}

	return polylines;
}

static std::vector< std::vector<MercatorPoint> > SplitPolyline(
	const std::vector<MercatorPoint>& polyline,
	const int32_t bb_min_x,
	const int32_t bb_min_y,
	const int32_t bb_max_x,
	const int32_t bb_max_y )
{
	PM_ASSERT( polyline.size() >= 2u );
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

static std::vector<MercatorPoint> SplitConvexPolygon(
	const std::vector<MercatorPoint>& polygon,
	const int32_t distance,
	const int32_t normal_x,
	const int32_t normal_y )
{
	PM_ASSERT( polygon.size() >= 3u );

	const auto vertex_signed_plane_distance=
	[&]( const MercatorPoint& vertex ) -> int32_t
	{
		return vertex.x * normal_x + vertex.y * normal_y - distance;
	};

	const auto split_segment=
	[&]( const size_t segment_index ) -> MercatorPoint
	{
		const MercatorPoint& v0= polygon[ segment_index ];
		const MercatorPoint& v1= polygon[ ( segment_index + 1u ) % polygon.size() ];
		const int64_t dist0= int64_t( std::abs( vertex_signed_plane_distance(v0) ) );
		const int64_t dist1= int64_t( std::abs( vertex_signed_plane_distance(v1) ) );
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

	const int32_t first_vertex_pos= vertex_signed_plane_distance( polygon.front() );
	int32_t prev_vertex_pos= first_vertex_pos;
	std::vector<MercatorPoint> result_polygon;
	if( prev_vertex_pos >= 0 )
		result_polygon.push_back( polygon.front() );

	for( size_t i= 1u; i < polygon.size(); ++i )
	{
		const int32_t cur_vertex_pos= vertex_signed_plane_distance( polygon[i] );
			 if( prev_vertex_pos >= 0 && cur_vertex_pos >= 0 )
			result_polygon.push_back( polygon[i] );
		else if( prev_vertex_pos >= 0 && cur_vertex_pos < 0 )
			result_polygon.push_back( split_segment(i-1u) );
		else if( prev_vertex_pos < 0 && cur_vertex_pos >= 0 )
		{
			result_polygon.push_back( split_segment(i-1u) );
			result_polygon.push_back( polygon[i] );
		}
		else if( prev_vertex_pos < 0 && cur_vertex_pos < 0 )
		{}
		prev_vertex_pos= cur_vertex_pos;
	}

	if( ( first_vertex_pos >= 0 && prev_vertex_pos < 0 ) || ( first_vertex_pos < 0 && prev_vertex_pos >= 0 ) )
		result_polygon.push_back( split_segment( polygon.size() - 1u ) );

	PM_ASSERT( result_polygon.size() == 0u || result_polygon.size() >= 3u );

	return std::move(result_polygon);
}

static std::vector< std::vector<MercatorPoint> > SplitConvexPolygon(
	const std::vector<MercatorPoint>& polygon,
	const int32_t bb_min_x,
	const int32_t bb_min_y,
	const int32_t bb_max_x,
	const int32_t bb_max_y )
{
	PM_ASSERT( polygon.size() >= 3u );
	std::vector< std::vector<MercatorPoint> > polygons;

	polygons.push_back( polygon );

	const int32_t normals[4][2]{ { +1, 0 }, { -1, 0 }, { 0, +1 }, { 0, -1 }, };
	const int32_t distances[4]{ +bb_min_x, -bb_max_x, +bb_min_y, -bb_max_y };
	for( size_t i= 0u; i < 4u; ++i )
	{
		std::vector< std::vector<MercatorPoint> > new_polygons;
		for( const std::vector<MercatorPoint>& polygon : polygons )
		{
			std::vector<MercatorPoint> polygon_splitted= SplitConvexPolygon( polygon, distances[i], normals[i][0], normals[i][1] );
			if( polygon_splitted.size() >= 3u )
				new_polygons.push_back( std::move( polygon_splitted ) );
		}
		polygons= std::move( new_polygons );
	}

	return polygons;
}

using ChunkData= std::vector<unsigned char>;
using ChunksData= std::vector<ChunkData>;

static ChunksData DumpDataChunk(
	const PolygonsNormalizationPassResult& prepared_data,
	const int32_t chunk_offset_x,
	const int32_t chunk_offset_y,
	const int32_t chunk_size ) // Offset and size - in scaled coordinates.
{
	using namespace DataFileDescription;

	std::vector<unsigned char> result;
	result.resize( sizeof(Chunk), 0 );

	const auto get_chunk= [&]() -> Chunk& { return *reinterpret_cast<Chunk*>( result.data() ); };
	get_chunk().point_object_groups_count= get_chunk().linear_object_groups_count= get_chunk().areal_object_groups_count= 0;

	const CoordinatesTransformationPassResult::VertexTranspormed min_point{
		chunk_offset_x - ( 65535 - c_max_chunk_size ) / 2,
		chunk_offset_y - ( 65535 - c_max_chunk_size ) / 2 };

	get_chunk().coord_start_x= min_point.x;
	get_chunk().coord_start_y= min_point.y;
	get_chunk().min_x= chunk_offset_x;
	get_chunk().min_y= chunk_offset_y;
	get_chunk().max_x= chunk_offset_x + chunk_size;
	get_chunk().max_y= chunk_offset_y + chunk_size;

	std::vector<ChunkVertex> vertices;
	size_t linear_vertex_count= 0u;
	const ChunkVertex break_primitive_vertex{ std::numeric_limits<ChunkCoordType>::max(), 0 };
	{
		get_chunk().point_object_groups_offset= static_cast<uint32_t>(result.size());

		PointObjectClass prev_class= PointObjectClass::None;
		Chunk::PointObjectGroup group;
		for( const OSMParseResult::PointObject& object : prepared_data.point_objects )
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

		LinearObjectClass prev_class= LinearObjectClass::None;
		Chunk::LinearObjectGroup group;
		for( const OSMParseResult::LinearObject& object : prepared_data.linear_objects )
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
					++linear_vertex_count;
				}
				vertices.push_back(break_primitive_vertex);
				++linear_vertex_count;
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

		Chunk::ArealObjectGroup group;
		group.first_vertex= static_cast<uint16_t>(vertices.size());
		for( const OSMParseResult::ArealObject& object : prepared_data.areal_objects )
		{
			std::vector<MercatorPoint> polygon_vertices;
			polygon_vertices.reserve( object.vertex_count );
			for( size_t v= object.first_vertex_index; v < object.first_vertex_index + object.vertex_count; ++v )
				polygon_vertices.push_back( prepared_data.vertices[v] );

			for( const std::vector<MercatorPoint> ploygon_part_bbox_splitted : SplitConvexPolygon( polygon_vertices, chunk_offset_x, chunk_offset_y, chunk_offset_x + chunk_size, chunk_offset_y + chunk_size ) )
			{
				for( const MercatorPoint& polygon_part_vertex : ploygon_part_bbox_splitted )
				{
					const int32_t vertex_x= polygon_part_vertex.x - min_point.x;
					const int32_t vertex_y= polygon_part_vertex.y - min_point.y;
					vertices.push_back( ChunkVertex{ static_cast<ChunkCoordType>(vertex_x), static_cast<ChunkCoordType>(vertex_y) } );
				}
				vertices.push_back(break_primitive_vertex);
				vertices.back().y= static_cast<uint16_t>( object.class_ );
			}
		}

		group.vertex_count= static_cast<uint16_t>( vertices.size() - group.first_vertex );
		result.insert(
			result.end(),
			reinterpret_cast<const unsigned char*>(&group),
			reinterpret_cast<const unsigned char*>(&group) + sizeof(group) );
		++get_chunk().areal_object_groups_count;
	}

	// We have vertex limit= 2^16.
	// For linear objects we limit vertex count, using approximation 4 vertices for each line vertex.
	if( vertices.size() >= 65535u || linear_vertex_count >= 65535u / 4u )
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

	if( vertices.empty() )
	{
		// Chunk without geomery.
		result.clear();
		return { result };
	}
	Log::Info( "Chunk ", chunk_offset_x, " ", chunk_offset_y, " done. Vertices: ", vertices.size() );

	return { result };
}

static std::vector<unsigned char> DumpDataFile( const std::vector<PolygonsNormalizationPassResult>& prepared_data, const Styles& styles )
{
	Log::Info( "Final export: " );

	using namespace DataFileDescription;

	std::vector<unsigned char> result;

	result.resize( sizeof(DataFile), static_cast<unsigned char>(0) );
	const auto get_data_file= [&]() -> DataFile& { return *reinterpret_cast<DataFile*>( result.data() ); };

	std::memcpy( get_data_file().header, DataFile::c_expected_header, sizeof(get_data_file().header) );
	get_data_file().version= DataFile::c_expected_version;

	get_data_file().zoom_levels_offset= static_cast<uint32_t>( result.size() );
	get_data_file().zoom_level_count= static_cast<uint32_t>( prepared_data.size() );
	result.resize( result.size() + prepared_data.size() * sizeof(ZoomLevel) );
	const auto get_zoom_levels= [&]() -> ZoomLevel* { return reinterpret_cast<ZoomLevel*>( result.data() + get_data_file().zoom_levels_offset ); };
	const auto get_zoom_level= [&]( size_t index ) -> ZoomLevel& { return get_zoom_levels()[index]; };

	for( size_t zoom_level_index= 0u; zoom_level_index < prepared_data.size(); ++zoom_level_index )
	{
		Log::Info( "" );
		Log::Info( "-- ZOOM LEVEL ", zoom_level_index, " ---" );
		Log::Info( "" );

		const PolygonsNormalizationPassResult& zoom_level_data= prepared_data[zoom_level_index];
		const Styles::ZoomLevel& zoom_level_styles= styles.zoom_levels[zoom_level_index];

		// Dump zoom level chunks.
		const int32_t used_chunk_size= c_max_chunk_size;
		const int32_t chunks_x= ( zoom_level_data.max_point.x / zoom_level_data.coordinates_scale - zoom_level_data.start_point.x / zoom_level_data.coordinates_scale + (used_chunk_size-1) ) / used_chunk_size;
		const int32_t chunks_y= ( zoom_level_data.max_point.y / zoom_level_data.coordinates_scale - zoom_level_data.start_point.y / zoom_level_data.coordinates_scale + (used_chunk_size-1) ) / used_chunk_size;

		// All zoom levels must have same start point.
		PM_ASSERT( zoom_level_data.min_point == prepared_data.front().min_point );
		PM_ASSERT( zoom_level_data.start_point == prepared_data.front().start_point );

		ChunksData final_chunks_data;
		for( int32_t x= 0; x < chunks_x; ++x )
		for( int32_t y= 0; y < chunks_y; ++y )
		{
			ChunksData chunks_data=
				DumpDataChunk(
					zoom_level_data,
					x * used_chunk_size,
					y * used_chunk_size,
					used_chunk_size );
			for( ChunkData& chunk_data : chunks_data )
				if( !chunk_data.empty() )
					final_chunks_data.push_back( std::move( chunk_data ) );
		}

		get_zoom_level(zoom_level_index).unit_size_m= zoom_level_data.meters_in_unit;
		get_zoom_level(zoom_level_index).zoom_level_log2= static_cast<uint32_t>( zoom_level_data.zoom_level );
		get_zoom_level(zoom_level_index).chunk_count= static_cast<uint32_t>(final_chunks_data.size());
		get_zoom_level(zoom_level_index).chunks_description_offset= static_cast<uint32_t>(result.size());
		const auto get_chunks_description= [&]() -> DataFile::ChunkDescription*
		{
			return reinterpret_cast<DataFile::ChunkDescription*>( result.data() + get_zoom_level(zoom_level_index).chunks_description_offset );
		};
		result.resize( result.size() + sizeof(DataFile::ChunkDescription) * final_chunks_data.size() );

		Log::Info( final_chunks_data.size(), " chunks" );
		for( size_t i= 0u; i < final_chunks_data.size(); ++i )
		{
			get_chunks_description()[i].offset= static_cast<uint32_t>(result.size());
			get_chunks_description()[i].size= static_cast<uint32_t>(final_chunks_data[i].size());
			result.insert( result.end(), final_chunks_data[i].begin(), final_chunks_data[i].end() );
		}

		// Dump zoom level styles.

		get_zoom_level(zoom_level_index).point_styles_count= 0u;
		get_zoom_level(zoom_level_index).linear_styles_count= 0u;
		get_zoom_level(zoom_level_index).areal_styles_count= 0u;

		for( PointObjectClass object_class= PointObjectClass::None; object_class < PointObjectClass::Last; object_class= static_cast<PointObjectClass>( size_t(object_class) + 1u ) )
		{
		}

		get_zoom_level(zoom_level_index).linear_styles_offset= static_cast<uint32_t>( result.size() );
		for( LinearObjectClass object_class= LinearObjectClass::None; object_class < LinearObjectClass::Last; object_class= static_cast<LinearObjectClass>( size_t(object_class) + 1u ) )
		{
			const auto style_it= zoom_level_styles.linear_object_styles.find( object_class );

			result.resize( result.size() + sizeof(LinearObjectStyle) );
			LinearObjectStyle& out_style= *reinterpret_cast<LinearObjectStyle*>( result.data() + result.size() - sizeof(LinearObjectStyle) );
			if( style_it == zoom_level_styles.linear_object_styles.end() )
			{
				out_style.color[0]= out_style.color[1]= out_style.color[2]= 128u;
				out_style.color[3]= 255u;
				out_style.width_mul_256= 0u;
			}
			else
			{
				std::memcpy( out_style.color, style_it->second.color, sizeof(unsigned char) * 4u );
				out_style.width_mul_256= uint32_t( style_it->second.width_m / zoom_level_data.meters_in_unit * 256.0f );
			}

			++get_zoom_level(zoom_level_index).linear_styles_count;
		}

		get_zoom_level(zoom_level_index).areal_styles_offset= static_cast<uint32_t>( result.size() );
		for( ArealObjectClass object_class= ArealObjectClass::None; object_class < ArealObjectClass::Last; object_class= static_cast<ArealObjectClass>( size_t(object_class) + 1u ) )
		{
			const auto style_it= zoom_level_styles.areal_object_styles.find( object_class );

			result.resize( result.size() + sizeof(ArealObjectStyle) );
			ArealObjectStyle& out_style= *reinterpret_cast<ArealObjectStyle*>( result.data() + result.size() - sizeof(ArealObjectStyle) );
			if( style_it == zoom_level_styles.areal_object_styles.end() )
			{
				out_style.color[0]= out_style.color[1]= out_style.color[2]= 128u;
				out_style.color[3]= 255u;
			}
			else
				std::memcpy( out_style.color, style_it->second.color, sizeof(unsigned char) * 4u );

			++get_zoom_level(zoom_level_index).areal_styles_count;
		}

		get_zoom_level(zoom_level_index).point_styles_order_count= static_cast<uint32_t>( zoom_level_styles.point_classes_ordered.size() );
		get_zoom_level(zoom_level_index).point_styles_order_offset= static_cast<uint32_t>( result.size() );
		for( const PointObjectClass& object_class : zoom_level_styles.point_classes_ordered )
		{
			result.resize( result.size() + sizeof(PointStylesOrder) );
			PointStylesOrder& order= *reinterpret_cast<PointStylesOrder*>( result.data() + result.size() - sizeof(PointStylesOrder) );
			order.style_index= Chunk::StyleIndex(object_class);
		}

		get_zoom_level(zoom_level_index).linear_styles_order_count= static_cast<uint32_t>( zoom_level_styles.linear_classes_ordered.size() );
		get_zoom_level(zoom_level_index).linear_styles_order_offset= static_cast<uint32_t>( result.size() );
		for( const LinearObjectClass& object_class : zoom_level_styles.linear_classes_ordered )
		{
			result.resize( result.size() + sizeof(LinearStylesOrder) );
			LinearStylesOrder& order= *reinterpret_cast<LinearStylesOrder*>( result.data() + result.size() - sizeof(LinearStylesOrder) );
			order.style_index= Chunk::StyleIndex(object_class);
		}

		Log::Info( "" );
		Log::Info( "-- ZOOM LEVEL END ---" );
		Log::Info( "" );
	} // for zoom levels

	std::memcpy( get_data_file().common_style.background_color, styles.background_color, sizeof(unsigned char) * 4u );

	Log::Info( "result size is ", result.size(), " bytes (", result.size() / 1024u, "kb)" );

	return result;
}

static void WriteFile( const std::vector<unsigned char>& content, const char* file_name )
{
	std::FILE* const f= std::fopen( file_name, "wb" );
	if( f == nullptr )
	{
		Log::FatalError( "Error, opening file \"", file_name, "\"" );
		return;
	}

	size_t write_total= 0u;
	do
	{
		const size_t write= std::fwrite( content.data() + write_total, 1, content.size() - write_total, f );
		if( write == 0 )
			break;

		write_total+= write;
	} while( write_total < content.size() );
}

void CreateDataFile( const std::vector<PolygonsNormalizationPassResult>& prepared_data, const Styles& styles, const char* const file_name )
{
	WriteFile( DumpDataFile( prepared_data, styles ), file_name );
}

} // namespace PanzerMaps
