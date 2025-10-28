#pragma once
#include "RHI/ITexture.h"
#include <Platform/DirectX11/DX11VertexShader.h>
#include <Platform/DirectX11/DX11PixelShader.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

namespace Oeuvre
{
	struct  __declspec(align(16)) PreFilterCB
	{
		glm::mat4 mWorld;
		glm::mat4 mView;
		glm::mat4 mProjection;
		glm::vec4 mCustomData;
	};

	class IBL
	{
	public:
		void Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, std::string hdrTexturePath);

		void RenderCubeFromTheInsideInit(ID3D11Device* device);
		void RenderCubeFromTheInside(ID3D11DeviceContext* deviceContext);
		void CreateCubemapTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext, bool upsideDown, int width, int height, ID3D11ShaderResourceView** cubeSRV);
		void CreatePrefilterCubemapTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext, bool upsideDown, int width, int height, ID3D11ShaderResourceView** cubeSRV);
		void CreateBRDFTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext);

		void LoadHDRITexture(ID3D11Device* device, std::string path);
		void RenderHDRCubemap();

		ComPtr<ID3D11Buffer> m_CBMatrixes = nullptr;

		ComPtr<ID3D11Buffer> m_CubemapVertexBuffer = nullptr;
		ComPtr<ID3D11Buffer> m_CubemapIndexBuffer = nullptr;

		std::shared_ptr<DX11VertexShader> m_RectToCubeVertexShader = nullptr;
		std::shared_ptr<DX11PixelShader> m_RectToCubePixelShader = nullptr;
		std::shared_ptr<DX11VertexShader> m_CubemapVertexShader = nullptr;
		std::shared_ptr<DX11PixelShader> m_CubemapPixelShader = nullptr;

		std::shared_ptr<DX11VertexShader> m_IrradianceVertexShader = nullptr;
		std::shared_ptr<DX11PixelShader> m_IrradiancePixelShader = nullptr;
		std::shared_ptr<DX11VertexShader> m_PrefilterVertexShader = nullptr;
		std::shared_ptr<DX11PixelShader> m_PrefilterPixelShader = nullptr;

		ComPtr<ID3D11Buffer> m_PreFilterCB;

		std::shared_ptr<DX11VertexShader> m_BRDFVertexShader = nullptr;
		std::shared_ptr<DX11PixelShader> m_BRDFPixelShader = nullptr;

		std::shared_ptr<ITexture> m_BRDFTexture = nullptr;
		ComPtr<ID3D11Texture2D> m_d3dBRDFTexture = nullptr;
		ComPtr<ID3D11ShaderResourceView> m_d3dBRDFTextureSRV = nullptr;

		ComPtr<ID3D11ShaderResourceView> m_HDRIShaderResourceView = nullptr;
		ComPtr<ID3D11DepthStencilState> m_CubemapDepthStencilState = nullptr;
		ComPtr<ID3D11RasterizerState> m_CubemapRasterizerState = nullptr;
		ComPtr<ID3D11DepthStencilState> m_HDRDepthStencilState = nullptr;
		ComPtr<ID3D11RasterizerState> m_HDRrasterizerState = nullptr;
		ComPtr<ID3D11Texture2D> m_CubemapTexture = nullptr;
		ComPtr<ID3D11ShaderResourceView> m_CubemapShaderResourceView = nullptr;

		ComPtr<ID3D11ShaderResourceView> m_IrradianceShaderResourceView = nullptr;
		ComPtr<ID3D11ShaderResourceView> m_PrefilterShaderResourceView = nullptr;
	};
}