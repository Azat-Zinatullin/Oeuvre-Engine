#include "DX11Shader.h"

#include "Platform/DirectX11/DX11VertexShader.h"
#include "Platform/DirectX11/DX11PixelShader.h"


namespace Oeuvre
{
	DX11Shader::DX11Shader(const std::string filePath)
	{		
		m_FilePath = filePath;
	}

	DX11Shader::DX11Shader(const std::string filePath, D3D11_INPUT_ELEMENT_DESC* inputLayout, int inputLayoutSize)
	{
		m_FilePath = filePath;
		m_VertexShader = std::make_unique<DX11VertexShader>(m_FilePath.c_str(), inputLayout, inputLayoutSize);
		m_PixelShader = std::make_unique<DX11PixelShader>(m_FilePath.c_str());
	}

	DX11Shader::~DX11Shader()
	{

	}

	void DX11Shader::Compile()
	{
		//for (auto &e: m_InputLayout)
		//	std::cout << "SemanticName: " << e.SemanticName << ", offset: " << e.AlignedByteOffset << ", file path: " << m_FilePath << '\n';
		m_PixelShader = std::make_unique<DX11PixelShader>(m_FilePath.c_str());
		m_VertexShader = std::make_unique<DX11VertexShader>(m_FilePath.c_str(), m_InputLayout.data(), m_InputLayout.size());
	}

	void DX11Shader::Bind()
	{
		m_VertexShader->Bind();
		m_PixelShader->Bind();
	}

	void DX11Shader::Unbind()
	{
		m_VertexShader->Unbind();
		m_PixelShader->Unbind();
	}

	void DX11Shader::AddInputElement(const std::string& semanticName, DXGI_FORMAT format)
	{
		int offset = 0;
		switch (format)
		{
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			offset = 16;
			break;
		case DXGI_FORMAT_R32G32B32_FLOAT:
			offset = 12;
			break;
		case DXGI_FORMAT_R32G32_FLOAT:
			offset = 8;
			break;
		}	
 
		D3D11_INPUT_ELEMENT_DESC element = {(LPCSTR)semanticName.c_str(), 0, format, 0, m_InputLayoutOffsetSum, D3D11_INPUT_PER_VERTEX_DATA, 0};
		m_InputLayout.emplace_back(element);

		m_InputLayoutOffsetSum += offset;
	}
}


