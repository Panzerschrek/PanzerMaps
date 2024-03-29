#include "shaders.hpp"
#include "ui_drawer.hpp"

namespace PanzerMaps
{

struct UiVertex
{
	float pos[2];
};

UiDrawer::UiDrawer( const ViewportSize& viewport_size )
	: viewport_size_(viewport_size)
{
	static const UiVertex vertices[6]
	{
		{ { 0.0f, 0.0f } },
		{ { 0.0f, 1.0f } },
		{ { 1.0f, 1.0f } },
		{ { 0.0f, 0.0f } },
		{ { 1.0f, 1.0f } },
		{ { 1.0f, 0.0f } },
	};
	quad_polygon_buffer_.VertexData( vertices, sizeof(vertices), sizeof(UiVertex) );
	quad_polygon_buffer_.SetPrimitiveType( GL_TRIANGLES );
	quad_polygon_buffer_.VertexAttribPointer( 0, 2, GL_FLOAT, true, 0 );

	shader_.ShaderSource( Shaders::ui_fragment, Shaders::ui_vertex );
	shader_.SetAttribLocation( "pos", 0 );
	shader_.Create();
}

void UiDrawer::DrawUiElement( const int x, const int y, const int width, const int height, const r_Texture& texture )
{
	m_Mat4 scale_mat, shift_mat, view_mat;
	scale_mat.Scale( m_Vec3( 2.0f * float(width) / float(viewport_size_.width), 2.0f * float(height) / float(viewport_size_.height), 0.0f ) );
	shift_mat.Translate( m_Vec3( 2.0f * float(x) / float(viewport_size_.width) - 1.0f, 2.0f * float(y) / float(viewport_size_.height) - 1.0f, 0.0f ) );
	view_mat= scale_mat * shift_mat;

	shader_.Bind();
	shader_.Uniform( "view_matrix", view_mat );
	shader_.Uniform( "tex", 0 );
	texture.Bind();

	glEnable( GL_BLEND );
	quad_polygon_buffer_.Draw();
	glDisable( GL_BLEND );
}

void UiDrawer::DrawUiElementRotated( int center_x, int center_y, int width, int height, float angle_rad, const r_Texture& texture )
{
	m_Mat4 rotate_mat, scale_mat, shift_mat, view_mat;
	{
		m_Mat4 center_shift_mat0, center_shift_mat1, center_rotate_mat;
		center_shift_mat0.Translate( m_Vec3( -0.5f, -0.5f, 0.0f ) );
		center_shift_mat1.Translate( m_Vec3( +0.5f, +0.5f, 0.0f ) );
		center_rotate_mat.RotateZ( -angle_rad );
		rotate_mat= center_shift_mat0 * center_rotate_mat * center_shift_mat1;
	}

	scale_mat.Scale( m_Vec3( 2.0f * float(width) / float(viewport_size_.width), 2.0f * float(height) / float(viewport_size_.height), 0.0f ) );
	shift_mat.Translate( m_Vec3( 2.0f * float(center_x - width / 2) / float(viewport_size_.width) - 1.0f, 2.0f * float(center_y - height / 2 ) / float(viewport_size_.height) - 1.0f, 0.0f ) );
	view_mat= rotate_mat * scale_mat * shift_mat;

	shader_.Bind();
	shader_.Uniform( "view_matrix", view_mat );
	shader_.Uniform( "tex", 0 );
	texture.Bind();

	glEnable( GL_BLEND );
	quad_polygon_buffer_.Draw();
	glDisable( GL_BLEND );
}

} // namespace PanzerMaps
