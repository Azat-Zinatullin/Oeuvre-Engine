#pragma once

#include "Oeuvre/Renderer/Shader.h"
#include "Oeuvre/Renderer/Renderer.h"
#include <d3d11.h>

namespace Oeuvre
{
	class DX11DomainShader : public Shader
	{
	public:
		DX11DomainShader(const char* filePath);
		virtual ~DX11DomainShader();
		bool Create(const char* path);
		void Bind() override;
		void Unbind() override;
	private:
		ID3D11DomainShader* m_DomainShader = nullptr;
		ID3DBlob* m_blobOut = nullptr;
	};
}