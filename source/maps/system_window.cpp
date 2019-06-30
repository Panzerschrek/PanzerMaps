#include <SDL.h>
#include "../common/log.hpp"
#include "../panzer_ogl_lib/panzer_ogl_lib.hpp"
#include "../panzer_ogl_lib/shaders_loading.hpp"
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

#ifdef DEBUG
static void APIENTRY GLDebugMessageCallback(
	GLenum source, GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length, const GLchar* message,
	const void* userParam )
{
	(void)source;
	(void)type;
	(void)id;
	(void)severity;
	(void)length;
	(void)userParam;
	Log::Info( message );
}
#endif

SystemWindow::SystemWindow()
{
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
		Log::FatalError( "Can not initialize sdl video" );

	const bool fullscreen= false;
	const bool vsync= true;
	const int msaa_samples= 4; // 0 - no multisampling.

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 ); // TODO - we do not use depth buffer. Maybe set to 0?

	#ifdef DEBUG
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
	#endif

	#ifdef PM_OPENGL_ES
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES );
	#else
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	#endif

	if( msaa_samples > 0 )
	{
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, msaa_samples );
	}

	SDL_Rect display_rect;
	if( SDL_GetDisplayUsableBounds( 0, &display_rect ) != 0 )
		Log::FatalError( "Can not get display 0 bounds" );
	viewport_size_.width= display_rect.w;
	viewport_size_.height= display_rect.h;

	window_=
		SDL_CreateWindow(
			"PanzerMaps",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			viewport_size_.width, viewport_size_.height,
			SDL_WINDOW_OPENGL | ( fullscreen ? SDL_WINDOW_FULLSCREEN : 0 ) | SDL_WINDOW_SHOWN );

	if( window_ == nullptr )
		Log::FatalError( "Can not create window" );

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

	rSetShaderLoadingLogCallback( []( const char* log_data ){ Log::Warning( log_data ); } );
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
		{
			out_events.emplace_back();
			SystemEvent& out_event= out_events.back();
			out_event.type= SystemEvent::Type::Wheel;
			out_event.event.wheel.delta= event.wheel.y;
			int x= 0, y= 0;
			SDL_GetMouseState( &x, &y );
			out_event.event.wheel.x= static_cast<unsigned int>(x);
			out_event.event.wheel.y= static_cast<unsigned int>(y);
		}
		break;

		case SDL_FINGERDOWN:
		case SDL_FINGERUP:
		case SDL_FINGERMOTION:
		{
			out_events.emplace_back();
			SystemEvent& out_event= out_events.back();

			if( event.type == SDL_FINGERDOWN   ) out_event.type= SystemEvent::Type::TouchPress  ;
			if( event.type == SDL_FINGERUP     ) out_event.type= SystemEvent::Type::TouchRelease;
			if( event.type == SDL_FINGERMOTION ) out_event.type= SystemEvent::Type::TouchMove   ;

			out_event.event.touch.x= event.tfinger.x * float(viewport_size_.width );
			out_event.event.touch.y= event.tfinger.y * float(viewport_size_.height);
			out_event.event.touch.id= event.tfinger.fingerId;
		}

		default:
			break;
		};
	} // while events
}

void SystemWindow::BeginFrame()
{
	const float c_inches_to_meters= 2.54f / 100.0f;

	pixels_in_screen_meter_= 96.0f / c_inches_to_meters;
	float dots_per_inch;
	if( SDL_GetDisplayDPI( SDL_GetWindowDisplayIndex( window_ ), &dots_per_inch, nullptr, nullptr ) == 0 )
		pixels_in_screen_meter_= dots_per_inch / c_inches_to_meters;
}

void SystemWindow::EndFrame()
{
	SDL_GL_SwapWindow( window_ );
}

} // namespace PanzerMaps
