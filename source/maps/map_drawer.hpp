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
	~MapDrawer();

	void Draw();

	void ProcessEvent( const SystemEvent& event );

private:
	struct Chunk;

private:
	const ViewportSize viewport_size_;
	r_GLSLProgram point_objets_shader_;
	r_GLSLProgram linear_objets_shader_;
	r_GLSLProgram areal_objects_shader_;

	std::vector<Chunk> chunks_;

	bool mouse_pressed_= false;
	float scale_= 256.0f; // Scale = map units in pixel

	// Camera position in map space.
	m_Vec2 cam_pos_= m_Vec2( 0.0f, 0.0f );

	m_Vec2 min_cam_pos_;
	m_Vec2 max_cam_pos_;
};

} // namespace PanzerMaps
