#include "ovpch.h"
#include "WindowsWindow.h"

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_NATIVE_INCLUDE_NONE
#include "GLFW/glfw3native.h"

#include "Oeuvre/Events/EventHandler.h"
#include "stb_image.h"

#include "Oeuvre/Renderer/RendererAPI.h"

namespace Oeuvre
{
	std::shared_ptr<WindowsWindow> WindowsWindow::s_Window;

	std::shared_ptr<WindowsWindow> WindowsWindow::GetInstance(const WindowProperties& props)
	{
		if (!s_Window.get())
			s_Window = std::make_shared<WindowsWindow>(props);
		return s_Window;
	}

	static GLFWimage load_icon(const std::string& path)
	{
		int width, height, channels;
		unsigned char* img = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		if (img == NULL) 
		{
			printf("Error in loading the image\n");
			exit(1);		
		}
		GLFWimage image;
		image.width = width;
		image.height = height;
		image.pixels = img;
		return image;
	}

	WindowsWindow::WindowsWindow(const WindowProperties& windowProps)
	{
		Init(windowProps);

		glfwInit();

		if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		}
		else
		{
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		}	
		m_Monitor = glfwGetPrimaryMonitor();
		if (windowProps.StartFullscreen)
		{
			const GLFWvidmode* mode = glfwGetVideoMode(m_Monitor);

			glfwWindowHint(GLFW_RED_BITS, mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

			m_Window = glfwCreateWindow(mode->width, mode->height, windowProps.Title, m_Monitor, nullptr);
		}
		else
		{
			m_Window = glfwCreateWindow(windowProps.Width, windowProps.Height, windowProps.Title, nullptr, nullptr);
		}		
		glfwMakeContextCurrent(m_Window);
		glfwSwapInterval(0);
		if (m_Window == nullptr)
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
			return;
		}
		m_Hwnd = glfwGetWin32Window(m_Window);

		GLFWimage images[2];
		images[0] = load_icon("../resources/icons/OeuvreLogo.png");
		images[1] = load_icon("../resources/icons/OeuvreLogo_small.png");

		glfwSetWindowIcon(m_Window, 2, images);

		glfwSetFramebufferSizeCallback(m_Window, &FramebufferSizeCallback);
		glfwSetKeyCallback(m_Window, &KeyCallback);
		glfwSetCursorPosCallback(m_Window, &CursorPositionCallback);
		glfwSetMouseButtonCallback(m_Window, &MouseButtonCallback);
		glfwSetScrollCallback(m_Window, &MouseScrollCallback);
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
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	void WindowsWindow::OnResize(int width, int height)
	{
		m_Data.Width = width;
		m_Data.Height = height;
		std::cout << "WindowsWindow::OnResize()\n";
	}

	bool WindowsWindow::IsFullscreen()
	{
		return glfwGetWindowMonitor(m_Window) != nullptr;
	}

	void WindowsWindow::SetFullscreen(bool fullscreen)
	{
		if (IsFullscreen() == fullscreen)
			return;

		if (fullscreen)
		{
			// backup window position and window size
			glfwGetWindowPos(m_Window, &m_Data.Xpos, &m_Data.Ypos);
			glfwGetWindowSize(m_Window, &m_Data.Width, &m_Data.Height);

			// get resolution of monitor
			const GLFWvidmode* mode = glfwGetVideoMode(m_Monitor);

			// switch to full screen
			glfwSetWindowMonitor(m_Window, m_Monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		}
		else
		{
			// restore last window size and position
			glfwSetWindowMonitor(m_Window, nullptr, m_Data.Xpos, m_Data.Ypos, m_Data.Width, m_Data.Height, GLFW_DONT_CARE);
		}
	}

	bool WindowsWindow::Init(const WindowProperties& windowProps)
	{
		m_Data.Title = windowProps.Title;
		m_Data.Xpos = windowProps.Xpos;
		m_Data.Ypos = windowProps.Ypos;
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