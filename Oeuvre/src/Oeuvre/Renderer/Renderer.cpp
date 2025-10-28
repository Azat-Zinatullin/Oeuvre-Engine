#include "ovpch.h"
#include "Renderer.h"



namespace Oeuvre
{
	void Renderer::Init()
	{
		RendererAPI::GetInstance();
	}

	void Renderer::Shutdown()
	{
		RendererAPI::GetInstance()->Shutdown();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RendererAPI::GetInstance()->OnWindowResize(width, height);
	}

	void Renderer::BeginFrame()
	{
		RendererAPI::GetInstance()->BeginFrame();
	}

	void Renderer::EndFrame(bool vSyncEnabled)
	{
		RendererAPI::GetInstance()->EndFrame(vSyncEnabled);
	}
}


