#pragma once

#include "Oeuvre/Renderer/Shader.h"
#include "Oeuvre/Renderer/Renderer.h"
#include <d3d11.h>

namespace Oeuvre
{
	class DX11PixelShader : public Shader
	{
	public:
		DX11PixelShader(const char* filePath);
		virtual ~DX11PixelShader();
		bool Create(const char* path);
		void Bind() override;
		void Unbind() override;

		ID3DBlob* GetBlob() { return m_blobOut; }
	private:
		ID3D11PixelShader* m_pixelShader = nullptr;
		ID3DBlob* m_blobOut;
	};
}