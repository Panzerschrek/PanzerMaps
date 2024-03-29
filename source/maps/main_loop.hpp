#pragma once
#include "map_drawer.hpp"
#include "mouse_map_controller.hpp"
#include "system_window.hpp"
#include "touch_map_controller.hpp"
#include "zoom_controller.hpp"
#include "gps_button.hpp"

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
	UiDrawer ui_drawer_;
	MapDrawer map_drawer_;
	GPSService gps_service_;

	MouseMapController mouse_map_controller_;
	TouchMapController touch_map_controller_;
	ZoomController zoom_controller_;
	GPSButton gps_button_;
};

} // namespace PanzerMaps
