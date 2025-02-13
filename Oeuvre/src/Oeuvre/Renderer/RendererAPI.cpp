#include "RendererAPI.h"

#include "Platform/DirectX11/DX11RendererAPI.h"

namespace Oeuvre
{
	RendererAPI::API RendererAPI::s_API = RendererAPI::API::DirectX11;

	std::shared_ptr<RendererAPI> RendererAPI::s_RendererAPI;

	std::shared_ptr<RendererAPI> RendererAPI::GetInstance()
	{
		switch (s_API)
		{
		case RendererAPI::API::None:    assert(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::DirectX11:  if (!s_RendererAPI.get()) s_RendererAPI = std::make_shared<DX11RendererAPI>(); return s_RendererAPI;
		}
		assert(false, "Unknown RendererAPI!");
		return nullptr;
	}
}


