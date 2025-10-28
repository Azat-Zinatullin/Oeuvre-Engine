#pragma once

#include <memory>

#include "Camera.h"
#include "Shader.h"
#include "RendererAPI.h"

namespace Oeuvre
{
	class Renderer
	{
	public:
		Renderer() = default;
		~Renderer() = default;

		static void Init();
		static void Shutdown();

		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginFrame();
		static void EndFrame(bool vSyncEnabled);

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	};
}


