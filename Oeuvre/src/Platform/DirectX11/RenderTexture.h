#pragma once
#include <d3d11.h>
#include "glm/glm.hpp"
#include "wrl/client.h"

using Microsoft::WRL::ComPtr;

namespace Oeuvre
{
	class RenderTexture
	{
	public:
		RenderTexture();
		~RenderTexture();

		bool Initialize(int width, int height, int mipMaps, DXGI_FORMAT textureFormat, DXGI_FORMAT RTVformat, DXGI_FORMAT DSVformat, DXGI_FORMAT SRVformat, bool cubeTexture, bool arrayTexture, int arraySize);

		void SetRenderTarget(bool rtvToNull, bool depthToNull, bool bindUAV, ID3D11UnorderedAccessView** uav);
		void ClearRenderTarget(float red, float green, float blue, float alpha, float depth) const;
		ID3D11ShaderResourceView* GetSRV() const;
		ID3D11Texture2D* GetTexture() const;
		ID3D11DepthStencilView* GetDSV() const { return _pDepthStencilView; }
		ID3D11Texture2D* GetDepthStencilTexture() { return _pDepthStencil; }
		ID3D11ShaderResourceView* GetDepthStencilSRV() { return _pDepthStencilSRV; }
		ID3D11RenderTargetView* GetRTV() { return _pRenderTargetView; }

	private:
		int _width;
		int _height;
		ID3D11Texture2D* _pRenderTargetTexture = nullptr;
		ID3D11RenderTargetView* _pRenderTargetView = nullptr;
		ID3D11ShaderResourceView* _pShaderResourceView = nullptr;
		ID3D11Texture2D* _pDepthStencil = nullptr;
		ID3D11DepthStencilView* _pDepthStencilView = nullptr;	
		ID3D11ShaderResourceView* _pDepthStencilSRV = nullptr;
	};
}

