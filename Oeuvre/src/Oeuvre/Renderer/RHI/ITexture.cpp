#include "ovpch.h"
#include "ITexture.h"

#include "Oeuvre/Renderer/Renderer.h"
#include "Platform/DirectX11/DX11Texture2D.h"
#include "Platform/DirectX12/DX12Texture2D.h"
#include <Platform/OpenGL/OGLTexture2D.h>
#include <Platform/Vulkan/VulkanTexture2D.h>

namespace Oeuvre
{
	std::shared_ptr<ITexture> ITexture::Create(const TextureDesc& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    assert(false && "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::DirectX11:  return std::make_shared<DX11Texture2D>(desc);
		case RendererAPI::API::DirectX12:  return std::make_shared<DX12Texture2D>(desc);
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanTexture2D>(desc);
		case RendererAPI::API::OpenGL:  return std::make_shared<OGLTexture2D>(desc);
		}

		assert(false && "Unknown RendererAPI!");
		return nullptr;
	}
	std::shared_ptr<ITexture> ITexture::Create(const std::string& path)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    assert(false && "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::DirectX11:  return std::make_shared<DX11Texture2D>(path);
		case RendererAPI::API::DirectX12:  return std::make_shared<DX12Texture2D>(path);
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanTexture2D>(path);
		case RendererAPI::API::OpenGL:  return std::make_shared<OGLTexture2D>(path);
		}

		assert(false && "Unknown RendererAPI!");
		return nullptr;
	}
}


