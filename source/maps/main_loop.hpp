#pragma once
#include "map_drawer.hpp"
#include "mouse_map_controller.hpp"
#include "system_window.hpp"
#include "touch_map_controller.hpp"
#include "ui_drawer.hpp"

namespace PanzerMaps
{

class MainLoop final
{
public:
	MainLoop();
	~MainLoop();

	// Returns false, if needs quit.
	bool Loop();

private:
	SystemWindow system_window_;
	MapDrawer map_drawer_;
	UiDrawer ui_drawer_;

	MouseMapController mouse_map_controller_;
	TouchMapController touch_map_controller_;
};

} // namespace PanzerMaps
