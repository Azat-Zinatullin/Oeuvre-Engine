#include "ovpch.h"
#include "OGLRendererAPI.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <Oeuvre/Core/Log.h>
#include "Platform/Windows/WindowsWindow.h"

namespace Oeuvre
{
	void OpenGLMessageCallback(
		unsigned source,
		unsigned type,
		unsigned id,
		unsigned severity,
		int length,
		const char* message,
		const void* userParam)
	{
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:         OV_CORE_CRITICAL(message); return;
		case GL_DEBUG_SEVERITY_MEDIUM:       OV_CORE_ERROR(message); return;
		case GL_DEBUG_SEVERITY_LOW:          OV_CORE_WARN(message); return;
		case GL_DEBUG_SEVERITY_NOTIFICATION: OV_CORE_TRACE(message); return;
		}

		assert(false && "Unknown severity level!");
	}

	OGLRendererAPI::OGLRendererAPI()
	{
		Init();
	}

	OGLRendererAPI::~OGLRendererAPI()
	{
	}

	void OGLRendererAPI::Init()
	{
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cout << "Failed to initialize GLAD" << std::endl;
			return;
		}

#ifdef _DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(OpenGLMessageCallback, nullptr);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

		RECT rect;
		GetClientRect(WindowsWindow::GetInstance()->GetHWND(), &rect);
		m_Width = rect.right - rect.left;
		m_Height = rect.bottom - rect.top;
	}

	void OGLRendererAPI::Shutdown()
	{
	}

	void OGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void OGLRendererAPI::SetClearColor(const glm::vec4& color)
	{
		m_ClearColor[0] = color.r;
		m_ClearColor[1] = color.g;
		m_ClearColor[2] = color.b;
		m_ClearColor[3] = color.a;
	}

	void OGLRendererAPI::Clear()
	{
		glClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void OGLRendererAPI::BeginFrame()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);		
		SetViewport(0, 0, m_Width, m_Height);
		Clear();
	}

	void OGLRendererAPI::EndFrame(bool vSyncEnabled)
	{
	}

	void OGLRendererAPI::OnWindowResize(int width, int height)
	{
		m_Width = width;
		m_Height = height;
	}

	void OGLRendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
	{
	}

	void OGLRendererAPI::ResetRenderTarget()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//glfwGetFramebufferSize(WindowsWindow::GetInstance()->GetGLFWwindow(), &m_Width, &m_Height);
		SetViewport(0, 0, m_Width, m_Height);
	}
}


