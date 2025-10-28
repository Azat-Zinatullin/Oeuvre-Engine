#pragma once

#include "Oeuvre/Renderer/Shader.h"
#include <d3d11.h>

#include "DX11InputLayout.h"


namespace Oeuvre
{
	class DX11VertexShader : public Shader
	{
	public:
		DX11VertexShader(const char* filePath, D3D11_INPUT_ELEMENT_DESC* inputLayout, int inputLayoutSize);
		virtual ~DX11VertexShader();
		bool Create(const char* path, D3D11_INPUT_ELEMENT_DESC* inputLayout, int inputLayoutSize);
		void Bind() override;
		void Unbind() override;
		ID3D11VertexShader* GetD3DShader();
		bool CreateInputLayout(D3D11_INPUT_ELEMENT_DESC layout[], UINT numElements, ID3DBlob* vsBlob);
		ID3DBlob* GetBlob() { return m_blobOut; }

		ID3D11InputLayout* GetRawInputLayout() { return m_inputLayout.GetLayout(); }

	private:
		ID3D11VertexShader* m_vertexShader = nullptr;
		DX11InputLayout m_inputLayout;
		ID3DBlob* m_blobOut = nullptr;
	};
}

