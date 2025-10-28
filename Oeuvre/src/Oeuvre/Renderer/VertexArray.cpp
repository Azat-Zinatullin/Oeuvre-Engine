#include "ovpch.h"
#include "VertexArray.h"

#include "Oeuvre/Renderer/Renderer.h"
#include "Platform/DirectX11/DX11VertexArray.h"

namespace Oeuvre
{
	std::shared_ptr<VertexArray> VertexArray::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    assert(false && "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::DirectX11:  return std::make_shared<DX11VertexArray>();
		}

		assert(false && "Unknown RendererAPI!");
		return nullptr;
	}
}


