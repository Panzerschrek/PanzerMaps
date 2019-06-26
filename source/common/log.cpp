#ifdef HAVE_SDL
#include <SDL_messagebox.h>
#endif
#include "log.hpp"

namespace PanzerMaps
{

std::ofstream Log::log_file_{ "panzer_maps.log" };

void Log::ShowFatalMessageBox( const std::string& error_message )
{
	#ifdef HAVE_SDL
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR,
		"Fatal error",
		error_message.c_str(),
		nullptr );
	#else
	(void)error_message;
	#endif
}

} // namespace PanzerMaps
