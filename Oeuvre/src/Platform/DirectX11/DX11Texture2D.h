#pragma once

#include "Oeuvre/Renderer/RHI/ITexture.h"
#include <d3d11.h>

namespace Oeuvre
{
	class DX11Texture2D : public ITexture
	{
	public:
		virtual ~DX11Texture2D();

		DX11Texture2D(const TextureDesc& specification);
		DX11Texture2D(const std::string& path);

		const TextureDesc& GetDesc() const override;
		uint32_t GetWidth() const override;
		uint32_t GetHeight() const override;
		uint32_t GetRendererID() const override;

		const std::string& GetPath() const override;
		void SetData(void* data, uint32_t size) override;
		void Bind(uint32_t slot) override;

		bool IsLoaded() const override;
		bool operator==(const ITexture& other) const override;

		ID3D11ShaderResourceView* GetSRV() { return m_ShaderResourceView; }
		ID3D11Texture2D* GetTexture() { return m_Texture2D; }

	private:
		bool Load();
		TextureDesc m_Specification;
		uint32_t m_RendererID;
		std::string m_Path;
		bool m_IsLoaded = false;
		uint32_t m_Width, m_Height;
		ID3D11ShaderResourceView* m_ShaderResourceView = nullptr;
		ID3D11Texture2D* m_Texture2D = nullptr;
		DXGI_FORMAT m_DataFormat;
	};
}