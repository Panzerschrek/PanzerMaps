#include "mouse_map_controller.hpp"

namespace PanzerMaps
{

MouseMapController::MouseMapController( MapDrawer& map_drawer )
	: map_drawer_(map_drawer)
{}

void MouseMapController::ProcessEvent( const SystemEvent& event )
{
	switch( event.type )
	{
	case SystemEvent::Type::MouseKey:
		if( event.event.mouse_key.mouse_button == SystemEvent::MouseKeyEvent::Button::Left )
			mouse_pressed_= event.event.mouse_key.pressed;
		break;

	case SystemEvent::Type::MouseMove:
		if( mouse_pressed_ )
			map_drawer_.SetPosition(
				map_drawer_.GetPosition() +
				m_Vec2( -float(event.event.mouse_move.dx), -float(event.event.mouse_move.dy) ) * map_drawer_.GetScale() );
		break;

	case SystemEvent::Type::Wheel:
		{
			const m_Vec2 pix_delta(
				-float(map_drawer_.GetViewportSize().width ) * 0.5f + float(event.event.wheel.x),
				-float(map_drawer_.GetViewportSize().height) * 0.5f + float(event.event.wheel.y) );

			m_Vec2 position= map_drawer_.GetPosition();
			float scale= map_drawer_.GetScale();
			position+= pix_delta * scale;
			scale*= std::exp2( -float( event.event.wheel.delta ) * 0.25f );
			position-= pix_delta * scale;

			map_drawer_.SetPosition( position );
			map_drawer_.SetScale( scale );
		}
		break;

	case SystemEvent::Type::TouchPress:
	case SystemEvent::Type::TouchRelease:
	case SystemEvent::Type::TouchMove:
	case SystemEvent::Type::Quit:
	case SystemEvent::Type::Redraw:
		return;
	}
}

} // namespace PanzerMaps
