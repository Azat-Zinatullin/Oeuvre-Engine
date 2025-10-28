#pragma once

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "runtimeobject.lib")

#include "Oeuvre/Renderer/RendererAPI.h"
#include "ovpch.h"


namespace Oeuvre
{
	class DX11RendererAPI : public RendererAPI
	{
	public:
		DX11RendererAPI();
		~DX11RendererAPI();

		virtual void Init() override;
		virtual void Shutdown() override;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		virtual void SetClearColor(const glm::vec4& color) override;
		virtual void Clear() override;

		void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount) override;

		virtual void BeginFrame() override;
		virtual void EndFrame(bool vSyncEnabled) override;

		virtual void OnWindowResize(int width, int height);

		IDXGISwapChain* GetSwapchain() { return m_pSwapChain; }
		ID3D11Device* GetDevice() { return m_pD3dDevice; }	
		ID3D11DeviceContext* GetDeviceContext() { return m_pDeviceContext; }
		ID3D11DepthStencilView* GetDepthStencilView() { return m_pDepthStencilView; }
		ID3D11Texture2D* GetDepthStencilTexture() { return m_pDepthStencil; }

		void ResetRenderTargetView();
		void TurnZBufferOn();
		void TurnZBufferOff();

		void SetCullBack();
		void SetCullFront();

	private:
		HRESULT InitDevice(HWND hWnd);

		float m_ClearColor[4] = { 0.f, 0.f, 0.f, 1.f };
			
		HWND m_hWnd = 0;
		D3D_DRIVER_TYPE m_driverType = D3D_DRIVER_TYPE_HARDWARE;
		D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;

		ID3D11Device* m_pD3dDevice = nullptr;
		ID3D11DeviceContext* m_pDeviceContext = nullptr;
		IDXGISwapChain* m_pSwapChain = nullptr;
		ID3D11RenderTargetView* m_pRenderTargetView = nullptr;
		ID3D11Texture2D* m_pDepthStencil = nullptr;
		ID3D11DepthStencilView* m_pDepthStencilView = nullptr;
		ID3D11RasterizerState* m_pRasterizerState = nullptr;

		int m_viewportWidth = 0, m_viewportHeight = 0;

		ID3D11DepthStencilState* m_pDepthStencilState = nullptr;
		ID3D11DepthStencilState* m_pDepthDisabledStencilState = nullptr;

		ID3D11RasterizerState* m_pRasterizerCullFrontState = nullptr;		
	};
}

