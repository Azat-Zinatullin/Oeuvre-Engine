#include "ovpch.h"
#include "glad/glad.h"
#include "OGLTexture2D.h"
#include <stb_image.h>


namespace Oeuvre
{
	namespace Utils {

		static GLenum OeuvreImageFormatToGLDataFormat(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::R8G8B8A8_UNORM: return GL_RGBA;
			}

			assert(false);
			return 0;
		}

		static GLenum OeuvreImageFormatToGLInternalFormat(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::R8G8B8A8_UNORM: return GL_RGBA8;
			}

			assert(false);
			return 0;
		}
	}


	OGLTexture2D::~OGLTexture2D()
	{
		if (m_IsLoaded)
		{
			glDeleteTextures(1, &m_RendererID);
		}	
	}

	OGLTexture2D::OGLTexture2D(const TextureDesc& specification)
	{
		m_InternalFormat = Utils::OeuvreImageFormatToGLInternalFormat(m_Specification.Format);
		m_DataFormat = Utils::OeuvreImageFormatToGLDataFormat(m_Specification.Format);

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	OGLTexture2D::OGLTexture2D(const std::string& path)
		: m_Path(path)
	{
		Load();
	}

	bool OGLTexture2D::Load()
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(0);
		stbi_uc* data = nullptr;
		{
			data = stbi_load(m_Path.c_str(), &width, &height, &channels, 0);
		}

		if (data)
		{
			m_IsLoaded = true;

			m_Width = width;
			m_Height = height;

			GLenum internalFormat = 0, dataFormat = 0;
			if (channels == 4)
			{
				internalFormat = GL_RGBA8;
				dataFormat = GL_RGBA;
			}
			else if (channels == 3)
			{
				internalFormat = GL_RGB8;
				dataFormat = GL_RGB;
			}
			else if (channels == 1)
			{
				internalFormat = GL_RED;
				dataFormat = GL_RED;
			}

			m_InternalFormat = internalFormat;
			m_DataFormat = dataFormat;

			if (internalFormat == 0 || dataFormat == 0)
			{
				std::cout << "Format not supported for texture from path: " << m_Path.c_str() << "\n";
			}

			assert(internalFormat & dataFormat && "Format not supported!");

			glGenTextures(1, &m_RendererID);
			glBindTexture(GL_TEXTURE_2D, m_RendererID);
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

			stbi_image_free(data);

			std::cout << "Texture loaded from path: " << m_Path.c_str() << "\n";
			glBindTexture(GL_TEXTURE_2D, 0);

			return true;
		}
		return false;
	}

	const TextureDesc& OGLTexture2D::GetDesc() const
	{
		return m_Specification;
	}

	uint32_t OGLTexture2D::GetWidth() const
	{
		return m_Width;
	}

	uint32_t OGLTexture2D::GetHeight() const
	{
		return m_Height;
	}

	uint32_t OGLTexture2D::GetRendererID() const
	{
		return m_RendererID;
	}

	const std::string& OGLTexture2D::GetPath() const
	{
		return m_Path;
	}

	void OGLTexture2D::SetData(void* data, uint32_t size)
	{
		uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
		assert(size == m_Width * m_Height * bpp && "Data must be entire texture!");
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
	}

	void OGLTexture2D::Bind(uint32_t slot)
	{
		glBindTextureUnit(slot, m_RendererID);
	}

	bool OGLTexture2D::IsLoaded() const
	{
		return m_IsLoaded;
	}

	bool OGLTexture2D::operator==(const ITexture& other) const
	{
		return m_RendererID == other.GetRendererID();
	}
}