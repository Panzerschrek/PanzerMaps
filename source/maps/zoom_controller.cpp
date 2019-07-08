#include "textures_generation.hpp"
#include "zoom_controller.hpp"

namespace PanzerMaps
{

static const float g_scale_speed= 3.0f;

ZoomController::ZoomController( UiDrawer& ui_drawer, MapDrawer& map_drawer )
	: ui_drawer_(ui_drawer), map_drawer_(map_drawer)
{
	const int button_size= std::min( map_drawer_.GetViewportSize().width, map_drawer.GetViewportSize().height ) / 6u;
	const int c_border_size= 4;
	const int left= map_drawer_.GetViewportSize().width - button_size - c_border_size;
	const int top= map_drawer_.GetViewportSize().height * 5u / 8u;

	buttons_[0].scale_direction= -g_scale_speed;
	buttons_[0].texture= GenTexture_ZoomPlus( button_size );
	buttons_[0].presed_texture= GenTexture_ZoomPlusPressed( button_size );
	buttons_[0].x= left;
	buttons_[0].y= top;
	buttons_[0].size= int(button_size);

	buttons_[1].scale_direction= +g_scale_speed;
	buttons_[1].texture= GenTexture_ZoomMinus( button_size );
	buttons_[1].presed_texture= GenTexture_ZoomMinusPressed( button_size );
	buttons_[1].x= left;
	buttons_[1].y= top - button_size - c_border_size;
	buttons_[1].size= int(button_size);

	prev_time_= std::chrono::steady_clock::now();
}

bool ZoomController::ProcessEvent( const SystemEvent& event )
{
	switch( event.type )
	{
	case SystemEvent::Type::MouseKey:
		if( event.event.mouse_key.mouse_button == SystemEvent::MouseKeyEvent::Button::Left )
		{
			if( event.event.mouse_key.pressed )
				for( size_t i= 0u; i < 2u; ++i )
					buttons_[i].pressed=
						int(event.event.mouse_key.x) >= buttons_[i].x && int(event.event.mouse_key.x) < buttons_[i].x + buttons_[i].size &&
						int(event.event.mouse_key.y) >= buttons_[i].y && int(event.event.mouse_key.y) < buttons_[i].y + buttons_[i].size;
			else
				for( size_t i= 0u; i < 2u; ++i )
					buttons_[i].pressed= false;

			for( size_t i= 0u; i < 2u; ++i )
				if( buttons_[i].pressed )
					return true;
		}
		break;

	case SystemEvent::Type::MouseMove:
		for( size_t i= 0u; i < 2u; ++i )
			if( buttons_[i].pressed )
				return true;
		break;

	default:
		break;
	}

	return false;
}

void ZoomController::DoMove()
{
	const auto cur_time= std::chrono::steady_clock::now();
	const float time_delta_s= float( std::chrono::duration_cast<std::chrono::milliseconds>( cur_time - prev_time_ ).count() ) * 0.001f;
	prev_time_= cur_time;

	for( size_t i= 0u; i < 2u; ++i )
		if( buttons_[i].pressed )
			map_drawer_.SetScale( map_drawer_.GetScale() * std::exp2( buttons_[i].scale_direction * time_delta_s ) );
}

void ZoomController::Draw()
{
	for( size_t i= 0u; i < 2u; ++i )
		ui_drawer_.DrawUiElement(
			buttons_[i].x, buttons_[i].y,
			buttons_[i].size, buttons_[i].size,
			buttons_[i].pressed ? buttons_[i].presed_texture : buttons_[i].texture );
}

} // namespace PanzerMaps
