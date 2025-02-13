#pragma once
#include <memory>

namespace Oeuvre
{
	struct WindowProperties
	{
		const char* Title;
		uint32_t Width;
		uint32_t Height;
		bool StartFullscreen;
		bool StartMaximized;

		WindowProperties(const char* title = "Oeuvre",
			uint32_t width = 800, uint32_t height = 600, bool startFullscreen = false, bool startMaximized = false)
			: Title(title), Width(width), Height(height), StartFullscreen(startFullscreen), StartMaximized(startMaximized) 
		{
		}
	};

	class Window
	{
	public:
		virtual ~Window() {}

		virtual void OnUpdate() = 0;
		virtual void OnDraw() = 0;
		virtual void OnResize(int width, int height) = 0;
		virtual void OnClose() = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
	};
}



