#pragma once
#include <vector>
#include <SDL_video.h>

namespace PanzerMaps
{

struct SystemEvent
{
	enum class Type
	{
		MouseKey,
		MouseMove,
		Wheel,
		Quit,
	};

	struct MouseKeyEvent
	{
		enum class Button
		{
			Unknown= 0,
			Left, Right, Middle,
			ButtonCount
		};

		Button mouse_button;
		unsigned int x, y;
		bool pressed;
	};

	struct MouseMoveEvent
	{
		int dx, dy;
	};

	struct WheelEvent
	{
		int delta;
	};

	struct QuitEvent
	{};

	Type type;
	union
	{
		MouseKeyEvent mouse_key;
		MouseMoveEvent mouse_move;
		WheelEvent wheel;
		QuitEvent quit;
	} event;
};

typedef std::vector<SystemEvent> SystemEvents;

class SystemWindow final
{
public:
	SystemWindow();
	~SystemWindow();

	void GetInput( SystemEvents& out_events );

	void BeginFrame();
	void EndFrame();

private:
	SDL_Window* window_= nullptr;
	SDL_GLContext gl_context_= nullptr;
};

} // namespace PanzerMaps
