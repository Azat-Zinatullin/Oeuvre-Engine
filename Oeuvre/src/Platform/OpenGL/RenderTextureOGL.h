#pragma once
#include "glad/glad.h"

namespace Oeuvre
{
	class RenderTextureOGL
	{
	public:
		RenderTextureOGL();
		~RenderTextureOGL();

		bool Initialize(int width, int height, int mipMaps, GLenum internalTextureFormat, GLenum dataTextureFormat);

		void Resize(int width, int height);

		void SetRenderTarget(bool rtvToNull, bool depthToNull) const;
		void ClearRenderTarget(float red, float green, float blue, float alpha, float depth) const;

		unsigned int GetTexture() { return m_TextureColorbuffer; }

	private:
		unsigned int m_Framebuffer = 0;
		unsigned int m_TextureColorbuffer = 0;
		unsigned int m_Rbo = 0;

		unsigned m_MipMaps = 0;
		GLenum m_TexureInternalFormat = 0;
		GLenum m_TexureDataFormat = 0;
	};
}
