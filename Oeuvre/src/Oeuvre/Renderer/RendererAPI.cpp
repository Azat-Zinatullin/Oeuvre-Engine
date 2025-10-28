#include "ovpch.h"
#include "RendererAPI.h"

#include "Platform/DirectX11/DX11RendererAPI.h"
#include "Platform/DirectX12/DX12RendererAPI.h"
#include "Platform/Vulkan/VulkanRendererAPI.h"
#include <Platform/OpenGL/OGLRendererAPI.h>

namespace Oeuvre
{
	RendererAPI::API RendererAPI::s_API = RendererAPI::API::OpenGL;

	std::shared_ptr<RendererAPI> RendererAPI::s_RendererAPI;

	void RendererAPI::SetAPI(API api)
	{
		s_API = api;
	}

	std::shared_ptr<RendererAPI> RendererAPI::GetInstance()
	{
		switch (s_API)
		{
		case RendererAPI::API::None:    assert(false && "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::DirectX11:  if (!s_RendererAPI.get()) s_RendererAPI = std::make_shared<DX11RendererAPI>(); return s_RendererAPI;
		case RendererAPI::API::DirectX12:  if (!s_RendererAPI.get()) s_RendererAPI = std::make_shared<DX12RendererAPI>(); return s_RendererAPI;
		case RendererAPI::API::Vulkan:     if (!s_RendererAPI.get()) s_RendererAPI = std::make_shared<VulkanRendererAPI>(); return s_RendererAPI;
		case RendererAPI::API::OpenGL:     if (!s_RendererAPI.get()) s_RendererAPI = std::make_shared<OGLRendererAPI>(); return s_RendererAPI;
		}
		assert(false && "Unknown RendererAPI!");
		return nullptr;
	}
}


