#include <memory>
#include "main_loop.hpp"

#ifdef __ANDROID__
extern "C" __attribute__((visibility("default"))) int SDL_main()
#else
int main()
#endif
{
	const std::unique_ptr<PanzerMaps::MainLoop> main_loop( new PanzerMaps::MainLoop() );

	while( main_loop->Loop() )
	{
	}

	return 0;
}
