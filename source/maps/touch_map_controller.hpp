#pragma once
#include <unordered_map>
#include "map_drawer.hpp"

namespace PanzerMaps
{

class TouchMapController final
{
public:
	explicit TouchMapController( MapDrawer& map_drawer );
	void ProcessEvent( const SystemEvent& event );

	void DoMove();

private:
	MapDrawer& map_drawer_;

	using TouchState= std::unordered_map< int64_t, m_Vec2 >;
	TouchState prev_touch_state_;
	TouchState touch_state_;
};

} // namespace PanzerMaps
