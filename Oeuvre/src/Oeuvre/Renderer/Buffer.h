#pragma once
#include <cstdint>
#include <memory>

namespace Oeuvre
{
	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		static std::shared_ptr<VertexBuffer> Create(float* vertices, uint32_t size, uint32_t stride);
	};

	class IndexBuffer
	{
	public:
		virtual ~IndexBuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		virtual uint32_t GetCount() = 0;

		static std::shared_ptr<IndexBuffer> Create(unsigned int* indices, uint32_t count);
	};
}