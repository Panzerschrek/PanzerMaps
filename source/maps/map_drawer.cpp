#include <algorithm>
#include <cstring>
#include "../common/assert.hpp"
#include "../common/coordinates_conversion.hpp"
#include "../common/data_file.hpp"
#include "../common/log.hpp"
#include "../common/memory_mapped_file.hpp"
#include "map_drawer.hpp"

namespace PanzerMaps
{

namespace Shaders
{

const char point_vertex[]=
R"(
	#version 330
	uniform mat4 view_matrix;
	in vec2 pos;
	in float color_index;
	out vec4 f_color;
	void main()
	{
		f_color= vec4( mod( color_index, 2.0 ), mod( color_index, 4.0 ) / 3.0, mod( color_index, 8.0 ) / 7.0, 0.5 );
		gl_Position= view_matrix * vec4( pos, 0.0, 1.0 );
	}
)";

const char point_fragment[]=
R"(
	#version 330
	in vec4 f_color;
	out vec4 color;
	void main()
	{
		vec2 r= gl_PointCoord * 2.0 - vec2( 1.0, 1.0 );
		if( dot( r, r ) > 1.0 )
			discard;
		color= f_color * step( 0.0, r.x * r.y );
	}
)";

const char linear_vertex[]=
R"(
	#version 330
	uniform sampler1D tex;
	uniform mat4 view_matrix;
	in vec2 pos;
	in float color_index;
	out vec4 f_color;
	void main()
	{
		f_color= texelFetch( tex, int(color_index), 0 );
		gl_Position= view_matrix * vec4( pos, 0.0, 1.0 );
	}
)";

const char linear_fragment[]=
R"(
	#version 330
	in vec4 f_color;
	out vec4 color;
	void main()
	{
		color= f_color;
	}
)";

const char areal_vertex[]=
R"(
	#version 330
	uniform sampler1D tex;
	uniform mat4 view_matrix;
	in vec2 pos;
	in float color_index;
	out vec4 f_color;
	void main()
	{
		f_color= texelFetch( tex, int(color_index), 0 );
		gl_Position= view_matrix * vec4( pos, 0.0, 1.0 );
	}
)";

const char areal_fragment[]=
R"(
	#version 330
	in vec4 f_color;
	out vec4 color;
	void main()
	{
		color= f_color;
	}
)";

}

struct PointObjectVertex
{
	uint16_t xy[2];
	uint32_t color_index;
};

struct LinearObjectVertex
{
	uint16_t xy[2];
	uint32_t color_index;
};

struct PolygonalLinearObjectVertex
{
	float xy[2];
	uint32_t color_index;
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

static void SimplifyLine( std::vector<DataFileDescription::ChunkVertex>& line )
{
	// TODO - simplify, using line width.
	// TODO - fix equal points in source data.
	line.erase( std::unique( line.begin(), line.end() ), line.end() );
}

// Creates triangle strip mesh.
static void CreatePolygonalLine(
	const DataFileDescription::ChunkVertex* const in_vertices,
	const size_t vertex_count,
	const uint32_t color_index,
	const float half_width,
	std::vector<PolygonalLinearObjectVertex>& out_vertices,
	std::vector<uint16_t>& out_indices )
{
	PM_ASSERT( vertex_count >= 2u );

	// Use float coordinates, because uint16_t is too low for polygonal lines with small width.
	m_Vec2 prev_edge_base_vec;
	{
		const m_Vec2 edge_dir( float(in_vertices[1u].x) - float(in_vertices[0u].x), float(in_vertices[1u].y) - float(in_vertices[0u].y) );
		PM_ASSERT( edge_dir.Length() > 0.0f );
		const float edge_inv_length= edge_dir.InvLength();
		const m_Vec2 edge_base_vec( edge_dir.y * edge_inv_length, -edge_dir.x * edge_inv_length );

		const float edge_shift_x= edge_base_vec.x * half_width;
		const float edge_shift_y= edge_base_vec.y * half_width;

		const m_Vec2 vert0( float(in_vertices[0u].x), float(in_vertices[0u].y) );
		// Cup.
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert0.x + edge_shift_y,
					vert0.y - edge_shift_x },
				color_index } );
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert0.x + edge_shift_x * c_cos_minus_45 - edge_shift_y * c_sin_minus_45,
					vert0.y + edge_shift_x * c_sin_minus_45 + edge_shift_y * c_cos_minus_45 },
				color_index } );
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert0.x - edge_shift_x * c_cos_plus_45 + edge_shift_y * c_sin_plus_45,
					vert0.y - edge_shift_x * c_sin_plus_45 - edge_shift_y * c_cos_plus_45 },
				color_index } );

		// Start of line.
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert0.x + edge_shift_x,
					vert0.y + edge_shift_y },
				color_index } );
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert0.x - edge_shift_x,
					vert0.y - edge_shift_y },
				color_index } );

		prev_edge_base_vec= edge_base_vec;
	}

	for( size_t i= 1u; i < vertex_count - 1u; ++i )
	{
		const m_Vec2 vert( float(in_vertices[i].x), float(in_vertices[i].y) );

		const m_Vec2 edge_dir( float(in_vertices[i+1u].x) - vert.x, float(in_vertices[i+1u].y) - vert.y );
		PM_ASSERT( edge_dir.Length() > 0.0f );
		const float edge_inv_length= edge_dir.InvLength();
		const m_Vec2 edge_base_vec( edge_dir.y * edge_inv_length, -edge_dir.x * edge_inv_length );

		const m_Vec2 vertex_base_vec= ( prev_edge_base_vec + edge_base_vec ) * 0.5f;
		const float vertex_base_vec_inv_square_len= 1.0f / std::max( 0.1f, vertex_base_vec.SquareLength() );

		const float edges_dir_dot= prev_edge_base_vec * edge_base_vec;

		const float c_rounding_ange= float(Constants::pi / 5.0);
		const float c_rounding_ange_cos= std::cos(c_rounding_ange);
		if( edges_dir_dot >= c_rounding_ange_cos )
		{
			const float vertex_shift_x= vertex_base_vec.x * ( half_width * vertex_base_vec_inv_square_len );
			const float vertex_shift_y= vertex_base_vec.y * ( half_width * vertex_base_vec_inv_square_len );

			out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
			out_vertices.push_back(
				PolygonalLinearObjectVertex{ {
						vert.x + vertex_shift_x,
						vert.y + vertex_shift_y },
					color_index } );
			out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
			out_vertices.push_back(
				PolygonalLinearObjectVertex{ {
						vert.x - vertex_shift_x,
						vert.y - vertex_shift_y },
					color_index } );
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
					color_index } );

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
						color_index } );
			}
		}

		prev_edge_base_vec= edge_base_vec;
	}

	{
		const float edge_shift_x= prev_edge_base_vec.x * half_width;
		const float edge_shift_y= prev_edge_base_vec.y * half_width;

		const m_Vec2 vert_last( float(in_vertices[vertex_count-1u].x), float(in_vertices[vertex_count-1u].y) );

		// End of line
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert_last.x + edge_shift_x,
					vert_last.y + edge_shift_y },
				color_index } );
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert_last.x - edge_shift_x,
					vert_last.y - edge_shift_y },
				color_index } );

		// Cup.
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert_last.x + edge_shift_x * c_cos_plus_45 - edge_shift_y * c_sin_plus_45,
					vert_last.y + edge_shift_x * c_sin_plus_45 + edge_shift_y * c_cos_plus_45 },
				color_index } );
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert_last.x - edge_shift_x * c_cos_minus_45 + edge_shift_y * c_sin_minus_45,
					vert_last.y - edge_shift_x * c_sin_minus_45 - edge_shift_y * c_cos_minus_45 },
				color_index } );
		out_indices.push_back( static_cast<uint16_t>(out_vertices.size()) );
		out_vertices.push_back(
			PolygonalLinearObjectVertex{ {
					vert_last.x - edge_shift_y,
					vert_last.y + edge_shift_x },
				color_index } );
	}
	out_indices.push_back( c_primitive_restart_index );
}

struct MapDrawer::Chunk
{
	Chunk(
		const DataFileDescription::Chunk& in_chunk,
		const DataFileDescription::LinearObjectStyle* const linear_styles,
		const DataFileDescription::ArealObjectStyle* const areal_styles )
		: coord_start_x_(in_chunk.coord_start_x), coord_start_y_(in_chunk.coord_start_y)
		, bb_min_x_(in_chunk.min_x), bb_min_y_(in_chunk.min_y), bb_max_x_(in_chunk.max_x), bb_max_y_(in_chunk.max_y)
	{
		(void)areal_styles;

		const unsigned char* const chunk_data= reinterpret_cast<const unsigned char*>(&in_chunk);
		const auto vertices= reinterpret_cast<const DataFileDescription::ChunkVertex*>( chunk_data + in_chunk.vertices_offset );
		const auto point_object_groups= reinterpret_cast<const DataFileDescription::Chunk::PointObjectGroup*>( chunk_data + in_chunk.point_object_groups_offset );
		const auto linear_object_groups= reinterpret_cast<const DataFileDescription::Chunk::LinearObjectGroup*>( chunk_data + in_chunk.linear_object_groups_offset );
		const auto areal_object_groups= reinterpret_cast<const DataFileDescription::Chunk::ArealObjectGroup*>( chunk_data + in_chunk.areal_object_groups_offset );
		std::vector<PointObjectVertex> point_objects_vertices;

		std::vector<LinearObjectVertex> linear_objects_vertices;
		std::vector<uint16_t> linear_objects_indicies;

		std::vector<PolygonalLinearObjectVertex> linear_objects_as_triangles_vertices;
		std::vector<uint16_t> linear_objects_as_triangles_indicies;

		std::vector<ArealObjectVertex> areal_objects_vertices;
		std::vector<uint16_t> areal_objects_indicies;

		for( uint16_t i= 0u; i < in_chunk.point_object_groups_count; ++i )
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
		for( uint16_t i= 0u; i < in_chunk.linear_object_groups_count; ++i )
		{
			const DataFileDescription::Chunk::LinearObjectGroup group= linear_object_groups[i];
			if( linear_styles[group.style_index].width_mul_256 > 0 )
			{
				const float half_width= float(linear_styles[group.style_index].width_mul_256) / ( 256.0f * 2.0f );
				for( uint16_t v= group.first_vertex; v < group.first_vertex + group.vertex_count; ++v )
				{
					const DataFileDescription::ChunkVertex& vertex= vertices[v];
					if( ( vertex.x & vertex.y ) == 65535u )
					{
						SimplifyLine( tmp_vertices );
						if( tmp_vertices.size() >= 2u )
							CreatePolygonalLine( tmp_vertices.data(), tmp_vertices.size(), group.style_index, half_width, linear_objects_as_triangles_vertices, linear_objects_as_triangles_indicies );
						tmp_vertices.clear();
					}
					else
						tmp_vertices.push_back(vertex);
				}
			}
			else
			{
				for( uint16_t v= group.first_vertex; v < group.first_vertex + group.vertex_count; ++v )
				{
					const DataFileDescription::ChunkVertex& vertex= vertices[v];
					if( ( vertex.x & vertex.y ) == 65535u )
						linear_objects_indicies.push_back( c_primitive_restart_index );
					else
					{
						LinearObjectVertex out_vertex;
						out_vertex.xy[0]= vertex.x;
						out_vertex.xy[1]= vertex.y;
						out_vertex.color_index= group.style_index;
						linear_objects_indicies.push_back( static_cast<uint16_t>( linear_objects_vertices.size() ) );
						linear_objects_vertices.push_back( out_vertex );
					}
				}
			}
		}

		// Areal objects must be convex polygons.
		// Draw convex polygons, using "GL_TRIANGLE_FAN" primitive with primitive restart index.
		for( uint16_t i= 0u; i < in_chunk.areal_object_groups_count; ++i )
		{
			const DataFileDescription::Chunk::ArealObjectGroup group= areal_object_groups[i];
			for( uint16_t v= group.first_vertex; v < group.first_vertex + group.vertex_count; ++v )
			{
				const DataFileDescription::ChunkVertex& vertex= vertices[v];
				if( ( vertex.x & vertex.y ) == 65535u )
					areal_objects_indicies.push_back( c_primitive_restart_index );
				else
				{
					ArealObjectVertex out_vertex;
					out_vertex.xy[0]= vertex.x;
					out_vertex.xy[1]= vertex.y;
					out_vertex.color_index= group.style_index;
					areal_objects_indicies.push_back( static_cast<uint16_t>( areal_objects_vertices.size() ) );
					areal_objects_vertices.push_back( out_vertex );
				}
			}
		}

		point_objects_polygon_buffer_.VertexData( point_objects_vertices.data(), point_objects_vertices.size() * sizeof(PointObjectVertex), sizeof(PointObjectVertex) );
		point_objects_polygon_buffer_.SetPrimitiveType( GL_POINTS );
		point_objects_polygon_buffer_.VertexAttribPointer( 0, 2, GL_UNSIGNED_SHORT, false, 0 );
		point_objects_polygon_buffer_.VertexAttribPointer( 1, 1, GL_UNSIGNED_INT, false, sizeof(uint16_t) * 2 );

		PM_ASSERT( linear_objects_vertices.size() < 65535u );
		linear_objects_polygon_buffer_.VertexData( linear_objects_vertices.data(), linear_objects_vertices.size() * sizeof(LinearObjectVertex), sizeof(LinearObjectVertex) );
		linear_objects_polygon_buffer_.IndexData( linear_objects_indicies.data(), linear_objects_indicies.size() * sizeof(uint16_t), GL_UNSIGNED_SHORT, GL_LINE_STRIP );
		linear_objects_polygon_buffer_.VertexAttribPointer( 0, 2, GL_UNSIGNED_SHORT, false, 0 );
		linear_objects_polygon_buffer_.VertexAttribPointer( 1, 1, GL_UNSIGNED_INT, false, sizeof(uint16_t) * 2 );

		PM_ASSERT( linear_objects_as_triangles_vertices.size() < 65535u );
		linear_objects_as_triangles_buffer_.VertexData( linear_objects_as_triangles_vertices.data(), linear_objects_as_triangles_vertices.size() * sizeof(PolygonalLinearObjectVertex), sizeof(PolygonalLinearObjectVertex) );
		linear_objects_as_triangles_buffer_.IndexData( linear_objects_as_triangles_indicies.data(), linear_objects_as_triangles_indicies.size() * sizeof(uint16_t), GL_UNSIGNED_SHORT, GL_TRIANGLE_STRIP );
		linear_objects_as_triangles_buffer_.VertexAttribPointer( 0, 2, GL_FLOAT, true, 0 );
		linear_objects_as_triangles_buffer_.VertexAttribPointer( 1, 1, GL_UNSIGNED_INT, false, sizeof(float) * 2 );

		PM_ASSERT( areal_objects_vertices.size() < 65535u );
		areal_objects_polygon_buffer_.VertexData( areal_objects_vertices.data(), areal_objects_vertices.size() * sizeof(ArealObjectVertex), sizeof(ArealObjectVertex) );
		areal_objects_polygon_buffer_.IndexData( areal_objects_indicies.data(), areal_objects_indicies.size() * sizeof(uint16_t), GL_UNSIGNED_SHORT, GL_TRIANGLE_FAN );
		areal_objects_polygon_buffer_.VertexAttribPointer( 0, 2, GL_UNSIGNED_SHORT, false, 0 );
		areal_objects_polygon_buffer_.VertexAttribPointer( 1, 1, GL_UNSIGNED_INT, false, sizeof(uint16_t) * 2 );
	}

	Chunk( const Chunk& )= delete;
	Chunk( Chunk&& )= default;

	Chunk& operator=( const Chunk& )= delete;
	Chunk& operator=( Chunk&& )= default;

	r_PolygonBuffer point_objects_polygon_buffer_;
	r_PolygonBuffer linear_objects_polygon_buffer_;
	r_PolygonBuffer linear_objects_as_triangles_buffer_;
	r_PolygonBuffer areal_objects_polygon_buffer_;
	const int32_t coord_start_x_;
	const int32_t coord_start_y_;
	const int32_t bb_min_x_;
	const int32_t bb_min_y_;
	const int32_t bb_max_x_;
	const int32_t bb_max_y_;
};

struct MapDrawer::ChunkToDraw
{
	const MapDrawer::Chunk& chunk;
	m_Mat4 matrix;
};

MapDrawer::MapDrawer( const ViewportSize& viewport_size )
	: viewport_size_(viewport_size)
{
	const MemoryMappedFilePtr file= MemoryMappedFile::Create( "map.pm" );
	if( file == nullptr )
	{
		Log::FatalError( "Error, opening map file" );
		return;
	}
	if( file->Size() < sizeof(DataFileDescription::DataFile) )
	{
		Log::FatalError( "Map file is too small" );
		return;
	}

	const unsigned char* const file_content= static_cast<const unsigned char*>(file->Data());
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

	const auto linear_styles= reinterpret_cast<const DataFileDescription::LinearObjectStyle*>( file_content + data_file.linear_styles_offset );
	const auto areal_styles= reinterpret_cast<const DataFileDescription::ArealObjectStyle*>( file_content + data_file.areal_styles_offset );

	const auto chunks_description= reinterpret_cast<const DataFileDescription::DataFile::ChunkDescription*>( file_content + data_file.chunks_description_offset );

	for( uint32_t chunk_index= 0u; chunk_index < data_file.chunk_count; ++chunk_index )
	{
		const size_t chunk_offset= chunks_description[chunk_index].offset;
		const unsigned char* const chunk_data= file_content + chunk_offset;
		const DataFileDescription::Chunk& chunk= *reinterpret_cast<const DataFileDescription::Chunk*>(chunk_data);
		chunks_.emplace_back( chunk, linear_styles, areal_styles );
	}

	// Create textures
	{
		DataFileDescription::ColorRGBA texture_data[256u]= {0};

		for( uint32_t i= 0u; i < data_file.linear_styles_count; ++i )
			std::memcpy( texture_data[i], linear_styles[i].color, sizeof(DataFileDescription::ColorRGBA) );

		glGenTextures( 1, &linear_objects_texture_id_ );
		glBindTexture( GL_TEXTURE_1D, linear_objects_texture_id_ );
		glTexImage1D( GL_TEXTURE_1D, 0, GL_RGBA8, 256u, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data );
		glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	}
	{
		DataFileDescription::ColorRGBA texture_data[256u]= {0};

		for( uint32_t i= 0u; i < data_file.areal_styles_count; ++i )
			std::memcpy( texture_data[i], areal_styles[i].color, sizeof(DataFileDescription::ColorRGBA) );

		glGenTextures( 1, &areal_objects_texture_id_ );
		glBindTexture( GL_TEXTURE_1D, areal_objects_texture_id_ );
		glTexImage1D( GL_TEXTURE_1D, 0, GL_RGBA8, 256u, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data );
		glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	}
	std::memcpy( background_color_, data_file.common_style.background_color, sizeof(background_color_) );

	// Create shaders

	point_objets_shader_.ShaderSource( Shaders::point_fragment, Shaders::point_vertex );
	point_objets_shader_.SetAttribLocation( "pos", 0 );
	point_objets_shader_.SetAttribLocation( "color_index", 1 );
	point_objets_shader_.Create();

	linear_objets_shader_.ShaderSource( Shaders::linear_fragment, Shaders::linear_vertex );
	linear_objets_shader_.SetAttribLocation( "pos", 0 );
	linear_objets_shader_.SetAttribLocation( "color_index", 1 );
	linear_objets_shader_.Create();

	areal_objects_shader_.ShaderSource( Shaders::areal_fragment, Shaders::areal_vertex );
	areal_objects_shader_.SetAttribLocation( "pos", 0 );
	areal_objects_shader_.SetAttribLocation( "color_index", 1 );
	areal_objects_shader_.Create();

	// Setup camera
	if( !chunks_.empty() )
	{
		min_cam_pos_.x= +1e20f;
		min_cam_pos_.y= +1e20f;
		max_cam_pos_.x= -1e20f;
		max_cam_pos_.y= -1e20f;
		for( const Chunk& chunk : chunks_ )
		{
			min_cam_pos_.x= std::min( min_cam_pos_.x, float(chunk.coord_start_x_) );
			min_cam_pos_.y= std::min( min_cam_pos_.y, float(chunk.coord_start_y_) );
			max_cam_pos_.x= std::max( max_cam_pos_.x, float(chunk.coord_start_x_ + 65536) );
			max_cam_pos_.y= std::max( max_cam_pos_.y, float(chunk.coord_start_y_ + 65536) );
		}
	}
	else
		min_cam_pos_.x= max_cam_pos_.x= min_cam_pos_.y= max_cam_pos_.y= 0.0f;
	cam_pos_.x= ( min_cam_pos_.x + max_cam_pos_.x ) * 0.5f;
	cam_pos_.y= ( min_cam_pos_.y + max_cam_pos_.y ) * 0.5f;
	min_scale_= 1.0f;
	max_scale_= std::max( max_cam_pos_.x - min_cam_pos_.x, max_cam_pos_.y - min_cam_pos_.y ) / float( std::max( viewport_size_.width, viewport_size_.height ) );
	scale_= max_scale_;
}

MapDrawer::~MapDrawer()
{
	glDeleteTextures( 1, &linear_objects_texture_id_ );
	glDeleteTextures( 1, &areal_objects_texture_id_ );
}

void MapDrawer::Draw()
{
	// Clear background.
	glClearColor( float(background_color_[0]) / 255.0f, float(background_color_[1]) / 255.0f, float(background_color_[2]) / 255.0f, float(background_color_[3]) / 255.0f );
	glClear( GL_COLOR_BUFFER_BIT );

	// Calculate view matrix.
	m_Mat4 view_matrix, scale_matrix, translate_matrix, aspect_matrix;
	scale_matrix.Scale( 1.0f / scale_ );
	translate_matrix.Translate( m_Vec3( -cam_pos_, 0.0f ) );
	aspect_matrix.Scale( 2.0f * m_Vec3( 1.0f / float(viewport_size_.width), 1.0f / float(viewport_size_.height), 1.0f ) );
	view_matrix= translate_matrix * scale_matrix * aspect_matrix;

	// Calculate viewport bounding box.
	const int32_t bb_extend_eps= 16;
	const int32_t viewport_half_size_x_world_space= int32_t( float(viewport_size_.width  ) * 0.5f * scale_ ) + bb_extend_eps;
	const int32_t viewport_half_size_y_world_space= int32_t( float(viewport_size_.height ) * 0.5f * scale_ ) + bb_extend_eps;
	const int32_t cam_pos_x_world_space= int32_t(cam_pos_.x);
	const int32_t cam_pos_y_world_space= int32_t(cam_pos_.y);
	const int32_t bb_min_x= cam_pos_x_world_space - viewport_half_size_x_world_space;
	const int32_t bb_max_x= cam_pos_x_world_space + viewport_half_size_x_world_space;
	const int32_t bb_min_y= cam_pos_y_world_space - viewport_half_size_y_world_space;
	const int32_t bb_max_y= cam_pos_y_world_space + viewport_half_size_y_world_space;

	// Setup chunks list, calculate matrices.
	std::vector<ChunkToDraw> visible_chunks;
	for( const Chunk& chunk : chunks_ )
	{
		if( chunk.bb_min_x_ >= bb_max_x || chunk.bb_min_y_ >= bb_max_y ||
			chunk.bb_max_x_ <= bb_min_x || chunk.bb_max_y_ <= bb_min_y )
			continue;

		m_Mat4 coords_shift_matrix, chunk_view_matrix;
		coords_shift_matrix.Translate( m_Vec3( float(chunk.coord_start_x_), float(chunk.coord_start_y_), 0.0f ) );
		chunk_view_matrix= coords_shift_matrix * view_matrix;

		visible_chunks.push_back( ChunkToDraw{ chunk, chunk_view_matrix } );
	}

	// Draw chunks.
	{
		areal_objects_shader_.Bind();
		areal_objects_shader_.Uniform( "tex", 0 );

		glBindTexture( GL_TEXTURE_1D, areal_objects_texture_id_ );

		glEnable( GL_PRIMITIVE_RESTART );
		glPrimitiveRestartIndex( c_primitive_restart_index );
		for( const ChunkToDraw& chunk_to_draw : visible_chunks )
		{
			if( chunk_to_draw.chunk.areal_objects_polygon_buffer_.GetVertexDataSize() == 0u )
				continue;

			areal_objects_shader_.Uniform( "view_matrix", chunk_to_draw.matrix );
			chunk_to_draw.chunk.areal_objects_polygon_buffer_.Draw();
		}
		glDisable( GL_PRIMITIVE_RESTART );
	}
	{
		linear_objets_shader_.Bind();
		linear_objets_shader_.Uniform( "tex", 0 );

		glBindTexture( GL_TEXTURE_1D, linear_objects_texture_id_ );

		glEnable( GL_PRIMITIVE_RESTART );
		glPrimitiveRestartIndex( c_primitive_restart_index );
		for( const ChunkToDraw& chunk_to_draw : visible_chunks )
		{
			if( chunk_to_draw.chunk.linear_objects_polygon_buffer_.GetVertexDataSize() == 0u &&
				chunk_to_draw.chunk.linear_objects_as_triangles_buffer_.GetVertexDataSize() == 0u )
				continue;

			linear_objets_shader_.Uniform( "view_matrix", chunk_to_draw.matrix );
			chunk_to_draw.chunk.linear_objects_polygon_buffer_.Draw();
			chunk_to_draw.chunk.linear_objects_as_triangles_buffer_.Draw();
		}
		glDisable( GL_PRIMITIVE_RESTART );
	}
	{
		point_objets_shader_.Bind();

		glPointSize( 12.0f );
		for( const ChunkToDraw& chunk_to_draw : visible_chunks )
		{
			if( chunk_to_draw.chunk.point_objects_polygon_buffer_.GetVertexDataSize() == 0u )
				continue;

			point_objets_shader_.Uniform( "view_matrix", chunk_to_draw.matrix );
			chunk_to_draw.chunk.point_objects_polygon_buffer_.Draw();
		}
	}
}

void MapDrawer::ProcessEvent( const SystemEvent& event )
{
	switch( event.type )
	{
	case SystemEvent::Type::MouseKey:
		if( event.event.mouse_key.mouse_button == SystemEvent::MouseKeyEvent::Button::Left )
			mouse_pressed_= event.event.mouse_key.pressed;
		break;

	case SystemEvent::Type::MouseMove:
		if( mouse_pressed_ )
		{
			cam_pos_+= m_Vec2( -float(event.event.mouse_move.dx), float(event.event.mouse_move.dy) ) * scale_;
			cam_pos_.x= std::max( min_cam_pos_.x, std::min( cam_pos_.x, max_cam_pos_.x ) );
			cam_pos_.y= std::max( min_cam_pos_.y, std::min( cam_pos_.y, max_cam_pos_.y ) );
			Log::User( "Shift is ", cam_pos_.x, " ", cam_pos_.y );
		}
		break;

	case SystemEvent::Type::Wheel:
		scale_*= std::exp2( -float( event.event.wheel.delta ) * 0.5f );
		scale_= std::max( min_scale_, std::min( scale_, max_scale_ ) );
		Log::User( "Scale is ", scale_ );
		break;

	case SystemEvent::Type::Quit:
		return;
	}
}

} // namespace PanzerMaps
