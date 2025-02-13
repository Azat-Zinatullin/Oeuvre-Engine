#include "DX11RendererAPI.h"
#include "../Windows/WindowsWindow.h"
#include "Oeuvre/Core/Log.h"


namespace Oeuvre
{
	DX11RendererAPI::DX11RendererAPI()
	{
		Init();
	}

	DX11RendererAPI::~DX11RendererAPI()
	{
#ifdef _DEBUG
		ID3D11Debug* d3d11Debug;
		m_pD3dDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&d3d11Debug));
		d3d11Debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif
		Shutdown();
	}

	void DX11RendererAPI::Init()
	{
		if (FAILED(InitDevice(WindowsWindow::GetInstance()->GetHWND())))
			OV_CORE_ERROR("Unable to initialize DX11RendererAPI!");			
	}

	void DX11RendererAPI::Shutdown()
	{
		if (m_pRasterizerCullFrontState) m_pRasterizerCullFrontState->Release();
		if (m_pDepthDisabledStencilState) m_pDepthDisabledStencilState->Release();
		if (m_pDepthStencilState) m_pDepthStencilState->Release();
		if (m_pRasterizerState) m_pRasterizerState->Release();
		if (m_pDepthStencilView) m_pDepthStencilView->Release();
		if (m_pDepthStencil) m_pDepthStencil->Release();
		if (m_pRenderTargetView) m_pRenderTargetView->Release();
		if (m_pSwapChain) m_pSwapChain->Release();
		if (m_pDeviceContext) m_pDeviceContext->Release();
		if (m_pD3dDevice) m_pD3dDevice->Release();
	}

	void DX11RendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		D3D11_VIEWPORT vp;
		vp.Width = (FLOAT)width;
		vp.Height = (FLOAT)height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = x;
		vp.TopLeftY = y;
		m_pDeviceContext->RSSetViewports(1, &vp);
	}

	void DX11RendererAPI::SetClearColor(const glm::vec4& color)
	{
		m_ClearColor[0] = color.r;
		m_ClearColor[1] = color.g;
		m_ClearColor[2] = color.b;
		m_ClearColor[3] = color.a;
	}

	void DX11RendererAPI::Clear()
	{
		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);		
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, m_ClearColor);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.f, 0);
	}

	void DX11RendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
	{
		vertexArray->Bind();
		m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	
		m_pDeviceContext->DrawIndexed(indexCount, 0, 0);
	}

	void DX11RendererAPI::DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount)
	{
		vertexArray->Bind();
		m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		m_pDeviceContext->DrawIndexed(vertexArray->GetIndexBuffer()->GetCount(), 0, 0);
	}

	void DX11RendererAPI::SetLineWidth(float width)
	{
	}

	void DX11RendererAPI::OnWindowResize(int width, int height)
	{
		m_pDeviceContext->OMSetRenderTargets(0, 0, 0);
		if (m_pRenderTargetView)
			m_pRenderTargetView->Release();
		if (m_pDepthStencilView)
			m_pDepthStencilView->Release();
		if (m_pDepthStencil)
			m_pDepthStencil->Release();
		HRESULT hr;
		hr = m_pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
		if (FAILED(hr)) return;
		ID3D11Texture2D* pBuffer;
		hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBuffer);
		if (FAILED(hr)) return;

		hr = m_pD3dDevice->CreateRenderTargetView(pBuffer, NULL,
			&m_pRenderTargetView);
		if (FAILED(hr))
		{
			pBuffer->Release();
			return;
		}
		pBuffer->Release();

		D3D11_TEXTURE2D_DESC descDepth;
		ZeroMemory(&descDepth, sizeof(descDepth));
		descDepth.Width = width;
		descDepth.Height = height;
		descDepth.MipLevels = 1;
		descDepth.ArraySize = 1;
		descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		descDepth.SampleDesc.Count = 1;
		descDepth.SampleDesc.Quality = 0;
		descDepth.Usage = D3D11_USAGE_DEFAULT;
		descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		descDepth.CPUAccessFlags = 0;
		descDepth.MiscFlags = 0;

		hr = m_pD3dDevice->CreateTexture2D(&descDepth, NULL, &m_pDepthStencil);
		if (FAILED(hr)) return;

		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
		ZeroMemory(&descDSV, sizeof(descDSV));
		descDSV.Format = descDepth.Format;
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		descDSV.Texture2D.MipSlice = 0;

		hr = m_pD3dDevice->CreateDepthStencilView(m_pDepthStencil, &descDSV, &m_pDepthStencilView);
		if (FAILED(hr)) return;

		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

		D3D11_VIEWPORT vp;
		vp.Width = width;
		vp.Height = height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		m_pDeviceContext->RSSetViewports(1, &vp);

		m_viewportWidth = width;
		m_viewportHeight = height;
	}

	void DX11RendererAPI::ResetRenderTargetView()
	{
		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);
	}

	void DX11RendererAPI::TurnZBufferOn()
	{
		m_pDeviceContext->OMSetDepthStencilState(m_pDepthStencilState, 1);
	}

	void DX11RendererAPI::TurnZBufferOff()
	{
		m_pDeviceContext->OMSetDepthStencilState(m_pDepthDisabledStencilState, 1);
	}

	void DX11RendererAPI::SetCullBack()
	{
		m_pDeviceContext->RSSetState(m_pRasterizerState);
	}

	void DX11RendererAPI::SetCullFront()
	{
		m_pDeviceContext->RSSetState(m_pRasterizerCullFrontState);
	}

	HRESULT DX11RendererAPI::InitDevice(HWND hWnd)
	{
		m_hWnd = hWnd;
		HRESULT hr = S_OK;

		RECT rc;
		GetClientRect(m_hWnd, &rc);
		UINT width = rc.right - rc.left;
		UINT height = rc.bottom - rc.top;

		UINT createDeviceFlags = 0;
#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_DRIVER_TYPE driverTypes[] =
		{
			D3D_DRIVER_TYPE_HARDWARE,
			D3D_DRIVER_TYPE_WARP,
			D3D_DRIVER_TYPE_REFERENCE
		};
		UINT numDriverTypes = ARRAYSIZE(driverTypes);

		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0
		};

		UINT numFeatureLevels = ARRAYSIZE(featureLevels);

		IDXGIFactory* factory;
		IDXGIAdapter* adapter;
		IDXGIOutput* adapterOutput;
		DXGI_MODE_DESC* displayModeList;
		unsigned int numModes, numerator, denominator;

		// Create a DirectX graphics interface factory.
		hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
		if (FAILED(hr)) return hr;


		// Use the factory to create an adapter for the primary graphics interface (video card).
		hr = factory->EnumAdapters(0, &adapter);
		if (FAILED(hr)) return hr;

		// Enumerate the primary adapter output (monitor).
		hr = adapter->EnumOutputs(0, &adapterOutput);
		if (FAILED(hr)) return hr;

		// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
		hr = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
		if (FAILED(hr)) return hr;

		// Create a list to hold all the possible display modes for this monitor/video card combination.
		displayModeList = new DXGI_MODE_DESC[numModes];
		if (FAILED(hr)) return hr;

		// Now fill the display mode list structures.
		hr = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
		if (FAILED(hr)) return hr;

		// Now go through all the display modes and find the one that matches the screen width and height.
		// When a match is found store the numerator and denominator of the refresh rate for that monitor.
		for (int i = 0; i < numModes; i++)
		{
			if (displayModeList[i].Width == (unsigned int)width)
			{
				if (displayModeList[i].Height == (unsigned int)height)
				{
					numerator = displayModeList[i].RefreshRate.Numerator;
					denominator = displayModeList[i].RefreshRate.Denominator;
				}
			}
		}

		// Release the display mode list.
		delete[] displayModeList;
		displayModeList = 0;

		// Release the adapter output.
		adapterOutput->Release();
		adapterOutput = 0;

		// Release the adapter.
		adapter->Release();
		adapter = 0;

		// Release the factory.
		factory->Release();
		factory = 0;

		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 2;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = numerator;
		sd.BufferDesc.RefreshRate.Denominator = denominator;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = m_hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
		{
			m_driverType = driverTypes[driverTypeIndex];
			hr = D3D11CreateDeviceAndSwapChain(NULL, m_driverType, NULL, createDeviceFlags, featureLevels,
				numFeatureLevels, D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pD3dDevice, &m_featureLevel, &m_pDeviceContext);
			if (SUCCEEDED(hr))
				break;
		}
		if (FAILED(hr)) return hr;

		ID3D11Texture2D* pBackBuffer;
		hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		if (FAILED(hr)) return hr;

		hr = m_pD3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pRenderTargetView);
		pBackBuffer->Release();
		if (FAILED(hr)) return hr;

		D3D11_TEXTURE2D_DESC descDepth;
		ZeroMemory(&descDepth, sizeof(descDepth));
		descDepth.Width = width;
		descDepth.Height = height;
		descDepth.MipLevels = 1;
		descDepth.ArraySize = 1;
		descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		descDepth.SampleDesc.Count = 1;
		descDepth.SampleDesc.Quality = 0;
		descDepth.Usage = D3D11_USAGE_DEFAULT;
		descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		descDepth.CPUAccessFlags = 0;
		descDepth.MiscFlags = 0;

		hr = m_pD3dDevice->CreateTexture2D(&descDepth, NULL, &m_pDepthStencil);
		if (FAILED(hr)) return hr;

		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
		ZeroMemory(&descDSV, sizeof(descDSV));
		descDSV.Format = descDepth.Format;
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		descDSV.Texture2D.MipSlice = 0;

		hr = m_pD3dDevice->CreateDepthStencilView(m_pDepthStencil, &descDSV, &m_pDepthStencilView);
		if (FAILED(hr)) return hr;

		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

		D3D11_RASTERIZER_DESC rasterDesc;
		// Setup the raster description which will determine how and what polygons will be drawn.
		rasterDesc.AntialiasedLineEnable = true;
		rasterDesc.CullMode = D3D11_CULL_BACK;
		rasterDesc.DepthBias = 0;
		rasterDesc.DepthBiasClamp = 0.0f;
		rasterDesc.DepthClipEnable = true;
		rasterDesc.FillMode = D3D11_FILL_SOLID;
		rasterDesc.FrontCounterClockwise = false;
		rasterDesc.MultisampleEnable = true;
		rasterDesc.ScissorEnable = false;
		rasterDesc.SlopeScaledDepthBias = 0.0f;

		// Create the rasterizer state from the description we just filled out.
		hr = m_pD3dDevice->CreateRasterizerState(&rasterDesc, &m_pRasterizerState);
		if (FAILED(hr)) return hr;

		// Now set the rasterizer state.
		m_pDeviceContext->RSSetState(m_pRasterizerState);

		D3D11_VIEWPORT vp;
		vp.Width = (FLOAT)width;
		vp.Height = (FLOAT)height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		m_pDeviceContext->RSSetViewports(1, &vp);

		m_viewportWidth = width;
		m_viewportHeight = height;

		D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
		D3D11_DEPTH_STENCIL_DESC depthDisabledStencilDesc;

		// Initialize the description of the stencil state.
		ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

		// Set up the description of the stencil state.
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

		depthStencilDesc.StencilEnable = true;
		depthStencilDesc.StencilReadMask = 0xFF;
		depthStencilDesc.StencilWriteMask = 0xFF;

		// Stencil operations if pixel is front-facing.
		depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		// Stencil operations if pixel is back-facing.
		depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		// Create the depth stencil state.
		hr = m_pD3dDevice->CreateDepthStencilState(&depthStencilDesc, &m_pDepthStencilState);
		if (FAILED(hr)) return hr;

		// Set the depth stencil state.
		m_pDeviceContext->OMSetDepthStencilState(m_pDepthStencilState, 1);

		ZeroMemory(&depthDisabledStencilDesc, sizeof(depthDisabledStencilDesc));

		// Now create a second depth stencil state which turns off the Z buffer for 2D rendering.  The only difference is 
		// that DepthEnable is set to false, all other parameters are the same as the other depth stencil state.
		depthDisabledStencilDesc.DepthEnable = false;
		depthDisabledStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthDisabledStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
		depthDisabledStencilDesc.StencilEnable = true;
		depthDisabledStencilDesc.StencilReadMask = 0xFF;
		depthDisabledStencilDesc.StencilWriteMask = 0xFF;
		depthDisabledStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depthDisabledStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		depthDisabledStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depthDisabledStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depthDisabledStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depthDisabledStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		depthDisabledStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depthDisabledStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		// Create the state using the device.
		hr = m_pD3dDevice->CreateDepthStencilState(&depthDisabledStencilDesc, &m_pDepthDisabledStencilState);
		if (FAILED(hr)) return hr;

		rasterDesc.CullMode = D3D11_CULL_FRONT;

		// Create the rasterizer state from the description we just filled out.
		hr = m_pD3dDevice->CreateRasterizerState(&rasterDesc, &m_pRasterizerCullFrontState);
		if (FAILED(hr)) return hr;

		return S_OK;
	}
}


