#include "DX11VertexShader.h"

#include "Platform/DirectX11/DX11RendererAPI.h"
#include <string>
#include <iostream>
#include <d3dcompiler.h>

namespace Oeuvre
{
	DX11VertexShader::DX11VertexShader(const char* filePath, D3D11_INPUT_ELEMENT_DESC* inputLayout, int inputLayoutSize)
	{
		if (!Create(filePath, inputLayout, inputLayoutSize))
			std::cout << "ERROR::Can't create DX11VertexShader!\n";
	}

	DX11VertexShader::~DX11VertexShader()
	{
		if (m_vertexShader) m_vertexShader->Release();	
	}

	bool DX11VertexShader::Create(const char* path, D3D11_INPUT_ELEMENT_DESC* inputLayout, int inputLayoutSize)
	{
		std::string pathStr{ path };
		std::wstring wPath{ pathStr.begin(), pathStr.end()};
		//ID3DBlob* blobOut = nullptr;
		blobOut = nullptr;
		ID3DBlob* blobErr = nullptr;
		HRESULT hr = S_OK;
		hr = D3DCompileFromFile(wPath.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0",
			D3DCOMPILE_ENABLE_STRICTNESS, 0, &blobOut, &blobErr);
		if (FAILED(hr))
		{
			if (blobErr != nullptr)
			{
				OutputDebugStringA((char*)blobErr->GetBufferPointer());
				blobErr->Release();
			}
			return false;
		}

		auto device = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDevice();
		hr = device->CreateVertexShader(blobOut->GetBufferPointer(),
			blobOut->GetBufferSize(), 0, &m_vertexShader);
		if (FAILED(hr))
		{
			blobOut->Release();
			return false;
		}

		if (!CreateInputLayout(inputLayout, inputLayoutSize, blobOut))
			return false;

		return true;
	}

	void DX11VertexShader::Bind()
	{	
		m_inputLayout.Bind();
		auto deviceContext = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDeviceContext();
		deviceContext->VSSetShader(m_vertexShader, 0, 0);
	}

	void DX11VertexShader::Unbind()
	{
		m_inputLayout.Unbind();
		auto deviceContext = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDeviceContext();
		deviceContext->VSSetShader(nullptr, 0, 0);
	}
	ID3D11VertexShader* DX11VertexShader::GetD3DShader()
	{
		return m_vertexShader;
	}
	bool DX11VertexShader::CreateInputLayout(D3D11_INPUT_ELEMENT_DESC layout[], UINT numElements, ID3DBlob* vsBlob)
	{
		m_inputLayout = DX11InputLayout();
		if (!m_inputLayout.Create(layout, numElements, vsBlob))
		{
			std::cout << "ERROR::Can't create D3D11InputLayout!\n";
			return false;
		}		
		return true;
	}
}


