#pragma once

#include "Oeuvre/Renderer/RendererAPI.h"
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

using namespace Microsoft::WRL;

#include "UploadBuffer.h"
#include <cassert>
#include "ExampleDescriptorHeapAllocator.h"
#include "RenderTextureD3D12.h"
#include "DirectXMath.h"

using namespace DirectX;

namespace Oeuvre
{
	class DX12RendererAPI : public RendererAPI
	{
	public:
		DX12RendererAPI();
		virtual ~DX12RendererAPI() = default;

		void Init() override;
		void Shutdown() override;
		void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		void SetClearColor(const glm::vec4& color) override;
		void Clear() override;
		void OnWindowResize(int width, int height) override;
		void OnMouseMove(int x, int y);

		void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount) override;

		void BeginFrame() override;
		void EndFrame(bool vSyncEnabled) override;

		void ResetRenderTarget();
		void FlushCommandQueue();
		void CloseAndFlushCommandlist();
		void ResetCommandlist();

		ComPtr<ID3D12Device14> GetDevice() { return _d3dDevice; }
		ComPtr<ID3D12CommandQueue> GetCommandQueue() { return _CommandQueue; }
		UINT32 GetSwapchainBufferCount() { return SwapChainBufferCount; }
		ID3D12DescriptorHeap* GetSRVDescriptorHeap() { return _cbvHeap.Get(); }
		ComPtr<ID3D12GraphicsCommandList10> GetCommandList() { return _CommandList; }

		static ExampleDescriptorHeapAllocator g_pd3dSrvDescHeapAlloc;

		int _ImageWidth = 1600;
		int _ImageHeight = 900;
		bool _ImageSizeChanged = true;

		ComPtr<ID3D12Fence1> _Fence = nullptr;
		UINT _CurrentFence = 0;

	private:
		void CreateRtvAndDsvDescriptorHeaps();
		void BuildCbvDescriptorHeaps();

		void ResetSizeDependentResources();
		void LoadSizeDependentResources();
		void Resize();

		ID3D12Resource* GetCurrentBackBuffer();
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView();
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView();

		void SetShouldResize(bool value) { _ShouldResize = value; }
		bool GetWindowVisible() { return _WindowVisible; }

		ComPtr<ID3D12DescriptorHeap> GetCBVHeap() { return _cbvHeap; }

	private:
		ComPtr<ID3D12Device14> _d3dDevice = nullptr;
		ComPtr<IDXGIFactory7> _dxgiFactory = nullptr;

		UINT _rtvDescriptorSize = 0;
		UINT _dsvDescriptorSize = 0;
		UINT _cbvSrvDescriptorSize = 0;

		ComPtr<ID3D12CommandQueue> _CommandQueue = nullptr;
		ComPtr<ID3D12CommandAllocator> _CommandAllocator = nullptr;
		ComPtr<ID3D12GraphicsCommandList10> _CommandList = nullptr;

		const DXGI_FORMAT _BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		const DXGI_FORMAT _DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		int _CurrentBackBuffer = 0;
		ComPtr<IDXGISwapChain> _SwapChain = nullptr;

		ComPtr<ID3D12DescriptorHeap> _rtvHeap = nullptr;
		ComPtr<ID3D12DescriptorHeap> _dsvHeap = nullptr;
		ComPtr<ID3D12DescriptorHeap> _cbvHeap = nullptr;

		D3D12_CPU_DESCRIPTOR_HANDLE _cbvCPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE _cbvGPUHandle;

		static const UINT32 SwapChainBufferCount = 2;
		ComPtr<ID3D12Resource> _SwapChainBuffer[SwapChainBufferCount];
		ComPtr<ID3D12Resource> _DepthStencilBuffer;

		std::unique_ptr<Texture2D> _SomeTexture;

		ComPtr<ID3D12RootSignature> _RootSignature = nullptr;

		ComPtr<ID3DBlob> _vsByteCode = nullptr;
		ComPtr<ID3DBlob> _psByteCode = nullptr;

		std::vector<D3D12_INPUT_ELEMENT_DESC> _InputLayout;

		ComPtr<ID3D12PipelineState> _PSO = nullptr;

		D3D12_VIEWPORT _ScreenViewport{};
		RECT _ScissorRect{};

		HWND _hWnd;
		UINT _ClientWidth = 800;
		UINT _ClientHeight = 600;

		glm::mat4 _World = glm::mat4(1.f);
		glm::mat4 _View = glm::mat4(1.f);
		glm::mat4 _Proj = glm::mat4(1.f);

		float _Theta = 1.5f * XM_PI;
		float _Phi = XM_PIDIV4;
		float _Radius = 5.0f;

		POINT _LastMousePos;

		bool _ShouldResize = false;
		bool _WindowedMode = true;
		bool _WindowVisible = true;

		bool _Fpressed = false;
	};
}



