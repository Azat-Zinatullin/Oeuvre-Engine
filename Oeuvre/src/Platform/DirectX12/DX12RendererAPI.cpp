#include "ovpch.h"
#include "DX12RendererAPI.h"
#include "Oeuvre/Utils/HelperMacros.h"
#include <glm/gtc/matrix_transform.hpp>
#include "GLFW/glfw3.h"
#include "../Windows/WindowsWindow.h"
#include <DirectXTex.h>
#include <comdef.h>

namespace Oeuvre
{
	ExampleDescriptorHeapAllocator DX12RendererAPI::g_pd3dSrvDescHeapAlloc = {};

	DX12RendererAPI::DX12RendererAPI()
	{
		Init();
	}

	void DX12RendererAPI::Init()
	{
#ifdef _DEBUG
		{
			ComPtr<ID3D12Debug6> debugController;
			ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			debugController->EnableDebugLayer();
		}
#endif // _DEBUG

		ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory)));

		HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_d3dDevice));
		if (FAILED(hr))
		{
			ComPtr<IDXGIAdapter4> pWarpAdapter;
			ThrowIfFailed(_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

			ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_d3dDevice)));
		}

		ThrowIfFailed(_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_Fence)));

		_rtvDescriptorSize = _d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_dsvDescriptorSize = _d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		_cbvSrvDescriptorSize = _d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_COMMAND_QUEUE_DESC queueDesc{};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		ThrowIfFailed(_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_CommandQueue)));

		ThrowIfFailed(_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_CommandAllocator)));

		ThrowIfFailed(_d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&_CommandList)));

		_hWnd = WindowsWindow::GetInstance()->GetHWND();

		RECT clientRect;
		GetClientRect(_hWnd, &clientRect);

		_ClientWidth = clientRect.right - clientRect.left;
		_ClientHeight = clientRect.bottom - clientRect.top;

		DXGI_SWAP_CHAIN_DESC sd{};
		sd.BufferDesc.Width = _ClientWidth;
		sd.BufferDesc.Height = _ClientHeight;
		sd.BufferDesc.RefreshRate.Numerator = 144;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.Format = _BackBufferFormat;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = SwapChainBufferCount;
		sd.OutputWindow = _hWnd;
		sd.Windowed = true;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		ThrowIfFailed(_dxgiFactory->CreateSwapChain(_CommandQueue.Get(), &sd, &_SwapChain));

		CreateRtvAndDsvDescriptorHeaps();

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < SwapChainBufferCount; i++)
		{
			ThrowIfFailed(_SwapChain->GetBuffer(i, IID_PPV_ARGS(&_SwapChainBuffer[i])));

			_d3dDevice->CreateRenderTargetView(_SwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);

			rtvHeapHandle.Offset(1, _rtvDescriptorSize);
		}

		D3D12_RESOURCE_DESC depthStencilDesc{};
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = _ClientWidth;
		depthStencilDesc.Height = _ClientHeight;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = _DepthStencilFormat;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear;
		optClear.Format = _DepthStencilFormat;
		optClear.DepthStencil.Depth = 1.f;
		optClear.DepthStencil.Stencil = 0;

		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(_d3dDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optClear,
			IID_PPV_ARGS(&_DepthStencilBuffer)));

		_d3dDevice->CreateDepthStencilView(_DepthStencilBuffer.Get(), nullptr, GetDepthStencilView());

		{
			// Transition the resource from its initial state to be used as a depth buffer.
			auto transition = CD3DX12_RESOURCE_BARRIER::Transition(_DepthStencilBuffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			_CommandList->ResourceBarrier(1, &transition);

			// Execute the resize commands.
			ThrowIfFailed(_CommandList->Close());
			ID3D12CommandList* cmdsLists[] = { _CommandList.Get() };
			_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

			// Wait until resize is complete.
			FlushCommandQueue();
		}

		_ScreenViewport.TopLeftX = 0.f;
		_ScreenViewport.TopLeftY = 0.f;
		_ScreenViewport.Width = _ClientWidth;
		_ScreenViewport.Height = _ClientHeight;
		_ScreenViewport.MinDepth = 0.f;
		_ScreenViewport.MaxDepth = 1.f;

		_ScissorRect.left = 0;
		_ScissorRect.top = 0;
		_ScissorRect.right = _ClientWidth;
		_ScissorRect.bottom = _ClientHeight;


		BuildCbvDescriptorHeaps();

		// Reuse the memory associated with command recording.
		// We can only reset when the associated command lists have finished execution on the GPU.
		ThrowIfFailed(_CommandAllocator->Reset());

		// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
		// Reusing the command list reuses memory.
		ThrowIfFailed(_CommandList->Reset(_CommandAllocator.Get(), nullptr));
	}

	void DX12RendererAPI::Shutdown()
	{
	}

	void DX12RendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
	}

	void DX12RendererAPI::SetClearColor(const glm::vec4& color)
	{
	}

	void DX12RendererAPI::Clear()
	{
	}

	void DX12RendererAPI::OnWindowResize(int width, int height)
	{
		_ShouldResize = true;
	}

	void DX12RendererAPI::BeginFrame()
	{
		if (_ShouldResize)
		{
			FlushCommandQueue();
			Resize();
		}

		if (!_WindowVisible)
			return;

		// Reuse the memory associated with command recording.
		// We can only reset when the associated command lists have finished execution on the GPU.
		//ThrowIfFailed(_CommandAllocator->Reset());

		// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
		// Reusing the command list reuses memory.
		//ThrowIfFailed(_CommandList->Reset(_CommandAllocator.Get(), nullptr));

		// Indicate a state transition on the resource usage.
		auto transition = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		_CommandList->ResourceBarrier(1, &transition);

		D3D12_VIEWPORT viewport;
		RECT scissor;

		viewport.TopLeftX = viewport.TopLeftY = 0.f;
		viewport.Width = _ClientWidth;
		viewport.Height = _ClientHeight;
		viewport.MinDepth = 1.f;
		viewport.MaxDepth = 0.f;

		scissor.left = scissor.top = 0;
		scissor.right = _ClientWidth;
		scissor.bottom = _ClientHeight;

		// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
		_CommandList->RSSetViewports(1, &viewport);
		_CommandList->RSSetScissorRects(1, &scissor);

		// Clear the back buffer and depth buffer.
		float clearColor[]{ 0.f, 1.f, 1.f, 1.f };
		_CommandList->ClearRenderTargetView(GetCurrentBackBufferView(), clearColor, 0, nullptr);
		_CommandList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		// Specify the buffers we are going to render to.
		auto depthStencilView = GetDepthStencilView();
		auto currBackBufferView = GetCurrentBackBufferView();
		_CommandList->OMSetRenderTargets(1, &currBackBufferView, true, &depthStencilView);
	}

	void DX12RendererAPI::EndFrame(bool vSyncEnabled)
	{
		if (!_WindowVisible)
			return;

		// Indicate a state transition on the resource usage.
		auto transition = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		_CommandList->ResourceBarrier(1, &transition);

		// Done recording commands.
		ThrowIfFailed(_CommandList->Close());

		// Add the command list to the queue for execution.
		ID3D12CommandList* cmdsLists[] = { _CommandList.Get() };
		_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		UINT presentFlags = _WindowedMode ? DXGI_PRESENT_ALLOW_TEARING : 0;

		// swap the back and front buffers
		ThrowIfFailed(_SwapChain->Present(0, presentFlags));
		_CurrentBackBuffer = (_CurrentBackBuffer + 1) % SwapChainBufferCount;

		// Wait until frame commands are complete.  This waiting is inefficient and is
		// done for simplicity.  Later we will show how to organize our rendering code
		// so we do not have to wait per frame.
		//FlushCommandQueue();
	}

	void DX12RendererAPI::FlushCommandQueue()
	{
		// Advance the fence value to mark commands up to this fence point.
		_CurrentFence++;

		// Add an instruction to the command queue to set a new fence point.  Because we 
	   // are on the GPU timeline, the new fence point won't be set until the GPU finishes
	   // processing all the commands prior to this Signal().
		ThrowIfFailed(_CommandQueue->Signal(_Fence.Get(), _CurrentFence));

		// Wait until the GPU has completed commands up to this fence point.
		if (_Fence->GetCompletedValue() < _CurrentFence)
		{
			HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

			// Fire event when GPU hits current fence.  
			ThrowIfFailed(_Fence->SetEventOnCompletion(_CurrentFence, eventHandle));

			// Wait until the GPU hits current fence event is fired.
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}

	void DX12RendererAPI::CreateRtvAndDsvDescriptorHeaps()
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
		rtvHeapDesc.NumDescriptors = SwapChainBufferCount + 1;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		ThrowIfFailed(_d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_rtvHeap)));

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.NodeMask = 0;
		ThrowIfFailed(_d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_dsvHeap)));
	}

	void DX12RendererAPI::BuildCbvDescriptorHeaps()
	{
		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
		cbvHeapDesc.NumDescriptors = 4096;
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.NodeMask = 0;
		ThrowIfFailed(_d3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
			IID_PPV_ARGS(&_cbvHeap)));
		_cbvHeap->SetName(L"My SRV/CBV Heap");
		g_pd3dSrvDescHeapAlloc.Create(_d3dDevice.Get(), _cbvHeap.Get());
	}

	void DX12RendererAPI::ResetSizeDependentResources()
	{
		for (UINT n = 0; n < SwapChainBufferCount; n++)
		{
			_SwapChainBuffer[n].Reset();
		}
		_DepthStencilBuffer.Reset();
	}

	void DX12RendererAPI::LoadSizeDependentResources()
	{
		// Create frame resources.
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < SwapChainBufferCount; i++)
		{
			ThrowIfFailed(_SwapChain->GetBuffer(i, IID_PPV_ARGS(&_SwapChainBuffer[i])));

			_d3dDevice->CreateRenderTargetView(_SwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);

			rtvHeapHandle.Offset(1, _rtvDescriptorSize);
		}

		D3D12_RESOURCE_DESC depthStencilDesc{};
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = _ClientWidth;
		depthStencilDesc.Height = _ClientHeight;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = _DepthStencilFormat;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear;
		optClear.Format = _DepthStencilFormat;
		optClear.DepthStencil.Depth = 1.f;
		optClear.DepthStencil.Stencil = 0;

		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(_d3dDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&optClear,
			IID_PPV_ARGS(&_DepthStencilBuffer)));

		_d3dDevice->CreateDepthStencilView(_DepthStencilBuffer.Get(), nullptr, GetDepthStencilView());

		_CurrentBackBuffer = 0;
	}

	void DX12RendererAPI::Resize()
	{ 
		RECT rect;
		GetClientRect(_hWnd, &rect);

		UINT width = rect.right - rect.left;
		UINT height = rect.bottom - rect.top;

		if (width <= 0 || height <= 0)
		{
			_WindowVisible = false;
			_ShouldResize = false;
			return;
		}
		else
			_WindowVisible = true;

		_ClientWidth = width;
		_ClientHeight = height;

		ResetSizeDependentResources();

		// Resize the swap chain to the desired dimensions.
		DXGI_SWAP_CHAIN_DESC desc = {};
		_SwapChain->GetDesc(&desc);
		ThrowIfFailed(_SwapChain->ResizeBuffers(SwapChainBufferCount, width, height, desc.BufferDesc.Format, desc.Flags));

		BOOL fullscreenState;
		ThrowIfFailed(_SwapChain->GetFullscreenState(&fullscreenState, nullptr));
		_WindowedMode = !fullscreenState;

		LoadSizeDependentResources();

		_ShouldResize = false;
	}

	void DX12RendererAPI::OnMouseMove(int x, int y)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = glm::radians(0.25f * static_cast<float>(x - _LastMousePos.x));
		float dy = glm::radians(0.25f * static_cast<float>(y - _LastMousePos.y));

		// Update angles based on input to orbit camera around box.
		_Theta += dx;
		_Phi += dy;

		// Restrict the angle mPhi.
		_Phi = glm::clamp(_Phi, 0.1f, XM_PI - 0.1f);

		_LastMousePos.x = x;
		_LastMousePos.y = y;
	}

	void DX12RendererAPI::ResetRenderTarget()
	{
		// Specify the buffers we are going to render to.
		auto depthStencilView = GetDepthStencilView();
		auto currBackBufferView = GetCurrentBackBufferView();
		_CommandList->OMSetRenderTargets(1, &currBackBufferView, true, &depthStencilView);
	}

	void DX12RendererAPI::CloseAndFlushCommandlist()
	{
		// Execute the initialization commands.
		ThrowIfFailed(_CommandList->Close());
		ID3D12CommandList* cmdsLists[] = { _CommandList.Get() };
		_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		// Wait until initialization is complete.
		FlushCommandQueue();
	}

	void DX12RendererAPI::ResetCommandlist()
	{
		// Reuse the memory associated with command recording.
		// We can only reset when the associated command lists have finished execution on the GPU.
		ThrowIfFailed(_CommandAllocator->Reset());

		// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
		// Reusing the command list reuses memory.
		ThrowIfFailed(_CommandList->Reset(_CommandAllocator.Get(), nullptr));
	}

	ID3D12Resource* DX12RendererAPI::GetCurrentBackBuffer()
	{
		return _SwapChainBuffer[_CurrentBackBuffer].Get();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DX12RendererAPI::GetCurrentBackBufferView()
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(_rtvHeap->GetCPUDescriptorHandleForHeapStart(), _CurrentBackBuffer, _rtvDescriptorSize);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DX12RendererAPI::GetDepthStencilView()
	{
		return _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	}

	void DX12RendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
	{
	}
}
