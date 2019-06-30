#pragma once
#include "map_drawer.hpp"

namespace PanzerMaps
{

class MouseMapController final
{
public:
	explicit MouseMapController( MapDrawer& map_drawer );
	void ProcessEvent( const SystemEvent& event );

private:
	MapDrawer& map_drawer_;
	bool mouse_pressed_= false;
};

} // namespace PanzerMaps
