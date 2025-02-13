#pragma once
#include <d3d11.h>
#include "glm/glm.hpp"

namespace Oeuvre
{
	class RenderTexture
	{
	public:
		RenderTexture();
		~RenderTexture();

		bool Initialize(int width, int height, int mipMaps, DXGI_FORMAT textureFormat, DXGI_FORMAT RTVformat, DXGI_FORMAT DSVformat, DXGI_FORMAT SRVformat, bool cubeTexture);

		void SetRenderTarget(bool rtvToNull) const;
		void ClearRenderTarget(float red, float green, float blue, float alpha, float depth) const;
		ID3D11ShaderResourceView* GetSRV() const;
		ID3D11Texture2D* GetTexture() const;
		ID3D11DepthStencilView* GetDSV() const { return _pDepthStencilView; }
		ID3D11Texture2D* GetDepthStencilTexture() { return _pDepthStencil; }
		ID3D11ShaderResourceView* GetDepthStencilSRV() { return _pDepthStencilSRV; }

		glm::mat4 GetOrthoMatrix() { return m_OrthoMatrix; }
		glm::mat4 GetProjectionMatrix() { return m_ProjectionMatrix; }

	private:
		int _width;
		int _height;
		ID3D11Texture2D* _pRenderTargetTexture = nullptr;
		ID3D11RenderTargetView* _pRenderTargetView = nullptr;
		ID3D11ShaderResourceView* _pShaderResourceView = nullptr;
		ID3D11Texture2D* _pDepthStencil = nullptr;
		ID3D11DepthStencilView* _pDepthStencilView = nullptr;	
		ID3D11ShaderResourceView* _pDepthStencilSRV = nullptr;

		glm::mat4 m_OrthoMatrix;
		glm::mat4 m_ProjectionMatrix;
	};
}

