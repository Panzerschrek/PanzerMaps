#include <SDL_system.h>
#include "../common/log.hpp"
#include "main_loop.hpp"

namespace PanzerMaps
{

static std::string GetMapFile()
{
	const std::string map_file_base= "map.pm";
#ifdef __ANDROID__
	Log::Info( "External storage state: ", SDL_AndroidGetExternalStorageState() );
	return std::string(SDL_AndroidGetExternalStoragePath()) + "/" + map_file_base;
#else
	return map_file_base;
#endif
}

MainLoop::MainLoop()
	: system_window_()
	, map_drawer_( system_window_, GetMapFile().c_str() )
	, mouse_map_controller_( map_drawer_ )
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
		mouse_map_controller_.ProcessEvent( event );
	}

	system_window_.BeginFrame();
	map_drawer_.Draw();
	system_window_.EndFrame();

	return true;
}

} // namespace PanzerMaps
