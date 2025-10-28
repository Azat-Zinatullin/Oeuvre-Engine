#pragma once

#include "Oeuvre/Renderer/Shader.h"
#include "Oeuvre/Renderer/Renderer.h"
#include <d3d11.h>

namespace Oeuvre
{
	class DX11ComputeShader : public Shader
	{
	public:
		DX11ComputeShader(const char* filePath, const char* mainFunctionName);
		virtual ~DX11ComputeShader();
		bool Create(const char* path, const char* mainFunctionName);
		void Bind() override;
		void Unbind() override;
	private:
		ID3D11ComputeShader* m_ComputeShader = nullptr;
		ID3DBlob* m_blobOut = nullptr;
	};
}