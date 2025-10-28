#pragma once
#include "Oeuvre/Renderer/Shader.h"
#include <glm/glm.hpp>

typedef unsigned int GLenum;
typedef unsigned int GLuint;

namespace Oeuvre 
{
	class OGLShader : public Shader
	{
	public:
		OGLShader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);
		virtual ~OGLShader();

		virtual void Bind() override;
		virtual void Unbind() override;

		virtual void SetInt(const std::string& name, int value);
		virtual void SetFloat(const std::string& name, float value);
		virtual void SetFloat2(const std::string& name, const glm::vec2& value);
		virtual void SetFloat3(const std::string& name, const glm::vec3& value);
		virtual void SetFloat4(const std::string& name, const glm::vec4& value);
		virtual void SetMat3(const std::string& name, const glm::mat3& value);
		virtual void SetMat4(const std::string& name, const glm::mat4& value);

	private:
		void checkCompileErrors(GLuint shader, std::string type);

		uint32_t m_RendererID;
	};

}
