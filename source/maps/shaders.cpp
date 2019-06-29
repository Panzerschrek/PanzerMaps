#include "shaders.hpp"

#define GLSL_VERSION "#version 330\n"

namespace PanzerMaps
{

namespace Shaders
{

const char point_vertex[]=
GLSL_VERSION
R"(
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
GLSL_VERSION
R"(
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
GLSL_VERSION
R"(
	uniform sampler2D tex;
	uniform mat4 view_matrix;
	in vec2 pos;
	in float color_index;
	out vec4 f_color;
	void main()
	{
		f_color= texelFetch( tex, ivec2( int(color_index), 0 ), 0 );
		gl_Position= view_matrix * vec4( pos, 0.0, 1.0 );
	}
)";

const char linear_fragment[]=
GLSL_VERSION
R"(
	in vec4 f_color;
	out vec4 color;
	void main()
	{
		color= f_color;
	}
)";

const char areal_vertex[]=
GLSL_VERSION
R"(
	uniform sampler2D tex;
	uniform mat4 view_matrix;
	in vec2 pos;
	in float color_index;
	out vec4 f_color;
	void main()
	{
		f_color= texelFetch( tex, ivec2( int(color_index), 0 ), 0 );
		gl_Position= view_matrix * vec4( pos, 0.0, 1.0 );
	}
)";

const char areal_fragment[]=
GLSL_VERSION
R"(
	in vec4 f_color;
	out vec4 color;
	void main()
	{
		color= f_color;
	}
)";

} // namespace Shaders

} // namespace PanzerMaps
