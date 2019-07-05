#pragma once
#include "../common/memory_mapped_file.hpp"
#include "../panzer_ogl_lib/glsl_program.hpp"
#include "../panzer_ogl_lib/polygon_buffer.hpp"
#include "system_window.hpp"

namespace PanzerMaps
{

class MapDrawer final
{
public:
	MapDrawer( const SystemWindow& system_window, const char* map_file );
	~MapDrawer();

	void Draw();
	float GetScale() const;
	void SetScale( float scale );

	const m_Vec2& GetPosition() const;
	void SetPosition( const m_Vec2& position );

	const ViewportSize& GetViewportSize() const;

private:
	struct Chunk;
	struct ZoomLevel;
	struct ChunkToDraw;

private:
	ZoomLevel& SelectZoomLevel();

private:
	const ViewportSize viewport_size_;
	const SystemWindow& system_window_;
	const MemoryMappedFilePtr data_file_;
	r_GLSLProgram point_objets_shader_;
	r_GLSLProgram linear_objets_shader_;
	r_GLSLProgram linear_textured_objets_shader_;
	r_GLSLProgram areal_objects_shader_;

	std::vector<ZoomLevel> zoom_levels_;

	size_t frame_number_= 0u;

	float scale_; // Scale = map units in pixel
	// Camera position in map space.
	m_Vec2 cam_pos_;

	m_Vec2 min_cam_pos_;
	m_Vec2 max_cam_pos_;
	float min_scale_;
	float max_scale_;
	float unit_size_m_;

	unsigned char background_color_[4]= {0};
};

} // namespace PanzerMaps
