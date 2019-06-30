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
		unsigned int x, y;
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

struct ViewportSize
{
	unsigned int width;
	unsigned int height;
};

class SystemWindow final
{
public:
	SystemWindow();
	~SystemWindow();

	SystemWindow(const SystemWindow&)= delete;
	SystemWindow(SystemWindow&&)= delete;
	SystemWindow& operator=(const SystemWindow&)= delete;
	SystemWindow& operator=(SystemWindow&&)= delete;

	void GetInput( SystemEvents& out_events );

	void BeginFrame();
	void EndFrame();

	const ViewportSize GetViewportSize() const { return viewport_size_; }
	float GetPixelsInScreenMeter() const { return pixels_in_screen_meter_; }

private:
	ViewportSize viewport_size_;
	float pixels_in_screen_meter_;
	SDL_Window* window_= nullptr;
	SDL_GLContext gl_context_= nullptr;
};

} // namespace PanzerMaps
