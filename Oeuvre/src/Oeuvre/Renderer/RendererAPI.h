#pragma once

#include "ovpch.h"
#include <glm/glm.hpp>

#include "Oeuvre/Renderer/VertexArray.h"

namespace Oeuvre
{
	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0, DirectX11 = 1, DirectX12 = 2, Vulkan = 3, OpenGL = 4
		};

		RendererAPI() = default;
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void Shutdown() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void Clear() = 0;

		virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame(bool vSyncEnabled) = 0;

		virtual void OnWindowResize(int width, int height) = 0;

		static void SetAPI(API api);
		static API GetAPI() { return s_API; }
		static std::shared_ptr<RendererAPI> GetInstance();

	private:
		static API s_API;
		static std::shared_ptr<RendererAPI> s_RendererAPI;
	};
}

