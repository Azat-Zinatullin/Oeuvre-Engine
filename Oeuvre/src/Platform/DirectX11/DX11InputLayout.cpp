#include "ovpch.h"
#include "DX11InputLayout.h"
#include "Platform/DirectX11/DX11RendererAPI.h"

namespace Oeuvre
{
	DX11InputLayout::DX11InputLayout()
	{
	}

	DX11InputLayout::~DX11InputLayout()
	{
		if (m_inputLayout) m_inputLayout->Release();
	}

	bool DX11InputLayout::Create(D3D11_INPUT_ELEMENT_DESC layout[], UINT numElements, ID3DBlob* vsBlob)
	{
		auto device = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDevice();

		HRESULT hr;
		hr = device->CreateInputLayout(layout, numElements, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
		if (FAILED(hr)) return false;
		return true;
	}

	void DX11InputLayout::Bind()
	{
		auto deviceContext = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDeviceContext();
		deviceContext->IASetInputLayout(m_inputLayout);
	}

	void DX11InputLayout::Unbind()
	{
		auto deviceContext = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDeviceContext();
		deviceContext->IASetInputLayout(nullptr);
	}
}


