#include <memory>
#include "main_loop.hpp"

int main()
{
	const std::unique_ptr<PanzerMaps::MainLoop> main_loop( new PanzerMaps::MainLoop() );

	while( main_loop->Loop() )
	{
	}

	return 0;
}
