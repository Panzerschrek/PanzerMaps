#include <cstdio>
#include <cstring>
#include "../common/data_file.hpp"
#include "log.hpp"
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

static bool ReadFile( const char* const name, std::vector<unsigned char>& out_file_content )
{
	std::FILE* const f= std::fopen( name, "rb" );
	if( f == nullptr )
		return false;

	std::fseek( f, 0, SEEK_END );
	const size_t file_size= size_t(std::ftell( f ));
	std::fseek( f, 0, SEEK_SET );

	out_file_content.resize(file_size);

	size_t read_total= 0u;
	bool read_error= false;
	do
	{
		const size_t read= std::fread( out_file_content.data() + read_total, 1, file_size - read_total, f );
		if( std::ferror(f) != 0 )
		{
			read_error= true;
			break;
		}
		if( read == 0 )
			break;

		read_total+= read;
	} while( read_total < file_size );

	std::fclose(f);

	return !read_error;
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

struct ArealObjectVertex
{
	uint16_t xy[2];
	uint32_t color_index;
};

static const uint16_t c_primitive_restart_index= 65535u;

struct MapDrawer::Chunk
{
	explicit Chunk( const DataFileDescription::Chunk& in_chunk )
		: coord_start_x_(in_chunk.coord_start_x), coord_start_y_(in_chunk.coord_start_y)
		, bb_min_x_(in_chunk.min_x), bb_min_y_(in_chunk.min_y), bb_max_x_(in_chunk.max_x), bb_max_y_(in_chunk.max_y)
	{
		const unsigned char* const chunk_data= reinterpret_cast<const unsigned char*>(&in_chunk);
		const auto vertices= reinterpret_cast<const DataFileDescription::ChunkVertex*>( chunk_data + in_chunk.vertices_offset );
		const auto point_object_groups= reinterpret_cast<const DataFileDescription::Chunk::PointObjectGroup*>( chunk_data + in_chunk.point_object_groups_offset );
		const auto linear_object_groups= reinterpret_cast<const DataFileDescription::Chunk::LinearObjectGroup*>( chunk_data + in_chunk.linear_object_groups_offset );
		const auto areal_object_groups= reinterpret_cast<const DataFileDescription::Chunk::ArealObjectGroup*>( chunk_data + in_chunk.areal_object_groups_offset );
		std::vector<PointObjectVertex> point_objects_vertices;

		std::vector<LinearObjectVertex> linear_objects_vertices;
		std::vector<uint16_t> linear_objects_indicies;

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
		for( uint16_t i= 0u; i < in_chunk.linear_object_groups_count; ++i )
		{
			const DataFileDescription::Chunk::LinearObjectGroup group= linear_object_groups[i];
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

		linear_objects_polygon_buffer_.VertexData( linear_objects_vertices.data(), linear_objects_vertices.size() * sizeof(LinearObjectVertex), sizeof(LinearObjectVertex) );
		linear_objects_polygon_buffer_.IndexData( linear_objects_indicies.data(), linear_objects_indicies.size() * sizeof(uint16_t), GL_UNSIGNED_SHORT, GL_LINE_STRIP );
		linear_objects_polygon_buffer_.VertexAttribPointer( 0, 2, GL_UNSIGNED_SHORT, false, 0 );
		linear_objects_polygon_buffer_.VertexAttribPointer( 1, 1, GL_UNSIGNED_INT, false, sizeof(uint16_t) * 2 );

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
	std::vector<unsigned char> file_content;
	const bool read_ok= ReadFile( "map.pm", file_content );
	if( !read_ok )
		return;
	if( file_content.size() < sizeof(DataFileDescription::DataFile) )
		return;

	const DataFileDescription::DataFile& data_file= *reinterpret_cast<const DataFileDescription::DataFile*>( file_content.data() );

	if( std::memcmp( data_file.header, DataFileDescription::DataFile::c_expected_header, sizeof(data_file.header) ) != 0 )
		return;
	if( data_file.version != DataFileDescription::DataFile::c_expected_version )
		return;

	const auto chunks_description= reinterpret_cast<const DataFileDescription::DataFile::ChunkDescription*>( file_content.data() + data_file.chunks_description_offset );

	for( uint32_t chunk_index= 0u; chunk_index < data_file.chunk_count; ++chunk_index )
	{
		const size_t chunk_offset= chunks_description[chunk_index].offset;
		const unsigned char* const chunk_data= file_content.data() + chunk_offset;
		const DataFileDescription::Chunk& chunk= *reinterpret_cast<const DataFileDescription::Chunk*>(chunk_data);
		chunks_.emplace_back( chunk );
	}

	// Create textures
	{
		DataFileDescription::ColorRGBA texture_data[256u]= {0};

		const auto linear_styles= reinterpret_cast<const DataFileDescription::LinearObjectStyle*>( file_content.data() + data_file.linear_styles_offset );

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

		const auto areal_styles= reinterpret_cast<const DataFileDescription::ArealObjectStyle*>( file_content.data() + data_file.areal_styles_offset );

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
			if( chunk_to_draw.chunk.linear_objects_polygon_buffer_.GetVertexDataSize() == 0u )
				continue;

			linear_objets_shader_.Uniform( "view_matrix", chunk_to_draw.matrix );
			chunk_to_draw.chunk.linear_objects_polygon_buffer_.Draw();
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
