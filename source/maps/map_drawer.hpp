#pragma once
#include "../common/coordinates_conversion.hpp"
#include "../common/memory_mapped_file.hpp"
#include "../panzer_ogl_lib/glsl_program.hpp"
#include "../panzer_ogl_lib/polygon_buffer.hpp"
#include "../panzer_ogl_lib/texture.hpp"
#include "system_window.hpp"
#include "ui_drawer.hpp"

namespace PanzerMaps
{

class MapDrawer final
{
public:
	MapDrawer( const SystemWindow& system_window, UiDrawer& ui_drawer, const char* map_file );
	~MapDrawer();

	void Draw();
	float GetScale() const;
	void SetScale( float scale );

	const m_Vec2& GetPosition() const;
	void SetPosition( const m_Vec2& position );

	const ViewportSize& GetViewportSize() const;

	void SetGPSMarkerPosition( const GeoPoint& gps_marker_position );

private:
	struct Chunk;
	struct ZoomLevel;
	struct ChunkToDraw;

private:
	ZoomLevel& SelectZoomLevel();
	void ClearGPUData();

private:
	const ViewportSize viewport_size_;
	const SystemWindow& system_window_;
	UiDrawer& ui_drawer_;
	const MemoryMappedFilePtr data_file_;
	r_GLSLProgram point_objets_shader_;
	r_GLSLProgram linear_objets_shader_;
	r_GLSLProgram linear_textured_objets_shader_;
	r_GLSLProgram areal_objects_shader_;
	r_GLSLProgram gps_marker_shader_;
	r_Texture copyright_texture_;

	std::vector<ZoomLevel> zoom_levels_;

	size_t frame_number_= 0u;

	float scale_; // Scale = map units in pixel
	// Camera position in map space.
	m_Vec2 cam_pos_;

	m_Vec2 min_cam_pos_;
	m_Vec2 max_cam_pos_;
	float min_scale_;
	float max_scale_;

	GeoPoint gps_marker_position_{ 1000.0, 1000.0 };

	unsigned char background_color_[4]= {0};
};

} // namespace PanzerMaps
