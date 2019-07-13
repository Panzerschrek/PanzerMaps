#pragma once
#include "system_window.hpp"
#include "ui_drawer.hpp"
#include "gps_service.hpp"

namespace PanzerMaps
{

class GPSButton final
{
public:
	GPSButton( UiDrawer& ui_drawer, GPSService& gps_service );

	// Returns true, if event grabbed.
	bool ProcessEvent( const SystemEvent& event );

	void Draw();
	bool RedrawRequired() const { return redraw_required_; }

private:
	UiDrawer& ui_drawer_;
	GPSService& gps_service_;

	int button_x_, button_y_, button_size_;
	r_Texture texture_unactive_, texture_active_;
	bool redraw_required_= true;
};

} // namespace PanzerMaps
