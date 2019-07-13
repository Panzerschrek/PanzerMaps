#include "textures_generation.hpp"
#include "gps_button.hpp"

namespace PanzerMaps
{

GPSButton::GPSButton( UiDrawer& ui_drawer , GPSService& gps_service )
	: ui_drawer_(ui_drawer), gps_service_(gps_service)
{
	// TODO - move UI constants to one place.
	const int c_border_size= 4;
	button_x_= c_border_size;
	button_y_= ui_drawer.GetViewportSize().height * 5u / 8u;
	button_size_= std::min( ui_drawer.GetViewportSize().width, ui_drawer.GetViewportSize().height ) / 6u;
	texture_unactive_= GenTexture_GPSButtonUnactive( button_size_ );
	texture_active_  = GenTexture_GPSButtonActive( button_size_ );
}

bool GPSButton::ProcessEvent( const SystemEvent& event )
{
	if(
		event.type == SystemEvent::Type::MouseKey &&
		event.event.mouse_key.pressed &&
		event.event.mouse_key.mouse_button == SystemEvent::MouseKeyEvent::Button::Left &&
		int(event.event.mouse_key.x) >= button_x_ && int(event.event.mouse_key.x) < button_x_ + button_size_ &&
		int(event.event.mouse_key.y) >= button_y_ && int(event.event.mouse_key.y) < button_y_ + button_size_ )
	{
		gps_service_.SetEnabled( !gps_service_.GetEnabled() );

		redraw_required_= true;
		return true;
	}
	return false;
}

void GPSButton::Draw()
{
	redraw_required_= false;
	ui_drawer_.DrawUiElement( button_x_, button_y_, button_size_, button_size_, gps_service_.GetEnabled() ? texture_active_ : texture_unactive_ );
}

} // namespace PanzerMaps
