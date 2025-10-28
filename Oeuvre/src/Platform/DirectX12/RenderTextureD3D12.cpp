#include "ovpch.h"

#include "RenderTextureD3D12.h"
#include <d3dx12.h>
#include "Oeuvre/Utils/HelperMacros.h"
#include "GbufferD3D12.h"
#include "Platform/DirectX12/DX12RendererAPI.h"

namespace Oeuvre
{
	RenderTextureD3D12::RenderTextureD3D12(DXGI_FORMAT format, bool depthOnly, bool cubeTexture, bool array, uint32_t arraySize) noexcept :
		m_state(D3D12_RESOURCE_STATE_COMMON),
		m_srvDescriptorCPU{},
		m_srvDescriptorGPU{},
		m_rtvDescriptor{},
		m_dsvDescriptor{},
		m_clearColor{},
		m_format(format),
		m_width(0),
		m_height(0),
		m_depthOnly(depthOnly),
		m_cubeTexture(cubeTexture),
		m_array(array),
		m_arraySize(arraySize)
	{
	}

	void RenderTextureD3D12::SetDevice(_In_ ID3D12Device* device)
	{
		if (device == m_device.Get())
			return;

		if (m_device)
		{
			ReleaseDevice();
		}

		{
			D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = { m_format, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE };
			if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport))))
			{
				throw std::runtime_error("CheckFeatureSupport");
			}

			UINT required = D3D12_FORMAT_SUPPORT1_TEXTURE2D | D3D12_FORMAT_SUPPORT1_RENDER_TARGET;
			if ((formatSupport.Support1 & required) != required)
			{
#ifdef _DEBUG
				char buff[128] = {};
				sprintf_s(buff, "RenderTextureD3D12: Device does not support the requested format (%u)!\n", m_format);
				OutputDebugStringA(buff);
#endif
				throw std::runtime_error("RenderTextureD3D12");
			}
		}

		if (!m_depthOnly)
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
			rtvHeapDesc.NumDescriptors = 1;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			rtvHeapDesc.NodeMask = 0;
			ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

			m_rtvDescriptor = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		}

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.NodeMask = 0;
		ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

		m_dsvDescriptor = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

		auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
		dx12Api->g_pd3dSrvDescHeapAlloc.Alloc(&m_srvDescriptorCPU, &m_srvDescriptorGPU);

		m_device = device;
	}

	void RenderTextureD3D12::SizeResources(size_t width, size_t height)
	{
		if (width == m_width && height == m_height || width <= 0 || height <= 0)
			return;

		if (m_width > UINT32_MAX || m_height > UINT32_MAX)
		{
			throw std::out_of_range("Invalid width/height");
		}

		if (!m_device)
			return;

		m_width = m_height = 0;

		auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
		dx12Api->FlushCommandQueue();

		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		uint32_t arraySize = m_cubeTexture ? 6 : m_array ? m_arraySize : 1;

		if (!m_depthOnly)
		{
			D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(m_format,
				static_cast<UINT64>(width),
				static_cast<UINT>(height),
				arraySize, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

			D3D12_CLEAR_VALUE clearValue = { m_format, {} };
			memcpy(clearValue.Color, m_clearColor, sizeof(clearValue.Color));

			m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;

			// Create a render target
			ThrowIfFailed(
				m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
					&desc,
					m_state, &clearValue,
					IID_PPV_ARGS(m_resource.ReleaseAndGetAddressOf()))
			);

			// Create RTV.
			m_device->CreateRenderTargetView(m_resource.Get(), nullptr, m_rtvDescriptor);
		}
		else
		{
			// Depth stencil
			D3D12_RESOURCE_DESC resourceDesc{};
			ZeroMemory(&resourceDesc, sizeof(resourceDesc));
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			resourceDesc.Alignment = 0;
			resourceDesc.Width = width;
			resourceDesc.Height = height;
			resourceDesc.DepthOrArraySize = arraySize;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.SampleDesc.Quality = 0;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			D3D12_CLEAR_VALUE optClear;
			optClear.Format = resourceDesc.Format;
			optClear.DepthStencil.Depth = 1.f;
			optClear.DepthStencil.Stencil = 0;

			ThrowIfFailed(m_device->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_COMMON,
				&optClear,
				IID_PPV_ARGS(&m_resource)));

			// Create DSV
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
			ZeroMemory(&dsvDesc, sizeof(dsvDesc));
			dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = m_array && m_arraySize > 1 || m_cubeTexture ? D3D12_DSV_DIMENSION_TEXTURE2DARRAY : D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2DArray.ArraySize = arraySize;
			dsvDesc.Texture2DArray.FirstArraySlice = 0;
			dsvDesc.Texture2DArray.MipSlice = 0;

			m_device->CreateDepthStencilView(m_resource.Get(), &dsvDesc, m_dsvDescriptor);
		}

		// Create SRV
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = m_depthOnly ? DXGI_FORMAT_R32_FLOAT : m_format;
		if (m_cubeTexture)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MipLevels = 1;
			srvDesc.TextureCube.MostDetailedMip = 0;
		}
		else if (m_array && m_arraySize > 1)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.ArraySize = arraySize;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
		}
		else
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;
		}
		m_device->CreateShaderResourceView(m_resource.Get(), &srvDesc, m_srvDescriptorCPU);

		m_width = width;
		m_height = height;
	}

	void RenderTextureD3D12::ReleaseDevice() noexcept
	{
		m_resource.Reset();
		m_device.Reset();

		m_state = D3D12_RESOURCE_STATE_COMMON;
		m_width = m_height = 0;

		m_srvDescriptorCPU.ptr = m_srvDescriptorGPU.ptr = m_rtvDescriptor.ptr = m_dsvDescriptor.ptr = 0;
	}

	void RenderTextureD3D12::TransitionTo(_In_ ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES afterState)
	{
		if (m_state == afterState)
			return;

		auto transition = CD3DX12_RESOURCE_BARRIER::Transition(m_resource.Get(),
			m_state, afterState);
		commandList->ResourceBarrier(1, &transition);
		m_state = afterState;
	}

	void RenderTextureD3D12::SetWindow(const RECT& output)
	{
		// Determine the render target size in pixels.
		auto width = size_t(std::max<LONG>(output.right - output.left, 1));
		auto height = size_t(std::max<LONG>(output.bottom - output.top, 1));

		SizeResources(width, height);
	}

	void RenderTextureD3D12::BeginScene(_In_ ID3D12GraphicsCommandList* commandList)
	{
		if (!m_depthOnly)
		{
			TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

			commandList->OMSetRenderTargets(1, &m_rtvDescriptor, TRUE, nullptr);
			commandList->ClearRenderTargetView(m_rtvDescriptor, m_clearColor, 0, nullptr);

			// Set the viewport and scissor rect.
			D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)m_width, (float)m_height , D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
			D3D12_RECT scissorRect = { 0, 0, m_width, m_height };
			commandList->RSSetViewports(1, &viewport);
			commandList->RSSetScissorRects(1, &scissorRect);
		}
		else
		{
			TransitionTo(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

			commandList->OMSetRenderTargets(0, nullptr, FALSE, &m_dsvDescriptor);
			commandList->ClearDepthStencilView(m_dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
		}
	}

	void RenderTextureD3D12::EndScene(_In_ ID3D12GraphicsCommandList* commandList)
	{
		TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
}

