#include "Texture.h"

#include "Oeuvre/Renderer/Renderer.h"
#include "Platform/DirectX11/DX11Texture2D.h"

namespace Oeuvre
{
	std::shared_ptr<Texture2D> Texture2D::Create(const TextureSpecification& specification)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    assert(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::DirectX11:  return std::make_shared<DX11Texture2D>(specification);
		}

		assert(false, "Unknown RendererAPI!");
		return nullptr;
	}
	std::shared_ptr<Texture2D> Texture2D::Create(const std::string& path)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    assert(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::DirectX11:  return std::make_shared<DX11Texture2D>(path);
		}

		assert(false, "Unknown RendererAPI!");
		return nullptr;
	}
}


