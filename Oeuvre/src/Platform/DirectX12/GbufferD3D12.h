#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Platform/DirectX12/ExampleDescriptorHeapAllocator.h"

using namespace  Microsoft::WRL;

namespace Oeuvre
{
	class GBufferD3D12
	{
    public:
        static const int BUFFER_COUNT = 4;

        GBufferD3D12(DXGI_FORMAT formats[BUFFER_COUNT]) noexcept;

        void SetDevice(_In_ ID3D12Device* device);

        void SizeResources(size_t width, size_t height);

        void ReleaseDevice() noexcept;

        void TransitionTo(_In_ ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES afterState);

        void BeginScene(_In_ ID3D12GraphicsCommandList* commandList);

        void EndScene(_In_ ID3D12GraphicsCommandList* commandList);

        void SetClearColor(float r, float g, float b, float a)
        {
            m_clearColor[0] = r;
            m_clearColor[1] = g;
            m_clearColor[2] = b;
            m_clearColor[3] = a;
        }

        ID3D12Resource* GetResource(int i) const noexcept { return m_resources[i].Get(); }
        D3D12_RESOURCE_STATES GetCurrentState() const noexcept { return m_state; }

        void SetWindow(const RECT& rect);

        DXGI_FORMAT GetFormat(int i) const noexcept { return m_formats[i]; }
        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptor(int i) { return m_srvGPUDescriptors[i]; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() { return m_dsvHeap->GetCPUDescriptorHandleForHeapStart(); }

        ComPtr<ID3D12Resource> GetResource(int i) { return m_resources[i]; }

    private:
        ComPtr<ID3D12Device>                                m_device;
        ComPtr<ID3D12Resource>                              m_resources[BUFFER_COUNT];
        D3D12_RESOURCE_STATES                               m_state;
        D3D12_CPU_DESCRIPTOR_HANDLE                         m_srvCPUDescriptors[BUFFER_COUNT];
        D3D12_GPU_DESCRIPTOR_HANDLE                         m_srvGPUDescriptors[BUFFER_COUNT];

        ComPtr<ID3D12DescriptorHeap> m_rtvHeap = nullptr;
        ComPtr<ID3D12DescriptorHeap> m_srvHeap = nullptr;
        ComPtr<ID3D12DescriptorHeap> m_dsvHeap = nullptr;
        ExampleDescriptorHeapAllocator m_srvHeapAllocator;

        ComPtr<ID3D12Resource> m_depthStencilBuffer;

        UINT m_rtvDescriptorSize = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE                         m_rtvDescriptors[BUFFER_COUNT];
        float                                               m_clearColor[4];

        DXGI_FORMAT                                         m_formats[BUFFER_COUNT];

        size_t                                              m_width;
        size_t                                              m_height;
	};
}