#include "main_loop.hpp"

namespace PanzerMaps
{

MainLoop::MainLoop()
{
}

MainLoop::~MainLoop()
{
}

bool MainLoop::Loop()
{
	SystemEvents input_events;
	system_window_.GetInput( input_events );
	for( const SystemEvent& event : input_events )
	{
		switch( event.type )
		{
		case SystemEvent::Type::MouseKey:
		case SystemEvent::Type::MouseMove:
		case SystemEvent::Type::Wheel:
			break;
		case SystemEvent::Type::Quit:
			return false;
		}
		map_drawer_.ProcessEvent( event );
	}

	system_window_.BeginFrame();
	map_drawer_.Draw();
	system_window_.EndFrame();

	return true;
}

} // namespace PanzerMaps
