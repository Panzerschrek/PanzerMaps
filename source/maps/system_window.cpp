#include <SDL.h>
#include "../panzer_ogl_lib/panzer_ogl_lib.hpp"
#include "log.hpp"
#include "system_window.hpp"

namespace PanzerMaps
{

static SystemEvent::MouseKeyEvent::Button TranslateMouseButton( const Uint8 button )
{
	using Button= SystemEvent::MouseKeyEvent::Button;
	switch(button)
	{
	case SDL_BUTTON_LEFT: return Button::Left;
	case SDL_BUTTON_RIGHT: return Button::Right;
	case SDL_BUTTON_MIDDLE: return Button::Middle;
	};

	return Button::Unknown;
}

SystemWindow::SystemWindow()
{
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
		Log::FatalError( "Can not initialize sdl video" );

	const int width= 1024;
	const int height= 768;
	const bool fullscreen= false;
	const bool vsync= true;

	window_=
		SDL_CreateWindow(
			"PanzerMaps",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			width, height,
			SDL_WINDOW_OPENGL | ( fullscreen ? SDL_WINDOW_FULLSCREEN : 0 ) | SDL_WINDOW_SHOWN );

	gl_context_= SDL_GL_CreateContext( window_ );
	if( gl_context_ == nullptr )
		Log::FatalError( "Can not create OpenGL context" );

	Log::Info("");
	Log::Info( "OpenGL configuration: " );
	Log::Info( "Vendor: ", glGetString( GL_VENDOR ) );
	Log::Info( "Renderer: ", glGetString( GL_RENDERER ) );
	Log::Info( "Version: ", glGetString( GL_VERSION ) );
	Log::Info("");

	SDL_GL_SetSwapInterval( vsync ? 1 : 0 );

	GetGLFunctions( SDL_GL_GetProcAddress );
	#ifdef DEBUG
	// Do reinterpret_cast, because on different platforms arguments of GLDEBUGPROC have
	// different const qualifier. Just ignore this cualifiers - we always have 'const'.
	if( glDebugMessageCallback != nullptr )
		glDebugMessageCallback( reinterpret_cast<GLDEBUGPROC>(&GLDebugMessageCallback), NULL );
	#endif
}

SystemWindow::~SystemWindow()
{
	SDL_GL_DeleteContext( gl_context_ );
	SDL_DestroyWindow( window_ );
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
}

void SystemWindow::GetInput( SystemEvents& out_events )
{
	out_events.clear();

	SDL_Event event;
	while( SDL_PollEvent(&event) )
	{
		switch(event.type)
		{
		case SDL_WINDOWEVENT:
			if( event.window.event == SDL_WINDOWEVENT_CLOSE )
			{
				out_events.emplace_back();
				out_events.back().type= SystemEvent::Type::Quit;
			}
			break;

		case SDL_QUIT:
			out_events.emplace_back();
			out_events.back().type= SystemEvent::Type::Quit;
			break;

		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			out_events.emplace_back();
			out_events.back().type= SystemEvent::Type::MouseKey;
			out_events.back().event.mouse_key.mouse_button= TranslateMouseButton( event.button.button );
			out_events.back().event.mouse_key.x= event.button.x;
			out_events.back().event.mouse_key.y= event.button.y;
			out_events.back().event.mouse_key.pressed= event.type == SDL_MOUSEBUTTONUP ? false : true;
			break;

		case SDL_MOUSEMOTION:
			out_events.emplace_back();
			out_events.back().type= SystemEvent::Type::MouseMove;
			out_events.back().event.mouse_move.dx= event.motion.xrel;
			out_events.back().event.mouse_move.dy= event.motion.yrel;
			break;

		case SDL_MOUSEWHEEL:
			out_events.emplace_back();
			out_events.back().type= SystemEvent::Type::Wheel;
				out_events.back().event.wheel.delta= event.wheel.y;
			break;

		// TODO - fill other events here

		default:
			break;
		};
	} // while events
}

void SystemWindow::BeginFrame()
{
	glClearColor( 0.1f, 0.0f, 0.1f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT );
}

void SystemWindow::EndFrame()
{
	SDL_GL_SwapWindow( window_ );
}

} // namespace PanzerMaps
