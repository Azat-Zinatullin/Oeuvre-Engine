#pragma once
#include "Oeuvre/Renderer/RendererAPI.h"


namespace Oeuvre
{
	class OGLRendererAPI : public RendererAPI
	{
	public:
		OGLRendererAPI();
		~OGLRendererAPI();

		void Init() override;
		void Shutdown() override;
		void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		void SetClearColor(const glm::vec4& color) override;
		void Clear() override;
		void BeginFrame() override;
		void EndFrame(bool vSyncEnabled) override;
		void OnWindowResize(int width, int height) override;

		void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount) override;

		void ResetRenderTarget();

	private:
		float m_ClearColor[4] = { 0.f, 0.f, 0.f, 1.f };

		int m_Width = 1600;
		int m_Height = 800;
	};
}