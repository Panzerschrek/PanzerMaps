#include <SDL_messagebox.h>

#include "log.hpp"

namespace PanzerMaps
{

std::ofstream Log::log_file_{ "panzer_maps.log" };

void Log::ShowFatalMessageBox( const std::string& error_message )
{
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR,
		"Fatal error",
		error_message.c_str(),
		nullptr );
}

} // namespace PanzerMaps
