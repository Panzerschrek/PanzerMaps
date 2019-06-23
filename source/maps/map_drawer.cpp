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
	uniform mat4 view_matrix;
	in vec2 pos;
	in float color_index;
	out vec4 f_color;
	void main()
	{
		f_color= vec4( mod( color_index, 2.0 ), mod( color_index, 4.0 ) / 3.0, mod( color_index, 8.0 ) / 7.0, 0.5 );
		f_color= vec4( 1.0, 1.0, 1.0, 1.0 ) - f_color;
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

MapDrawer::MapDrawer( const ViewportSize& viewport_size )
	: viewport_size_(viewport_size)
{
	using namespace DataFileDescription;

	std::vector<unsigned char> file_content;
	const bool read_ok= ReadFile( "map.pm", file_content );
	if( !read_ok )
		return;
	if( file_content.size() < sizeof(DataFile) )
		return;

	const DataFile& data_file= *reinterpret_cast<const DataFile*>( file_content.data() );

	if( std::memcmp( data_file.header, DataFile::c_expected_header, sizeof(data_file.header) ) != 0 )
		return;
	if( data_file.version != DataFile::c_expected_version )
		return;

	std::vector<PointObjectVertex> point_objects_vertices;

	std::vector<LinearObjectVertex> linear_objects_vertices;
	std::vector<uint16_t> linear_objects_indicies;

	std::vector<ArealObjectVertex> areal_objects_vertices;
	std::vector<uint16_t> areal_objects_indicies;

	for( uint32_t chunk_index= 0u; chunk_index < data_file.chunk_count; ++chunk_index )
	{
		const size_t chunk_offset= data_file.chunks_offset + chunk_index * sizeof(Chunk);
		const unsigned char* const chunk_data= file_content.data() + chunk_offset;

		const Chunk& chunk= *reinterpret_cast<const Chunk*>(chunk_data);
		const ChunkVertex* const vertices= reinterpret_cast<const ChunkVertex*>( chunk_data + chunk.vertices_offset );
		const Chunk::PointObjectGroup* const point_object_groups= reinterpret_cast<const Chunk::PointObjectGroup*>( chunk_data + chunk.point_object_groups_offset );
		const Chunk::LinearObjectGroup* const linear_object_groups= reinterpret_cast<const Chunk::LinearObjectGroup*>( chunk_data + chunk.linear_object_groups_offset );
		const Chunk::ArealObjectGroup* const areal_object_groups= reinterpret_cast<const Chunk::ArealObjectGroup*>( chunk_data + chunk.areal_object_groups_offset );

		for( uint16_t i= 0u; i < chunk.point_object_groups_count; ++i )
		{
			const Chunk::PointObjectGroup group= point_object_groups[i];
			for( uint16_t v= group.first_vertex; v < group.first_vertex + group.vertex_count; ++v )
			{
				const ChunkVertex& vertex= vertices[v];
				PointObjectVertex out_vertex;
				out_vertex.xy[0]= vertex.x;
				out_vertex.xy[1]= vertex.y;
				out_vertex.color_index= group.style_index;
				point_objects_vertices.push_back( out_vertex );
			}
		}

		// Draw polylines, using "GL_LINE_STRIP" primitive with primitive restart index.
		for( uint16_t i= 0u; i < chunk.linear_object_groups_count; ++i )
		{
			const Chunk::LinearObjectGroup group= linear_object_groups[i];
			for( uint16_t v= group.first_vertex; v < group.first_vertex + group.vertex_count; ++v )
			{
				const ChunkVertex& vertex= vertices[v];
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
		for( uint16_t i= 0u; i < chunk.areal_object_groups_count; ++i )
		{
			const Chunk::ArealObjectGroup group= areal_object_groups[i];
			for( uint16_t v= group.first_vertex; v < group.first_vertex + group.vertex_count; ++v )
			{
				const ChunkVertex& vertex= vertices[v];
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
}

void MapDrawer::Draw()
{

	m_Mat4 view_matrix, scale_matrix, translate_matrix, aspect_matrix;

	scale_matrix.Scale( 1.0f / scale_ );
	translate_matrix.Translate( m_Vec3( -cam_pos_, 0.0f ) );
	aspect_matrix.Scale( 2.0f * m_Vec3( 1.0f / float(viewport_size_.width), 1.0f / float(viewport_size_.height), 1.0f ) );
	view_matrix= translate_matrix * scale_matrix * aspect_matrix;

	{
		areal_objects_shader_.Bind();
		areal_objects_shader_.Uniform( "view_matrix", view_matrix );

		glEnable( GL_PRIMITIVE_RESTART );
		glPrimitiveRestartIndex( c_primitive_restart_index );
		areal_objects_polygon_buffer_.Draw();
		glDisable( GL_PRIMITIVE_RESTART );
	}
	{
		linear_objets_shader_.Bind();
		linear_objets_shader_.Uniform( "view_matrix", view_matrix );

		glEnable( GL_PRIMITIVE_RESTART );
		glPrimitiveRestartIndex( c_primitive_restart_index );
		linear_objects_polygon_buffer_.Draw();
		glDisable( GL_PRIMITIVE_RESTART );
	}
	{
		point_objets_shader_.Bind();
		point_objets_shader_.Uniform( "view_matrix", view_matrix );

		glPointSize( 12.0f );
		point_objects_polygon_buffer_.Draw();
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
			cam_pos_.x= std::max( -65536.0f, std::min( cam_pos_.x, 65536.0f ) );
			cam_pos_.y= std::max( -65536.0f, std::min( cam_pos_.y, 65536.0f ) );
			Log::User( "Shift is ", cam_pos_.x, " ", cam_pos_.y );
		}
		break;

	case SystemEvent::Type::Wheel:
		scale_*= std::exp2( -float( event.event.wheel.delta ) );
		scale_= std::max( 1.0f, std::min( scale_, 4096.0f ) );
		Log::User( "Scale is ", scale_ );
		break;

	case SystemEvent::Type::Quit:
		return;
	}
}

} // namespace PanzerMaps
