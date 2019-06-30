#pragma once
#include "map_drawer.hpp"
#include "mouse_map_controller.hpp"
#include "system_window.hpp"

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
	MouseMapController mouse_map_controller_;
};

} // namespace PanzerMaps
