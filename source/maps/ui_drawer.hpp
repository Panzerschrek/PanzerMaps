#pragma once
#include "../panzer_ogl_lib/glsl_program.hpp"
#include "../panzer_ogl_lib/polygon_buffer.hpp"
#include "../panzer_ogl_lib/texture.hpp"
#include "system_window.hpp"

namespace PanzerMaps
{

class UiDrawer final
{
public:
	explicit UiDrawer( const ViewportSize& viewport_size );

	const ViewportSize& GetViewportSize() const { return viewport_size_; }

	void DrawUiElement( int x, int y, int width, int height, const r_Texture& texture );

private:
	const ViewportSize viewport_size_;
	r_PolygonBuffer quad_polygon_buffer_;
	r_GLSLProgram shader_;
};

} // namespace PanzerMaps
