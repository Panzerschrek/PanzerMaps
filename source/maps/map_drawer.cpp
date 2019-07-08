#include <algorithm>
#include <cstring>
#include <unordered_map>
#include "../common/assert.hpp"
#include "../common/coordinates_conversion.hpp"
#include "../common/data_file.hpp"
#include "../common/log.hpp"
#include "shaders.hpp"
#include "map_drawer.hpp"

namespace PanzerMaps
{

struct PointObjectVertex
{
	uint16_t xy[2];
	uint32_t color_index;
};

struct LinearObjectVertex
{
	uint16_t xy[2];
	// For regular linex x is color index, 0 - unused
	// For dashed line .y is texture coordinate.
	uint16_t tex_coord[2];
};

struct PolygonalLinearObjectVertex
{
	float xy[2];

	// For regular linex x is color index, 0 - unused
	// For dashed line .y is texture coordinate.
	float tex_coord[2];
};

struct ArealObjectVertex
{
	uint16_t xy[2];
	uint32_t color_index;
};

static const uint16_t c_primitive_restart_index= 65535u;
static const float c_cos_plus_45 = +std::sqrt(0.5f);
static const float c_sin_plus_45 = +std::sqrt(0.5f);
static const float c_cos_minus_45= +std::sqrt(0.5f);
static const float c_sin_minus_45= -std::sqrt(0.5f);

static void SimplifyLine( std::vector<DataFileDescription::ChunkVertex>& line, const float suqare_half_width )
{
	// TODO - fix equal points in source data.
	PM_ASSERT( line.size() >= 1u );

	const int32_t square_half_width_int= std::max( 1, int32_t(suqare_half_width) );
	const auto last_vertex= line.back();

	line.erase(
		std::unique(
			line.begin(), line.end(),
			[square_half_width_int]( const DataFileDescription::ChunkVertex& v0, const DataFileDescription::ChunkVertex& v1 ) -> bool
			{
				//return v0 == v1;
				const int32_t dx= int32_t(v1.x) - int32_t(v0.x);
				const int32_t dy= int32_t(v1.y) - int32_t(v0.y);
				return dx * dx + dy * dy < square_half_width_int;
			}),
		line.end() );

	// keep last vertex.
	const int32_t dx= int32_t(last_vertex.x) - int32_t(line.back().x);
	const int32_t dy= int32_t(last_vertex.y) - int32_t(line.back().y);
	const int32_t square_dist= dx * dx + dy * dy;
	if( square_dist != 0 && square_dist < square_half_width_int )
	{
		if( line.size() <= 1u )
			line.push_back(last_vertex);
		else
			line.back()= last_vertex;
	}
}

// Creates triangle strip mesh.
static void CreatePolygonalLine(
	const DataFileDescription::ChunkVertex* const in_vertices,
	const size_t vertex_count,
	const uint32_t color_index,
	const float half_width,
	const float tex_coord_scale,
	std::vector<PolygonalLinearObjectVertex>& out_vertices,
	std::vector<uint16_t>& out_indices )
{
	PM_ASSERT( vertex_count != 0u );

	const float color_index_f= float(color_index);

	float tex_coord= 0.0f; // TODO - do not calculate tex_coord for lines with single color.
	const float cup_tex_coord_add= 0.5f * half_width * tex_coord_scale;
	if( vertex_count == 1u )
	{
		// Line was too simplifyed, draw only caps.
		const m_Vec2 vert( float(in_vertices[0u].x), float(in_vertices[0u].y) );
		const m_Vec2 edge_shift( 0.0f, half_width );

		// Cup0
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert.x + edge_shift.y,
					vert.y - edge_shift.x },
				{ color_index_f, tex_coord } } );
		tex_coord+= cup_tex_coord_add;
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert.x + edge_shift.x * c_cos_minus_45 - edge_shift.y * c_sin_minus_45,
					vert.y + edge_shift.x * c_sin_minus_45 + edge_shift.y * c_cos_minus_45 },
				{ color_index_f, tex_coord } } );
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert.x - edge_shift.x * c_cos_plus_45 + edge_shift.y * c_sin_plus_45,
					vert.y - edge_shift.x * c_sin_plus_45 - edge_shift.y * c_cos_plus_45 },
				{ color_index_f, tex_coord } } );
		// Center.
		tex_coord+= cup_tex_coord_add;
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert.x + edge_shift.x,
					vert.y + edge_shift.y },
				{ color_index_f, tex_coord } } );
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert.x - edge_shift.x,
					vert.y - edge_shift.y },
				{ color_index_f, tex_coord } } );
		// Cup1
		tex_coord+= cup_tex_coord_add;
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert.x + edge_shift.x * c_cos_plus_45 - edge_shift.y * c_sin_plus_45,
					vert.y + edge_shift.x * c_sin_plus_45 + edge_shift.y * c_cos_plus_45 },
				{ color_index_f, tex_coord } } );
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert.x - edge_shift.x * c_cos_minus_45 + edge_shift.y * c_sin_minus_45,
					vert.y - edge_shift.x * c_sin_minus_45 - edge_shift.y * c_cos_minus_45 },
				{ color_index_f, tex_coord } } );
		tex_coord+= cup_tex_coord_add;
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert.x - edge_shift.y,
					vert.y + edge_shift.x },
				{ color_index_f, tex_coord } } );

		out_indices.push_back( c_primitive_restart_index );
		return;
	}

	// Use float coordinates, because uint16_t is too low for polygonal lines with small width.
	m_Vec2 prev_edge_base_vec;
	{
		const m_Vec2 vert0( float(in_vertices[0u].x), float(in_vertices[0u].y) );

		const m_Vec2 edge_dir( float(in_vertices[1u].x) - vert0.x, float(in_vertices[1u].y) - vert0.y );
		const float edge_length= edge_dir.Length();
		PM_ASSERT( edge_length > 0.0f );
		const float edge_inv_length= 1.0f / edge_length;
		const m_Vec2 edge_base_vec( edge_dir.y * edge_inv_length, -edge_dir.x * edge_inv_length );

		const m_Vec2 edge_shift= edge_base_vec * half_width;

		// Cup.
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert0.x + edge_shift.y,
					vert0.y - edge_shift.x },
				{ color_index_f, tex_coord } } );
		tex_coord+= cup_tex_coord_add;
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert0.x + edge_shift.x * c_cos_minus_45 - edge_shift.y * c_sin_minus_45,
					vert0.y + edge_shift.x * c_sin_minus_45 + edge_shift.y * c_cos_minus_45 },
				{ color_index_f, tex_coord } } );
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert0.x - edge_shift.x * c_cos_plus_45 + edge_shift.y * c_sin_plus_45,
					vert0.y - edge_shift.x * c_sin_plus_45 - edge_shift.y * c_cos_plus_45 },
				{ color_index_f, tex_coord } } );

		// Start of line.
		tex_coord+= cup_tex_coord_add;
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert0.x + edge_shift.x,
					vert0.y + edge_shift.y },
				{ color_index_f, tex_coord } } );
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert0.x - edge_shift.x,
					vert0.y - edge_shift.y },
				{ color_index_f, tex_coord } } );

		prev_edge_base_vec= edge_base_vec;
		tex_coord+= edge_length * tex_coord_scale;
	}

	for( size_t i= 1u; i < vertex_count - 1u; ++i )
	{
		const m_Vec2 vert( float(in_vertices[i].x), float(in_vertices[i].y) );
		const m_Vec2 edge_dir( float(in_vertices[i+1u].x) - vert.x, float(in_vertices[i+1u].y) - vert.y );
		const float edge_length= edge_dir.Length();
		PM_ASSERT( edge_length > 0.0f );
		const float edge_inv_length= 1.0f / edge_length;
		const m_Vec2 edge_base_vec( edge_dir.y * edge_inv_length, -edge_dir.x * edge_inv_length );

		const m_Vec2 vertex_base_vec= ( prev_edge_base_vec + edge_base_vec ) * 0.5f;
		const float vertex_base_vec_inv_square_len= 1.0f / std::max( 0.1f, vertex_base_vec.SquareLength() );

		const float edges_dir_dot= prev_edge_base_vec * edge_base_vec;

		const float c_rounding_ange= float(Constants::pi / 5.0);
		const float c_rounding_ange_cos= std::cos(c_rounding_ange);
		if( edges_dir_dot >= c_rounding_ange_cos )
		{
			const m_Vec2 vertex_shift= vertex_base_vec * ( half_width * vertex_base_vec_inv_square_len );

			out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
			out_vertices.push_back(
				PolygonalLinearObjectVertex{ {
						vert.x + vertex_shift.x,
						vert.y + vertex_shift.y },
					{ color_index_f, tex_coord } } );
			out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
			out_vertices.push_back(
				PolygonalLinearObjectVertex{ {
						vert.x - vertex_shift.x,
						vert.y - vertex_shift.y },
					{ color_index_f, tex_coord } } );
		}
		else
		{
			const float angle= std::atan2( mVec2Cross( prev_edge_base_vec, edge_base_vec ), edges_dir_dot );
			const float angle_abs= std::abs(angle);
			const size_t rounding_edges= std::max( size_t(1u), size_t(angle_abs / c_rounding_ange) );

			const float sign= angle > 0.0f ? 1.0f : -1.0f;

			const uint16_t corner_vertex_index= static_cast<uint16_t>(out_vertices.size());
			out_vertices.push_back(
				PolygonalLinearObjectVertex{ {
						vert.x - vertex_base_vec.x * ( half_width * vertex_base_vec_inv_square_len * sign ),
						vert.y - vertex_base_vec.y * ( half_width * vertex_base_vec_inv_square_len * sign ) },
					{ color_index_f, tex_coord } } );

			const m_Vec2 vertex_shift= prev_edge_base_vec * ( half_width * sign );
			const float angle_step= angle / float(rounding_edges);
			for( size_t i= 0u; i <= rounding_edges; ++i )
			{
				// Create one normal and one degenerated triangle.
				if( sign > 0.0f )
				{
					out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
					out_indices.push_back( corner_vertex_index );
				}
				else
				{
					out_indices.push_back( corner_vertex_index );
					out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
				}

				const float vert_angle= angle_step * float(i);
				const float vert_angle_cos= std::cos(vert_angle);
				const float vert_angle_sin= std::sin(vert_angle);

				out_vertices.push_back(
					PolygonalLinearObjectVertex{ {
							vert.x + vertex_shift.x * vert_angle_cos - vertex_shift.y * vert_angle_sin,
							vert.y + vertex_shift.x * vert_angle_sin + vertex_shift.y * vert_angle_cos },
						{ color_index_f, tex_coord } } );
			}
		}
		prev_edge_base_vec= edge_base_vec;
		tex_coord+= edge_length * tex_coord_scale;
	}

	{
		const m_Vec2 vert_last( float(in_vertices[vertex_count-1u].x), float(in_vertices[vertex_count-1u].y) );
		const m_Vec2 edge_shift= prev_edge_base_vec * half_width;

		// End of line
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert_last.x + edge_shift.x,
					vert_last.y + edge_shift.y },
				{ color_index_f, tex_coord } } );
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert_last.x - edge_shift.x,
					vert_last.y - edge_shift.y },
				{ color_index_f, tex_coord } } );

		// Cup.
		tex_coord+= cup_tex_coord_add;
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert_last.x + edge_shift.x * c_cos_plus_45 - edge_shift.y * c_sin_plus_45,
					vert_last.y + edge_shift.x * c_sin_plus_45 + edge_shift.y * c_cos_plus_45 },
				{ color_index_f, tex_coord } } );
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert_last.x - edge_shift.x * c_cos_minus_45 + edge_shift.y * c_sin_minus_45,
					vert_last.y - edge_shift.x * c_sin_minus_45 - edge_shift.y * c_cos_minus_45 },
				{ color_index_f, tex_coord } } );
		tex_coord+= cup_tex_coord_add;
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert_last.x - edge_shift.y,
					vert_last.y + edge_shift.x },
				{ color_index_f, tex_coord } } );
	}
	out_indices.push_back( c_primitive_restart_index );
}

struct MapDrawer::Chunk
{
public:
	Chunk(
		const DataFileDescription::Chunk& in_chunk,
		const DataFileDescription::LinearObjectStyle* const linear_styles,
		const DataFileDescription::ArealObjectStyle* const areal_styles )
		: src_chunk_(in_chunk), linear_styles_(linear_styles), areal_styles_(areal_styles)
		, coord_start_x_(in_chunk.coord_start_x), coord_start_y_(in_chunk.coord_start_y)
		, bb_min_x_(in_chunk.min_x), bb_min_y_(in_chunk.min_y), bb_max_x_(in_chunk.max_x), bb_max_y_(in_chunk.max_y)
	{
	}

	void PrepareGPUData()
	{
		if( gpu_data_prepared_ )
			return;
		gpu_data_prepared_= true;

		const unsigned char* const chunk_data= reinterpret_cast<const unsigned char*>(&src_chunk_);
		const auto vertices= reinterpret_cast<const DataFileDescription::ChunkVertex*>( chunk_data + src_chunk_.vertices_offset );
		const auto point_object_groups= reinterpret_cast<const DataFileDescription::Chunk::PointObjectGroup*>( chunk_data + src_chunk_.point_object_groups_offset );
		const auto linear_object_groups= reinterpret_cast<const DataFileDescription::Chunk::LinearObjectGroup*>( chunk_data + src_chunk_.linear_object_groups_offset );
		const auto areal_object_groups= reinterpret_cast<const DataFileDescription::Chunk::ArealObjectGroup*>( chunk_data + src_chunk_.areal_object_groups_offset );
		std::vector<PointObjectVertex> point_objects_vertices;

		std::vector<LinearObjectVertex> linear_objects_vertices;
		std::vector<uint16_t> linear_objects_indicies;

		std::vector<PolygonalLinearObjectVertex> linear_objects_as_triangles_vertices;
		std::vector<uint16_t> linear_objects_as_triangles_indicies;

		std::vector<ArealObjectVertex> areal_objects_vertices;
		std::vector<uint16_t> areal_objects_indicies;

		for( uint16_t i= 0u; i < src_chunk_.point_object_groups_count; ++i )
		{
			const DataFileDescription::Chunk::PointObjectGroup group= point_object_groups[i];
			for( uint16_t v= group.first_vertex; v < group.first_vertex + group.vertex_count; ++v )
			{
				const DataFileDescription::ChunkVertex& vertex= vertices[v];
				PointObjectVertex out_vertex;
				out_vertex.xy[0]= vertex.x;
				out_vertex.xy[1]= vertex.y;
				out_vertex.color_index= group.style_index;
				point_objects_vertices.push_back( out_vertex );
			}
		}

		// Draw polylines, using "GL_LINE_STRIP" primitive with primitive restart index.
		// Or draw it as "GL_TRIANGLE_STRIP".
		std::vector<DataFileDescription::ChunkVertex> tmp_vertices;
		for( uint16_t i= 0u; i < src_chunk_.linear_object_groups_count; ++i )
		{
			const DataFileDescription::Chunk::LinearObjectGroup group= linear_object_groups[i];
			LinearObjectsGroup out_group;
			out_group.style_index= group.style_index;
			out_group.z_level= uint8_t( group.z_level );

			if( linear_styles_[group.style_index].width_mul_256 > 0 )
			{
				out_group.first_index= linear_objects_as_triangles_indicies.size();

				const float half_width= float(linear_styles_[group.style_index].width_mul_256) / ( 256.0f * 2.0f );
				const float square_half_width= half_width * half_width;

				const float tex_coord_scale= 256.0f / float(linear_styles_[group.style_index].dash_size_mul_256);
				for( uint16_t v= group.first_vertex; v < group.first_vertex + group.vertex_count; ++v )
				{
					const DataFileDescription::ChunkVertex& vertex= vertices[v];
					if( vertex.x == 65535u )
					{
						SimplifyLine( tmp_vertices, square_half_width );
						if( !tmp_vertices.empty() )
							CreatePolygonalLine( tmp_vertices.data(), tmp_vertices.size(), group.style_index, half_width, tex_coord_scale, linear_objects_as_triangles_vertices, linear_objects_as_triangles_indicies );
						tmp_vertices.clear();
					}
					else
						tmp_vertices.push_back(vertex);
				}
				out_group.index_count= linear_objects_as_triangles_indicies.size() - out_group.first_index;
				out_group.primitive_type= GL_TRIANGLE_STRIP;
			}
			else
			{
				out_group.first_index= linear_objects_indicies.size();

				for( uint16_t v= group.first_vertex; v < group.first_vertex + group.vertex_count; ++v )
				{
					const DataFileDescription::ChunkVertex& vertex= vertices[v];
					if( vertex.x  == 65535u )
						linear_objects_indicies.push_back( c_primitive_restart_index );
					else
					{
						LinearObjectVertex out_vertex;
						out_vertex.xy[0]= vertex.x;
						out_vertex.xy[1]= vertex.y;
						out_vertex.tex_coord[0]= group.style_index;
						linear_objects_indicies.push_back( static_cast<uint16_t>( linear_objects_vertices.size() ) );
						linear_objects_vertices.push_back( out_vertex );
					}
				}
				out_group.index_count= linear_objects_indicies.size() - out_group.first_index;
				out_group.primitive_type= GL_LINE_STRIP;
			}
			linear_objects_groups_.push_back( out_group );
		}

		// Areal objects must be convex polygons.
		// Draw convex polygons, using "GL_TRIANGLE_FAN" primitive with primitive restart index.
		size_t prev_polygon_start_vertex_index= 0u;
		for( uint16_t i= 0u; i < src_chunk_.areal_object_groups_count; ++i )
		{
			const DataFileDescription::Chunk::ArealObjectGroup group= areal_object_groups[i];
			ArealObjectsGroup out_group;
			out_group.first_index= areal_objects_indicies.size();
			out_group.z_level= uint8_t( group.z_level );
			for( uint16_t v= group.first_vertex; v < group.first_vertex + group.vertex_count; ++v )
			{
				const DataFileDescription::ChunkVertex& vertex= vertices[v];
				if( vertex.x == 65535u )
				{
					areal_objects_indicies.push_back( c_primitive_restart_index );
					for( size_t i= prev_polygon_start_vertex_index; i < areal_objects_vertices.size(); ++i )
						areal_objects_vertices[i].color_index= vertex.y;
					prev_polygon_start_vertex_index= areal_objects_vertices.size();
				}
				else
				{
					ArealObjectVertex out_vertex;
					out_vertex.xy[0]= vertex.x;
					out_vertex.xy[1]= vertex.y;
					areal_objects_indicies.push_back( static_cast<uint16_t>( areal_objects_vertices.size() ) );
					areal_objects_vertices.push_back( out_vertex );
				}
			}

			out_group.index_count= areal_objects_indicies.size() - out_group.first_index;
			areal_objects_groups_.push_back(out_group);
		}

		point_objects_polygon_buffer_.VertexData( point_objects_vertices.data(), point_objects_vertices.size() * sizeof(PointObjectVertex), sizeof(PointObjectVertex) );
		point_objects_polygon_buffer_.SetPrimitiveType( GL_POINTS );
		point_objects_polygon_buffer_.VertexAttribPointer( 0, 2, GL_UNSIGNED_SHORT, false, 0 );
		point_objects_polygon_buffer_.VertexAttribPointer( 1, 1, GL_UNSIGNED_INT, false, sizeof(uint16_t) * 2 );

		PM_ASSERT( linear_objects_vertices.size() < 65535u );
		linear_objects_polygon_buffer_.VertexData( linear_objects_vertices.data(), linear_objects_vertices.size() * sizeof(LinearObjectVertex), sizeof(LinearObjectVertex) );
		linear_objects_polygon_buffer_.IndexData( linear_objects_indicies.data(), linear_objects_indicies.size() * sizeof(uint16_t), GL_UNSIGNED_SHORT, GL_LINE_STRIP );
		linear_objects_polygon_buffer_.VertexAttribPointer( 0, 2, GL_UNSIGNED_SHORT, false, 0 );
		linear_objects_polygon_buffer_.VertexAttribPointer( 1, 2, GL_UNSIGNED_SHORT, false, sizeof(uint16_t) * 2 );

		PM_ASSERT( linear_objects_as_triangles_vertices.size() < 65535u );
		linear_objects_as_triangles_buffer_.VertexData( linear_objects_as_triangles_vertices.data(), linear_objects_as_triangles_vertices.size() * sizeof(PolygonalLinearObjectVertex), sizeof(PolygonalLinearObjectVertex) );
		linear_objects_as_triangles_buffer_.IndexData( linear_objects_as_triangles_indicies.data(), linear_objects_as_triangles_indicies.size() * sizeof(uint16_t), GL_UNSIGNED_SHORT, GL_TRIANGLE_STRIP );
		linear_objects_as_triangles_buffer_.VertexAttribPointer( 0, 2, GL_FLOAT, true, 0 );
		linear_objects_as_triangles_buffer_.VertexAttribPointer( 1, 2, GL_FLOAT, false, sizeof(float) * 2 );

		PM_ASSERT( areal_objects_vertices.size() < 65535u );
		areal_objects_polygon_buffer_.VertexData( areal_objects_vertices.data(), areal_objects_vertices.size() * sizeof(ArealObjectVertex), sizeof(ArealObjectVertex) );
		areal_objects_polygon_buffer_.IndexData( areal_objects_indicies.data(), areal_objects_indicies.size() * sizeof(uint16_t), GL_UNSIGNED_SHORT, GL_TRIANGLE_FAN );
		areal_objects_polygon_buffer_.VertexAttribPointer( 0, 2, GL_UNSIGNED_SHORT, false, 0 );
		areal_objects_polygon_buffer_.VertexAttribPointer( 1, 1, GL_UNSIGNED_INT, false, sizeof(uint16_t) * 2 );
	}

	void ClearGPUData()
	{
		if( !gpu_data_prepared_ )
			return;

		gpu_data_prepared_= false;
		point_objects_polygon_buffer_= r_PolygonBuffer();
		linear_objects_polygon_buffer_= r_PolygonBuffer();
		linear_objects_as_triangles_buffer_= r_PolygonBuffer();
		areal_objects_polygon_buffer_= r_PolygonBuffer();
	}

	size_t GetGPUDataSize() const
	{
		size_t data_size= 0u;

		data_size+= point_objects_polygon_buffer_.GetVertexDataSize();
		data_size+= point_objects_polygon_buffer_.GetIndexDataSize();
		data_size+= linear_objects_polygon_buffer_.GetVertexDataSize();
		data_size+= linear_objects_polygon_buffer_.GetIndexDataSize();
		data_size+= linear_objects_as_triangles_buffer_.GetVertexDataSize();
		data_size+= linear_objects_as_triangles_buffer_.GetIndexDataSize();
		data_size+= areal_objects_polygon_buffer_.GetVertexDataSize();
		data_size+= areal_objects_polygon_buffer_.GetIndexDataSize();

		return data_size;
	}

	Chunk( const Chunk& )= delete;
	Chunk( Chunk&& )= default;

	Chunk& operator=( const Chunk& )= delete;
	Chunk& operator=( Chunk&& )= delete;

public:
	struct LinearObjectsGroup
	{
		size_t first_index;
		size_t index_count;
		GLenum primitive_type;
		uint8_t style_index;
		uint8_t z_level;
	};
	struct ArealObjectsGroup
	{
		size_t first_index;
		size_t index_count;
		uint8_t z_level;
	};

public:
	// References to memory mapped data file.
	// Memory mapped file must live longer, than "struct Chunk".
	const DataFileDescription::Chunk& src_chunk_;
	const DataFileDescription::LinearObjectStyle* const linear_styles_;
	const DataFileDescription::ArealObjectStyle* const areal_styles_;

	const int32_t coord_start_x_;
	const int32_t coord_start_y_;
	const int32_t bb_min_x_;
	const int32_t bb_min_y_;
	const int32_t bb_max_x_;
	const int32_t bb_max_y_;

	bool gpu_data_prepared_= false;
	r_PolygonBuffer point_objects_polygon_buffer_;
	r_PolygonBuffer linear_objects_polygon_buffer_;
	r_PolygonBuffer linear_objects_as_triangles_buffer_;
	r_PolygonBuffer areal_objects_polygon_buffer_;

	std::vector<LinearObjectsGroup> linear_objects_groups_;
	std::vector<ArealObjectsGroup> areal_objects_groups_;
};

struct MapDrawer::ZoomLevel
{
public:
	ZoomLevel( const DataFileDescription::ZoomLevel& in_zoom_level, const unsigned char* const file_content )
		: zoom_level_log2(in_zoom_level.zoom_level_log2)
	{
		const auto point_styles= reinterpret_cast<const DataFileDescription::PointObjectStyle*>( file_content + in_zoom_level.point_styles_offset );
		const auto linear_styles= reinterpret_cast<const DataFileDescription::LinearObjectStyle*>( file_content + in_zoom_level.linear_styles_offset );
		const auto areal_styles= reinterpret_cast<const DataFileDescription::ArealObjectStyle*>( file_content + in_zoom_level.areal_styles_offset );

		const auto chunks_description= reinterpret_cast<const DataFileDescription::DataFile::ChunkDescription*>( file_content + in_zoom_level.chunks_description_offset );

		for( uint32_t chunk_index= 0u; chunk_index < in_zoom_level.chunk_count; ++chunk_index )
		{
			const size_t chunk_offset= chunks_description[chunk_index].offset;
			const unsigned char* const chunk_data= file_content + chunk_offset;
			const DataFileDescription::Chunk& chunk= *reinterpret_cast<const DataFileDescription::Chunk*>(chunk_data);
			chunks.emplace_back( chunk, linear_styles, areal_styles );
		}

		// Extract linear styles
		this->linear_styles.insert( this->linear_styles.end(), linear_styles, linear_styles + in_zoom_level.linear_styles_count );

		// Extract styles order
		const auto in_linear_styles_order= reinterpret_cast<const DataFileDescription::LinearStylesOrder*>( file_content + in_zoom_level.linear_styles_order_offset );
		linear_styles_order.reserve( in_zoom_level.linear_styles_order_count );
		for( uint32_t i= 0u; i < in_zoom_level.linear_styles_order_count; ++i )
			linear_styles_order.push_back( in_linear_styles_order[i].style_index );

		// Create textures.
		// Use 2d textures for color tables, instead of 1d, because OpenGL ES does not support 1d textures.
		{
			const size_t texture_with_colors_width= 256u;
			const size_t texture_with_colors_height= 8u;
			DataFileDescription::ColorRGBA texture_data[ texture_with_colors_width * texture_with_colors_height ];

			std::memset( texture_data, 0, sizeof(texture_data) );
			for( uint32_t x= 0u; x < in_zoom_level.linear_styles_count; ++x )
			for( uint32_t y= 0u; y < texture_with_colors_height; ++y )
				std::memcpy( texture_data[ x + y * texture_with_colors_width ], linear_styles[x].color, sizeof(DataFileDescription::ColorRGBA) );
			linear_objects_texture= r_Texture( r_Texture::PixelFormat::RGBA8, texture_with_colors_width, texture_with_colors_height, reinterpret_cast<const unsigned char*>( texture_data ) );
			linear_objects_texture.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );

			std::memset( texture_data, 0, sizeof(texture_data) );
			for( uint32_t x= 0u; x < in_zoom_level.areal_styles_count; ++x )
			for( uint32_t y= 0u; y < texture_with_colors_height; ++y )
				std::memcpy( texture_data[ x + y * texture_with_colors_width ], areal_styles[x].color, sizeof(DataFileDescription::ColorRGBA) );
			areal_objects_texture= r_Texture( r_Texture::PixelFormat::RGBA8, texture_with_colors_width, texture_with_colors_height, reinterpret_cast<const unsigned char*>( texture_data ) );
			areal_objects_texture.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );
		}

		// Create textures for dashed lines.
		{
			const size_t tex_width= 128u;
			const size_t tex_height= 8u;
			DataFileDescription::ColorRGBA tex_data[ tex_width * tex_height ];

			for( DataFileDescription::Chunk::StyleIndex style_index= 0u; style_index < in_zoom_level.linear_styles_count; ++ style_index )
			{
				const DataFileDescription::LinearObjectStyle& style= linear_styles[style_index];
				if( std::memcmp( style.color, style.color2, sizeof(DataFileDescription::ColorRGBA) ) == 0 )
					continue;

				for( size_t y= 0u; y < tex_height; ++y )
				{
					for( size_t x= 0u; x < tex_width / 2u; ++x )
						std::memcpy( tex_data[ x + y * tex_width ], style.color , sizeof(DataFileDescription::ColorRGBA) );
					for( size_t x= tex_width / 2u; x < tex_width; ++x )
						std::memcpy( tex_data[ x + y * tex_width ], style.color2, sizeof(DataFileDescription::ColorRGBA) );
				}

				r_Texture texture( r_Texture::PixelFormat::RGBA8, tex_width, tex_height, reinterpret_cast<const unsigned char*>(tex_data) );
				texture.SetFiltration( r_Texture::Filtration::LinearMipmapLinear, r_Texture::Filtration::Linear );
				texture.BuildMips();
				dashed_lines_textures[style_index]= std::move(texture);
			}
		}

		// Create textures for point objects
		if( in_zoom_level.point_styles_count > 0u )
		{
			const size_t c_border_size= 4u;

			const size_t atlas_palce_height= DataFileDescription::PointObjectStyle::c_icon_size + c_border_size;
			const size_t atlas_height= atlas_palce_height * in_zoom_level.point_styles_count;
			std::vector<unsigned char> texture_data;
			texture_data.resize( DataFileDescription::PointObjectStyle::c_icon_size * atlas_height * 4u );

			for( uint16_t style_index= 0u; style_index < in_zoom_level.point_styles_count; ++style_index )
			{
				unsigned char* const dst_data= texture_data.data() + 4u * ( style_index * DataFileDescription::PointObjectStyle::c_icon_size * atlas_palce_height );
				std::memcpy(
					dst_data,
					point_styles[style_index].icon,
					sizeof(DataFileDescription::ColorRGBA) * DataFileDescription::PointObjectStyle::c_icon_size * DataFileDescription::PointObjectStyle::c_icon_size );

				std::memset(
					dst_data + 4u * DataFileDescription::PointObjectStyle::c_icon_size * DataFileDescription::PointObjectStyle::c_icon_size,
					0,
					sizeof(DataFileDescription::ColorRGBA) * DataFileDescription::PointObjectStyle::c_icon_size * c_border_size );
			}
			point_objects_icons_atlas=
				r_Texture(
					r_Texture::PixelFormat::RGBA8,
					DataFileDescription::PointObjectStyle::c_icon_size,
					static_cast<unsigned int>(atlas_height),
					texture_data.data() );
			point_objects_icons_atlas.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );
			point_objects_icons_atlas.BuildMips();

			point_object_icon_tc_step= 1.0f / float(in_zoom_level.point_styles_count);
			point_object_icon_tc_scale= float(DataFileDescription::PointObjectStyle::c_icon_size) / float(atlas_height);
			point_object_icon_size= float(DataFileDescription::PointObjectStyle::c_icon_size);
		}
	}

	ZoomLevel()= delete;

	ZoomLevel(const ZoomLevel&)= delete;
	ZoomLevel(ZoomLevel&&)= default;

	ZoomLevel& operator=(const ZoomLevel&)= delete;
	ZoomLevel&operator=( ZoomLevel&& other )= delete;

public:
	const size_t zoom_level_log2;
	std::vector<Chunk> chunks;
	std::vector< DataFileDescription::LinearObjectStyle > linear_styles;
	std::vector<uint8_t> linear_styles_order;

	std::unordered_map< DataFileDescription::Chunk::StyleIndex, r_Texture > dashed_lines_textures;

	// Textures in atlas palced from bottom to top.
	r_Texture point_objects_icons_atlas;
	float point_object_icon_tc_step, point_object_icon_tc_scale, point_object_icon_size;

	r_Texture linear_objects_texture;
	r_Texture areal_objects_texture;
};

struct MapDrawer::ChunkToDraw
{
	const MapDrawer::Chunk& chunk;
	m_Mat4 matrix;
};

MapDrawer::MapDrawer( const SystemWindow& system_window, UiDrawer& ui_drawer, const char* const map_file )
	: viewport_size_(system_window.GetViewportSize())
	, system_window_(system_window)
	, ui_drawer_(ui_drawer)
	, data_file_( MemoryMappedFile::Create( map_file ) )
{
	if( data_file_ == nullptr )
	{
		Log::FatalError( "Error, opening map file" );
		return;
	}
	if( data_file_->Size() < sizeof(DataFileDescription::DataFile) )
	{
		Log::FatalError( "Map file is too small" );
		return;
	}

	const unsigned char* const file_content= static_cast<const unsigned char*>(data_file_->Data());
	const DataFileDescription::DataFile& data_file= *reinterpret_cast<const DataFileDescription::DataFile*>( file_content);

	if( std::memcmp( data_file.header, DataFileDescription::DataFile::c_expected_header, sizeof(data_file.header) ) != 0 )
	{
		Log::FatalError( "File is not \"PanzerMaps\" file" );
		return;
	}
	if( data_file.version != DataFileDescription::DataFile::c_expected_version )
	{
		Log::FatalError( "Unsupported map file version. Expected ", DataFileDescription::DataFile::c_expected_version, ", got ", data_file.version, "." );
		return;
	}

	const auto zoom_levels= reinterpret_cast<const DataFileDescription::ZoomLevel*>( file_content + data_file.zoom_levels_offset );
	for( uint32_t zoom_level_index= 0u; zoom_level_index < data_file.zoom_level_count; ++zoom_level_index )
		zoom_levels_.emplace_back( zoom_levels[zoom_level_index], file_content );

	std::memcpy( background_color_, data_file.common_style.background_color, sizeof(background_color_) );

	if( data_file.common_style.copyright_image_width > 0 && data_file.common_style.copyright_image_height > 0 )
	{
		copyright_texture_=
			r_Texture(
				r_Texture::PixelFormat::RGBA8,
				data_file.common_style.copyright_image_width, data_file.common_style.copyright_image_height,
				file_content + data_file.common_style.copyright_image_offset );
		copyright_texture_.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );
	}

	// Create shaders

	point_objets_shader_.ShaderSource( Shaders::point_fragment, Shaders::point_vertex );
	point_objets_shader_.SetAttribLocation( "pos", 0 );
	point_objets_shader_.SetAttribLocation( "color_index", 1 );
	point_objets_shader_.Create();

	linear_objets_shader_.ShaderSource( Shaders::linear_fragment, Shaders::linear_vertex );
	linear_objets_shader_.SetAttribLocation( "pos", 0 );
	linear_objets_shader_.SetAttribLocation( "tex_coord", 1 );
	linear_objets_shader_.Create();

	linear_textured_objets_shader_.ShaderSource( Shaders::linear_textured_fragment, Shaders::linear_textured_vertex );
	linear_textured_objets_shader_.SetAttribLocation( "pos", 0 );
	linear_textured_objets_shader_.SetAttribLocation( "tex_coord", 1 );
	linear_textured_objets_shader_.Create();

	areal_objects_shader_.ShaderSource( Shaders::areal_fragment, Shaders::areal_vertex );
	areal_objects_shader_.SetAttribLocation( "pos", 0 );
	areal_objects_shader_.SetAttribLocation( "color_index", 1 );
	areal_objects_shader_.Create();

	// Setup camera
	if( !zoom_levels_.empty() )
	{
		min_cam_pos_= m_Vec2( +1e20f, +1e20f );
		max_cam_pos_= m_Vec2( -1e20f, -1e20f );
		for( const Chunk& chunk : zoom_levels_.front().chunks )
		{
			min_cam_pos_.x= std::min( min_cam_pos_.x, float(chunk.coord_start_x_) );
			min_cam_pos_.y= std::min( min_cam_pos_.y, float(chunk.coord_start_y_) );
			max_cam_pos_.x= std::max( max_cam_pos_.x, float(chunk.coord_start_x_ + 65536) );
			max_cam_pos_.y= std::max( max_cam_pos_.y, float(chunk.coord_start_y_ + 65536) );
		}
	}
	else
		min_cam_pos_= max_cam_pos_= m_Vec2( 0.0f, 0.0f );
	cam_pos_= ( min_cam_pos_ + max_cam_pos_ ) * 0.5f;

	unit_size_m_= zoom_levels[0u].unit_size_m;

	min_scale_= 1.0f;
	max_scale_= 2.0f * std::max( max_cam_pos_.x - min_cam_pos_.x, max_cam_pos_.y - min_cam_pos_.y ) / float( std::max( viewport_size_.width, viewport_size_.height ) );
	scale_= max_scale_;
}

MapDrawer::~MapDrawer()
{
}

void MapDrawer::Draw()
{
#ifdef PM_OPENGL_ES
	const auto enable_primitive_restart= [] { glEnable( GL_PRIMITIVE_RESTART_FIXED_INDEX ); };
	const auto disable_primitive_restart= [] { glDisable( GL_PRIMITIVE_RESTART_FIXED_INDEX ); };
#else
	const auto enable_primitive_restart= [] { glEnable( GL_PRIMITIVE_RESTART ); glPrimitiveRestartIndex( c_primitive_restart_index ); };
	const auto disable_primitive_restart= [] { glDisable( GL_PRIMITIVE_RESTART ); };
#endif

	// Clear background.
	glClearColor( float(background_color_[0]) / 255.0f, float(background_color_[1]) / 255.0f, float(background_color_[2]) / 255.0f, float(background_color_[3]) / 255.0f );
	glClear( GL_COLOR_BUFFER_BIT );

	// Setup OpenGL state.
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	ZoomLevel& zoom_level= SelectZoomLevel();

	// Calculate view matrix.
	// TODO - maybe use m_Mat3?
	m_Mat4 zoom_level_matrix, translate_matrix, scale_matrix, aspect_matrix, view_matrix;
	zoom_level_matrix.Scale( float( 1u << zoom_level.zoom_level_log2 ) );
	translate_matrix.Translate( m_Vec3( -cam_pos_, 0.0f ) );
	scale_matrix.Scale( 1.0f / scale_ );
	aspect_matrix.Scale( 2.0f * m_Vec3( 1.0f / float(viewport_size_.width), 1.0f / float(viewport_size_.height), 1.0f ) );
	view_matrix= zoom_level_matrix * translate_matrix * scale_matrix * aspect_matrix;

	// Calculate viewport bounding box.
	const int32_t bb_extend_eps= 16;
	const int32_t viewport_half_size_x_world_space= int32_t( float(viewport_size_.width  ) * 0.5f * scale_ ) + bb_extend_eps;
	const int32_t viewport_half_size_y_world_space= int32_t( float(viewport_size_.height ) * 0.5f * scale_ ) + bb_extend_eps;
	const int32_t cam_pos_x_world_space= int32_t(cam_pos_.x);
	const int32_t cam_pos_y_world_space= int32_t(cam_pos_.y);
	const int32_t bb_min_x= ( cam_pos_x_world_space - viewport_half_size_x_world_space ) >> zoom_level.zoom_level_log2;
	const int32_t bb_max_x= ( cam_pos_x_world_space + viewport_half_size_x_world_space ) >> zoom_level.zoom_level_log2;
	const int32_t bb_min_y= ( cam_pos_y_world_space - viewport_half_size_y_world_space ) >> zoom_level.zoom_level_log2;
	const int32_t bb_max_y= ( cam_pos_y_world_space + viewport_half_size_y_world_space ) >> zoom_level.zoom_level_log2;

	// Setup chunks list, calculate matrices.
	std::vector<ChunkToDraw> visible_chunks;
	uint16_t min_z_level= 100u, max_z_level= 0u;
	for( Chunk& chunk : zoom_level.chunks )
	{
		if( chunk.bb_min_x_ >= bb_max_x || chunk.bb_min_y_ >= bb_max_y ||
			chunk.bb_max_x_ <= bb_min_x || chunk.bb_max_y_ <= bb_min_y )
			continue;

		chunk.PrepareGPUData();

		m_Mat4 coords_shift_matrix, chunk_view_matrix;
		coords_shift_matrix.Translate( m_Vec3( float(chunk.coord_start_x_), float(chunk.coord_start_y_), 0.0f ) );
		chunk_view_matrix= coords_shift_matrix * view_matrix;

		visible_chunks.push_back( ChunkToDraw{ chunk, chunk_view_matrix } );

		min_z_level= std::min( min_z_level, chunk.src_chunk_.min_z_level );
		max_z_level= std::max( max_z_level, chunk.src_chunk_.max_z_level );
	}

	// Draw chunks.
	size_t draw_calls= 0u;
	size_t primitive_count= 0u;

	// Draw areal and linear objects ordered by z_level.
	for( uint16_t z_level= min_z_level; z_level <= max_z_level; ++z_level )
	{
		// -- Areal --

		areal_objects_shader_.Bind();
		areal_objects_shader_.Uniform( "tex", 0 );

		zoom_level.areal_objects_texture.Bind(0);

		enable_primitive_restart();
		for( const ChunkToDraw& chunk_to_draw : visible_chunks )
		{
			if( chunk_to_draw.chunk.areal_objects_polygon_buffer_.GetVertexDataSize() == 0u )
				continue;

			areal_objects_shader_.Uniform( "view_matrix", chunk_to_draw.matrix );

			for( const Chunk::ArealObjectsGroup& group : chunk_to_draw.chunk.areal_objects_groups_ )
			{
				if( group.z_level == z_level && group.index_count > 0u )
				{
					chunk_to_draw.chunk.areal_objects_polygon_buffer_.Bind();
					glDrawElements( GL_TRIANGLE_FAN, static_cast<int>(group.index_count), GL_UNSIGNED_SHORT, reinterpret_cast<GLsizei*>( group.first_index * sizeof(uint16_t) ) );
					++draw_calls;
					primitive_count+= group.index_count;
				}
			}
		}
		disable_primitive_restart();

		// -- linear --

		enable_primitive_restart();

		// Draw linear objects ordered by style and z_level but not by chunk, because linear objects of neighboring chunks may overlap.
		for( const uint8_t style_index : zoom_level.linear_styles_order )
		{
			for( const ChunkToDraw& chunk_to_draw : visible_chunks )
			{
				if( chunk_to_draw.chunk.linear_objects_polygon_buffer_.GetVertexDataSize() == 0u &&
					chunk_to_draw.chunk.linear_objects_as_triangles_buffer_.GetVertexDataSize() == 0u )
					continue;

				for( const Chunk::LinearObjectsGroup& group : chunk_to_draw.chunk.linear_objects_groups_ )
				{
					if( group.style_index == style_index && group.z_level == z_level && group.index_count > 0u )
					{
						const auto tex_it= zoom_level.dashed_lines_textures.find( group.style_index );
						if( tex_it != zoom_level.dashed_lines_textures.end() )
						{
							linear_textured_objets_shader_.Bind();
							linear_textured_objets_shader_.Uniform( "tex", 0 );
							linear_textured_objets_shader_.Uniform( "view_matrix", chunk_to_draw.matrix );
							tex_it->second.Bind(0);

							glEnable( GL_BLEND );
						}
						else
						{
							linear_objets_shader_.Bind();
							linear_objets_shader_.Uniform( "tex", 0 );
							linear_objets_shader_.Uniform( "view_matrix", chunk_to_draw.matrix );
							zoom_level.linear_objects_texture.Bind(0);

							if( zoom_level.linear_styles[style_index].color[3] != 255u )
								glEnable( GL_BLEND );
						}

						if( group.primitive_type == GL_LINE_STRIP )
							chunk_to_draw.chunk.linear_objects_polygon_buffer_.Bind();
						else
							chunk_to_draw.chunk.linear_objects_as_triangles_buffer_.Bind();
						glDrawElements( group.primitive_type, static_cast<int>(group.index_count), GL_UNSIGNED_SHORT, reinterpret_cast<GLsizei*>( group.first_index * sizeof(uint16_t) ) );
						++draw_calls;
						primitive_count+= group.index_count;

						glDisable( GL_BLEND );
					}
				}
			}
		}
		disable_primitive_restart();
	}
	if( !zoom_level.point_objects_icons_atlas.IsEmpty() )
	{
		point_objets_shader_.Bind();
		point_objets_shader_.Uniform( "tex", 0 );
		point_objets_shader_.Uniform( "point_size", zoom_level.point_object_icon_size );
		point_objets_shader_.Uniform( "icon_tc_step", zoom_level.point_object_icon_tc_step );
		point_objets_shader_.Uniform( "icon_tc_scale", zoom_level.point_object_icon_tc_scale );

		zoom_level.point_objects_icons_atlas.Bind();

		#ifndef PM_OPENGL_ES
		glEnable( GL_PROGRAM_POINT_SIZE ); // In OpenGL ES program point size is default behaviour.
		#endif
		glEnable( GL_BLEND );
		for( const ChunkToDraw& chunk_to_draw : visible_chunks )
		{
			if( chunk_to_draw.chunk.point_objects_polygon_buffer_.GetVertexDataSize() == 0u )
				continue;

			point_objets_shader_.Uniform( "view_matrix", chunk_to_draw.matrix );
			chunk_to_draw.chunk.point_objects_polygon_buffer_.Draw();
			++draw_calls;
			primitive_count+= chunk_to_draw.chunk.point_objects_polygon_buffer_.GetVertexDataSize() / sizeof(PointObjectVertex);
		}
		glDisable( GL_BLEND );
		#ifndef PM_OPENGL_ES
		glDisable( GL_PROGRAM_POINT_SIZE );
		#endif
	}

	if( !copyright_texture_.IsEmpty() )
	{
		const unsigned int c_border= 4u;
		ui_drawer_.DrawUiElement(
			viewport_size_.width - copyright_texture_.Width() - c_border,
			c_border + copyright_texture_.Height(),
			+copyright_texture_.Width (),
			-copyright_texture_.Height(),
			copyright_texture_ );
	}

	if( ( frame_number_ & 15u ) == 0u )
		ClearGPUData();

	if( ( frame_number_ & 63u ) == 0u )
		Log::User( "Visible chunks: ", visible_chunks.size(), " draw calls: ", draw_calls, " index count: ", primitive_count );

	if( ( frame_number_ & 255u ) == 0u )
	{
		// Calculate GPU statistics.
		size_t total_gpu_data_size= 0u;
		for( const ZoomLevel& zoom_level : zoom_levels_ )
		{
			size_t zoom_level_gpu_data_size= 0u;
			for( const Chunk& chunk : zoom_level.chunks )
				zoom_level_gpu_data_size+= chunk.GetGPUDataSize();
			Log::Info( "Zoom level ", &zoom_level - zoom_levels_.data(), " GPU data size: ", zoom_level_gpu_data_size / 1024u, "kb" );
			total_gpu_data_size+= zoom_level_gpu_data_size;
		}
		Log::Info( "Total GPU data size: ", total_gpu_data_size / 1024u, "kb = ", total_gpu_data_size / 1024u / 1024, "mb" );
	}

	++frame_number_;
}

float MapDrawer::GetScale() const
{
	return scale_;
}

void MapDrawer::SetScale( const float scale )
{
	scale_= std::max( min_scale_, std::min( scale, max_scale_ ) );
}

const m_Vec2& MapDrawer::GetPosition() const
{
	return cam_pos_;
}

void MapDrawer::SetPosition( const m_Vec2& position )
{
	cam_pos_.x= std::max( min_cam_pos_.x, std::min( position.x, max_cam_pos_.x ) );
	cam_pos_.y= std::max( min_cam_pos_.y, std::min( position.y, max_cam_pos_.y ) );
}

const ViewportSize& MapDrawer::GetViewportSize() const
{
	return viewport_size_;
}

MapDrawer::ZoomLevel& MapDrawer::SelectZoomLevel()
{
	// Assume, that nearest scale is equivalent for WebMercator zoom 18.

	const float c_default_pixels_in_m= 3779.0f;
	const float pixel_density_scaler= system_window_.GetPixelsInScreenMeter() / c_default_pixels_in_m;

	for( size_t i= 1u; i < zoom_levels_.size(); ++i )
		if( scale_ * 0.5f * pixel_density_scaler < float( 1u << zoom_levels_[i].zoom_level_log2 ) )
			return zoom_levels_[i-1u];

	return zoom_levels_.back();
}

void MapDrawer::ClearGPUData()
{
	size_t total_gpu_data_size= 0u;
	for( const ZoomLevel& zoom_level : zoom_levels_ )
	{
		size_t zoom_level_gpu_data_size= 0u;
		for( const Chunk& chunk : zoom_level.chunks )
			zoom_level_gpu_data_size+= chunk.GetGPUDataSize();
		total_gpu_data_size+= zoom_level_gpu_data_size;
	}

#ifdef __ANDROID__
	const size_t c_gpu_memory_limit=  64u * 1024u * 1024u;
#else
	const size_t c_gpu_memory_limit= 128u * 1024u * 1024u;
#endif

	if( total_gpu_data_size <= c_gpu_memory_limit )
		return;

	size_t current_zoom_level_log2= SelectZoomLevel().zoom_level_log2;

	// Try clear GPU data of some chunks.
	// Chunks, with greater distance to camera removed firstly.
	// Chunks of zoom levels, different, from current, removed firstly.
	struct QueueChunk
	{
		Chunk* chunk;
		int32_t weight;
	};
	std::vector< QueueChunk > chunks_queue;

	for( ZoomLevel& zoom_level : zoom_levels_ )
	for( Chunk& chunk : zoom_level.chunks )
	{
		const int32_t center_x= ( ( chunk.bb_min_x_ + chunk.bb_max_x_ ) >> 1 ) << zoom_level.zoom_level_log2;
		const int32_t center_y= ( ( chunk.bb_min_y_ + chunk.bb_max_y_ ) >> 1 ) << zoom_level.zoom_level_log2;
		const int64_t dx= center_x - int32_t(cam_pos_.x);
		const int64_t dy= center_y - int32_t(cam_pos_.y);
		const int32_t distance= int32_t( std::sqrt( double( dx * dx + dy * dy ) ) ) >> zoom_level.zoom_level_log2;
		const int32_t weight=
		65535 * std::abs( int32_t(zoom_level.zoom_level_log2) - int32_t(current_zoom_level_log2) ) +
			1 * distance;
		PM_ASSERT( weight >= 0 );

		chunks_queue.emplace_back( QueueChunk{ &chunk, weight } );
	}

	// Sort by weight in descent order.
	std::sort(
		chunks_queue.begin(), chunks_queue.end(),
		[](const QueueChunk& l, const QueueChunk& r) { return l.weight > r.weight; });

	for( const QueueChunk& queue_chunk : chunks_queue )
	{
		total_gpu_data_size-= queue_chunk.chunk->GetGPUDataSize();
		queue_chunk.chunk->ClearGPUData();
		if( total_gpu_data_size <= c_gpu_memory_limit * 3u / 4u )
			break;
	}
}

} // namespace PanzerMaps
