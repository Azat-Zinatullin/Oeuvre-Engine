#pragma once

#include "Oeuvre/Renderer/Shader.h"
#include "Oeuvre/Renderer/Renderer.h"
#include <d3d11.h>

namespace Oeuvre
{
	class DX11HullShader : public Shader
	{
	public:
		DX11HullShader(const char* filePath);
		virtual ~DX11HullShader();
		bool Create(const char* path);
		void Bind() override;
		void Unbind() override;
	private:
		ID3D11HullShader* m_HullShader = nullptr;
		ID3DBlob* m_blobOut = nullptr;
	};
}