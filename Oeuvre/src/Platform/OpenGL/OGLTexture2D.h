#pragma once
#include "Oeuvre/Renderer/RHI/ITexture.h"

typedef unsigned int GLenum;

namespace Oeuvre
{
	class OGLTexture2D : public ITexture
	{
	public:
		virtual ~OGLTexture2D();

		OGLTexture2D(const TextureDesc& specification);
		OGLTexture2D(const std::string& path);

		const TextureDesc& GetDesc() const override;
		uint32_t GetWidth() const override;
		uint32_t GetHeight() const override;
		uint32_t GetRendererID() const override;

		const std::string& GetPath() const override;
		void SetData(void* data, uint32_t size) override;
		void Bind(uint32_t slot) override;

		bool IsLoaded() const override;
		bool operator==(const ITexture& other) const override;

	private:
		bool Load();
		TextureDesc m_Specification;
		uint32_t m_RendererID = 0;
		std::string m_Path = "";
		bool m_IsLoaded = false;
		uint32_t m_Width = 0, m_Height = 0;
		GLenum m_InternalFormat = 0, m_DataFormat = 0;
	};
}



