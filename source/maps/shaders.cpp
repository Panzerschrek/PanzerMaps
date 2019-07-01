#include "shaders.hpp"

#ifdef PM_OPENGL_ES
#define GLSL_VERSION "#version 300 es\n"
#else
#define GLSL_VERSION "#version 330\n"
#endif

namespace PanzerMaps
{

namespace Shaders
{

const char point_vertex[]=
GLSL_VERSION
R"(
	uniform highp mat4 view_matrix;
	in highp vec2 pos;
	in highp float color_index;
	out lowp vec4 f_color;
	void main()
	{
		f_color= vec4( mod( color_index, 2.0 ), mod( color_index, 4.0 ) / 3.0, mod( color_index, 8.0 ) / 7.0, 0.5 );
		gl_Position= view_matrix * vec4( pos, 0.0, 1.0 );
		gl_PointSize= 12.0;
	}
)";

const char point_fragment[]=
GLSL_VERSION
R"(
	in lowp vec4 f_color;
	out lowp vec4 color;
	void main()
	{
		mediump vec2 r= gl_PointCoord * 2.0 - vec2( 1.0, 1.0 );
		if( dot( r, r ) > 1.0 )
			discard;
		color= f_color * step( 0.0, r.x * r.y );
	}
)";

const char linear_vertex[]=
GLSL_VERSION
R"(
	uniform sampler2D tex;
	uniform highp mat4 view_matrix;
	in highp vec2 pos;
	in highp vec2 tex_coord;
	out lowp vec4 f_color;
	void main()
	{
		f_color= texelFetch( tex, ivec2( int(tex_coord.x), 0 ), 0 );
		gl_Position= view_matrix * vec4( pos, 0.0, 1.0 );
	}
)";

const char linear_fragment[]=
GLSL_VERSION
R"(
	in lowp vec4 f_color;
	out lowp vec4 color;
	void main()
	{
		color= f_color;
	}
)";

const char areal_vertex[]=
GLSL_VERSION
R"(
	uniform sampler2D tex;
	uniform highp mat4 view_matrix;
	in highp vec2 pos;
	in highp float color_index;
	out lowp vec4 f_color;
	void main()
	{
		f_color= texelFetch( tex, ivec2( int(color_index), 0 ), 0 );
		gl_Position= view_matrix * vec4( pos, 0.0, 1.0 );
	}
)";

const char linear_textured_vertex[]=
GLSL_VERSION
R"(
	uniform highp mat4 view_matrix;
	in highp vec2 pos;
	in highp vec2 tex_coord;
	out mediump vec2 f_tex_coord;
	void main()
	{
		f_tex_coord= vec2( tex_coord.y, 0.5 );
		gl_Position= view_matrix * vec4( pos, 0.0, 1.0 );
	}
)";

const char linear_textured_fragment[]=
GLSL_VERSION
R"(
	uniform sampler2D tex;
	in mediump vec2 f_tex_coord;
	out lowp vec4 color;
	void main()
	{
		color= texture( tex, f_tex_coord );
	}
)";

const char areal_fragment[]=
GLSL_VERSION
R"(
	in lowp vec4 f_color;
	out lowp vec4 color;
	void main()
	{
		color= f_color;
	}
)";

} // namespace Shaders

} // namespace PanzerMaps
