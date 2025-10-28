#include "ovpch.h"
#include "Buffer.h"

#include "Renderer.h"
#include "RendererAPI.h"
#include "Platform/DirectX11/DX11Buffer.h"

namespace Oeuvre
{
	std::shared_ptr<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size, uint32_t stride)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:      assert(false && "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::DirectX11: return std::make_shared<DX11VertexBuffer>(vertices, size, stride);
		}
		assert(false && "Unknown RendererAPI!");
		return nullptr;
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::Create(unsigned int* indices, uint32_t count)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:      assert(false && "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::DirectX11: return std::make_shared<DX11IndexBuffer>(indices, count);
		}
		assert(false && "Unknown RendererAPI!");
		return nullptr;
	}
}


