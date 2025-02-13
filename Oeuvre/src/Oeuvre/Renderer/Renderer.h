#pragma once

#include <memory>

#include "Camera.h"
#include "Shader.h"
#include "Platform/DirectX11/DX11RendererAPI.h"

namespace Oeuvre
{
	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene();
		static void EndScene(bool vSyncEnabled);

		static void Submit(const std::vector<std::shared_ptr<Shader>>& shaders, const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	};
}


