#include "DX11GBuffer.h"

#include "Platform/DirectX11/DX11RendererAPI.h"

namespace Oeuvre
{
	DX11GBuffer::DX11GBuffer()
	{
		for (int i = 0; i < BUFFER_COUNT; i++)
		{
			m_RenderTargetTextureArray[i] = nullptr;
			m_RenderTargetViewArray[i] = nullptr;
			m_ShaderResourceViewArray[i] = nullptr;
		}

		m_DepthStencilBuffer = nullptr;
		m_DepthStencilView = nullptr;
	}

	DX11GBuffer::~DX11GBuffer()
	{

	}

	bool DX11GBuffer::Initialize(int textureWidth, int textureHeight)
	{
        D3D11_TEXTURE2D_DESC textureDesc;
        HRESULT result;
        D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
        D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
        D3D11_TEXTURE2D_DESC depthBufferDesc;
        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
        int i;

        // Store the width and height of the render texture.
        m_TextureWidth = textureWidth;
        m_TextureHeight = textureHeight;

        // Initialize the render target texture description.
        ZeroMemory(&textureDesc, sizeof(textureDesc));

        // Setup the render target texture description.
        textureDesc.Width = textureWidth;
        textureDesc.Height = textureHeight;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;

        auto device = ((DX11RendererAPI*)DX11RendererAPI::GetInstance().get())->GetDevice();
        // Create the render target textures.
        for (i = 0; i < BUFFER_COUNT; i++)
        {
            result = device->CreateTexture2D(&textureDesc, NULL, &m_RenderTargetTextureArray[i]);
            if (FAILED(result))
            {
                return false;
            }
        }

        // Setup the description of the render target view.
        renderTargetViewDesc.Format = textureDesc.Format;
        renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        renderTargetViewDesc.Texture2D.MipSlice = 0;

        // Create the render target views.
        for (i = 0; i < BUFFER_COUNT; i++)
        {   
            result = device->CreateRenderTargetView(m_RenderTargetTextureArray[i], &renderTargetViewDesc, &m_RenderTargetViewArray[i]);
            if (FAILED(result))
            {
                return false;
            }
        }

        // Setup the description of the shader resource view.
        shaderResourceViewDesc.Format = textureDesc.Format;
        shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
        shaderResourceViewDesc.Texture2D.MipLevels = 1;

        // Create the shader resource views.
        for (i = 0; i < BUFFER_COUNT; i++)
        {       
            result = device->CreateShaderResourceView(m_RenderTargetTextureArray[i], &shaderResourceViewDesc, &m_ShaderResourceViewArray[i]);
            if (FAILED(result))
            {
                return false;
            }
        }

        // Initialize the description of the depth buffer.
        ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

        // Set up the description of the depth buffer.
        depthBufferDesc.Width = textureWidth;
        depthBufferDesc.Height = textureHeight;
        depthBufferDesc.MipLevels = 1;
        depthBufferDesc.ArraySize = 1;
        depthBufferDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; //DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthBufferDesc.SampleDesc.Count = 1;
        depthBufferDesc.SampleDesc.Quality = 0;
        depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        depthBufferDesc.CPUAccessFlags = 0;
        depthBufferDesc.MiscFlags = 0;

        // Create the texture for the depth buffer using the filled out description.
        result = device->CreateTexture2D(&depthBufferDesc, NULL, &m_DepthStencilBuffer);
        if (FAILED(result))
        {
            return false;
        }

        // Initailze the depth stencil view description.
        ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

        // Set up the depth stencil view description.
        depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDesc.Texture2D.MipSlice = 0;

        // Create the depth stencil view.
        result = device->CreateDepthStencilView(m_DepthStencilBuffer, &depthStencilViewDesc, &m_DepthStencilView);
        if (FAILED(result))
        {
            return false;
        }

        shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

        result = device->CreateShaderResourceView(m_DepthStencilBuffer, &shaderResourceViewDesc, &m_DepthShaderResourceView);

        // Setup the viewport for rendering.
        m_Viewport.Width = (float)textureWidth;
        m_Viewport.Height = (float)textureHeight;
        m_Viewport.MinDepth = 0.0f;
        m_Viewport.MaxDepth = 1.0f;
        m_Viewport.TopLeftX = 0.0f;
        m_Viewport.TopLeftY = 0.0f;

        return true;
	}

    void DX11GBuffer::Shutdown()
    {
        if (m_DepthShaderResourceView)
        {
            m_DepthShaderResourceView->Release();
            m_DepthShaderResourceView = nullptr;
        }

        if (m_DepthStencilView)
        {
            m_DepthStencilView->Release();
            m_DepthStencilView = nullptr;
        }

        if (m_DepthStencilBuffer)
        {
            m_DepthStencilBuffer->Release();
            m_DepthStencilBuffer = nullptr;
        }

        for (int i = 0; i < BUFFER_COUNT; i++)
        {
            if (m_ShaderResourceViewArray[i])
            {
                m_ShaderResourceViewArray[i]->Release();
                m_ShaderResourceViewArray[i] = nullptr;
            }

            if (m_RenderTargetViewArray[i])
            {
                m_RenderTargetViewArray[i]->Release();
                m_RenderTargetViewArray[i] = nullptr;
            }

            if (m_RenderTargetTextureArray[i])
            {
                m_RenderTargetTextureArray[i]->Release();
                m_RenderTargetTextureArray[i] = nullptr;
            }
        }
    }

    void DX11GBuffer::SetRenderTargets()
    {
        auto deviceContext = ((DX11RendererAPI*)DX11RendererAPI::GetInstance().get())->GetDeviceContext();
 
        // Bind the render target view array and depth stencil buffer to the output render pipeline.
        deviceContext->OMSetRenderTargets(BUFFER_COUNT, m_RenderTargetViewArray, m_DepthStencilView);

        // Set the viewport.
        deviceContext->RSSetViewports(1, &m_Viewport);
    }

    void DX11GBuffer::ClearRenderTargets(float red, float green, float blue, float alpha)
    {
        float color[4];

        // Setup the color to clear the buffer to.
        color[0] = red;
        color[1] = green;
        color[2] = blue;
        color[3] = alpha;

        auto deviceContext = ((DX11RendererAPI*)DX11RendererAPI::GetInstance().get())->GetDeviceContext();

        // Clear the render target buffers.
        for (int i = 0; i < BUFFER_COUNT; i++)
        {
            if (i == 3)
            {
                color[0] = 1.f;
                color[1] = 0.f;
                color[2] = 0.f;
                color[3] = 0.f;
            }               
            deviceContext->ClearRenderTargetView(m_RenderTargetViewArray[i], color);
        }

        // Clear the depth buffer.
        deviceContext->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    }

    ID3D11ShaderResourceView* DX11GBuffer::GetShaderResourceView(int view)
    {
        return m_ShaderResourceViewArray[view];
    }
}


