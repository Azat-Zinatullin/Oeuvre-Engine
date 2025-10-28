#pragma once
#include <memory>

struct GLFWwindow;

namespace Oeuvre
{
	struct WindowProperties
	{
		const char* Title;
		uint32_t Xpos;
		uint32_t Ypos;
		uint32_t Width;
		uint32_t Height;
		bool StartFullscreen;
		bool StartMaximized;

		WindowProperties(const char* title = "Oeuvre", uint32_t xpos = 200, uint32_t ypos = 200,
			uint32_t width = 1600, uint32_t height = 900, bool startFullscreen = false, bool startMaximized = false)
			: Title(title), Xpos(xpos), Ypos(ypos), Width(width), Height(height), StartFullscreen(startFullscreen), StartMaximized(startMaximized) 
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

		virtual bool IsFullscreen() = 0;
		virtual void SetFullscreen(bool fullscreen) = 0;

		GLFWwindow* GetGLFWwindow() { return m_Window; }

	protected:
		GLFWwindow* m_Window = nullptr;
	};
}



