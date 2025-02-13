#include "Shader.h"

#include "Oeuvre/Renderer/RendererAPI.h"
#include "Platform/DirectX11/DX11Shader.h"


namespace Oeuvre
{
	std::shared_ptr<Shader> Shader::Create(const std::string& filepath)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:    assert(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::DirectX11:  return std::make_shared<DX11Shader>(filepath);
		}

		assert(false, "Unknown RendererAPI!");
		return nullptr;
	}
}


