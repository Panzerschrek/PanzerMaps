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
	const std::string external_storage_path= SDL_AndroidGetExternalStoragePath();
	Log::Info( "External storage path: ", external_storage_path );
	return external_storage_path + "/" + map_file_base;
#else
	return map_file_base;
#endif
}

MainLoop::MainLoop()
	: system_window_()
	, ui_drawer_( system_window_.GetViewportSize() )
	, map_drawer_( system_window_, ui_drawer_, GetMapFile().c_str() )
	, mouse_map_controller_( map_drawer_ )
	, touch_map_controller_( map_drawer_ )
	, zoom_controller_( ui_drawer_, map_drawer_ )
	#ifdef __ANDROID__
	, gps_button_( ui_drawer_, gps_service_ )
	#else
	, gps_button_( ui_drawer_ )
	#endif
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
		case SystemEvent::Type::TouchPress:
		case SystemEvent::Type::TouchRelease:
		case SystemEvent::Type::TouchMove:
			break;
		case SystemEvent::Type::Quit:
			return false;
		}

		if( zoom_controller_.ProcessEvent( event ) )
			continue;
		if( gps_button_.ProcessEvent( event ) )
			continue;

		#ifdef __ANDROID__
		touch_map_controller_.ProcessEvent( event );
		#else
		mouse_map_controller_.ProcessEvent( event );
		#endif
	}

	touch_map_controller_.DoMove();
	zoom_controller_.DoMove();

	#ifdef __ANDROID__
	gps_service_.Update();
	map_drawer_.SetGPSMarkerPosition( gps_service_.GetGPSPosition() );
	#endif

	system_window_.BeginFrame();
	map_drawer_.Draw();
	zoom_controller_.Draw();
	gps_button_.Draw();
	system_window_.EndFrame();

	return true;
}

} // namespace PanzerMaps
