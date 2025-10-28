#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

using namespace  Microsoft::WRL;

namespace Oeuvre
{
    class RenderTextureD3D12
    {
    public:
        RenderTextureD3D12(DXGI_FORMAT format, bool depthOnly, bool cubeTexture = false, bool array = false, uint32_t arraySize = 1) noexcept;

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

        ID3D12Resource* GetResource() const noexcept { return m_resource.Get(); }
        D3D12_RESOURCE_STATES GetCurrentState() const noexcept { return m_state; }

        void SetWindow(const RECT& rect);

        DXGI_FORMAT GetFormat() const noexcept { return m_format; }
        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptor() { return m_srvDescriptorGPU; }

    private:
        ComPtr<ID3D12Device>                                m_device;
        ComPtr<ID3D12Resource>                              m_resource;
        D3D12_RESOURCE_STATES                               m_state;
        D3D12_CPU_DESCRIPTOR_HANDLE                         m_srvDescriptorCPU;
        D3D12_GPU_DESCRIPTOR_HANDLE                         m_srvDescriptorGPU;
        D3D12_CPU_DESCRIPTOR_HANDLE                         m_rtvDescriptor;
        D3D12_CPU_DESCRIPTOR_HANDLE                         m_dsvDescriptor;
        float                                               m_clearColor[4];

        ComPtr<ID3D12DescriptorHeap> m_rtvHeap = nullptr;
        ComPtr<ID3D12DescriptorHeap> m_dsvHeap = nullptr;

        bool m_depthOnly = false;
        bool m_cubeTexture = false;
        bool m_array = false;
        uint32_t m_arraySize = 1;
        

        DXGI_FORMAT                                         m_format;

        size_t                                              m_width;
        size_t                                              m_height;
    };
}