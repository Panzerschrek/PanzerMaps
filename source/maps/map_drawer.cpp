#include <cstdio>
#include <cstring>
#include "../common/data_file.hpp"
#include "map_drawer.hpp"

namespace PanzerMaps
{

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

struct LinearObjectVertex
{
	uint16_t xy[2];
	uint32_t color_index;
};

static const uint16_t c_primitive_restart_index= 65535u;

MapDrawer::MapDrawer()
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

	std::vector<LinearObjectVertex> linear_objects_vertices;
	std::vector<uint16_t> linear_objects_indicies;

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
		}

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

		for( uint16_t i= 0u; i < chunk.areal_object_groups_count; ++i )
		{
		}
	}

	linear_objcts_polygon_buffer_.VertexData( linear_objects_vertices.data(), linear_objects_vertices.size() * sizeof(LinearObjectVertex), sizeof(LinearObjectVertex) );
	linear_objcts_polygon_buffer_.IndexData( linear_objects_indicies.data(), linear_objects_indicies.size() * sizeof(uint16_t), GL_UNSIGNED_SHORT, GL_LINE_STRIP );
	linear_objcts_polygon_buffer_.VertexAttribPointer( 0, 2, GL_UNSIGNED_SHORT, false, 0 );
	linear_objcts_polygon_buffer_.VertexAttribPointer( 1, 1, GL_UNSIGNED_INT, false, sizeof(uint16_t) * 2 );


	// Create shaders

	const char* const c_vertex_shader=
	R"(
		#version 330
		uniform mat4 view_matrix;

		in vec2 pos;
		in float color_index;

		out vec4 f_color;

		void main()
		{
			//f_color= vec4( color_index, mod( color_index, 2.0 ), mod( color_index, 4.0 ) / 2.0, 0.5 );
			f_color= vec4( 1.0, 0.0, 1.0, 0.0 );
			gl_Position= view_matrix * vec4( pos, 0.0, 1.0 );
		}
	)";

	const char* const c_fragment_shader=
	R"(
		#version 330
		in vec4 f_color;
		out vec4 color;

		void main()
		{
			color= f_color;
		}
	)";

	linear_objets_shader_.ShaderSource( c_fragment_shader, c_vertex_shader );
	linear_objets_shader_.SetAttribLocation( "pos", 0 );
	linear_objets_shader_.SetAttribLocation( "color_index", 1 );
	linear_objets_shader_.Create();
}

void MapDrawer::Draw()
{
	linear_objets_shader_.Bind();

	m_Mat4 view_matrix;
	view_matrix.Scale( 1.0f / 16384.0f );

	linear_objets_shader_.Uniform( "view_matrix", view_matrix );

	glEnable( GL_PRIMITIVE_RESTART );
	glPrimitiveRestartIndex( c_primitive_restart_index );
	linear_objcts_polygon_buffer_.Bind();
	linear_objcts_polygon_buffer_.Draw();

	glDisable( GL_PRIMITIVE_RESTART );
}

void MapDrawer::ProcessEvent( const SystemEvent& event )
{
}

} // namespace PanzerMaps
