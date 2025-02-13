#pragma once

#include "../../Oeuvre/Core/Window.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

struct GLFWwindow;

namespace Oeuvre
{
	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProperties& windowProps);
		WindowsWindow(const WindowsWindow&) = delete;
		WindowsWindow& operator=(const WindowsWindow&) = delete;
		virtual ~WindowsWindow() {}

		virtual void OnUpdate() override;
		virtual void OnDraw() override;
		virtual void OnClose() override;
		virtual uint32_t GetWidth() const override { return m_Data.Width; };
		virtual uint32_t GetHeight() const override { return m_Data.Height; };

		virtual void OnResize(int width, int height) override;

		HWND GetHWND() { return m_Hwnd; }
		GLFWwindow* GetGLFWwindow() { return m_Window; }

		static std::shared_ptr<WindowsWindow> GetInstance(const WindowProperties& props = WindowProperties());
	private:
		bool Init(const WindowProperties& windowProps);
	private:
		struct WindowData
		{
			const char* Title;
			uint32_t Width, Height;
		} m_Data;

		HWND m_Hwnd;
		GLFWwindow* m_Window;

		static std::shared_ptr<WindowsWindow> s_Window;
	};

	void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
	void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
	void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
}


