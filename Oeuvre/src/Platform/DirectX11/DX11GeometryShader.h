#pragma once

#include "Oeuvre/Renderer/Shader.h"
#include "Oeuvre/Renderer/Renderer.h"
#include <d3d11.h>

namespace Oeuvre
{
	class DX11GeometryShader : public Shader
	{
	public:
		DX11GeometryShader(const char* filePath);
		virtual ~DX11GeometryShader();
		bool Create(const char* path);
		void Bind() override;
		void Unbind() override;
	private:
		ID3D11GeometryShader* m_GeometryShader = nullptr;
	};
}