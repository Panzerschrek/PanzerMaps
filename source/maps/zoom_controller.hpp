#pragma once
#include <chrono>
#include "map_drawer.hpp"
#include "ui_drawer.hpp"

namespace PanzerMaps
{

class ZoomController final
{
public:
	ZoomController( UiDrawer& ui_drawer, MapDrawer& map_drawer );

	// Returns true, if event grabbed.
	bool ProcessEvent( const SystemEvent& event );

	void DoMove();
	void Draw();

private:
	struct Button
	{
		float scale_direction;
		r_Texture texture;
		r_Texture presed_texture;
		int x, y, size;
		bool pressed= false;
	};

private:
	UiDrawer& ui_drawer_;
	MapDrawer& map_drawer_;

	Button buttons_[2];
	std::chrono::steady_clock::time_point prev_time_;
};

} // namespace PanzerMaps
