#pragma once

#include "Oeuvre/Renderer/Shader.h"
#include "ovpch.h"
#include "d3d11.h"
#include "Platform/DirectX11/DX11VertexShader.h"
#include "Platform/DirectX11/DX11PixelShader.h"

namespace Oeuvre
{
	class DX11Shader : public Shader
	{
	public:
		DX11Shader(const std::string filePath);
		DX11Shader(const std::string filePath, D3D11_INPUT_ELEMENT_DESC* inputLayout, int inputLayoutSize);
		~DX11Shader();
		void Compile();
		void Bind() override;
		void Unbind() override;

		void AddInputElement(const std::string& semanticName, DXGI_FORMAT format);
	private:
		std::string m_FilePath;

		std::unique_ptr<DX11VertexShader> m_VertexShader;
		std::unique_ptr<DX11PixelShader> m_PixelShader;

		std::vector<D3D11_INPUT_ELEMENT_DESC> m_InputLayout;
		int m_InputLayoutOffsetSum = 0;
	};
}

