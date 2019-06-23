#pragma once
#include "../panzer_ogl_lib/glsl_program.hpp"
#include "../panzer_ogl_lib/polygon_buffer.hpp"
#include "system_window.hpp"

namespace PanzerMaps
{

class MapDrawer final
{
public:
	MapDrawer();

	void Draw();

	void ProcessEvent( const SystemEvent& event );

private:
	r_GLSLProgram linear_objets_shader_;
	r_PolygonBuffer linear_objcts_polygon_buffer_;
};

} // namespace PanzerMaps
