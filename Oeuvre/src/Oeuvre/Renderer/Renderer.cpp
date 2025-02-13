#include "Renderer.h"



namespace Oeuvre
{
	void Renderer::Init()
	{
	}

	void Renderer::Shutdown()
	{
		//RendererAPI::GetInstance()->Shutdown();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RendererAPI::GetInstance()->OnWindowResize(width, height);
	}

	void Renderer::BeginScene()
	{
		RendererAPI::GetInstance()->Clear();
	}

	void Renderer::EndScene(bool vSyncEnabled)
	{
		if (vSyncEnabled)
			((DX11RendererAPI*)RendererAPI::GetInstance().get())->GetSwapchain()->Present(1, 0);
		else
			((DX11RendererAPI*)RendererAPI::GetInstance().get())->GetSwapchain()->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	}

	void Renderer::Submit(const std::vector<std::shared_ptr<Shader>>& shaders, const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4& transform)
	{
		for (auto& shader : shaders)
			shader->Bind();
		vertexArray->Bind();
	}
}


