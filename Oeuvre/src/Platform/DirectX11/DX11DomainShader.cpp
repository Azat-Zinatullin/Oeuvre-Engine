#include "ovpch.h"
#include "DX11DomainShader.h"


#include "Platform/DirectX11/DX11RendererAPI.h"

namespace Oeuvre
{
	DX11DomainShader::DX11DomainShader(const char* filePath)
	{
		if (!Create(filePath))
			std::cout << "ERROR::Can't create DX11DomainShader!\n";
	}

	DX11DomainShader::~DX11DomainShader()
	{
		if (m_blobOut) m_blobOut->Release();
		if (m_DomainShader) m_DomainShader->Release();
	}

	bool DX11DomainShader::Create(const char* path)
	{
		std::string pathStr{ path };
		std::wstring wPath{ pathStr.begin(), pathStr.end() };
		//ID3DBlob* blobOut = nullptr;
		m_blobOut = nullptr;
		ID3DBlob* blobErr = nullptr;
		HRESULT hr = S_OK;
		hr = D3DCompileFromFile(wPath.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "DSMain", "ds_5_0",
			D3DCOMPILE_ENABLE_STRICTNESS, 0, &m_blobOut, &blobErr);
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
		hr = device->CreateDomainShader(m_blobOut->GetBufferPointer(),
			m_blobOut->GetBufferSize(), 0, &m_DomainShader);
		if (FAILED(hr))
		{
			m_blobOut->Release();
			return false;
		}

		return true;
	}

	void DX11DomainShader::Bind()
	{
		auto deviceContext = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDeviceContext();
		deviceContext->DSSetShader(m_DomainShader, 0, 0);
	}

	void DX11DomainShader::Unbind()
	{
		auto deviceContext = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDeviceContext();
		deviceContext->DSSetShader(nullptr, 0, 0);
	}
}

