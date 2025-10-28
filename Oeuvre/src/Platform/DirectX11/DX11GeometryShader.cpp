#include "ovpch.h"
#include "DX11GeometryShader.h"

#include "Platform/DirectX11/DX11RendererAPI.h"

namespace Oeuvre
{
	DX11GeometryShader::DX11GeometryShader(const char* filePath)
	{
		if (!Create(filePath))
			std::cout << "ERROR::Can't create DX11GeometryShader!\n";
	}

	DX11GeometryShader::~DX11GeometryShader()
	{
		if (m_blobOut) m_blobOut->Release();
		if (m_GeometryShader) m_GeometryShader->Release();
	}

	bool DX11GeometryShader::Create(const char* path)
	{
		std::string pathStr{ path };
		std::wstring wPath{ pathStr.begin(), pathStr.end() };
		//ID3DBlob* blobOut = nullptr;
		m_blobOut = nullptr;
		ID3DBlob* blobErr = nullptr;
		HRESULT hr = S_OK;
		hr = D3DCompileFromFile(wPath.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GSMain", "gs_5_0",
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
		hr = device->CreateGeometryShader(m_blobOut->GetBufferPointer(),
			m_blobOut->GetBufferSize(), 0, &m_GeometryShader);
		if (FAILED(hr))
		{
			m_blobOut->Release();
			return false;
		}

		return true;
	}

	void DX11GeometryShader::Bind()
	{
		auto deviceContext = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDeviceContext();
		deviceContext->GSSetShader(m_GeometryShader, 0, 0);
	}

	void DX11GeometryShader::Unbind()
	{
		auto deviceContext = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDeviceContext();
		deviceContext->GSSetShader(nullptr, 0, 0);
	}
}

