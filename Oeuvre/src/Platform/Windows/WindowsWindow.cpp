#include "WindowsWindow.h"
#include <iostream>

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_NATIVE_INCLUDE_NONE
#include "GLFW/glfw3native.h"

#include "Oeuvre/Events/EventHandler.h"

namespace Oeuvre
{
	std::shared_ptr<WindowsWindow> WindowsWindow::s_Window;

	std::shared_ptr<WindowsWindow> WindowsWindow::GetInstance(const WindowProperties& props)
	{
		if (!s_Window.get())
			s_Window = std::make_shared<WindowsWindow>(props);
		return s_Window;
	}

	WindowsWindow::WindowsWindow(const WindowProperties& windowProps)
	{
		Init(windowProps);

		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		GLFWwindow* window = glfwCreateWindow(windowProps.Width, windowProps.Height, "Oeuvre", NULL, NULL);
		glfwMakeContextCurrent(window);
		glfwSwapInterval(0);
		if (window == NULL)
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
			return;
		}
		m_Window = window;
		m_Hwnd = glfwGetWin32Window(window);

		glfwSetFramebufferSizeCallback(window, &FramebufferSizeCallback);
		glfwSetKeyCallback(window, &KeyCallback);
		glfwSetCursorPosCallback(window, &CursorPositionCallback);
		glfwSetMouseButtonCallback(window, &MouseButtonCallback);
		glfwSetScrollCallback(window, &MouseScrollCallback);
	}

	void WindowsWindow::OnUpdate()
	{		
		glfwPollEvents();
		if (glfwWindowShouldClose(m_Window))
		{
			WindowCloseEvent ce;
			SEND_WINDOW_EVENT(ce);
		}
	}

	void WindowsWindow::OnDraw()
	{
		glfwSwapBuffers(m_Window);
	}

	void WindowsWindow::OnClose()
	{
		glfwTerminate();
	}

	void WindowsWindow::OnResize(int width, int height)
	{
		m_Data.Width = width;
		m_Data.Height = height;
		std::cout << "WindowsWindow::OnResize()\n";
	}

	bool WindowsWindow::Init(const WindowProperties& windowProps)
	{
		m_Data.Title = windowProps.Title;
		m_Data.Width = windowProps.Width;
		m_Data.Height = windowProps.Height;

		return true;
	}

	void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
	{
		WindowResizeEvent re;
		re.width = width;
		re.height = height;

		SEND_WINDOW_EVENT(re);
	}

	void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (action == GLFW_PRESS)
		{
			KeyDownEvent kde;
			kde.keycode = key;
			SEND_KEY_EVENT(kde);
		}	
		else if (action == GLFW_RELEASE)
		{
			KeyUpEvent kue;
			kue.keycode = key;
			SEND_KEY_EVENT(kue);
		}
	}

	void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
	{
		MouseMovedEvent mme;
		mme.x = xpos;
		mme.y = ypos;
		SEND_MOUSE_EVENT(mme);
	}

	void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		if (action == GLFW_PRESS)
		{
			MouseButtonDownEvent mbde;
			mbde.button = button;
			SEND_MOUSE_EVENT(mbde);
			
		}
		else if (action == GLFW_RELEASE)
		{
			MouseButtonUpEvent mbue;
			mbue.button = button;
			SEND_MOUSE_EVENT(mbue);
		}
	}

	void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		MouseScrollEvent mse;
		mse.xoffset = xoffset;
		mse.yoffset = yoffset;
		SEND_MOUSE_EVENT(mse);
	}
}