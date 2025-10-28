#include "ovpch.h"
#include "DX11ComputeShader.h"

#include "Platform/DirectX11/DX11RendererAPI.h"

namespace Oeuvre
{
	DX11ComputeShader::DX11ComputeShader(const char* filePath, const char* mainFunctionName)
	{
		if (!Create(filePath, mainFunctionName))
			std::cout << "ERROR::Can't create DX11ComputeShader from path " << filePath << "!\n";
	}

	DX11ComputeShader::~DX11ComputeShader()
	{
		if (m_blobOut) m_blobOut->Release();
		if (m_ComputeShader) m_ComputeShader->Release();
	}

	bool DX11ComputeShader::Create(const char* path, const char* mainFunctionName)
	{
		std::string pathStr{ path };
		std::wstring wPath{ pathStr.begin(), pathStr.end() };
		//ID3DBlob* blobOut = nullptr;
		m_blobOut = nullptr;
		ID3DBlob* blobErr = nullptr;
		HRESULT hr = S_OK;
		hr = D3DCompileFromFile(wPath.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, mainFunctionName, "cs_5_0",
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
		hr = device->CreateComputeShader(m_blobOut->GetBufferPointer(),
			m_blobOut->GetBufferSize(), 0, &m_ComputeShader);
		if (FAILED(hr))
		{
			m_blobOut->Release();
			return false;
		}

		return true;
	}

	void DX11ComputeShader::Bind()
	{
		auto deviceContext = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDeviceContext();
		deviceContext->CSSetShader(m_ComputeShader, 0, 0);
	}

	void DX11ComputeShader::Unbind()
	{
		auto deviceContext = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDeviceContext();
		deviceContext->CSSetShader(nullptr, 0, 0);
	}
}

