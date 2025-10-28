#pragma once

#include "Oeuvre/Renderer/Buffer.h"
#include "d3d11.h"

namespace Oeuvre
{
	class DX11VertexBuffer : public VertexBuffer
	{
	public:
		DX11VertexBuffer(float* vertices, uint32_t size, uint32_t stride);
		virtual ~DX11VertexBuffer();

		virtual void Bind();
		virtual void Unbind();

	private:
		uint32_t m_Stride;
		ID3D11Buffer* m_VertexBuffer = nullptr;
	};

	class DX11IndexBuffer : public IndexBuffer
	{
	public:
		DX11IndexBuffer(unsigned int* indices, uint32_t count);
		virtual ~DX11IndexBuffer();

		virtual void Bind();
		virtual void Unbind();

		virtual uint32_t GetCount() { return m_Count; }
	private:
		int32_t m_Count;
		ID3D11Buffer* m_IndexBuffer = nullptr;
	};

}