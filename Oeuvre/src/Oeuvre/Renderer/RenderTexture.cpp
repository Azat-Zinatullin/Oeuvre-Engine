#include "RenderTexture.h"

#include "Platform/DirectX11/DX11RendererAPI.h"
#include "glm/gtc/matrix_transform.hpp"

namespace Oeuvre
{
	RenderTexture::RenderTexture()
	{
	}

	RenderTexture::~RenderTexture()
	{
		if (_pDepthStencilSRV)
		{
			_pDepthStencilSRV->Release();
			_pDepthStencilSRV = nullptr;
		}

		if (_pDepthStencilView)
		{
			_pDepthStencilView->Release();
			_pDepthStencilView = nullptr;
		}

		if (_pDepthStencil)
		{
			_pDepthStencil->Release();
			_pDepthStencil = nullptr;
		}

		if (_pShaderResourceView)
		{
			_pShaderResourceView->Release();
			_pShaderResourceView = nullptr;
		}

		if (_pRenderTargetView)
		{
			_pRenderTargetView->Release();
			_pRenderTargetView = nullptr;
		}

		if (_pRenderTargetTexture)
		{
			_pRenderTargetTexture->Release();
			_pRenderTargetTexture = nullptr;
		}
	}

	bool RenderTexture::Initialize(int width, int height, int mipMaps, DXGI_FORMAT textureFormat, DXGI_FORMAT RTVformat, DXGI_FORMAT DSVformat, DXGI_FORMAT SRVformat, bool cubeTexture)
	{
		D3D11_TEXTURE2D_DESC textureDesc;
		D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

		_width = width;
		_height = height;

		// Initialize the render target texture description.
		ZeroMemory(&textureDesc, sizeof(textureDesc));

		// Setup the render target texture description.
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = mipMaps;
		textureDesc.ArraySize = 1;
		textureDesc.Format = textureFormat; // DXGI_FORMAT_R16G16B16A16_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		auto device = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDevice();
		// Create the render target texture.
		HRESULT result = device->CreateTexture2D(&textureDesc, nullptr, &_pRenderTargetTexture);
		if (FAILED(result))
		{
			return false;
		}

		// Setup the description of the render target view.
		ZeroMemory(&renderTargetViewDesc, sizeof(renderTargetViewDesc));
		renderTargetViewDesc.Format = RTVformat;
		renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		renderTargetViewDesc.Texture2D.MipSlice = 0;

		// Create the render target view.
		result = device->CreateRenderTargetView(_pRenderTargetTexture, &renderTargetViewDesc, &_pRenderTargetView);
		if (FAILED(result))
		{
			return false;
		}
	
		// Setup the description of the shader resource view.
		ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
		shaderResourceViewDesc.Format = SRVformat;
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;

		// Create the shader resource view.
		result = device->CreateShaderResourceView(_pRenderTargetTexture, &shaderResourceViewDesc, &_pShaderResourceView);
		if (FAILED(result)) return false;

		D3D11_TEXTURE2D_DESC descDepth;
		ZeroMemory(&descDepth, sizeof(descDepth));
		descDepth.Width = width;
		descDepth.Height = height;
		descDepth.MipLevels = 1;
		descDepth.ArraySize = 1;
		descDepth.Format = DSVformat;
		descDepth.SampleDesc.Count = 1;
		descDepth.SampleDesc.Quality = 0;
		descDepth.Usage = D3D11_USAGE_DEFAULT;
		descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		descDepth.CPUAccessFlags = 0;
		descDepth.MiscFlags = 0;

		if (cubeTexture)
		{
			descDepth.Format = textureFormat;
			descDepth.ArraySize = 6;
			descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
			descDepth.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
		}

		result = device->CreateTexture2D(&descDepth, NULL, &_pDepthStencil);
		if (FAILED(result)) return false;

		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
		ZeroMemory(&descDSV, sizeof(descDSV));
		descDSV.Format = DSVformat;
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		descDSV.Texture2D.MipSlice = 0;

		if (cubeTexture)
		{
			descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			descDSV.Texture2DArray.MipSlice = 0;
			descDSV.Texture2DArray.ArraySize = 6;
		}

		result = device->CreateDepthStencilView(_pDepthStencil, &descDSV, &_pDepthStencilView);

		if (cubeTexture)
		{
			ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
			shaderResourceViewDesc.Format = SRVformat;
			shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			shaderResourceViewDesc.TextureCube.MipLevels = 1;
			shaderResourceViewDesc.TextureCube.MostDetailedMip = 0;

			result = device->CreateShaderResourceView(_pDepthStencil, &shaderResourceViewDesc, &_pDepthStencilSRV);
			if (FAILED(result)) return false;
		}	

		m_OrthoMatrix = glm::orthoLH(0.f, (float)_width, 0.f, (float)_height, 0.1f, 100.f);
		m_ProjectionMatrix = glm::perspectiveFovLH(XM_PIDIV2, (float)_width, (float)_height, 0.1f, 100.f);

		return !FAILED(result);
	}

	void RenderTexture::SetRenderTarget(bool rtvToNull) const
	{
		auto deviceContext = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDeviceContext();
		if(rtvToNull)
			deviceContext->OMSetRenderTargets(0, nullptr, _pDepthStencilView);
		else
			deviceContext->OMSetRenderTargets(1, &_pRenderTargetView, _pDepthStencilView);
	}

	void Oeuvre::RenderTexture::ClearRenderTarget(float red, float green, float blue, float alpha, float depth) const
	{
		auto deviceContext = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDeviceContext();

		float color[4];

		// Setup the color to clear the buffer to.
		color[0] = red;
		color[1] = green;
		color[2] = blue;
		color[3] = alpha;

		// Clear the back buffer.
		deviceContext->ClearRenderTargetView(_pRenderTargetView, color);

		// Clear the depth buffer.
		deviceContext->ClearDepthStencilView(_pDepthStencilView, D3D11_CLEAR_DEPTH, depth, 0);
	}

	ID3D11ShaderResourceView* RenderTexture::GetSRV() const
	{
		return _pShaderResourceView;
	}

	ID3D11Texture2D* RenderTexture::GetTexture() const
	{
		return _pRenderTargetTexture;
	}
}


