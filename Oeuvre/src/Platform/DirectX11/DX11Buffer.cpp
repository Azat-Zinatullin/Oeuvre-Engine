#include "ovpch.h"
#include "DX11Buffer.h"

#include "Platform/DirectX11/DX11RendererAPI.h"

namespace Oeuvre
{
	// VertexBuffer ////////////////////////////////////////////////////////////////////
	DX11VertexBuffer::DX11VertexBuffer(float* vertices, uint32_t size, uint32_t stride)
		: m_Stride(stride)
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = size;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.pSysMem = vertices;

		auto device = static_cast<DX11RendererAPI*>(DX11RendererAPI::GetInstance().get())->GetDevice();
		HRESULT hr =  device->CreateBuffer(&bd, &sd, &m_VertexBuffer);
		if (FAILED(hr))
		{
			std::cout << "Failed to create vertex buffer!\n";
			return;
		}
	}

	DX11VertexBuffer::~DX11VertexBuffer()
	{
		if (m_VertexBuffer) m_VertexBuffer->Release();
	}

	void DX11VertexBuffer::Bind()
	{
		UINT stride = m_Stride;
		UINT offset = 0;
		auto deviceContext = static_cast<DX11RendererAPI*>(DX11RendererAPI::GetInstance().get())->GetDeviceContext();
		deviceContext->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
	}

	void DX11VertexBuffer::Unbind()
	{
		auto deviceContext = static_cast<DX11RendererAPI*>(DX11RendererAPI::GetInstance().get())->GetDeviceContext();
		deviceContext->IASetVertexBuffers(0, 0, nullptr, 0, 0);
	}


	// IndexBuffer ////////////////////////////////////////////////////
	DX11IndexBuffer::DX11IndexBuffer(unsigned int* indices, uint32_t count)
		: m_Count(count)
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = count * sizeof(unsigned int);
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.pSysMem = indices;

		auto device = static_cast<DX11RendererAPI*>(DX11RendererAPI::GetInstance().get())->GetDevice();
		HRESULT hr = device->CreateBuffer(&bd, &sd, &m_IndexBuffer);
		if (FAILED(hr))
		{
			std::cout << "Failed to create index buffer!\n";
			return;
		}
	}

	DX11IndexBuffer::~DX11IndexBuffer()
	{
		if (m_IndexBuffer) m_IndexBuffer->Release();
	}
	void DX11IndexBuffer::Bind()
	{
		auto deviceContext = static_cast<DX11RendererAPI*>(DX11RendererAPI::GetInstance().get())->GetDeviceContext();
		deviceContext->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	}

	void DX11IndexBuffer::Unbind()
	{
		auto deviceContext = static_cast<DX11RendererAPI*>(DX11RendererAPI::GetInstance().get())->GetDeviceContext();
		deviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	}
}


