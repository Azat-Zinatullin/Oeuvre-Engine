#pragma once

#include "Oeuvre//Renderer/Texture.h"
#include <d3d11.h>

namespace Oeuvre
{
	class DX11Texture2D : public Texture2D
	{
	public:
		virtual ~DX11Texture2D();

		DX11Texture2D(const TextureSpecification& specification);
		DX11Texture2D(const std::string& path);

		const TextureSpecification& GetSpecification() const override;
		uint32_t GetWidth() const override;
		uint32_t GetHeight() const override;
		uint32_t GetRendererID() const override;

		const std::string& GetPath() const override;
		void SetData(void* data, uint32_t size) override;
		void Bind(uint32_t slot) override;

		bool IsLoaded() const override;
		bool operator==(const Texture& other) const override;

		ID3D11ShaderResourceView* GetSRV() { return m_ShaderResourceView; }
		ID3D11Texture2D* GetTexture() { return m_Texture2D; }

	private:
		TextureSpecification m_Specification;
		uint32_t m_RendererID;
		std::string m_Path;
		bool m_IsLoaded = false;
		uint32_t m_Width, m_Height;
		ID3D11ShaderResourceView* m_ShaderResourceView = nullptr;
		ID3D11Texture2D* m_Texture2D = nullptr;
		DXGI_FORMAT m_DataFormat;
	};
}