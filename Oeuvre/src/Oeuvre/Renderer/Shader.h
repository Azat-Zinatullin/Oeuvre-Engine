#pragma once

#include <memory>

namespace Oeuvre
{
	class Shader
	{
	public:
		virtual ~Shader() = default;
		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		static std::shared_ptr<Shader> Create(const std::string& filepath);
	};
}

