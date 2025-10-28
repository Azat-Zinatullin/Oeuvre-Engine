#include "ovpch.h"
#include "GbufferD3D12.h"
#include <d3dx12.h>
#include "Oeuvre/Utils/HelperMacros.h"
#include "Platform/DirectX12/DX12RendererAPI.h"


namespace Oeuvre
{
   GBufferD3D12::GBufferD3D12(DXGI_FORMAT formats[BUFFER_COUNT]) noexcept :
        m_state(D3D12_RESOURCE_STATE_COMMON),
        m_srvCPUDescriptors{},
        m_srvGPUDescriptors{},
        m_rtvDescriptors{},
        m_clearColor{},
        m_width(0),
        m_height(0)
    {
        for (int i = 0; i < BUFFER_COUNT; i++)
            m_formats[i] = formats[i];
    }

    void GBufferD3D12::SetDevice(_In_ ID3D12Device* device)
    {
        if (device == m_device.Get())
            return;

        if (m_device)
        {
            ReleaseDevice();
        }

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.NumDescriptors = BUFFER_COUNT;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtvHeapDesc.NodeMask = 0;
        ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        //D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc;
        //srvHeapDesc.NumDescriptors = BUFFER_COUNT;
        //srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        //srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        //srvHeapDesc.NodeMask = 0;
        //ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));

        //m_srvHeap->SetName(L"Gbuffer SRV Heap");

        //m_srvHeapAllocator.Create(device, m_srvHeap.Get());

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        dsvHeapDesc.NodeMask = 0;
        ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));


        auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();

        m_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        for (int i = 0; i < BUFFER_COUNT; i++)
        {
            //m_srvHeapAllocator.Alloc(&m_srvCPUDescriptors[i], &m_srvGPUDescriptors[i]);
            dx12Api->g_pd3dSrvDescHeapAlloc.Alloc(&m_srvCPUDescriptors[i], &m_srvGPUDescriptors[i]);

            if (i == 0)
            {
                m_rtvDescriptors[i] = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
            }
            else
            {
                m_rtvDescriptors[i].ptr = m_rtvDescriptors[i - 1].ptr + m_rtvDescriptorSize;
            }
        }

        m_device = device;
    }

    void GBufferD3D12::SizeResources(size_t width, size_t height)
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

        for (int i = 0; i < BUFFER_COUNT; i++)
        {
            D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(m_formats[i],
                static_cast<UINT64>(width),
                static_cast<UINT>(height),
                1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

            D3D12_CLEAR_VALUE clearValue = { m_formats[i], {}};
            memcpy(clearValue.Color, m_clearColor, sizeof(clearValue.Color));

            m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;

            // Create a render target
            ThrowIfFailed(
                m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
                    &desc,
                    m_state, &clearValue,
                    IID_PPV_ARGS(m_resources[i].ReleaseAndGetAddressOf()))
            );

            // Create RTV.
            m_device->CreateRenderTargetView(m_resources[i].Get(), nullptr, m_rtvDescriptors[i]);

            // Create SRV.
            m_device->CreateShaderResourceView(m_resources[i].Get(), nullptr, m_srvCPUDescriptors[i]);
        }   

        // Create DSV
        D3D12_RESOURCE_DESC depthStencilDesc{};
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthStencilDesc.Alignment = 0;
        depthStencilDesc.Width = width;
        depthStencilDesc.Height = height;
        depthStencilDesc.DepthOrArraySize = 1;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE optClear;
        optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        optClear.DepthStencil.Depth = 1.f;
        optClear.DepthStencil.Stencil = 0;

        ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_COMMON,
            &optClear,
            IID_PPV_ARGS(m_depthStencilBuffer.ReleaseAndGetAddressOf())));

        m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), nullptr, GetDepthStencilView());

        //auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
        auto& commandList = dx12Api->GetCommandList();
        // Transition the resource from its initial state to be used as a depth buffer.
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(m_depthStencilBuffer.Get(),
            D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        commandList->ResourceBarrier(1, &transition);

        m_width = width;
        m_height = height;
    }

    void GBufferD3D12::ReleaseDevice() noexcept
    {
        for (int i = 0; i < BUFFER_COUNT; i++)
        {
            m_resources[i].Reset();
            m_srvCPUDescriptors[i].ptr = m_srvGPUDescriptors[i].ptr = m_rtvDescriptors[i].ptr = 0;
        }

        m_device.Reset();

        m_state = D3D12_RESOURCE_STATE_COMMON;
        m_width = m_height = 0;
    }

    void GBufferD3D12::TransitionTo(_In_ ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES afterState)
    {
        if (m_state == afterState)
            return;

        for (int i = 0; i < BUFFER_COUNT; i++)
        {
            auto transition = CD3DX12_RESOURCE_BARRIER::Transition(m_resources[i].Get(),
                m_state, afterState);
            commandList->ResourceBarrier(1, &transition);
        }      
        m_state = afterState;
    }

    void GBufferD3D12::SetWindow(const RECT& output)
    {
        // Determine the render target size in pixels.
        auto width = size_t(std::max<LONG>(output.right - output.left, 1));
        auto height = size_t(std::max<LONG>(output.bottom - output.top, 1));

        SizeResources(width, height);
    }

    void GBufferD3D12::BeginScene(_In_ ID3D12GraphicsCommandList* commandList)
    {
        TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        commandList->OMSetRenderTargets(BUFFER_COUNT, m_rtvDescriptors, FALSE, &GetDepthStencilView());
        for (int i = 0; i < BUFFER_COUNT; i++)
            commandList->ClearRenderTargetView(m_rtvDescriptors[i], m_clearColor, 0, nullptr);
        commandList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        // Set the viewport and scissor rect.
        D3D12_VIEWPORT viewports[BUFFER_COUNT] =
        {
             {0.0f, 0.0f, (float)m_width, (float)m_height , D3D12_MIN_DEPTH, D3D12_MAX_DEPTH},
             {0.0f, 0.0f, (float)m_width, (float)m_height , D3D12_MIN_DEPTH, D3D12_MAX_DEPTH},
             {0.0f, 0.0f, (float)m_width, (float)m_height , D3D12_MIN_DEPTH, D3D12_MAX_DEPTH},
             {0.0f, 0.0f, (float)m_width, (float)m_height , D3D12_MIN_DEPTH, D3D12_MAX_DEPTH}
        };
        D3D12_RECT scissorRects[BUFFER_COUNT] =
        {
            {0, 0, m_width, m_height},
            {0, 0, m_width, m_height},
            {0, 0, m_width, m_height},
            {0, 0, m_width, m_height}
        };
        commandList->RSSetViewports(BUFFER_COUNT, viewports);
        commandList->RSSetScissorRects(BUFFER_COUNT, scissorRects);
    }

    void GBufferD3D12::EndScene(_In_ ID3D12GraphicsCommandList* commandList)
    {
        TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

}