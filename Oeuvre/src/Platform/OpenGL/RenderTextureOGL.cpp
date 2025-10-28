#include "ovpch.h"
#include "RenderTextureOGL.h"

namespace Oeuvre
{
	RenderTextureOGL::RenderTextureOGL()
	{
		glGenFramebuffers(1, &m_Framebuffer);		
	}

	RenderTextureOGL::~RenderTextureOGL()
	{
		glDeleteRenderbuffers(1, &m_Rbo);
		glDeleteTextures(1, &m_TextureColorbuffer);
		glDeleteFramebuffers(1, &m_Framebuffer);
	}

	bool RenderTextureOGL::Initialize(int width, int height, int mipMaps, GLenum internalTextureFormat, GLenum dataTextureFormat)
	{	
		m_MipMaps = mipMaps;
		m_TexureInternalFormat = internalTextureFormat;
		m_TexureDataFormat = dataTextureFormat;

		glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

		// generate texture		
		glGenTextures(1, &m_TextureColorbuffer);
		glBindTexture(GL_TEXTURE_2D, m_TextureColorbuffer);
		glTexImage2D(GL_TEXTURE_2D, mipMaps, internalTextureFormat, width, height, 0, dataTextureFormat, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		// attach it to currently bound framebuffer object
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_TextureColorbuffer, 0);

		glGenRenderbuffers(1, &m_Rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, m_Rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_Rbo);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";
			return false;
		}
			
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		return true;
	}

	void RenderTextureOGL::Resize(int width, int height)
	{
		if (width <= 0 || height <= 0)
			return;

		glDeleteTextures(1, &m_TextureColorbuffer);
		glDeleteRenderbuffers(1, &m_Rbo);

		Initialize(width, height, m_MipMaps, m_TexureInternalFormat, m_TexureDataFormat);
		glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
	}

	void RenderTextureOGL::SetRenderTarget(bool rtvToNull, bool depthToNull) const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

		if (!rtvToNull)
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		else
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		if (!depthToNull)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
	}

	void RenderTextureOGL::ClearRenderTarget(float red, float green, float blue, float alpha, float depth) const
	{
		glClearColor(red, green, blue, alpha);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
}