#pragma once

#include <d3d11.h>

namespace Oeuvre
{
	class DX11InputLayout
	{
	public:
		DX11InputLayout();
		virtual ~DX11InputLayout();
		bool Create(D3D11_INPUT_ELEMENT_DESC layout[], UINT numElements, ID3DBlob* vsBlob);
		void Bind();
		void Unbind();

		ID3D11InputLayout* GetLayout() { return m_inputLayout; }
	private:
		ID3D11InputLayout* m_inputLayout = nullptr;
	};
}

