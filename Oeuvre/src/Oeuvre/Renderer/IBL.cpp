#include "ovpch.h"
#include "IBL.h"
#include "Mesh.h"
#include <Platform/Windows/WindowsWindow.h>
#include <Platform/DirectX11/RenderTexture.h>
#include <DirectXTex.h>
#include <Platform/DirectX11/DX11RendererAPI.h>
#include "Oeuvre/Renderer/ConstantBuffers.h"
#include "Oeuvre/Renderer/ResourceManager.h"

namespace Oeuvre
{
	void IBL::Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, std::string hdriTexturePath)
	{
		D3D11_INPUT_ELEMENT_DESC skeletalAnimationInputLayout[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BONEIDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		m_RectToCubeVertexShader = std::make_shared<DX11VertexShader>("src/Shaders/IBL/RectToCubeShaders.hlsl", skeletalAnimationInputLayout, ARRAYSIZE(skeletalAnimationInputLayout));
		m_RectToCubePixelShader = std::make_shared<DX11PixelShader>("src/Shaders/IBL/RectToCubeShaders.hlsl");
		m_CubemapVertexShader = std::make_shared<DX11VertexShader>("src/Shaders/IBL/CubemapShaders.hlsl", skeletalAnimationInputLayout, ARRAYSIZE(skeletalAnimationInputLayout));
		m_CubemapPixelShader = std::make_shared<DX11PixelShader>("src/Shaders/IBL/CubemapShaders.hlsl");

		m_IrradianceVertexShader = std::make_shared<DX11VertexShader>("src/Shaders/IBL/IrradianceShaders.hlsl", skeletalAnimationInputLayout, ARRAYSIZE(skeletalAnimationInputLayout));
		m_IrradiancePixelShader = std::make_shared<DX11PixelShader>("src/Shaders/IBL/IrradianceShaders.hlsl");
		m_PrefilterVertexShader = std::make_shared<DX11VertexShader>("src/Shaders/IBL/PrefilterShaders.hlsl", skeletalAnimationInputLayout, ARRAYSIZE(skeletalAnimationInputLayout));
		m_PrefilterPixelShader = std::make_shared<DX11PixelShader>("src/Shaders/IBL/PrefilterShaders.hlsl");

		m_BRDFVertexShader = std::make_shared<DX11VertexShader>("src/Shaders/IBL/IntegrateBRDFShaders.hlsl", skeletalAnimationInputLayout, ARRAYSIZE(skeletalAnimationInputLayout));
		m_BRDFPixelShader = std::make_shared<DX11PixelShader>("src/Shaders/IBL/IntegrateBRDFShaders.hlsl");

		HRESULT hr;

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.ByteWidth = sizeof(PreFilterCB);
		hr = device->CreateBuffer(&bd, NULL, m_PreFilterCB.GetAddressOf());
		if (FAILED(hr)) std::cout << "Can't create m_PreFilterCB!\n";

		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(MatricesCB);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		hr = device->CreateBuffer(&bd, NULL, m_CBMatrixes.GetAddressOf());
		if (FAILED(hr)) std::cout << "Can't create g_pCBMatrixes\n";

		D3D11_RASTERIZER_DESC rdesc;
		ZeroMemory(&rdesc, sizeof(rdesc));
		rdesc.CullMode = D3D11_CULL_NONE;
		rdesc.FillMode = D3D11_FILL_SOLID;
		hr = device->CreateRasterizerState(&rdesc, m_HDRrasterizerState.GetAddressOf());
		if (FAILED(hr)) std::cout << "Can't create m_HDRrasterizerState!\n";

		D3D11_DEPTH_STENCIL_DESC dsdesc;
		ZeroMemory(&dsdesc, sizeof(dsdesc));
		dsdesc.DepthEnable = true;
		dsdesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		hr = device->CreateDepthStencilState(&dsdesc, m_HDRDepthStencilState.GetAddressOf());
		if (FAILED(hr)) std::cout << "Can't create m_HDRdepthStencilState!\n";

		RenderCubeFromTheInsideInit(device);

		LoadHDRITexture(device, hdriTexturePath);

		m_RectToCubeVertexShader->Bind();
		m_RectToCubePixelShader->Bind();
		deviceContext->PSSetShaderResources(0, 1, m_HDRIShaderResourceView.GetAddressOf());
		CreateCubemapTexture(device, deviceContext, false, 1024, 1024, m_CubemapShaderResourceView.GetAddressOf());

		m_IrradianceVertexShader->Bind();
		m_IrradiancePixelShader->Bind();
		deviceContext->PSSetShaderResources(0, 1, m_CubemapShaderResourceView.GetAddressOf());
		CreateCubemapTexture(device, deviceContext, true, 256, 256, m_IrradianceShaderResourceView.GetAddressOf());

		m_PrefilterVertexShader->Bind();
		m_PrefilterPixelShader->Bind();
		deviceContext->PSSetShaderResources(0, 1, m_CubemapShaderResourceView.GetAddressOf());
		CreatePrefilterCubemapTexture(device, deviceContext, true, 1024, 1024, m_PrefilterShaderResourceView.GetAddressOf());

		//m_BRDFTexture = ResourceManager::GetOrCreateTexture("../resources/textures/ibl_brdf_lut.png");

		m_BRDFVertexShader->Bind();
		m_BRDFPixelShader->Bind();
		CreateBRDFTexture(device, deviceContext);
	}

	void IBL::RenderCubeFromTheInsideInit(ID3D11Device* device)
	{
		Vertex vertices[] =
		{
			{ glm::vec3(-1.0f, 1.0f, -1.0f),  glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(1.0f, 1.0f, -1.0f),   glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(1.0f, 1.0f, 1.0f),    glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(-1.0f, 1.0f, 1.0f),   glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},

			{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(1.0f, -1.0f, -1.0f),  glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(1.0f, -1.0f, 1.0f),   glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(-1.0f, -1.0f, 1.0f),  glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},

			{ glm::vec3(-1.0f, -1.0f, 1.0f),  glm::vec2(0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(-1.0f, 1.0f, -1.0f),  glm::vec2(1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(-1.0f, 1.0f, 1.0f),   glm::vec2(0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},

			{ glm::vec3(1.0f, -1.0f, 1.0f),	  glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(1.0f, -1.0f, -1.0f),  glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(1.0f, 1.0f, -1.0f),	  glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(1.0f, 1.0f, 1.0f),	  glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},

			{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(1.0f, -1.0f, -1.0f),  glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(1.0f, 1.0f, -1.0f),	  glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(-1.0f, 1.0f, -1.0f),  glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},

			{ glm::vec3(-1.0f, -1.0f, 1.0f),  glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(1.0f, -1.0f, 1.0f),   glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(1.0f, 1.0f, 1.0f),    glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ glm::vec3(-1.0f, 1.0f, 1.0f),   glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		};

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(Vertex) * 24;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = vertices;

		HRESULT hr = device->CreateBuffer(&bd, &InitData, m_CubemapVertexBuffer.GetAddressOf());
		if (FAILED(hr)) return;

		UINT indices[] =
		{
			0,1,3,
			3,1,2,

			5,4,6,
			6,4,7,

			8,9,11,
			11,9,10,

			13,12,14,
			14,12,15,

			16,17,19,
			19,17,18,

			21,20,22,
			22,20,23
		};


		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(UINT) * 36;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;

		InitData.pSysMem = indices;

		hr = device->CreateBuffer(&bd, &InitData, m_CubemapIndexBuffer.GetAddressOf());
		if (FAILED(hr)) return;
	}

	void IBL::RenderCubeFromTheInside(ID3D11DeviceContext* deviceContext)
	{
		UINT stride = sizeof(Vertex);
		UINT offset = 0;

		deviceContext->IASetVertexBuffers(0, 1, m_CubemapVertexBuffer.GetAddressOf(), &stride, &offset);
		deviceContext->IASetIndexBuffer(m_CubemapIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		deviceContext->DrawIndexed(36, 0, 0);
	}

	void IBL::CreateCubemapTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext, bool upsideDown, int width, int height, ID3D11ShaderResourceView** cubeSRV)
	{
		glm::vec3 vectors[] =
		{
			glm::vec3(0.f, 0.f, 0.f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f),
			glm::vec3(0.f, 0.f, 0.f), glm::vec3(1.0f,  0.0f,  0.0f),  glm::vec3(0.0f, -1.0f,  0.0f),
			glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f,  -1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  -1.0f),
			glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f, 1.0f,  0.0f),   glm::vec3(0.0f,  0.0f, 1.0f),
			glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f,  0.0f,  1.0f),  glm::vec3(0.0f, -1.0f,  0.0f),
			glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f,  0.0f, -1.0f),  glm::vec3(0.0f, -1.0f,  0.0f)
		};

		if (upsideDown)
		{
			vectors[1] = glm::vec3(1.0f, 0.0f, 0.0f);	vectors[2] = glm::vec3(0.0f, 1.0f, 0.0f);
			vectors[4] = glm::vec3(-1.0f, 0.0f, 0.0f);	vectors[5] = glm::vec3(0.0f, 1.0f, 0.0f);
			vectors[7] = glm::vec3(0.0f, 1.0f, 0.0f);	vectors[8] = glm::vec3(0.0f, 0.0f, -1.0f);
			vectors[10] = glm::vec3(0.0f, -1.0f, 0.0f);	vectors[11] = glm::vec3(0.0f, 0.0f, 1.0f);
			vectors[13] = glm::vec3(0.0f, 0.0f, 1.0f);	vectors[14] = glm::vec3(0.0f, 1.0f, 0.0f);
			vectors[16] = glm::vec3(0.0f, 0.0f, -1.0f);	vectors[17] = glm::vec3(0.0f, 1.0f, 0.0f);
		}

		glm::mat4 captureViews[] =
		{
			glm::lookAtLH(vectors[0], vectors[1], vectors[2]),
			glm::lookAtLH(vectors[3], vectors[4], vectors[5]),
			glm::lookAtLH(vectors[6], vectors[7], vectors[8]),
			glm::lookAtLH(vectors[9], vectors[10], vectors[11]),
			glm::lookAtLH(vectors[12], vectors[13], vectors[14]),
			glm::lookAtLH(vectors[15], vectors[16], vectors[17])
		};

		deviceContext->OMSetDepthStencilState(m_HDRDepthStencilState.Get(), 1);
		deviceContext->RSSetState(m_HDRrasterizerState.Get());

		std::vector<RenderTexture*> cubeFaces;

		// Create faces
		for (int i = 0; i < 6; ++i)
		{
			RenderTexture* renderTexture = new RenderTexture;
			renderTexture->Initialize(width, height, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R16G16B16A16_FLOAT, false, false, 0);
			cubeFaces.push_back(renderTexture);
		}

		for (int i = 0; i < 6; i++)
		{
			RenderTexture* texture = cubeFaces[i];
			// clear		
			RendererAPI::GetInstance()->OnWindowResize(width, height);
			texture->SetRenderTarget(false, false, false, nullptr);
			texture->ClearRenderTarget(0.0f, 0.0f, 0.0f, 1.0f, 1.0f);

			MatricesCB matricesCb;
			matricesCb.viewMat = captureViews[i];
			matricesCb.projMat = glm::perspectiveFovLH_ZO(glm::radians(90.f), (float)width, (float)height, 0.1f, 10.f);
			matricesCb.modelMat = glm::mat4(1.f);

			deviceContext->UpdateSubresource(m_CBMatrixes.Get(), 0, nullptr, &matricesCb, 0, 0);

			deviceContext->VSSetConstantBuffers(0, 1, m_CBMatrixes.GetAddressOf());
			deviceContext->PSSetConstantBuffers(0, 1, m_CBMatrixes.GetAddressOf());

			// draw cube
			RenderCubeFromTheInside(deviceContext);
		}

		ID3D11Texture2D* cubeTexture = nullptr;

		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 6;
		texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		HRESULT result = device->CreateTexture2D(&texDesc, nullptr, &cubeTexture);
		if (FAILED(result))
		{
			std::cout << "Can't create cubeTexture!\n";
			return;
		}

		D3D11_BOX sourceRegion;
		for (int i = 0; i < 6; ++i)
		{
			RenderTexture* texture = cubeFaces[i];

			sourceRegion.left = 0;
			sourceRegion.right = width;
			sourceRegion.top = 0;
			sourceRegion.bottom = height;
			sourceRegion.front = 0;
			sourceRegion.back = 1;

			deviceContext->CopySubresourceRegion(cubeTexture, D3D11CalcSubresource(0, i, 1), 0, 0, 0, texture->GetTexture(),
				0, &sourceRegion);
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = texDesc.MipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;

		result = device->CreateShaderResourceView(cubeTexture, &srvDesc, cubeSRV);
		if (FAILED(result))
		{
			std::cout << "Can't create SRV for cube texture!\n";
			return;
		}

		for (RenderTexture* cubeFace : cubeFaces)
			delete cubeFace;

		deviceContext->OMSetDepthStencilState(nullptr, 0);
		deviceContext->RSSetState(nullptr);
		((DX11RendererAPI*)RendererAPI::GetInstance().get())->ResetRenderTargetView();
		RECT rect;
		GetClientRect(WindowsWindow::GetInstance()->GetHWND(), &rect);
		RendererAPI::GetInstance()->OnWindowResize(rect.right - rect.left, rect.bottom - rect.top);
	}

	void IBL::CreatePrefilterCubemapTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext, bool upsideDown, int width, int height, ID3D11ShaderResourceView** cubeSRV)
	{
		ID3D11Texture2D* cubeTexture = nullptr;

		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = 5;
		texDesc.ArraySize = 6;
		texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		HRESULT result = device->CreateTexture2D(&texDesc, nullptr, &cubeTexture);
		if (FAILED(result)) return;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = texDesc.MipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;

		result = device->CreateShaderResourceView(cubeTexture, &srvDesc, cubeSRV);
		if (FAILED(result)) return;

		glm::vec3 vectors[] =
		{
			glm::vec3(0.f, 0.f, 0.f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f),
			glm::vec3(0.f, 0.f, 0.f), glm::vec3(1.0f,  0.0f,  0.0f),  glm::vec3(0.0f, -1.0f,  0.0f),
			glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f,  -1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  -1.0f),
			glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f, 1.0f,  0.0f),   glm::vec3(0.0f,  0.0f, 1.0f),
			glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f,  0.0f,  1.0f),  glm::vec3(0.0f, -1.0f,  0.0f),
			glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f,  0.0f, -1.0f),  glm::vec3(0.0f, -1.0f,  0.0f)
		};

		if (upsideDown)
		{
			vectors[1] = glm::vec3(1.0f, 0.0f, 0.0f);	vectors[2] = glm::vec3(0.0f, 1.0f, 0.0f);
			vectors[4] = glm::vec3(-1.0f, 0.0f, 0.0f);	vectors[5] = glm::vec3(0.0f, 1.0f, 0.0f);
			vectors[7] = glm::vec3(0.0f, 1.0f, 0.0f);	vectors[8] = glm::vec3(0.0f, 0.0f, -1.0f);
			vectors[10] = glm::vec3(0.0f, -1.0f, 0.0f);	vectors[11] = glm::vec3(0.0f, 0.0f, 1.0f);
			vectors[13] = glm::vec3(0.0f, 0.0f, 1.0f);	vectors[14] = glm::vec3(0.0f, 1.0f, 0.0f);
			vectors[16] = glm::vec3(0.0f, 0.0f, -1.0f);	vectors[17] = glm::vec3(0.0f, 1.0f, 0.0f);
		}

		glm::mat4 captureViews[] =
		{
			glm::lookAtLH(vectors[0], vectors[1], vectors[2]),
			glm::lookAtLH(vectors[3], vectors[4], vectors[5]),
			glm::lookAtLH(vectors[6], vectors[7], vectors[8]),
			glm::lookAtLH(vectors[9], vectors[10], vectors[11]),
			glm::lookAtLH(vectors[12], vectors[13], vectors[14]),
			glm::lookAtLH(vectors[15], vectors[16], vectors[17])
		};

		deviceContext->OMSetDepthStencilState(m_HDRDepthStencilState.Get(), 1);
		deviceContext->RSSetState(m_HDRrasterizerState.Get());

		std::vector<RenderTexture*> cubeFaces;

		// Create faces
		cubeFaces.resize(6);

		unsigned int maxMipLevels = 5;
		for (int mip = 0; mip < maxMipLevels; ++mip)
		{
			int preFilterSize = width;
			const unsigned int mipWidth = unsigned int(preFilterSize * pow(0.5, mip));
			const unsigned int mipHeight = unsigned int(preFilterSize * pow(0.5, mip));


			for (int i = 0; i < 6; ++i)
			{
				RenderTexture* renderTexture = new RenderTexture;
				renderTexture->Initialize(mipWidth, mipHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R16G16B16A16_FLOAT, false, false, 0);
				cubeFaces[i] = renderTexture;
			}

			for (int i = 0; i < 6; i++)
			{
				RenderTexture* texture = cubeFaces[i];
				// clear		
				RendererAPI::GetInstance()->OnWindowResize(mipWidth, mipHeight);
				texture->SetRenderTarget(false, false, false, nullptr);
				texture->ClearRenderTarget(0.0f, 0.0f, 0.0f, 1.0f, 1.0f);

				// prepare
				const float roughness = float(mip) / 4.0f;
				PreFilterCB pCb;
				pCb.mWorld = glm::mat4(1.f);
				pCb.mView = captureViews[i];
				pCb.mProjection = glm::perspectiveFovLH_ZO(glm::radians(90.f), (float)mipWidth, (float)mipHeight, 0.1f, 10.f);
				pCb.mCustomData = glm::vec4(roughness, 0.f, 0.f, 0.f);

				deviceContext->UpdateSubresource(m_PreFilterCB.Get(), 0, nullptr, &pCb, 0, 0);

				deviceContext->VSSetConstantBuffers(0, 1, m_PreFilterCB.GetAddressOf());
				deviceContext->PSSetConstantBuffers(0, 1, m_PreFilterCB.GetAddressOf());

				// draw cube
				RenderCubeFromTheInside(deviceContext);
			}

			D3D11_BOX sourceRegion;
			for (int i = 0; i < 6; ++i)
			{
				RenderTexture* texture = cubeFaces[i];

				sourceRegion.left = 0;
				sourceRegion.right = mipWidth;
				sourceRegion.top = 0;
				sourceRegion.bottom = mipHeight;
				sourceRegion.front = 0;
				sourceRegion.back = 1;

				deviceContext->CopySubresourceRegion(cubeTexture, D3D11CalcSubresource(mip, i, 5), 0, 0, 0, texture->GetTexture(),
					0, &sourceRegion);
			}
		}

		for (int i = 0; i < 6; ++i)
			delete cubeFaces[i];

		deviceContext->OMSetDepthStencilState(nullptr, 0);
		deviceContext->RSSetState(nullptr);
		((DX11RendererAPI*)RendererAPI::GetInstance().get())->ResetRenderTargetView();
		RECT rect;
		GetClientRect(WindowsWindow::GetInstance()->GetHWND(), &rect);
		RendererAPI::GetInstance()->OnWindowResize(rect.right - rect.left, rect.bottom - rect.top);
	}

	void IBL::CreateBRDFTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
	{
		int width = 512, height = 512;
		RenderTexture* texture = new RenderTexture;
		texture->Initialize(width, height, 1, DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R16G16_FLOAT, false, false, 0);
		// clear		
		RendererAPI::GetInstance()->OnWindowResize(width, height);
		texture->SetRenderTarget(false, true, false, nullptr);
		texture->ClearRenderTarget(0.0f, 0.0f, 0.0f, 1.0f, 1.0f);

		deviceContext->OMSetDepthStencilState(m_HDRDepthStencilState.Get(), 1);
		deviceContext->RSSetState(m_HDRrasterizerState.Get());

		glm::vec3 vectors[] =
		{
			glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, 1.0f, 0.0f),
		};
		glm::mat4 viewMatrix = glm::lookAtLH(vectors[0], vectors[1], vectors[2]);
		glm::mat4 projMatrix = glm::perspectiveFovLH_ZO(glm::radians(90.f), (float)width, (float)height, 0.1f, 100.f);

		// prepare
		PreFilterCB pCb;
		pCb.mWorld = glm::mat4(1.f);
		pCb.mView = viewMatrix;
		pCb.mProjection = projMatrix;

		deviceContext->UpdateSubresource(m_PreFilterCB.Get(), 0, nullptr, &pCb, 0, 0);

		deviceContext->VSSetConstantBuffers(0, 1, m_PreFilterCB.GetAddressOf());
		deviceContext->PSSetConstantBuffers(0, 1, m_PreFilterCB.GetAddressOf());

		// draw cube
		RenderCubeFromTheInside(deviceContext);

		// copy result to shader resource view
		D3D11_TEXTURE2D_DESC textureDesc;
		ZeroMemory(&textureDesc, sizeof(textureDesc));
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		HRESULT result = device->CreateTexture2D(&textureDesc, nullptr, m_d3dBRDFTexture.GetAddressOf());
		if (FAILED(result)) return;

		ID3D11Resource* srcResource;
		ID3D11Texture2D* srcTex;
		texture->GetSRV()->GetResource(&srcResource);
		srcTex = (ID3D11Texture2D*)srcResource;
		deviceContext->CopyResource(m_d3dBRDFTexture.Get(), srcTex);

		if (m_d3dBRDFTexture.Get())
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
			shaderResourceViewDesc.Format = textureDesc.Format;
			shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
			shaderResourceViewDesc.Texture2D.MipLevels = 1;
			device->CreateShaderResourceView(m_d3dBRDFTexture.Get(), &shaderResourceViewDesc, m_d3dBRDFTextureSRV.GetAddressOf());
		}
		delete texture;

		deviceContext->OMSetDepthStencilState(nullptr, 0);
		deviceContext->RSSetState(nullptr);
		((DX11RendererAPI*)RendererAPI::GetInstance().get())->ResetRenderTargetView();
		RECT rect;
		GetClientRect(WindowsWindow::GetInstance()->GetHWND(), &rect);
		RendererAPI::GetInstance()->OnWindowResize(rect.right - rect.left, rect.bottom - rect.top);
	}

	void IBL::LoadHDRITexture(ID3D11Device* device, std::string path)
	{
		HRESULT hr;
		auto image = std::make_unique<ScratchImage>();
		std::wstring wPath{ path.begin(), path.end() };
		hr = LoadFromHDRFile(wPath.c_str(), nullptr, *image);
		if (FAILED(hr))
		{
			std::cout << "Can't load HDR texture\n";
			return;
		}
		
		auto convertedImage = std::make_unique<ScratchImage>();
		hr = Convert(*image->GetImage(0, 0, 0), DXGI_FORMAT_R16G16B16A16_FLOAT, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, *convertedImage);
		if (FAILED(hr))
		{
			std::cout << "Can't convert HDR image to DXGI_FORMAT_R16G16B16A16_FLOAT\n";
			return;
		}

		hr = CreateShaderResourceView(device, convertedImage->GetImages(), convertedImage->GetImageCount(),
			convertedImage->GetMetadata(), &m_HDRIShaderResourceView);
		if (FAILED(hr))
		{
			std::cout << "Can't create SRV from HDR texture\n";
			return;
		}
	}
}