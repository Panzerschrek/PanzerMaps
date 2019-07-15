#include "textures_generation.hpp"
#include "ui_common.hpp"
#include "zoom_controller.hpp"

namespace PanzerMaps
{

static const float g_scale_speed= 3.0f;

ZoomController::ZoomController( UiDrawer& ui_drawer, MapDrawer& map_drawer )
	: ui_drawer_(ui_drawer), map_drawer_(map_drawer)
{
	const int button_size= GetButtonSize( map_drawer.GetViewportSize() );
	const int left= map_drawer_.GetViewportSize().width - button_size - UiConstants::border_size;
	const int top= GetButtonsTop( map_drawer_.GetViewportSize() );

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
	buttons_[1].y= top - button_size - UiConstants::border_size;
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
			redraw_required_= true;

			if( event.event.mouse_key.pressed )
				for( size_t i= 0u; i < 2u; ++i )
				{
					bool pressed=
						int(event.event.mouse_key.x) >= buttons_[i].x && int(event.event.mouse_key.x) < buttons_[i].x + buttons_[i].size &&
						int(event.event.mouse_key.y) >= buttons_[i].y && int(event.event.mouse_key.y) < buttons_[i].y + buttons_[i].size;
					if( pressed != buttons_[i].pressed )
					{
						buttons_[i].pressed= pressed;
						prev_time_= std::chrono::steady_clock::now();
					}
				}
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

void ZoomController::Update()
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
	redraw_required_= false;
	for( size_t i= 0u; i < 2u; ++i )
		ui_drawer_.DrawUiElement(
			buttons_[i].x, buttons_[i].y,
			buttons_[i].size, buttons_[i].size,
			buttons_[i].pressed ? buttons_[i].presed_texture : buttons_[i].texture );
}

bool ZoomController::RedrawRequired() const
{
	for( size_t i= 0u; i < 2u; ++i )
		if( buttons_[i].pressed )
			return true;

	return redraw_required_;
}

} // namespace PanzerMaps
