#include "../common/log.hpp"
#include "touch_map_controller.hpp"

namespace PanzerMaps
{

TouchMapController::TouchMapController( MapDrawer& map_drawer )
	: map_drawer_(map_drawer)
{}

void TouchMapController::ProcessEvent( const SystemEvent& event )
{
	switch( event.type )
	{
	case SystemEvent::Type::MouseKey:
	case SystemEvent::Type::MouseMove:
	case SystemEvent::Type::Wheel:
	case SystemEvent::Type::Quit:
		return;

	case SystemEvent::Type::TouchPress:
	case SystemEvent::Type::TouchMove:
		touch_state_[ event.event.touch.id ]= m_Vec2( event.event.touch.x, event.event.touch.y );
		touch_state_[ event.event.touch.id ]= m_Vec2( event.event.touch.x, event.event.touch.y );
		break;

	case SystemEvent::Type::TouchRelease:
		touch_state_.erase( event.event.touch.id );
		break;
	}
}

void TouchMapController::Update()
{
	if( !prev_touch_state_.empty() && !touch_state_.empty() )
	{
		// Do move.

		m_Vec2 pos_prev( 0.0f, 0.0f );
		for( const auto& touch_point : prev_touch_state_ )
			pos_prev+= touch_point.second;
		pos_prev/= float(prev_touch_state_.size());

		m_Vec2 pos_cur( 0.0f, 0.0f );
		for( const auto& touch_point : touch_state_ )
			pos_cur+= touch_point.second;
		pos_cur/= float(touch_state_.size());

		const m_Vec2 d_pos= pos_cur - pos_prev;
		if( prev_touch_state_.size() >= 2u && touch_state_.size() >= 2u )
		{
			// Do scale and move.

			float scale_prev= 0.0f;
			for( const auto& touch_point : prev_touch_state_ )
				scale_prev+= ( touch_point.second - pos_prev ).Length();
			scale_prev/= float(prev_touch_state_.size());

			float scale_cur= 0.0f;
			for( const auto& touch_point : touch_state_ )
				scale_cur+= ( touch_point.second - pos_cur ).Length();
			scale_cur/= float(touch_state_.size());

			const float scale_delta= scale_prev / scale_cur;

			const m_Vec2 pix_delta(
				-float(map_drawer_.GetViewportSize().width ) * 0.5f + pos_cur.x,
				-float(map_drawer_.GetViewportSize().height) * 0.5f + pos_cur.y );

			m_Vec2 position= map_drawer_.GetPosition();
			float scale= map_drawer_.GetScale();
			position+= pix_delta * scale;
			scale*= scale_delta;
			position-= pix_delta * scale;

			position+= m_Vec2( -d_pos.x, -d_pos.y ) * scale;

			map_drawer_.SetPosition( position );
			map_drawer_.SetScale( scale );
		}
		else if( prev_touch_state_.size() == touch_state_.size() )
		{
			// Do move only.
			map_drawer_.SetPosition(
				map_drawer_.GetPosition() +
				m_Vec2( -d_pos.x, -d_pos.y ) * map_drawer_.GetScale() );
		}
	}

	prev_touch_state_= touch_state_;
}

} // namespace PanzerMaps
