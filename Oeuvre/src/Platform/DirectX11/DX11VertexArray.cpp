#include "DX11VertexArray.h"

#include "d3d11.h"

namespace Oeuvre
{
	void DX11VertexArray::Bind() const
	{
		m_VertexBuffer->Bind();
		m_IndexBuffer->Bind();
	}

	void DX11VertexArray::Unbind() const
	{
		m_VertexBuffer->Unbind();
		m_IndexBuffer->Unbind();
	}

	void DX11VertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
	{
		m_VertexBuffer = vertexBuffer;
	}

	void DX11VertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
	{
		m_IndexBuffer = indexBuffer;
	}

	const std::vector<std::shared_ptr<VertexBuffer>>& DX11VertexArray::GetVertexBuffers() const
	{
		return m_VertexBuffers;
	}

	const std::shared_ptr<VertexBuffer>& DX11VertexArray::GetVertexBuffer() const
	{
		return m_VertexBuffer;
	}

	const std::shared_ptr<IndexBuffer>& DX11VertexArray::GetIndexBuffer() const
	{
		return m_IndexBuffer;
	}
}


