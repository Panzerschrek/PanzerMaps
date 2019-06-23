#pragma once
#include "../panzer_ogl_lib/glsl_program.hpp"
#include "../panzer_ogl_lib/polygon_buffer.hpp"
#include "system_window.hpp"

namespace PanzerMaps
{

class MapDrawer final
{
public:
	explicit MapDrawer( const ViewportSize& viewport_size );

	void Draw();

	void ProcessEvent( const SystemEvent& event );

private:
	const ViewportSize viewport_size_;
	r_GLSLProgram point_objets_shader_;
	r_GLSLProgram linear_objets_shader_;
	r_GLSLProgram areal_objects_shader_;

	r_PolygonBuffer point_objects_polygon_buffer_;
	r_PolygonBuffer linear_objects_polygon_buffer_;
	r_PolygonBuffer areal_objects_polygon_buffer_;

	bool mouse_pressed_= false;
	float scale_= 256.0f; // Scale = map units in pixel

	// Camera position in map space.
	m_Vec2 cam_pos_= m_Vec2( 0.0f, 0.0f );
};

} // namespace PanzerMaps
