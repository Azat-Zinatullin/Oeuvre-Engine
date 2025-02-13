#include "TestApplication.h"
#include "Oeuvre/Events/EventHandler.h"
#include <iostream>

#include "Oeuvre/Core/EntryPoint.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_dx11.h"
#include "misc/cpp/imgui_stdlib.h"

#include "GLFW/glfw3.h"

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "Platform/DirectX11/DX11RendererAPI.h"
#include "Oeuvre/Renderer/Texture.h"

#include <windowsx.h>
#include <wrl\wrappers\corewrappers.h>
#include <shobjidl.h> 

#include <DirectXTex.h>

#include "DirectXMath.h"

#include <wrl/client.h>

#include <fstream>
#include <sstream>

#include <Platform/DirectX11/DX11Texture2D.h>
#include "Oeuvre/Renderer/Frustum.h"

#define CHECK_STATUS(status) if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';

TestApplication::TestApplication()
{
	m_Window = WindowsWindow::GetInstance();
	m_Renderer = std::make_unique<Renderer>();

	ADD_WINDOW_EVENT_LISTENER(WindowEvents::WindowResize, TestApplication::OnWindowEvent, this);
	ADD_WINDOW_EVENT_LISTENER(WindowEvents::WindowClose, TestApplication::OnWindowEvent, this);

	ADD_MOUSE_EVENT_LISTENER(MouseEvents::MouseMoved, TestApplication::OnMouseEvent, this);
	ADD_MOUSE_EVENT_LISTENER(MouseEvents::MouseButtonDown, TestApplication::OnMouseEvent, this);
	size_t mouseButtonDownListenerId = ADD_MOUSE_EVENT_LISTENER(MouseEvents::MouseButtonUp, TestApplication::OnMouseEvent, this);
	//REMOVE_MOUSE_EVENT_LISTENER(MouseEvents::MouseButtonUp, mouseButtonDownListenerId);
	ADD_MOUSE_EVENT_LISTENER(MouseEvents::MouseScroll, TestApplication::OnMouseEvent, this);

	ADD_KEY_EVENT_LISTENER(KeyEvents::KeyDown, TestApplication::OnKeyEvent, this);
	ADD_KEY_EVENT_LISTENER(KeyEvents::KeyUp, TestApplication::OnKeyEvent, this);
}

std::unique_ptr<TestApplication> TestApplication::Create()
{
	return std::make_unique<TestApplication>();
}

struct __declspec(align(16)) ConstantBuffer
{
	glm::mat4 modelMat;
	glm::mat4  viewMat;
	glm::mat4  projMat;
	glm::mat4  normalMat;
	glm::mat4 viewProjMatInv;
};

struct __declspec(align(16)) LightCB
{
	glm::mat4 lightViewProjMat;
	glm::mat4 cubeView[6];
	glm::mat4 cubeProj;
	glm::vec4 lightPos[NUM_POINT_LIGHTS + 1];
	glm::vec4 conLinQuad;
	glm::vec3 lightColor;
	float bias;
	glm::vec3 camPos;
	int showDiffuseTexture;
	int numLights;
	int enableGI;
};

struct __declspec(align(16)) MaterialCB
{
	float sponza;
	float normalStrength;
	int renderLightCube;
};

bool TestApplication::Init()
{
	m_DX11Device = ((DX11RendererAPI*)RendererAPI::GetInstance().get())->GetDevice();
	m_DX11DeviceContext = ((DX11RendererAPI*)RendererAPI::GetInstance().get())->GetDeviceContext();

	//_crtBreakAlloc = 577569;

	m_Renderer->Init();

	ImguiInit();

	std::string prefix = "../resources/colt_python/";
	Model* model = new Model(prefix + "source/revolver_game.fbx", prefix + "textures/M_WP_Revolver_albedo.jpg", prefix + "textures/M_WP_Revolver_normal.png", prefix + "textures/M_WP_Revolver_roughness.jpg", prefix + "textures/M_WP_Revolver_metallic.jpg");
	models.emplace_back(model);
	models.emplace_back(new Model("..\\resources\\sponza\\glTF\\Sponza.gltf", "", "", "", ""));
	//models.emplace_back(new Model("E:\\Development\\Projects\\C_C++\\DirectX11\\resources\\Bistro_v5_2\\BistroInterior.fbx", "", "", "", ""));

	// Send all reports to STDOUT
	//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	//_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	//_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	//_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	//_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	//_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
	//_CrtDumpMemoryLeaks();

	props.emplace_back(Properties());
	props.emplace_back(Properties());
	props[0].scale[0] = props[0].scale[1] = props[0].scale[2] = 5.f;
	props[0].translation[1] = props[0].translation[0] = 2.5f;
	props[0].rotation[0] = -90.f;
	props[1].scale[0] = props[1].scale[1] = props[1].scale[2] = SPONZA_SCALE;
	props[1].rotation[1] = 90.f;
	models[1]->GetUseCombinedTextures() = false;


	// for sponza
	lightProps.lightPos[0] = { 0.f, 21.f, 0.f };
	lightProps.lightPos[1] = { 30.f, 4.8f, -2.95f };
	lightProps.lightPos[2] = { -30.f, 3.6f, 19.5f };

	// for bistro interior
	//props[1].rotation[0] = -90.f;
	//lightProps.lightPos[0][0] = 25.f;
	//lightProps.lightPos[0][1] = 15.7f;
	//lightProps.lightPos[0][2] = -2.5f;

	// many pistols
	//for (int i = 0; i < 48; i++)
	//{
	//	std::string prefix = "E:\\Development\\Projects\\C_C++\\DirectX11\\resources\\cerberus\\textures\\";
	//	models.emplace_back(new Model("E:\\Development\\Projects\\C_C++\\DirectX11\\resources\\cerberus\\Cerberus_LP.FBX", prefix + "Cerberus_A.jpg", prefix + "Cerberus_N.jpg", prefix + "Cerberus_R.jpg", prefix + "Cerberus_M.jpg"));
	//	props.emplace_back(Properties());
	//	props[2 + i].scale[0] = props[2 + i].scale[1] = props[2 + i].scale[2] = 0.02f;
	//	props[2 + i].rotation[0] = -90.f;
	//	props[2 + i].translation[1] = 5.f;
	//	props[2 + i].translation[0] = -30.f + 5.f * (i % 16);
	//	props[2 + i].translation[2] = -10.f + 10.f * i / 48.f;
	//}


	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	HRESULT hr = m_DX11Device->CreateBuffer(&bd, NULL, &g_pCBMatrixes);
	if (FAILED(hr)) std::cout << "Can't create g_pCBMatrixes\n";

	bd.ByteWidth = sizeof(LightCB);
	hr = m_DX11Device->CreateBuffer(&bd, NULL, &g_pCBLight);
	if (FAILED(hr)) std::cout << "Can't create g_pCBLight\n";

	bd.ByteWidth = sizeof(MaterialCB);
	hr = m_DX11Device->CreateBuffer(&bd, NULL, &g_pCBMaterial);
	if (FAILED(hr)) std::cout << "Can't create g_pCBMaterial\n";

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	//sampDesc.MaxAnisotropy = 16;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = m_DX11Device->CreateSamplerState(&sampDesc, &g_pSamplerLinear);

	FramebufferToTextureInit();


	D3D11_INPUT_ELEMENT_DESC defaultShaderInputLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	m_DefaultVertexShader = std::make_shared<DX11VertexShader>("src/DefaultVertexShader.hlsl", defaultShaderInputLayout, ARRAYSIZE(defaultShaderInputLayout));

	m_SpotLightDepthVertexShader = std::make_shared<DX11VertexShader>("src/SpotLightDepthVertexShader.hlsl", defaultShaderInputLayout, ARRAYSIZE(defaultShaderInputLayout));
	//m_SpotLightDepthPixelShader = std::make_shared<DX11PixelShader>("src/SpotLightDepthPixelShader.hlsl");

	m_SpotLightVertexShader = std::make_shared<DX11VertexShader>("src/SpotLightVertexShader.hlsl", defaultShaderInputLayout, ARRAYSIZE(defaultShaderInputLayout));
	m_SpotLightPixelShader = std::make_shared<DX11PixelShader>("src/SpotLightPixelShader.hlsl");

	m_PointLightDepthVertexShader = std::make_shared<DX11VertexShader>("src/PointLightDepthVertexShader.hlsl", defaultShaderInputLayout, ARRAYSIZE(defaultShaderInputLayout));
	m_PointLightDepthPixelShader = std::make_shared<DX11PixelShader>("src/PointLightDepthPixelShader.hlsl");
	m_PointLightDepthGeometryShader = std::make_shared<DX11GeometryShader>("src/PointLightDepthGeometryShader.hlsl");

	m_CubePixelShader = std::make_shared<DX11PixelShader>("src/CubePixelShader.hlsl");

	m_DeferredVertexShader = std::make_shared<DX11VertexShader>("src/DeferredVertexShader.hlsl", defaultShaderInputLayout, ARRAYSIZE(defaultShaderInputLayout));
	m_DeferredPixelShader = std::make_shared<DX11PixelShader>("src/DeferredPixelShader.hlsl");

	m_DeferredCompositingVertexShader = std::make_shared<DX11VertexShader>("src/DeferredCompositingVertexShader.hlsl", defaultShaderInputLayout, ARRAYSIZE(defaultShaderInputLayout));
	m_DeferredCompositingPixelShader = std::make_shared<DX11PixelShader>("src/DeferredCompositingPixelShader.hlsl");

	m_VoxelizationPixelShader = std::make_shared<DX11PixelShader>("src/VoxelizationPixelShader.hlsl");

	SpotLightDepthToTextureInit();
	for (int i = 0; i < NUM_POINT_LIGHTS; i++)
	{
		m_PointLightDepthSRVs[i] = nullptr;
		m_PointLightDepthTextures[i] = nullptr;
		m_PointLightDepthRenderTextures[i] = nullptr;
	}
	PointLightDepthToTextureInit();

	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MaxAnisotropy = 16;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = m_DX11Device->CreateSamplerState(&sampDesc, &g_pSamplerClamp);

	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.BorderColor[0] = 1.f;
	sampDesc.BorderColor[1] = 1.f;
	sampDesc.BorderColor[2] = 1.f;
	sampDesc.BorderColor[3] = 1.f;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = m_DX11Device->CreateSamplerState(&sampDesc, &g_pSamplerComparison);

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER;
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	const D3D11_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D11_STENCIL_OP_KEEP,
	  D3D11_STENCIL_OP_KEEP,
	  D3D11_STENCIL_OP_KEEP,
	  D3D11_COMPARISON_ALWAYS };

	depthStencilDesc.FrontFace = defaultStencilOp;
	depthStencilDesc.BackFace = defaultStencilOp;

	hr = m_DX11Device->CreateDepthStencilState(&depthStencilDesc, &m_pGBufferReverseZDepthStencilState);

	cameras.emplace_back(Camera()); // main camera 
	// light cameras and prev positions
	for (int i = 0; i < NUM_POINT_LIGHTS; i++)
	{
		cameras.emplace_back(Camera());
		lightPositionsPrev.emplace_back(glm::vec3(0.f));
	}
	cameras.emplace_back(Camera()); // for spotlight


	gBuffer = new DX11GBuffer();
	gBuffer->Initialize(texWidth, texHeight);

	NvidiaVXGIInit();
	RenderCubeInit();
	RenderQuadInit();

	initPhysics(true);

	auto coltBounds = models[0]->GetBounds();
	std::cout << "Colt Python bounds: (" <<
		coltBounds.lower.x << ", " << coltBounds.lower.y << ", " << coltBounds.lower.z
		<< " | " << coltBounds.upper.x << ", " << coltBounds.upper.y << ", " << coltBounds.upper.z << ")\n";


	m_BoxAlbedo = Texture2D::Create("..\\resources\\wood_planks_4k\\wood_planks_diff_4k.png");
	m_BoxNormal = Texture2D::Create("..\\resources\\wood_planks_4k\\wood_planks_nor_dx_4k.png");
	m_BoxRoughness = Texture2D::Create("..\\resources\\wood_planks_4k\\wood_planks_rough_4k.png");

	return true;
}

void TestApplication::RenderLoop()
{
	while (m_Running)
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_W) == GLFW_PRESS)
			cameras[SelectedCamera].ProcessKeyboard(CameraMovement::FORWARD, deltaTime);
		if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_S) == GLFW_PRESS)
			cameras[SelectedCamera].ProcessKeyboard(CameraMovement::BACKWARD, deltaTime);
		if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_A) == GLFW_PRESS)
			cameras[SelectedCamera].ProcessKeyboard(CameraMovement::LEFT, deltaTime);
		if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_D) == GLFW_PRESS)
			cameras[SelectedCamera].ProcessKeyboard(CameraMovement::RIGHT, deltaTime);

		if (viewPortActive)
			glfwSetInputMode(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		//for (int i = 0; i < 48; i++)
		//{
		//	props[2 + i].rotation[2] += glfwGetTime() * deltaTime * 5.f;
		//}

		//lightProps.lightPos[0][2] = glm::cos(glfwGetTime()/2.f) * 8.f;

		m_Window->OnUpdate();

		ImguiFrame();
		SetupImguiDockspace();

		ImGui::Begin("Properties");
		ImGui::Checkbox("VSync", &m_bEnableVsync);
		const char* items[]{ "Main Camera", "Light1", "Light2", "Light3", "SpotLight" };
		if (ImGui::Combo("Camera", &SelectedCamera, items, 5))
		{
		}
		ImGui::SliderFloat("Cam Speed", &cameras[SelectedCamera].GetMovementSpeed(), 1.f, 150.f);
		ImGui::SliderFloat3("Tr", props[currentModelId].translation, -10.f, 10.f);
		ImGui::SliderFloat3("Rt", props[currentModelId].rotation, -180.f, 180.f);
		ImGui::SliderFloat3("Sc", props[currentModelId].scale, 0.f, 5.f);
		if (ImGui::Button("Change Mesh"))
		{
			openFile(modelPath);
			if (!modelPath.empty())
			{
				delete models[currentModelId];
				models[currentModelId] = new Model(modelPath, "", "", "", "");
				modelPath = "";
			}
		}
		ImGui::Text("Textures");
		ImGui::Checkbox("Combined", &models[currentModelId]->GetUseCombinedTextures());
		if (ImGui::Button("A"))
		{
			openFile(props[currentModelId].albedoPath);
			if (!props[currentModelId].albedoPath.empty())
			{
				models[currentModelId]->ChangeTexture(props[currentModelId].albedoPath, TextureType::ALBEDO);
			}
		}
		ImGui::SameLine();
		ImGui::InputText("Albedo", &props[currentModelId].albedoPath);
		if (ImGui::Button("N"))
		{
			openFile(props[currentModelId].normalPath);
			if (!props[currentModelId].normalPath.empty())
			{
				models[currentModelId]->ChangeTexture(props[currentModelId].normalPath, TextureType::NORMAL);
			}
		}
		ImGui::SameLine();
		ImGui::InputText("Normal", &props[currentModelId].normalPath);
		if (ImGui::Button("R"))
		{
			openFile(props[currentModelId].roughnessPath);
			if (!props[currentModelId].roughnessPath.empty())
			{
				models[currentModelId]->ChangeTexture(props[currentModelId].roughnessPath, TextureType::ROUGHNESS);
			}
		}
		ImGui::SameLine();
		ImGui::InputText("Roughness", &props[currentModelId].roughnessPath);
		if (ImGui::Button("M"))
		{
			openFile(props[currentModelId].metallicPath);
			if (!props[currentModelId].metallicPath.empty())
			{
				models[currentModelId]->ChangeTexture(props[currentModelId].metallicPath, TextureType::METALLIC);
			}
		}
		ImGui::SameLine();
		ImGui::InputText("Metallic", &props[currentModelId].metallicPath);
		ImGui::SliderFloat("NormalStrength", &normalStrength, 0.001f, 10.f);

		ImGui::Text("Light");
		const char* lightItems[]{ "1", "2", "3" };
		if (ImGui::Combo("Light", &CurrentLightIndex, lightItems, 3))
		{
		}
		ImGui::SliderFloat3("Position", (float*)&lightProps.lightPos[CurrentLightIndex], -30.f, 30.f);
		ImGui::SliderFloat("Constant", &lightProps.constant, 0.001f, 10.f);
		ImGui::SliderFloat("Linear", &lightProps.linear, 0.001f, 1.f);
		ImGui::SliderFloat("Quadratic", &lightProps.quadratic, 0.001f, 1.f);
		ImGui::SliderFloat3("Color", lightProps.color, 0.001f, 1.f);
		ImGui::SliderFloat("Bias", &m_ShadowMapBias, 0.0f, 0.0005f, "%.5f");
		ImGui::SliderFloat("PreBias", &depthBias, 0.01f, 1000.f);
		ImGui::SliderFloat("SlopeBias", &slopeBias, 0.01f, 100.f);
		ImGui::SliderFloat("Clamp", &depthBiasClamp, 0.1f, 1.f);

		ImGui::Text("GI Settings");
		ImGui::Checkbox("Enable GI", &m_bEnableGI);
		ImGui::SliderFloat("DiffuseScale", &g_fDiffuseScale, 0.01f, 1.f);
		ImGui::SliderFloat("SpecularScale", &g_fSpecularScale, 0.0f, 1.f);
		ImGui::Checkbox("Temporal Filtering", &g_bTemporalFiltering);
		ImGui::SliderFloat("SamplingRate", &g_fSamplingRate, 0.25f, 1.f);
		ImGui::SliderFloat("Quality", &g_fQuality, 0.001f, 1.f);
		ImGui::SliderFloat("MultiBounceScale", &g_fMultiBounceScale, 0.001f, 2.f);
		ImGui::SliderFloat("VXAOScale", &m_fVxaoScale, 0.001f, 5.f);
		ImGui::Text("GI Debug View");
		ImGui::Checkbox("Show diffuse only", &m_bShowDiffuseTexture);
		if (m_bShowDiffuseTexture)
			ImGui::Checkbox("Render debug", &m_bVXGIRenderDebug);
		ImGui::Checkbox("Enable SSAO", &g_bEnableSSAO);

		ImGui::End();

		cameras[1].GetPosition() = lightProps.lightPos[0];
		cameras[2].GetPosition() = lightProps.lightPos[1];
		cameras[3].GetPosition() = lightProps.lightPos[2];


		ImGui::Begin("Scene");
		if (ImGui::Button("New"))
		{
			openFile(modelPath);
			if (!modelPath.empty())
			{
				models.emplace_back(new Model(modelPath, "", "", "", ""));
				props.emplace_back(Properties());
				modelPath = "";
			}
		}

		for (int i = 0; i < models.size(); i++)
		{
			char nodeName[16];
			snprintf(nodeName, 16, "Model_%d", i);
			ImGuiTreeNodeFlags flags = ((currentModelId == i) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
			bool opened = ImGui::TreeNodeEx(nodeName, flags);
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Delete"))
				{
					delete models[i];
					models.erase(models.begin() + i);
					props.erase(props.begin() + i);
					if (currentModelId == i)
						currentModelId = 0;
				}
				ImGui::EndPopup();
			}
			if (ImGui::IsItemClicked())
			{
				currentModelId = i;
			}
			if (opened)
			{
				ImGui::TreePop();
			}
		}
		ImGui::End();

		ImGui::Begin("Viewport");
		ImVec2 vMin = ImGui::GetWindowContentRegionMin();
		ImVec2 vMax = ImGui::GetWindowContentRegionMax();

		vMin.x += ImGui::GetWindowPos().x;
		vMin.y += ImGui::GetWindowPos().y;
		vMax.x += ImGui::GetWindowPos().x;
		vMax.y += ImGui::GetWindowPos().y;

		//vRegionMinX = vMin.x;
		//vRegionMinY = vMin.y;

		texWidth = vMax.x - vMin.x;
		texHeight = vMax.y - vMin.y;
		if (texWidth != texWidthPrev || texHeight != texHeightPrev)
		{
			DX11RendererAPI::GetInstance()->SetViewport(vMin.x, vMin.y, texWidth, texHeight);
			FramebufferToTextureInit();
			//gBuffer->Shutdown();
			//if (!gBuffer->Initialize(texWidth, texHeight))
			//	std::cout << "Failed to init gBuffer!\n";
			PrepareGbufferRenderTargets(texWidth, texHeight);
			texWidthPrev = texWidth;
			texHeightPrev = texHeight;
		}
		ImGui::Image((ImTextureID)(intptr_t)frameSRV, ImVec2(texWidth, texHeight));
		ImGui::End();

		m_Renderer->BeginScene();		

		//props[0].translation[0] = shapePos.x * 0.05f;
		//props[0].translation[1] = shapePos.y * 0.05f;
		//props[0].translation[2] = shapePos.z * 0.05f;
		
		if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed)
		{
			auto glmPos = cameras[0].GetPosition();
			auto glmDir = cameras[0].GetFrontVector();

			PxTransform t;
			t.p = PxVec3(glmPos.x, glmPos.y, glmPos.z);
			t.q = PxQuat(PxIDENTITY::PxIdentity);
				
			createDynamic(t, PxBoxGeometry(m_boxHalfExtent, m_boxHalfExtent, m_boxHalfExtent), PxVec3(glmDir.x, glmDir.y, glmDir.z) * 30);
			spacePressed = true;
		}
		if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_SPACE) == GLFW_RELEASE)
			spacePressed = false;

		if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_V) == GLFW_PRESS)
		{
			m_coltPhysicsActor->addTorque(PxVec3(300.f, 500.f, 0.f));
		}
			

		ChangeDepthBiasParameters(depthBias, slopeBias, depthBiasClamp);

		// rendering depth to render texture for point light
		bool lightPosChanged = false;
		for (int i = 0; i < NUM_POINT_LIGHTS; i++)
		{
			lightPosChanged = lightPositionsPrev[i] != cameras[1 + i].GetPosition();
			if (lightPosChanged)
				break;
		}
		if (lightPosChanged)
		{
			for (int i = 0; i < NUM_POINT_LIGHTS; i++)
			{
				PreparePointLightViewMatrixes(cameras[i + 1].GetPosition());
				//static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->SetCullFront();
				m_PointLightDepthVertexShader->Bind();
				m_PointLightDepthGeometryShader->Bind();
				m_PointLightDepthPixelShader->Bind();
				PointLightDepthToTextureBegin(i);
				RenderScene(cameras[0].GetViewMatrix(), lightProjMat, m_PointLightDepthVertexShader, m_PointLightDepthPixelShader, false, nullptr, 0, false);
				PointLightDepthToTextureEnd(i);
				m_PointLightDepthVertexShader->Unbind();
				m_PointLightDepthGeometryShader->Unbind();
				m_PointLightDepthPixelShader->Unbind();
				//static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->SetCullBack();
			}
			lightPositionsPrev[0] = cameras[1].GetPosition();
			lightPositionsPrev[1] = cameras[2].GetPosition();
			lightPositionsPrev[2] = cameras[3].GetPosition();
		}

		ResetRenderState();

		// rendering depth to render texture for spot light	
		//m_DX11DeviceContext->OMSetDepthStencilState(m_pGBufferReverseZDepthStencilState, 0); // for reverse z buffer
		SpotLightDepthToTextureBegin();
		auto reverseProjMat = reverseZ(spotlightProjMat);
		RenderScene(cameras[NUM_POINT_LIGHTS + 1].GetViewMatrix(), spotlightProjMat, m_SpotLightDepthVertexShader, m_SpotLightDepthPixelShader, false, nullptr, 0, false);
		SpotLightDepthToTextureEnd();
		m_DX11DeviceContext->OMSetDepthStencilState(nullptr, 0);

		ResetRenderState();

		if (m_bEnableGI)
		{
			// Scene voxelization
			NvidiaVXGIVoxelization();
			rendererInterface->clearState();
			ResetRenderState();
		}

		// Render to Gbuffer
		//m_DX11DeviceContext->OMSetDepthStencilState(m_pGBufferReverseZDepthStencilState, 0); // for reverse z buffer
		//gBuffer->SetRenderTargets();
		//gBuffer->ClearRenderTargets(0.f, 0.f, 0.f, 1.f);
		//auto perspectiveMat = glm::perspectiveFovLH(XM_PIDIV4, (FLOAT)texWidth, (FLOAT)texHeight, NEAR_PLANE, FAR_PLANE);
		////perspectiveMat = reverseZ(perspectiveMat);
		//RenderScene(cameras[SelectedCamera].GetViewMatrix(), perspectiveMat, m_DeferredVertexShader, m_DeferredPixelShader, false, nullptr, 0);
		//m_DeferredVertexShader->Unbind();
		//m_DeferredPixelShader->Unbind();

		RenderToGBufferBegin();
		if (m_MeshesDrawn > 0)
		{
			for (int i = 0; i < NUM_POINT_LIGHTS; i++)
			{
				auto modelMat = glm::mat4(1.f);
				auto translate = cameras[i + 1].GetPosition();
				auto scale = 0.1f;
				modelMat = glm::translate(modelMat, glm::vec3(translate[0], translate[1], translate[2]));
				modelMat = glm::scale(modelMat, glm::vec3(scale, scale, scale));			
				RenderCube(modelMat);
			}			
			//RenderModelBounds(0);
		}
		renderPhysics(deltaTime);
		RenderToGBufferEnd();

		//std::cout << "Meshes drawn: " << m_MeshesDrawn << '\n';

		// Reset state
		rendererInterface->clearState();
		ResetRenderState();

		auto albedoSRV = rendererInterface->getSRVForTexture(m_TargetAlbedo);
		auto normalRoughnessSRV = rendererInterface->getSRVForTexture(m_TargetNormalRoughness);
		auto positionMetallicSRV = rendererInterface->getSRVForTexture(m_TargetPositionMetallic);
		auto depthSRV = rendererInterface->getSRVForTexture(m_TargetDepth);
		auto lightViewPositionSRV = rendererInterface->getSRVForTexture(m_TargetLightViewPosition);

		// Main deferred render
	  /*auto albedoSRV = gBuffer->GetShaderResourceView(0);
		auto normalRoughnessSRV = gBuffer->GetShaderResourceView(1);
		auto positionMetallicSRV = gBuffer->GetShaderResourceView(2);
		auto depthSRV = rendererInterface->getSRVForTexture(rendererInterface->getHandleForTexture(gBuffer->GetDepthStencilTexture()));*/
		if (m_MeshesDrawn > 0)
		{
			if (m_bEnableGI)
			{
				if (!m_bVXGIRenderDebug)
				{
					// Cone tracing
					NvidiaVXGIConeTracing();

					ResetRenderState();
				}
				else
				{
					//rendererInterface->forgetAboutTexture(rendererInterface->getResourceForTexture(m_TargetAlbedo));
					//m_TargetAlbedo = rendererInterface->getHandleForTexture(gBuffer->GetTexture(0), NVRHI::Format::RGBA16_FLOAT);

					//rendererInterface->forgetAboutTexture(rendererInterface->getResourceForTexture(m_TargetDepth));
					//m_TargetDepth = rendererInterface->getHandleForTexture(gBuffer->GetDepthStencilTexture(), NVRHI::Format::D24S8);

					// Switching to debug rendering
					VXGI::DebugRenderParameters params;
					params.debugMode = VXGI::DebugRenderMode::EMITTANCE_TEXTURE;
					glm::mat4 viewMatrix = cameras[SelectedCamera].GetViewMatrix();
					glm::mat4 projMatrix = glm::perspectiveFovLH(XM_PIDIV4, (FLOAT)texWidth, (FLOAT)texHeight, NEAR_PLANE, FAR_PLANE);
					params.viewMatrix = *(VXGI::float4x4*)&viewMatrix;
					params.projMatrix = *(VXGI::float4x4*)&projMatrix;
					params.viewport = NVRHI::Viewport((float)texWidth, (float)texHeight);
					params.destinationTexture = m_TargetAlbedo;
					params.destinationDepth = m_TargetDepth;
					params.level = -1;
					params.blendState.blendEnable[0] = true;
					params.blendState.srcBlend[0] = NVRHI::BlendState::BLEND_SRC_ALPHA;
					params.blendState.destBlend[0] = NVRHI::BlendState::BLEND_INV_SRC_ALPHA;

					VXGI::Status::Enum status = giObject->renderDebug(params);
					if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';
					if (status == 0)
					{
						indirectSRV = rendererInterface->getSRVForTexture(m_TargetAlbedo);
						depthSRV = rendererInterface->getSRVForTexture(m_TargetDepth);
					}
				}
			}

			FramebufferToTextureBegin();
			m_DeferredCompositingVertexShader->Bind();
			m_DeferredCompositingPixelShader->Bind();
			m_DX11DeviceContext->PSSetShaderResources(0, 1, &albedoSRV);
			m_DX11DeviceContext->PSSetShaderResources(1, 1, &normalRoughnessSRV);
			m_DX11DeviceContext->PSSetShaderResources(2, 1, &positionMetallicSRV);
			m_DX11DeviceContext->PSSetShaderResources(3, 1, &depthSRV);
			m_DX11DeviceContext->PSSetShaderResources(4, 1, &lightViewPositionSRV);
			if (indirectSRV)
				m_DX11DeviceContext->PSSetShaderResources(5, 1, &indirectSRV);
			if (confidenceSRV)
				m_DX11DeviceContext->PSSetShaderResources(6, 1, &confidenceSRV);
			if (indirectSpecularSRV)
				m_DX11DeviceContext->PSSetShaderResources(7, 1, &indirectSpecularSRV);
			if (ssaoSRV)
				m_DX11DeviceContext->PSSetShaderResources(8, 1, &ssaoSRV);
			m_DX11DeviceContext->PSSetShaderResources(12, 1, &m_SpotLightDepthSRV); // spotlight
			m_DX11DeviceContext->PSSetShaderResources(15, NUM_POINT_LIGHTS, m_PointLightDepthSRVs);
			m_DX11DeviceContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
			m_DX11DeviceContext->PSSetSamplers(1, 1, &g_pSamplerClamp);
			m_DX11DeviceContext->PSSetSamplers(2, 1, &g_pSamplerComparison);
			RenderQuad();
			m_DeferredCompositingVertexShader->Unbind();
			m_DeferredCompositingPixelShader->Unbind();
			FramebufferToTextureEnd();
		}


		ImguiRender();

		m_Renderer->EndScene(m_bEnableVsync);
		m_Window->OnDraw();
	}

	Cleanup();
}

void TestApplication::ResetRenderState()
{
	// Reset state
	static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->ResetRenderTargetView();
	m_DX11DeviceContext->RSSetState(m_DepthRasterizerState);
	m_DX11DeviceContext->IASetInputLayout(m_DefaultVertexShader->GetRawInputLayout());
	m_DX11DeviceContext->OMSetBlendState(nullptr, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
	m_DX11DeviceContext->OMSetDepthStencilState(nullptr, 0);
}

void TestApplication::Cleanup()
{
	for (auto& model : models)
		delete model;

	SAFE_RELEASE(frameSRV);
	SAFE_RELEASE(frameTexture);
	delete renderTexture;

	SAFE_RELEASE(g_pCBMatrixes);
	SAFE_RELEASE(g_pCBLight);
	SAFE_RELEASE(g_pSamplerLinear);

	SAFE_RELEASE(m_SpotLightDepthSRV);
	SAFE_RELEASE(m_SpotLightDepthTexture);
	delete m_SpotLightDepthRenderTexture;

	SAFE_RELEASE(g_pSamplerComparison);
	SAFE_RELEASE(g_pSamplerClamp);

	for (int i = 0; i < NUM_POINT_LIGHTS; i++)
	{
		SAFE_RELEASE(m_PointLightDepthSRVs[i]);
		SAFE_RELEASE(m_PointLightDepthTextures[i]);
		delete m_PointLightDepthRenderTextures[i];
	}

	SAFE_RELEASE(m_DepthRasterizerState);

	SAFE_RELEASE(m_GBufferDepth);
	SAFE_RELEASE(m_GBufferNormal);

	SAFE_RELEASE(ssaoSRV);
	SAFE_RELEASE(ssao);

	SAFE_RELEASE(m_pGBufferReverseZDepthStencilState);

	SAFE_DELETE(gbufferState);
	SAFE_DELETE(voxelizationState);

	SAFE_RELEASE(m_VertexBuffer);
	SAFE_RELEASE(m_IndexBuffer);

	if (m_TargetAlbedo)
		rendererInterface->forgetAboutTexture(rendererInterface->getResourceForTexture(m_TargetAlbedo));
	if (m_TargetNormalRoughness)
		rendererInterface->forgetAboutTexture(rendererInterface->getResourceForTexture(m_TargetNormalRoughness));
	if (m_TargetNormalRoughnessPrev)
		rendererInterface->forgetAboutTexture(rendererInterface->getResourceForTexture(m_TargetNormalRoughnessPrev));
	if (m_TargetPositionMetallic)
		rendererInterface->forgetAboutTexture(rendererInterface->getResourceForTexture(m_TargetPositionMetallic));
	if (m_TargetDepth)
		rendererInterface->forgetAboutTexture(rendererInterface->getResourceForTexture(m_TargetDepth));
	if (m_TargetDepthPrev)
		rendererInterface->forgetAboutTexture(rendererInterface->getResourceForTexture(m_TargetDepthPrev));

	SAFE_RELEASE(m_pGlobalCBufferMatrixes);
	SAFE_RELEASE(m_pGlobalCBufferLight);
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pDefaultVS);
	SAFE_RELEASE(m_pDefaultSamplerState);
	SAFE_RELEASE(m_pComparisonSamplerState);
	SAFE_RELEASE(m_ShadowMap);
	SAFE_RELEASE(m_DiffuseTexture);

	indirectDiffuse = nullptr;
	indirectConfidence = nullptr;
	indirectSpecular = nullptr;
	indirectSRV = nullptr;
	confidenceSRV = nullptr;
	indirectSpecularSRV = nullptr;

	SAFE_DELETE(gBuffer);
	SAFE_DELETE(drawCallState);

	giObject->destroyUserDefinedShaderSet(psShaderSet);
	psShaderSet = nullptr;
	giObject->destroyUserDefinedShaderSet(gsShaderSet);
	gsShaderSet = nullptr;
	VXGI::VFX_VXGI_DestroyShaderCompiler(shaderCompiler);

	giObject->destroyTracer(basicViewTracer);
	basicViewTracer = nullptr;

	VFX_VXGI_DestroyGIObject(giObject);
	giObject = nullptr;

	SAFE_DELETE(rendererInterface);

	SAFE_RELEASE(pCubeVertexBuffer);
	SAFE_RELEASE(pCubeIndexBuffer);
	SAFE_RELEASE(pQuadVertexBuffer);
	SAFE_RELEASE(pQuadIndexBuffer);

	ImguiCleanup();
	m_Renderer->Shutdown();
	m_Window->OnClose();
}

void TestApplication::OnWindowEvent(const Event<WindowEvents>& e)
{
	if (e.GetType() == WindowEvents::WindowResize)
	{
		auto windowResizeEvent = e.ToType<WindowResizeEvent>();
		OV_INFO("Window Resize Event! New Dimensions: {}x{}", windowResizeEvent.width, windowResizeEvent.height);

		m_Window->OnResize(windowResizeEvent.width, windowResizeEvent.height);
		m_Renderer->OnWindowResize(windowResizeEvent.width, windowResizeEvent.height);
	}
	else if (e.GetType() == WindowEvents::WindowClose)
	{
		OV_INFO("Window Close Event!");
		m_Running = false;
	}
}

void TestApplication::OnMouseEvent(const Event<MouseEvents>& e)
{
	if (e.GetType() == MouseEvents::MouseMoved)
	{
		auto mouseMovedEvent = e.ToType<MouseMovedEvent>();
		//OV_INFO("Mouse Moved Event! Mouse Position: ({}, {})", mouseMovedEvent.x, mouseMovedEvent.y);

		float xpos = static_cast<float>(mouseMovedEvent.x);
		float ypos = static_cast<float>(mouseMovedEvent.y);

		static bool firstMouse = true;
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

		lastX = xpos;
		lastY = ypos;

		if (viewPortActive)
		{
			cameras[SelectedCamera].ProcessMouseMovement(xoffset, yoffset);
		}
	}
	else if (e.GetType() == MouseEvents::MouseButtonDown)
	{
		auto mouseButtonDownEvent = e.ToType<MouseButtonDownEvent>();
		//OV_INFO("Mouse Button Down Event! Mouse Button Down: {}", mouseButtonDownEvent.button);
	}
	else if (e.GetType() == MouseEvents::MouseButtonUp)
	{
		auto mouseButtonUpEvent = e.ToType<MouseButtonUpEvent>();
		//OV_INFO("Mouse Button Up Event! Mouse Button Up: {}", mouseButtonUpEvent.button);
	}
	else if (e.GetType() == MouseEvents::MouseScroll)
	{
		auto mouseScrollEvent = e.ToType<MouseScrollEvent>();
		//OV_INFO("Mouse Scroll Event! xoffset: {}, yoffset: {}", mouseScrollEvent.xoffset, mouseScrollEvent.yoffset);
	}
}

void TestApplication::OnKeyEvent(const Event<KeyEvents>& e)
{
	if (e.GetType() == KeyEvents::KeyDown)
	{
		auto keyDownEvent = e.ToType<KeyDownEvent>();
		//OV_INFO("Key Down Event! Key Down: {}", keyDownEvent.keycode);
		if (keyDownEvent.keycode == 256)
			m_Running = false;

		if (keyDownEvent.keycode == GLFW_KEY_C && !keyPressed)
		{
			viewPortActive = !viewPortActive;
			keyPressed = true;
		}
	}
	else if (e.GetType() == KeyEvents::KeyUp)
	{
		auto keyUpEvent = e.ToType<KeyUpEvent>();
		//OV_INFO("Key Up Event! Key Up: {}", keyUpEvent.keycode);

		if (keyUpEvent.keycode == GLFW_KEY_C)
			keyPressed = false;
	}
}

void TestApplication::ImguiInit()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch  

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOther(static_cast<WindowsWindow*>(m_Window.get())->GetGLFWwindow(), true);

	auto device = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDevice();
	auto deviceContext = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDeviceContext();
	ImGui_ImplDX11_Init(device, deviceContext);

	SetupImGuiStyle();
}

void TestApplication::ImguiFrame()
{
	// (Your code process and dispatch Win32 messages)
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	//ImGui::ShowDemoWindow(); // Show demo window! :)
}

void TestApplication::ImguiRender()
{
	// Rendering
	// (Your code clears your framebuffer, renders your other stuff etc.)
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	// (Your code calls swapchain's Present() function)
}

void TestApplication::ImguiCleanup()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void TestApplication::SetupImGuiStyle()
{
	// Hazy Dark style by kaitabuchi314 from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.6000000238418579f;
	style.WindowPadding = ImVec2(5.5f, 8.300000190734863f);
	style.WindowRounding = 4.5f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(32.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Left;
	style.ChildRounding = 3.200000047683716f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 2.700000047683716f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(4.0f, 3.0f);
	style.FrameRounding = 2.400000095367432f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(8.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 14.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 3.200000047683716f;
	style.TabRounding = 3.5f;
	style.TabBorderSize = 1.0f;
	style.TabMinWidthForCloseButton = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05882352963089943f, 0.05882352963089943f, 0.05882352963089943f, 0.9399999976158142f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.9399999976158142f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1372549086809158f, 0.1725490242242813f, 0.2274509817361832f, 0.5400000214576721f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2117647081613541f, 0.2549019753932953f, 0.3019607961177826f, 0.4000000059604645f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.04313725605607033f, 0.0470588244497776f, 0.0470588244497776f, 0.6700000166893005f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.03921568766236305f, 0.03921568766236305f, 0.03921568766236305f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0784313753247261f, 0.08235294371843338f, 0.09019608050584793f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.5099999904632568f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1372549086809158f, 0.1372549086809158f, 0.1372549086809158f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.5299999713897705f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.7176470756530762f, 0.7843137383460999f, 0.843137264251709f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.47843137383461f, 0.5254902243614197f, 0.572549045085907f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.2901960909366608f, 0.3176470696926117f, 0.3529411852359772f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.1490196138620377f, 0.1607843190431595f, 0.1764705926179886f, 0.4000000059604645f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1372549086809158f, 0.1450980454683304f, 0.1568627506494522f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.09019608050584793f, 1.0f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.196078434586525f, 0.2156862765550613f, 0.239215686917305f, 0.3100000023841858f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.1647058874368668f, 0.1764705926179886f, 0.1921568661928177f, 0.800000011920929f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.07450980693101883f, 0.08235294371843338f, 0.09019608050584793f, 1.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.239215686917305f, 0.3254902064800262f, 0.4235294163227081f, 0.7799999713897705f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.2745098173618317f, 0.3803921639919281f, 0.4980392158031464f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2901960909366608f, 0.3294117748737335f, 0.3764705955982208f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.239215686917305f, 0.2980392277240753f, 0.3686274588108063f, 0.6700000166893005f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.1647058874368668f, 0.1764705926179886f, 0.1882352977991104f, 0.949999988079071f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.1176470592617989f, 0.125490203499794f, 0.1333333402872086f, 0.8619999885559082f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.3294117748737335f, 0.407843142747879f, 0.501960813999176f, 0.800000011920929f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.2431372553110123f, 0.2470588237047195f, 0.2549019753932953f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428f, 0.1019607856869698f, 0.1450980454683304f, 0.9724000096321106f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 1.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3499999940395355f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
}

void TestApplication::SetupImguiDockspace()
{
	int windowFlags = ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoNavFocus;

	ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_::ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(m_Window.get()->GetWidth(), m_Window.get()->GetHeight()), ImGuiCond_::ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_WindowRounding, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_WindowBorderSize, 0.f);

	bool dockspace_open = true;
	ImGui::Begin("Dockspace", &dockspace_open, windowFlags);

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Create")) {
			}
			if (ImGui::MenuItem("Open", "Ctrl+O")) {
			}
			if (ImGui::MenuItem("Save", "Ctrl+S")) {
			}
			if (ImGui::MenuItem("Save as..")) {
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	ImGui::PopStyleVar(2);

	ImGui::DockSpace(ImGui::GetID("Dockspace"));

	ImGui::End();
}

void TestApplication::FramebufferToTextureInit()
{
	if (frameSRV) frameSRV->Release();
	if (frameTexture) frameTexture->Release();

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = texWidth;
	texDesc.Height = texHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	HRESULT result = m_DX11Device->CreateTexture2D(&texDesc, nullptr, &frameTexture);
	if (FAILED(result)) return;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;

	result = m_DX11Device->CreateShaderResourceView(frameTexture, &srvDesc, &frameSRV);
	if (FAILED(result)) return;

	if (renderTexture) delete renderTexture;
	renderTexture = new RenderTexture;
	renderTexture->Initialize(texWidth, texHeight, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R32G32B32A32_FLOAT, false);
}

void TestApplication::FramebufferToTextureBegin()
{
	DX11RendererAPI::GetInstance()->SetViewport(vRegionMinX, vRegionMinY, texWidth, texHeight);
	renderTexture->SetRenderTarget(false);
	renderTexture->ClearRenderTarget(0.01f, 0.01f, 0.01f, 1.0f, 1.f);
}

void TestApplication::FramebufferToTextureEnd()
{
	D3D11_BOX sourceRegion;
	sourceRegion.left = 0;
	sourceRegion.right = texWidth;
	sourceRegion.top = 0;
	sourceRegion.bottom = texHeight;
	sourceRegion.front = 0;
	sourceRegion.back = 1;

	m_DX11DeviceContext->CopySubresourceRegion(frameTexture, D3D11CalcSubresource(0, 0, 1), 0, 0, 0, renderTexture->GetTexture(),
		0, &sourceRegion);

	// save texture to disc
	//ScratchImage texture;
	//CaptureTexture(g_pd3dDevice, g_pDeviceContext, frameTexture, texture);
	//SaveToWICFile(*texture.GetImage(0, 0, 0), WIC_FLAGS_NONE, GetWICCodec(WIC_CODEC_PNG), L"NEW_IMAGE.PNG");

	//delete renderTexture;
	static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->ResetRenderTargetView();
}

bool TestApplication::openFile(std::string& pathRef)
{
	//  CREATE FILE OBJECT INSTANCE
	HRESULT f_SysHr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(f_SysHr))
		return FALSE;

	// CREATE FileOpenDialog OBJECT
	IFileOpenDialog* f_FileSystem;
	f_SysHr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&f_FileSystem));
	if (FAILED(f_SysHr)) {
		CoUninitialize();
		return FALSE;
	}

	//  SHOW OPEN FILE DIALOG WINDOW
	f_SysHr = f_FileSystem->Show(NULL);
	if (FAILED(f_SysHr)) {
		f_FileSystem->Release();
		CoUninitialize();
		return FALSE;
	}

	//  RETRIEVE FILE NAME FROM THE SELECTED ITEM
	IShellItem* f_Files;
	f_SysHr = f_FileSystem->GetResult(&f_Files);
	if (FAILED(f_SysHr)) {
		f_FileSystem->Release();
		CoUninitialize();
		return FALSE;
	}

	//  STORE AND CONVERT THE FILE NAME
	PWSTR f_Path;
	f_SysHr = f_Files->GetDisplayName(SIGDN_FILESYSPATH, &f_Path);
	if (FAILED(f_SysHr)) {
		f_Files->Release();
		f_FileSystem->Release();
		CoUninitialize();
		return FALSE;
	}

	//  FORMAT AND STORE THE FILE PATH
	std::wstring path(f_Path);
	std::string c(path.begin(), path.end());
	sFilePath = c;

	pathRef = c;

	//  FORMAT STRING FOR EXECUTABLE NAME
	const size_t slash = sFilePath.find_last_of("/\\");
	sSelectedFile = sFilePath.substr(slash + 1);

	//  SUCCESS, CLEAN UP
	CoTaskMemFree(f_Path);
	f_Files->Release();
	f_FileSystem->Release();
	CoUninitialize();
	return TRUE;

}

void TestApplication::SaveTextureToFile(ID3D11Texture2D* d3dTexture, const std::string fileName)
{
	ScratchImage image;
	HRESULT hr = CaptureTexture(m_DX11Device, m_DX11DeviceContext, d3dTexture, image);
	if (FAILED(hr)) std::cout << "Can't capture texture\n";
	std::wstring fileNameW{ fileName.begin(), fileName.end() };
	hr = SaveToWICFile(*image.GetImage(0, 0, 0), WIC_FLAGS_NONE, GetWICCodec(WIC_CODEC_PNG), fileNameW.c_str());
	if (FAILED(hr)) std::cout << "Can't save PNG file\n";
}

void TestApplication::RenderCubeInit()
{
	Vertex vertices[] =
	{
		{ glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),      glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
		{ glm::vec4(1.0f, 1.0f, -1.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
		{ glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
		{ glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)},

		{ glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),     glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
		{ glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),      glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
		{ glm::vec4(1.0f, -1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
		{ glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),      glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)},

		{ glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),      glm::vec2(0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)},
		{ glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),     glm::vec2(1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)},
		{ glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),      glm::vec2(1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)},
		{ glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)},

		{ glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),	glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)},
		{ glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),      glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)},
		{ glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),	glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f)},
		{ glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),	glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f)},

		{ glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),     glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)},
		{ glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),      glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)},
		{ glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),		glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)},
		{ glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),      glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)},

		{ glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),      glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)},
		{ glm::vec4(1.0f, -1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)},
		{ glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f)},
		{ glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f)},
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

	HRESULT hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pCubeVertexBuffer);
	if (FAILED(hr)) return;

	UINT indices[] =
	{
		3,1,0,
		2,1,3,

		6,4,5,
		7,4,6,

		11,9,8,
		10,9,11,

		14,12,13,
		15,12,14,

		19,17,16,
		18,17,19,

		22,20,21,
		23,20,22
	};


	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(UINT) * 36;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	InitData.pSysMem = indices;

	hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pCubeIndexBuffer);
	if (FAILED(hr)) return;
}

void TestApplication::RenderCube(const glm::mat4& modelMat)
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	m_DX11DeviceContext->IASetVertexBuffers(0, 1, &pCubeVertexBuffer, &stride, &offset);
	m_DX11DeviceContext->IASetIndexBuffer(pCubeIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_DX11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	ConstantBuffer cb;	
	cb.modelMat = modelMat;
	cb.viewMat = cameras[SelectedCamera].GetViewMatrix();
	cb.projMat = glm::perspectiveFovLH(XM_PIDIV4, (FLOAT)texWidth, (FLOAT)texHeight, NEAR_PLANE, FAR_PLANE);
	cb.normalMat = glm::transpose(glm::inverse(modelMat));

	m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, nullptr, &cb, 0, 0);
	m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);

	MaterialCB matCb;
	matCb.sponza = 0.f;
	matCb.normalStrength = normalStrength;
	matCb.renderLightCube = 1;

	m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, nullptr, &matCb, 0, 0);
	m_DX11DeviceContext->PSSetConstantBuffers(2, 1, &g_pCBMaterial);

	//m_CubePixelShader->Bind();

	m_DX11DeviceContext->DrawIndexed(36, 0, 0);

	//m_CubePixelShader->Unbind();
}

void TestApplication::RenderQuadInit()
{
	Vertex vertices[] =
	{
		{ glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)},
		{ glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)},
		{ glm::vec4(1.0f, -1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)},
		{ glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)},
	};

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Vertex) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;

	HRESULT hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pQuadVertexBuffer);
	if (FAILED(hr)) return;

	UINT indices[] =
	{
		0, 3, 2,
		0, 2, 1
	};

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(UINT) * 6;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	InitData.pSysMem = indices;

	hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pQuadIndexBuffer);
	if (FAILED(hr)) return;
}

void TestApplication::RenderQuad()
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	m_DX11DeviceContext->IASetVertexBuffers(0, 1, &pQuadVertexBuffer, &stride, &offset);
	m_DX11DeviceContext->IASetIndexBuffer(pQuadIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_DX11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ConstantBuffer cb;
	cb.modelMat = glm::scale(glm::mat4(1.f), glm::vec3((float)texWidth / texHeight * 1.f, 1.f, 1.f));
	cb.viewMat = glm::lookAtLH(glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
	//cb.projMat = glm::perspectiveFovLH(XM_PIDIV4, (FLOAT)texWidth, (FLOAT)texHeight, 0.01f, 100.f);
	auto projMatXm = XMMatrixOrthographicLH((float)texWidth / texHeight * 2.f, 2.f, 0.1f, 100.f);
	cb.projMat = *(glm::mat4*)&projMatXm;
	cb.normalMat = glm::mat4(1.f);
	glm::mat4 viewProjInv = glm::inverse(cameras[SelectedCamera].GetViewMatrix() * glm::perspectiveFovLH(XM_PIDIV4, (FLOAT)texWidth, (FLOAT)texHeight, 0.01f, 100.f));
	cb.viewProjMatInv = viewProjInv;

	m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, NULL, &cb, 0, 0);
	m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);
	m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);

	LightCB lightCb;
	lightCb.lightPos[0] = glm::vec4(cameras[1].GetPosition().x, cameras[1].GetPosition().y, cameras[1].GetPosition().z, 1.f);
	lightCb.lightPos[1] = glm::vec4(cameras[2].GetPosition().x, cameras[2].GetPosition().y, cameras[2].GetPosition().z, 1.f);
	lightCb.lightPos[2] = glm::vec4(cameras[3].GetPosition().x, cameras[3].GetPosition().y, cameras[3].GetPosition().z, 1.f);
	lightCb.lightPos[3] = glm::vec4(cameras[4].GetPosition().x, cameras[4].GetPosition().y, cameras[4].GetPosition().z, 1.f);
	lightCb.conLinQuad[0] = lightProps.constant;
	lightCb.conLinQuad[1] = lightProps.linear;
	lightCb.conLinQuad[2] = lightProps.quadratic;
	lightCb.lightColor = { lightProps.color[0], lightProps.color[1], lightProps.color[2] };
	lightCb.bias = m_ShadowMapBias;
	lightCb.camPos = cameras[SelectedCamera].GetPosition();
	lightCb.showDiffuseTexture = m_bShowDiffuseTexture;
	lightCb.numLights = NUM_POINT_LIGHTS;
	lightCb.enableGI = m_bEnableGI;
	lightCb.cubeProj = lightProjMat;
	lightCb.lightViewProjMat = spotlightProjMat * cameras[NUM_POINT_LIGHTS + 1].GetViewMatrix();


	m_DX11DeviceContext->UpdateSubresource(g_pCBLight, 0, NULL, &lightCb, 0, 0);
	m_DX11DeviceContext->VSSetConstantBuffers(1, 1, &g_pCBLight);
	m_DX11DeviceContext->PSSetConstantBuffers(1, 1, &g_pCBLight);

	m_DX11DeviceContext->DrawIndexed(6, 0, 0);
}

void TestApplication::SpotLightDepthToTextureInit()
{
	if (m_SpotLightDepthTexture) m_SpotLightDepthTexture->Release();
	if (m_SpotLightDepthSRV) m_SpotLightDepthSRV->Release();

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = SHADOWMAP_WIDTH;
	texDesc.Height = SHADOWMAP_HEIGHT;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R16_TYPELESS;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	HRESULT result = m_DX11Device->CreateTexture2D(&texDesc, nullptr, &m_SpotLightDepthTexture);
	if (FAILED(result)) return;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R16_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.MostDetailedMip = 0;

	result = m_DX11Device->CreateShaderResourceView(m_SpotLightDepthTexture, &srvDesc, &m_SpotLightDepthSRV);
	if (FAILED(result)) return;

	m_SpotLightDepthRenderTexture = new RenderTexture;
	m_SpotLightDepthRenderTexture->Initialize(SHADOWMAP_WIDTH, SHADOWMAP_WIDTH, 1, DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_D16_UNORM, DXGI_FORMAT_R16_UNORM, false);
}

void TestApplication::SpotLightDepthToTextureBegin()
{
	DX11RendererAPI::GetInstance()->SetViewport(0, 0, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT);

	m_SpotLightDepthRenderTexture->SetRenderTarget(true);
	m_SpotLightDepthRenderTexture->ClearRenderTarget(0.f, 0.f, 0.f, 1.f, 1.f);
}

void TestApplication::SpotLightDepthToTextureEnd()
{
	static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->ResetRenderTargetView();

	auto dsv = m_SpotLightDepthRenderTexture->GetDSV();
	ID3D11Resource* dsRes;
	dsv->GetResource(&dsRes);
	auto dsTex = (ID3D11Texture2D*)dsRes;
	m_DX11DeviceContext->CopyResource(m_SpotLightDepthTexture, dsTex);

	if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_K) == GLFW_PRESS)
		SaveTextureToFile(dsTex, "spotlight_depth.png");

	DX11RendererAPI::GetInstance()->SetViewport(vRegionMinX, vRegionMinY, texWidth, texHeight);

	//delete m_SpotLightDepthRenderTexture;
}

void TestApplication::PointLightDepthToTextureInit()
{
	for (int i = 0; i < NUM_POINT_LIGHTS; i++)
	{
		if (m_PointLightDepthSRVs[i]) m_PointLightDepthSRVs[i]->Release();
		if (m_PointLightDepthTextures[i]) m_PointLightDepthTextures[i]->Release();

		D3D11_TEXTURE2D_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(texDesc));
		texDesc.Width = SHADOWMAP_WIDTH;
		texDesc.Height = SHADOWMAP_HEIGHT;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 6;
		texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		HRESULT result = m_DX11Device->CreateTexture2D(&texDesc, nullptr, &m_PointLightDepthTextures[i]);
		if (FAILED(result)) return;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = 1;
		srvDesc.TextureCube.MostDetailedMip = 0;

		result = m_DX11Device->CreateShaderResourceView(m_PointLightDepthTextures[i], &srvDesc, &m_PointLightDepthSRVs[i]);
		if (FAILED(result)) return;

		m_PointLightDepthRenderTextures[i] = new RenderTexture;
		m_PointLightDepthRenderTextures[i]->Initialize(SHADOWMAP_WIDTH, SHADOWMAP_WIDTH, 1, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, true);
	}

}

void TestApplication::PointLightDepthToTextureBegin(int i)
{
	DX11RendererAPI::GetInstance()->SetViewport(0, 0, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT);

	float m_iShadowMapSize = SHADOWMAP_WIDTH;

	D3D11_VIEWPORT vp[6];
	for (int i = 0; i < 6; i++)
	{
		vp[i] = { 0, 0, m_iShadowMapSize, m_iShadowMapSize, 0.0f, 1.0f };
	}
	m_DX11DeviceContext->RSSetViewports(6, vp);

	m_PointLightDepthRenderTextures[i]->SetRenderTarget(true);
	m_PointLightDepthRenderTextures[i]->ClearRenderTarget(0.f, 0.f, 0.f, 1.f, 1.f);
}

void TestApplication::PointLightDepthToTextureEnd(int i)
{
	static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->ResetRenderTargetView();

	ID3D11Texture2D* dsTex = m_PointLightDepthRenderTextures[i]->GetDepthStencilTexture();

	for (int j = 0; j < 6; j++)
		m_DX11DeviceContext->CopySubresourceRegion(m_PointLightDepthTextures[i], D3D11CalcSubresource(0, j, 1), 0, 0, 0, dsTex,
			D3D11CalcSubresource(0, j, 1), nullptr);


	DX11RendererAPI::GetInstance()->SetViewport(vRegionMinX, vRegionMinY, texWidth, texHeight);
}

void TestApplication::PreparePointLightViewMatrixes(glm::vec3 lightPos)
{
	glm::vec3 vectors[] =
	{
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, 1.0f,  0.0f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, 1.0f,  0.0f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  -1.0f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, 1.0f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, 1.0f,  0.0f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, 1.0f,  0.0f)
	};

	m_PointLightDepthCaptureViews =
	{
		glm::lookAtLH(lightPos, lightPos + vectors[1], vectors[2]),
		glm::lookAtLH(lightPos, lightPos + vectors[4], vectors[5]),
		glm::lookAtLH(lightPos, lightPos + vectors[7], vectors[8]),
		glm::lookAtLH(lightPos, lightPos + vectors[10], vectors[11]),
		glm::lookAtLH(lightPos, lightPos + vectors[13], vectors[14]),
		glm::lookAtLH(lightPos, lightPos + vectors[16], vectors[17])
	};
}

void TestApplication::ChangeDepthBiasParameters(float depthBias = 40.f, float slopeBias = 4.0f, float depthBiasClamp = 1.0f)
{
	D3D11_RASTERIZER_DESC rasterDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT{});
	// Setup the raster description which will determine how and what polygons will be drawn.
	rasterDesc.DepthBias = depthBias;
	rasterDesc.DepthBiasClamp = depthBiasClamp;
	rasterDesc.SlopeScaledDepthBias = slopeBias;

	// Create the rasterizer state from the description we just filled out.
	HRESULT hr = m_DX11Device->CreateRasterizerState(&rasterDesc, &m_DepthRasterizerState);
	m_DX11DeviceContext->RSSetState(m_DepthRasterizerState);

	if (FAILED(hr)) return;
}

void TestApplication::NvidiaVXGIInit()
{
	VXGI::Status::Enum status;

	rendererInterface = new NVRHI::RendererInterfaceD3D11(&giErrorCallback, m_DX11DeviceContext);

	VXGI::GIParameters giParams = {};
	giParams.errorCallback = &giErrorCallback;
	giParams.rendererInterface = rendererInterface;
	VXGI::VFX_VXGI_CreateGIObject(giParams, &giObject);

	VXGI::ShaderCompilerParameters scParams = {};
	scParams.errorCallback = &giErrorCallback;
	scParams.d3dCompilerDLLName = "D3DCompiler_47.dll";
	scParams.graphicsAPI = NVRHI::GraphicsAPI::D3D11;
	VXGI::VFX_VXGI_CreateShaderCompiler(scParams, &shaderCompiler);

	ID3DBlob* vsD3dBlob = m_DefaultVertexShader->GetBlob();

	VXGI::IBlob* gsBlob;

	status = shaderCompiler->compileVoxelizationGeometryShaderFromVS(&gsBlob, vsD3dBlob->GetBufferPointer(), vsD3dBlob->GetBufferSize());
	if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';

	VXGI::IBlob* psBlob;
	/*status = shaderCompiler->compileVoxelizationDefaultPixelShader(&psBlob);
	if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';*/
	std::ifstream t("src/VoxelizationPixelShader.hlsl");
	std::stringstream psSource;
	psSource << t.rdbuf();
	VXGI::ShaderResources userShaderCodeResources = {};
	userShaderCodeResources.constantBufferCount = 1;
	userShaderCodeResources.constantBufferSlots[0] = 0;
	userShaderCodeResources.textureCount = 6;
	userShaderCodeResources.textureSlots[0] = 0;
	userShaderCodeResources.textureSlots[1] = 1;
	userShaderCodeResources.textureSlots[2] = 2;
	userShaderCodeResources.textureSlots[3] = 3;
	userShaderCodeResources.textureSlots[4] = 4;
	userShaderCodeResources.textureSlots[5] = 6;
	userShaderCodeResources.samplerCount = 2;
	userShaderCodeResources.samplerSlots[0] = 0;
	userShaderCodeResources.samplerSlots[1] = 1;
	status = shaderCompiler->compileVoxelizationPixelShader(&psBlob, psSource.str().c_str(), psSource.str().size(), "PSMain", userShaderCodeResources);
	if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';

	status = giObject->loadUserDefinedShaderSet(&gsShaderSet, gsBlob->getData(), gsBlob->getSize());
	if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';

	status = giObject->loadUserDefinedShaderSet(&psShaderSet, psBlob->getData(), psBlob->getSize());
	if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';

	psBlob->dispose();
	gsBlob->dispose();

	VXGI::VoxelizationParameters vParams = {};
	vParams.mapSize = VXGI::uint3(g_nMapSize);
	vParams.allocationMapLodBias = 1;
	vParams.enableMultiBounce = g_bEnableMultiBounce;
	vParams.persistentVoxelData = !g_bEnableMultiBounce;
	vParams.useEmittanceInterpolation = g_bEnableMultiBounce;

	status = giObject->validateVoxelizationParameters(vParams);
	if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';

	status = giObject->setVoxelizationParameters(vParams);
	if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';

	status = giObject->createBasicTracer(&basicViewTracer, shaderCompiler);
	if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';
}

void TestApplication::NvidiaVXGIVoxelization()
{
	VXGI::Status::Enum status;
	VXGI::UpdateVoxelizationParameters uvParams = {};
	glm::vec3 camPos = cameras[SelectedCamera].GetPosition();
	glm::vec3 frontVec = cameras[SelectedCamera].GetFrontVector();
	glm::vec3 caVec = camPos + frontVec * g_fVoxelSize * float(g_nMapSize) * 0.25f;
	uvParams.clipmapAnchor = VXGI::float3(caVec.x, caVec.y, caVec.z);

	uvParams.indirectIrradianceMapTracingParameters = {};
	uvParams.indirectIrradianceMapTracingParameters.irradianceScale = g_fMultiBounceScale;
	uvParams.indirectIrradianceMapTracingParameters.useAutoNormalization = true;
	uvParams.indirectIrradianceMapTracingParameters.lightLeakingAmount = VXGI::LightLeakingAmount::MINIMAL;
	uvParams.finestVoxelSize = g_fVoxelSize;

	bool performOpacityVoxelization = false;
	bool performEmittanceVoxelization = false;
	status = giObject->prepareForVoxelization(uvParams, performOpacityVoxelization, performEmittanceVoxelization);
	if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';

	if (performOpacityVoxelization)
	{
		//giObject->voxelizeTestScene({ 0.f, 0.f, 0.f }, 5.f, shaderCompiler);

		VXGI::VoxelizationViewParameters vvParams;
		status = giObject->getVoxelizationViewParameters(vvParams);
		if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';

		//memcpy(&voxelizationViewMatrix, &vvParams.viewMatrix, sizeof(vvParams.viewMatrix));
		//memcpy(&voxelizationProjMatrix, &vvParams.projectionMatrix, sizeof(vvParams.projectionMatrix));
		voxelizationViewMatrix = *(glm::mat4*)&vvParams.viewMatrix;
		voxelizationProjMatrix = *(glm::mat4*)&vvParams.projectionMatrix;


		VXGI::MaterialInfo matInfo = {};
		matInfo.geometryShader = gsShaderSet;
		matInfo.pixelShader = psShaderSet;

		status = giObject->getVoxelizationState(matInfo, true, *drawCallState);
		if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';

		rendererInterface->applyState(*drawCallState);

		std::shared_ptr<DX11VertexShader> nullVertexShader = nullptr;
		std::shared_ptr<DX11PixelShader> nullPixelShader = nullptr;

		m_DX11DeviceContext->PSSetShaderResources(6, NUM_POINT_LIGHTS, m_PointLightDepthSRVs);
		m_DX11DeviceContext->PSSetShaderResources(4, 1, &m_SpotLightDepthSRV);

		const uint32_t maxRegions = 128;
		uint32_t numRegions = 0;
		VXGI::Box3f regions[maxRegions];

		if (VXGI_SUCCEEDED(giObject->getInvalidatedRegions(regions, maxRegions, numRegions)))
		{
			RenderScene(voxelizationViewMatrix, voxelizationProjMatrix, m_DefaultVertexShader, nullPixelShader, true, regions, numRegions, false);
		}

		rendererInterface->clearState();
	}

	status = giObject->finalizeVoxelization();
	if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';
}

void TestApplication::NvidiaVXGIConeTracing()
{
	VXGI::Status::Enum status;

	basicViewTracer->beginFrame();

	VXGI::BasicDiffuseTracingParameters bdtParams;
	VXGI::BasicSpecularTracingParameters bstParams;
	VXGI::IBasicViewTracer::InputBuffers inputBuffers;

	bdtParams.enableTemporalReprojection = g_bTemporalFiltering;
	bdtParams.enableTemporalJitter = g_bTemporalFiltering;
	bdtParams.quality = g_fQuality;
	bdtParams.directionalSamplingRate = g_fSamplingRate;
	bdtParams.ambientRange = m_fVxaoRange;
	//bdtParams.interpolationWeightThreshold = 0.1f;
	bdtParams.ambientScale = m_fVxaoScale;

	bdtParams.irradianceScale = g_fDiffuseScale;
	bstParams.irradianceScale = g_fSpecularScale;
	bstParams.filter = g_bTemporalFiltering ? VXGI::SpecularTracingParameters::FILTER_TEMPORAL : VXGI::SpecularTracingParameters::FILTER_SIMPLE;
	bstParams.enableTemporalJitter = g_bTemporalFiltering;
	bstParams.enableConeJitter = true;

	//auto normalSRV = gBuffer->GetShaderResourceView(1);
	//ID3D11Resource* normalRes;
	//normalSRV->GetResource(&normalRes);
	//auto depthTexture = gBuffer->GetDepthStencilTexture();

	//m_GBufferDepth = rendererInterface->getHandleForTexture(depthTexture, NVRHI::Format::D24S8);
	//m_GBufferNormal = rendererInterface->getHandleForTexture(normalRes, NVRHI::Format::RGBA16_FLOAT);
	//normalRes->Release();

	//inputBuffers.gbufferDepth = m_GBufferDepth;
	//inputBuffers.gbufferNormal = m_GBufferNormal;
	inputBuffers.gbufferDepth = m_TargetDepth;
	inputBuffers.gbufferNormal = m_TargetNormalRoughness;
	inputBuffers.gbufferViewport = NVRHI::Viewport((float)texWidth, (float)texHeight);

	glm::mat4 gBufferProjMat = glm::perspectiveFovLH(XM_PIDIV4, (FLOAT)texWidth, (FLOAT)texHeight, NEAR_PLANE, FAR_PLANE);
	glm::mat4 gBufferViewMat = cameras[SelectedCamera].GetViewMatrix();
	//memcpy(&inputBuffers.viewMatrix, &gBufferViewMat, sizeof(gBufferViewMat));
	//memcpy(&inputBuffers.projMatrix, &gBufferProjMat, sizeof(gBufferProjMat));
	inputBuffers.viewMatrix = *(VXGI::float4x4*)&gBufferViewMat;
	inputBuffers.projMatrix = *(VXGI::float4x4*)&gBufferProjMat;

	indirectDiffuse = nullptr;
	indirectConfidence = nullptr;
	indirectSpecular = nullptr;
	ssao = nullptr;

	if (g_fDiffuseScale > 0)
	{
		status = basicViewTracer->computeDiffuseChannelBasic(bdtParams, inputBuffers, g_InputBuffersPrevValid ? &g_InputBuffersPrev : nullptr, indirectDiffuse, indirectConfidence);
		if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';
	}

	if (g_fSpecularScale > 0)
	{
		status = basicViewTracer->computeSpecularChannelBasic(bstParams, inputBuffers, g_InputBuffersPrevValid ? &g_InputBuffersPrev : nullptr, indirectSpecular);
		if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';
	}

	if (g_bEnableSSAO)
	{
		VXGI::SsaoParamaters ssaoParams;
		ssaoParams.radiusWorld = g_fSsaoRadius;

		basicViewTracer->computeSsaoChannelBasic(ssaoParams, inputBuffers, ssao);
	}

	if (indirectDiffuse && indirectConfidence)
	{
		indirectSRV = rendererInterface->getSRVForTexture(indirectDiffuse);
		confidenceSRV = rendererInterface->getSRVForTexture(indirectConfidence);
	}
	if (indirectSpecular)
	{
		indirectSpecularSRV = rendererInterface->getSRVForTexture(indirectSpecular);
	}
	if (ssao)
		ssaoSRV = rendererInterface->getSRVForTexture(ssao);

	//g_InputBuffersPrev = inputBuffers;
	//g_InputBuffersPrevValid = true;
}

void TestApplication::PrepareGbufferRenderTargets(int width, int height)
{
	NVRHI::TextureDesc gbufferDesc;
	gbufferDesc.width = width;
	gbufferDesc.height = height;
	gbufferDesc.isRenderTarget = true;
	gbufferDesc.useClearValue = true;
	gbufferDesc.sampleCount = 1;
	gbufferDesc.disableGPUsSync = true;

	gbufferDesc.format = NVRHI::Format::RGBA8_UNORM;
	gbufferDesc.clearValue = NVRHI::Color(0.f);
	gbufferDesc.debugName = "GbufferAlbedo";
	rendererInterface->forgetAboutTexture(rendererInterface->getResourceForTexture(m_TargetAlbedo));
	m_TargetAlbedo = rendererInterface->createTexture(gbufferDesc, nullptr);

	gbufferDesc.format = NVRHI::Format::RGBA16_FLOAT;
	gbufferDesc.clearValue = NVRHI::Color(0.f);
	gbufferDesc.debugName = "GbufferNormalRoughness";
	rendererInterface->forgetAboutTexture(rendererInterface->getResourceForTexture(m_TargetNormalRoughness));
	m_TargetNormalRoughness = rendererInterface->createTexture(gbufferDesc, nullptr);
	rendererInterface->forgetAboutTexture(rendererInterface->getResourceForTexture(m_TargetNormalRoughnessPrev));
	m_TargetNormalRoughnessPrev = rendererInterface->createTexture(gbufferDesc, nullptr);

	gbufferDesc.format = NVRHI::Format::RGBA16_FLOAT;
	gbufferDesc.clearValue = NVRHI::Color(0.f);
	gbufferDesc.debugName = "GbufferPositionMetallic";
	rendererInterface->forgetAboutTexture(rendererInterface->getResourceForTexture(m_TargetPositionMetallic));
	m_TargetPositionMetallic = rendererInterface->createTexture(gbufferDesc, nullptr);

	gbufferDesc.format = NVRHI::Format::D24S8;
	gbufferDesc.clearValue = NVRHI::Color(1.f, 0.f, 0.f, 0.f);
	gbufferDesc.debugName = "GbufferDepth";
	rendererInterface->forgetAboutTexture(rendererInterface->getResourceForTexture(m_TargetDepth));
	m_TargetDepth = rendererInterface->createTexture(gbufferDesc, nullptr);
	rendererInterface->forgetAboutTexture(rendererInterface->getResourceForTexture(m_TargetDepthPrev));
	m_TargetDepthPrev = rendererInterface->createTexture(gbufferDesc, nullptr);

	gbufferDesc.format = NVRHI::Format::RGBA16_FLOAT;
	gbufferDesc.clearValue = NVRHI::Color(0.f);
	gbufferDesc.debugName = "GbufferLightViewPosition";
	rendererInterface->forgetAboutTexture(rendererInterface->getResourceForTexture(m_TargetLightViewPosition));
	m_TargetLightViewPosition = rendererInterface->createTexture(gbufferDesc, nullptr);
}

void TestApplication::RenderToGBufferBegin()
{
	std::swap(m_TargetNormalRoughness, m_TargetNormalRoughnessPrev);
	std::swap(m_TargetDepth, m_TargetDepthPrev);

	gbufferState->renderState.targetCount = 4;
	gbufferState->renderState.targets[0] = m_TargetAlbedo;
	gbufferState->renderState.targets[1] = m_TargetNormalRoughness;
	gbufferState->renderState.targets[2] = m_TargetPositionMetallic;
	gbufferState->renderState.targets[3] = m_TargetLightViewPosition;
	gbufferState->renderState.clearColorTarget = true;
	gbufferState->renderState.clearColor = NVRHI::Color(0.529f, 0.808f, 0.922f, 1.f);
	gbufferState->renderState.depthTarget = m_TargetDepth;
	gbufferState->renderState.clearDepthTarget = true;
	gbufferState->renderState.clearDepth = 1.f;
	gbufferState->renderState.viewportCount = 1;
	gbufferState->renderState.viewports[0] = NVRHI::Viewport(float(texWidth), float(texHeight));

	rendererInterface->applyState(*gbufferState);

	m_DX11DeviceContext->PSSetShaderResources(4, 1, &m_SpotLightDepthSRV);

	auto perspectiveMat = glm::perspectiveFovLH(XM_PIDIV4, (FLOAT)texWidth, (FLOAT)texHeight, NEAR_PLANE, FAR_PLANE);
	//auto reverse_z = reverseZ(perspectiveMat);
	RenderScene(cameras[SelectedCamera].GetViewMatrix(), perspectiveMat, m_DeferredVertexShader, m_DeferredPixelShader, false, nullptr, 0, true);
}

void TestApplication::RenderToGBufferEnd()
{
	rendererInterface->clearState();
}

glm::mat4 TestApplication::reverseZ(const glm::mat4& perspeciveMat)
{
	constexpr glm::mat4 reverseZ = { 1.0f, 0.0f, 0.0f, 0.0f,
									0.0f, 1.0f,  0.0f, 0.0f,
									0.0f, 0.0f, -1.0f, 0.0f,
									0.0f, 0.0f,  1.0f, 1.0f };
	return reverseZ * perspeciveMat;
}

void TestApplication::RenderScene(glm::mat4 viewMatrix, glm::mat4 projMatrix, std::shared_ptr<DX11VertexShader>& vertexShader, std::shared_ptr<DX11PixelShader>& pixelShader, bool voxelizing, const VXGI::Box3f* clippingBoxes, uint32_t numBoxes, bool frustumCulling)
{
	LightCB lightCb;
	lightCb.lightPos[0] = glm::vec4(cameras[1].GetPosition().x, cameras[1].GetPosition().y, cameras[1].GetPosition().z, 1.f);
	lightCb.lightPos[1] = glm::vec4(cameras[2].GetPosition().x, cameras[2].GetPosition().y, cameras[2].GetPosition().z, 1.f);
	lightCb.lightPos[2] = glm::vec4(cameras[3].GetPosition().x, cameras[3].GetPosition().y, cameras[3].GetPosition().z, 1.f);
	lightCb.lightPos[3] = glm::vec4(cameras[4].GetPosition().x, cameras[4].GetPosition().y, cameras[4].GetPosition().z, 1.f);
	lightCb.conLinQuad[0] = lightProps.constant;
	lightCb.conLinQuad[1] = lightProps.linear;
	lightCb.conLinQuad[2] = lightProps.quadratic;
	lightCb.lightColor = { lightProps.color[0], lightProps.color[1], lightProps.color[2] };
	lightCb.lightViewProjMat = spotlightProjMat * cameras[NUM_POINT_LIGHTS + 1].GetViewMatrix();
	lightCb.bias = m_ShadowMapBias;
	lightCb.numLights = NUM_POINT_LIGHTS;
	for (int i = 0; i < 6; i++)
	{
		lightCb.cubeView[i] = m_PointLightDepthCaptureViews[i];
	}
	lightCb.cubeProj = lightProjMat;
	lightCb.camPos = cameras[SelectedCamera].GetPosition();
	lightCb.showDiffuseTexture = m_bShowDiffuseTexture;
	lightCb.enableGI = m_bEnableGI;

	m_DX11DeviceContext->UpdateSubresource(g_pCBLight, 0, NULL, &lightCb, 0, 0);
	if (!voxelizing)
	{
		m_DX11DeviceContext->VSSetConstantBuffers(1, 1, &g_pCBLight);
		m_DX11DeviceContext->PSSetConstantBuffers(1, 1, &g_pCBLight);
		m_DX11DeviceContext->GSSetConstantBuffers(1, 1, &g_pCBLight);
	}
	else
	{
		m_DX11DeviceContext->VSSetConstantBuffers(1, 1, &g_pCBLight);
		m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBLight);
	}

	m_MeshesDrawn = 0;
	for (int i = 0; i < models.size(); i++)
	{
		auto translate = props[i].translation;
		auto rotate = props[i].rotation;
		auto scale = props[i].scale;

		ConstantBuffer cb;
		auto modelMat = glm::mat4(1.f);
		modelMat = glm::translate(modelMat, glm::vec3(translate[0], translate[1], translate[2]));
		modelMat = glm::rotate(modelMat, glm::radians(rotate[0]), glm::vec3(1.f, 0.f, 0.f));
		modelMat = glm::rotate(modelMat, glm::radians(rotate[1]), glm::vec3(0.f, 1.f, 0.f));
		modelMat = glm::rotate(modelMat, glm::radians(rotate[2]), glm::vec3(0.f, 0.f, 1.f));
		modelMat = glm::scale(modelMat, glm::vec3(scale[0], scale[1], scale[2]));
		if (i == 0)
			modelMat = coltMat;    // games with physics
		cb.modelMat = modelMat;
		cb.viewMat = viewMatrix;
		cb.projMat = projMatrix;
		cb.normalMat = glm::transpose(glm::inverse(modelMat));

		MaterialCB matCb;
		if (models[i]->GetFilePath().find("Sponza.gltf") != std::string::npos)
			matCb.sponza = 1.0f;
		else
			matCb.sponza = 0.f;
		matCb.normalStrength = normalStrength;
		matCb.renderLightCube = 0;

		m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, NULL, &cb, 0, 0);
		m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, NULL, &matCb, 0, 0);

		m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);
		if (!voxelizing)
		{
			m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);
			m_DX11DeviceContext->VSSetConstantBuffers(2, 1, &g_pCBMaterial);
			m_DX11DeviceContext->PSSetConstantBuffers(2, 1, &g_pCBMaterial);
		}

		m_DX11DeviceContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
		if (!voxelizing)
		{
			m_DX11DeviceContext->PSSetSamplers(1, 1, &g_pSamplerClamp);
			m_DX11DeviceContext->PSSetSamplers(2, 1, &g_pSamplerComparison);
		}
		else
			m_DX11DeviceContext->PSSetSamplers(1, 1, &g_pSamplerComparison);


		if (frustumCulling)
		{
			Frustum frustum;
			frustum.ConstructFrustum(viewMatrix, projMatrix, FAR_PLANE);
			m_MeshesDrawn += models[i]->Draw(vertexShader, pixelShader, clippingBoxes, numBoxes, modelMat, &frustum);
		}
		else
		{
			models[i]->Draw(vertexShader, pixelShader, clippingBoxes, numBoxes, modelMat, nullptr);
		}
	}
}

std::unique_ptr<Application> Oeuvre::CreateApplication()
{
	return std::make_unique<TestApplication>();
}

void GIErrorCallback::signalError(const char* file, int line, const char* errorDesc)
{
	OV_ERROR("GI::ERROR::In file {} line {} occured error: {}", file, line, errorDesc);
}

void TestApplication::initPhysics(bool interactive)
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

	gPvd = PxCreatePvd(*gFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	gDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	gScene = gPhysics->createScene(sceneDesc);

	PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
	if (pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
	gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

	PxRigidStatic* groundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
	gScene->addActor(*groundPlane);


	for (PxU32 i = 0; i < 5; i++)
		createStack(PxTransform(PxVec3(0, 0, stackZ -= 5.0f)), 10, m_boxHalfExtent);

	if (!interactive)
		createDynamic(PxTransform(PxVec3(0, 40, 100)), PxBoxGeometry(m_boxHalfExtent, m_boxHalfExtent, m_boxHalfExtent), PxVec3(0, -50, -100));

	// create pystol actor
	PxTransform localTm(0, 5.f, 0);
	m_coltPhysicsActor = gPhysics->createRigidDynamic(localTm);
	auto coltMeshes = models[0]->GetMeshes();
	for (Mesh& mesh: coltMeshes)
	{
		auto meshBounds = mesh.GetBounds();

		meshBounds.lower *= 10.f;
		meshBounds.upper *= 10.f;

		auto xExtent = (meshBounds.upper.x - meshBounds.lower.x) / 2.f;
		auto yExtent = (meshBounds.upper.y - meshBounds.lower.y) / 2.f;
		auto zExtent = (meshBounds.upper.z - meshBounds.lower.z) / 2.f;

		PxShape* shape = gPhysics->createShape(PxBoxGeometry(xExtent, yExtent, zExtent), *gMaterial);
		PxMaterial* mMaterial = gPhysics->createMaterial(0.9f, 0.9f, 0.1f);
		if (mMaterial)
		{
			shape->setMaterials(&mMaterial, 1);
		}
		
		m_coltPhysicsActor->attachShape(*shape);		
	}
	PxRigidBodyExt::updateMassAndInertia(*m_coltPhysicsActor, 10.0f);
	gScene->addActor(*m_coltPhysicsActor);
}

void TestApplication::stepPhysics(float elapsedTime /*interactive*/)
{
	gScene->simulate(1.f/60.f);
	gScene->fetchResults(true);
}

void TestApplication::cleanupPhysics(bool /*interactive*/)
{
	PX_RELEASE(gScene);
	PX_RELEASE(gDispatcher);
	PX_RELEASE(gPhysics);
	if (gPvd)
	{
		PxPvdTransport* transport = gPvd->getTransport();
		PX_RELEASE(gPvd);
		PX_RELEASE(transport);
	}
	PX_RELEASE(gFoundation);

	printf("SnippetHelloWorld done.\n");
}

void TestApplication::renderPhysics(float deltaTime)
{
	stepPhysics(deltaTime);

	PxScene* scene;
	PxGetPhysics().getScenes(&scene, 1);
	PxU32 nbActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
	if (nbActors)
	{
		PxArray<PxRigidActor*> actors(nbActors);
		scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC, reinterpret_cast<PxActor**>(&actors[0]), nbActors);

		for (int i = 0; i < nbActors; i++)
		{
			PxRigidActor* actor = actors[i];

			PxShape* shapes[128];
			const PxU32 nbShapes = actor->getNbShapes();
			actor->getShapes(shapes, nbShapes);

			//std::cout << "Actor has " << nbShapes << " shapes\n";

			for (PxU32 j = 0; j < nbShapes; j++)
			{
				const PxMat44 shapePose(PxShapeExt::getGlobalPose(*shapes[j], *actor));
				const PxGeometry& geom = shapes[j]->getGeometry();

				auto pxPos = shapePose.getPosition();

				shapePos = glm::vec3(pxPos.x, pxPos.y, pxPos.z);

				auto shapePosMat = shapePose;
				//shapePosMat.scale(PxVec4(0.2f, 0.2f, 0.2f, 1.f));

				glm::mat4 boxModelMat = *(glm::mat4*)&shapePosMat;

				boxModelMat = glm::scale(boxModelMat, glm::vec3(m_boxHalfExtent));

				m_BoxAlbedo->Bind(0);
				m_BoxNormal->Bind(1);
				m_BoxRoughness->Bind(2);

				if (actor != m_coltPhysicsActor)
					RenderCube(boxModelMat);
			}
		}
		PxShape* shapes[128];
		const PxU32 nbShapes = m_coltPhysicsActor->getNbShapes();
		m_coltPhysicsActor->getShapes(shapes, nbShapes);
		PxMat44 shapePose(PxShapeExt::getGlobalPose(*shapes[0], *m_coltPhysicsActor));

		//shapePose.scale(PxVec4(5.f));
		coltMat = *(glm::mat4*)&shapePose;
		coltMat = glm::scale(coltMat, glm::vec3(10.f));
	}
}

PxRigidDynamic* TestApplication::createDynamic(const PxTransform& t, const PxGeometry& geometry, const PxVec3& velocity)
{
	PxRigidDynamic* dynamic = PxCreateDynamic(*gPhysics, t, geometry, *gMaterial, 100.0f);
	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);
	gScene->addActor(*dynamic);
	return dynamic;
}

void TestApplication::createStack(const PxTransform& t, PxU32 size, PxReal halfExtent)
{
	PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *gMaterial);
	for (PxU32 i = 0; i < size; i++)
	{
		for (PxU32 j = 0; j < size - i; j++)
		{
			PxTransform localTm(PxVec3(PxReal(j * 2) - PxReal(size - i), PxReal(i * 2 + 1), 0) * halfExtent);
			PxRigidDynamic* body = gPhysics->createRigidDynamic(t.transform(localTm));
			body->attachShape(*shape);
			PxRigidBodyExt::updateMassAndInertia(*body, 100.0f);
			gScene->addActor(*body);
		}
	}
	shape->release();
}

void TestApplication::RenderModelBounds(int modelIndex)
{
	auto modelBounds = models[modelIndex]->GetMeshBounds();
	for (auto& bound : modelBounds)
	{
		auto width = abs(bound.upper.x - bound.lower.x);
		auto height = abs(bound.upper.y - bound.lower.y);
		auto depth = abs(bound.upper.z - bound.lower.z);

		auto lowerDimension = std::min({ width, height, depth });

		auto modelMat = glm::mat4(1.f);

		auto translate = props[modelIndex].translation;
		auto rotate = props[modelIndex].rotation;
		auto scale = props[modelIndex].scale;

		//modelMat = glm::translate(modelMat, glm::vec3(translate[0], translate[1], translate[2]));
		//modelMat = glm::rotate(modelMat, glm::radians(rotate[0]), glm::vec3(1.f, 0.f, 0.f));
		//modelMat = glm::rotate(modelMat, glm::radians(rotate[1]), glm::vec3(0.f, 1.f, 0.f));
		//modelMat = glm::rotate(modelMat, glm::radians(rotate[2]), glm::vec3(0.f, 0.f, 1.f));
		//modelMat = glm::scale(modelMat, glm::vec3(scale[0], scale[1], scale[2]));

		if (modelIndex == 0)
			modelMat = coltMat;

		modelMat = glm::scale(modelMat, glm::vec3(lowerDimension/height / 5.f, lowerDimension/width / 5.f, lowerDimension/depth / 5.f));

		RenderCube(modelMat);
	}
}
