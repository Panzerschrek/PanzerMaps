#pragma once
#include "system_window.hpp"
#include "ui_drawer.hpp"
#ifdef __ANDROID__
#include "android/gps_service.hpp"
#endif

namespace PanzerMaps
{

class GPSButton final
{
public:
	GPSButton(
		UiDrawer& ui_drawer
		#ifdef __ANDROID__
		, GPSService& gps_service
		#endif
		);

	// Returns true, if event grabbed.
	bool ProcessEvent( const SystemEvent& event );

	void Draw();

private:
	UiDrawer& ui_drawer_;

	#ifdef __ANDROID__
	GPSService& gps_service_;
	#else
	bool active_= false;
	#endif

	int button_x_, button_y_, button_size_;
	r_Texture texture_unactive_, texture_active_;
};

} // namespace PanzerMaps
