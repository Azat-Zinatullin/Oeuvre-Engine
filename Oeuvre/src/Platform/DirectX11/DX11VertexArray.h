#pragma once

#include "Oeuvre/Renderer/VertexArray.h"

namespace Oeuvre
{
	class DX11VertexArray : public VertexArray
	{
	public:
		virtual ~DX11VertexArray() = default;

		virtual void Bind() const;
		virtual void Unbind() const;

		virtual void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer);
		virtual void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer);

		virtual const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const;
		virtual const std::shared_ptr<VertexBuffer>& GetVertexBuffer() const;
		virtual const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const;

	private:
		std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffers;
		std::shared_ptr<VertexBuffer> m_VertexBuffer;
		std::shared_ptr<IndexBuffer> m_IndexBuffer;
	};
}