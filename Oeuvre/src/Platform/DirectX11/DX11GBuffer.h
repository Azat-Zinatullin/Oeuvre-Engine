#pragma once
#include <d3d11.h>

namespace Oeuvre
{
	constexpr int GBUFFER_BUFFER_COUNT = 3;

	class DX11GBuffer
	{
	public:
		DX11GBuffer();
		~DX11GBuffer();

		bool Initialize(int textureWidth, int textureHeight);
		void Shutdown();

		void SetRenderTargets();
		void ClearRenderTargets(float red, float green, float blue, float alpha);

		ID3D11ShaderResourceView* GetShaderResourceView(int view);
		ID3D11Texture2D * GetDepthStencilTexture() { return m_DepthStencilBuffer; }
		ID3D11DepthStencilView* GetDepthStencilView() { return m_DepthStencilView; }
		ID3D11ShaderResourceView* GetDepthShaderResourceView() { return m_DepthShaderResourceView; }

		ID3D11Texture2D* GetTexture(int texture) { return m_RenderTargetTextureArray[texture]; }

	private:
		int m_TextureWidth, m_TextureHeight;

		ID3D11Texture2D* m_RenderTargetTextureArray[GBUFFER_BUFFER_COUNT];
		ID3D11RenderTargetView* m_RenderTargetViewArray[GBUFFER_BUFFER_COUNT];
		ID3D11ShaderResourceView* m_ShaderResourceViewArray[GBUFFER_BUFFER_COUNT];
		ID3D11Texture2D* m_DepthStencilBuffer;
		ID3D11DepthStencilView* m_DepthStencilView;
		ID3D11ShaderResourceView* m_DepthShaderResourceView;
	};
}