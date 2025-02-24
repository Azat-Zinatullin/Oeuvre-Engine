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

#include <limits>

#include <random>

#define CHECK_STATUS(status) if (status != 0) std::cout << "Something went wrong. Status: " << status << '\n';

physx::PxFilterFlags CollisionFilterShader(
	physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
	physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
	physx::PxPairFlags& pairFlags, const void* /*constantBlock*/, PxU32 /*constantBlockSize*/)
{
	// let triggers through
	if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		return PxFilterFlag::eDEFAULT;
	}
	// generate contacts for all that were not filtered above
	pairFlags = PxPairFlag::eCONTACT_DEFAULT;

	// trigger the contact callback for pairs (A,B) where
	// the filtermask of A contains the ID of B and vice versa.
	if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;

	return PxFilterFlag::eDEFAULT;
}

struct FilterGroup
{
	enum Enum
	{
		eFLOOR = (1 << 0),
		eOBJECT1 = (1 << 1),
		eOBJECT2 = (1 << 2),
		eHEIGHTFIELD = (1 << 3),
	};
};

void setupFiltering(PxShape* shape, PxU32 filterGroup, PxU32 filterMask)
{
	PxFilterData filterData;
	filterData.word0 = filterGroup; // word0 = own ID
	filterData.word1 = filterMask;  // word1 = ID mask to filter pairs that trigger a contact callback
	shape->setSimulationFilterData(filterData);
}

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

struct __declspec(align(16)) MatricesCB
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
	glm::vec4 rcpFrame;
	int numLights;
	int enableGI;
};

struct __declspec(align(16)) MaterialCB
{
	float sponza;
	float normalStrength;
	int renderLightCube;
};

struct __declspec(align(16)) SkeletalAnimationCB
{
	glm::mat4 finalBonesMatrices[MAX_BONES];
};

struct __declspec(align(16)) PostprocessingCB
{
	float padding;
	int enableFXAA;
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
	m_Models.emplace_back(model);
	m_Models.emplace_back(new Model("..\\resources\\sponza\\glTF\\Sponza.gltf", "", "", "", ""));
	//m_Models.emplace_back(new Model("..\\resources\\SunTemple_v4\\SunTemple.fbx", "", "", "", ""));
	//m_Models.emplace_back(new Model("..\\resources\\classic_sponza\\Meshes\\Sponza_Modular.FBX", "", "", "", ""));
	//m_Models.emplace_back(new Model("..\\resources\\sponza_pbr\\sponza.obj", "", "", "", ""));
	//m_Models.emplace_back(new Model("..\\resources\\sponza-gltf-pbr\\sponza.glb", "", "", "", ""));

	m_AnimatedModel = new Model("../resources/mixamo/Walking_fixed.fbx", "../resources/mixamo/eve/SpacePirate_M_diffuse.png", "../resources/mixamo/eve/SpacePirate_M_normal.png", "", "");

	m_Models.emplace_back(m_AnimatedModel);
	m_ModelProps.emplace_back(Properties());

	m_RunAnimation = new Animation("../resources/mixamo/Running_fixed.fbx", m_AnimatedModel);
	m_IdleAnimation = new Animation("../resources/mixamo/Pistol_Idle_fixed.fbx", m_AnimatedModel);
	m_LeftTurnAnimation = new Animation("../resources/mixamo/Left_Turn_fixed.fbx", m_AnimatedModel);
	m_RightTurnAnimation = new Animation("../resources/mixamo/Right_Turn_fixed.fbx", m_AnimatedModel);
	//m_ShootingAnimation = new Animation("../resources/mixamo/Pistol_Shooting_fixed.fbx", m_AnimatedModel);

	m_Animator = new Animator(m_IdleAnimation);

	// Send all reports to STDOUT
	//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	//_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	//_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	//_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	//_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	//_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
	//_CrtDumpMemoryLeaks();

	m_ModelProps.emplace_back(Properties());
	m_ModelProps.emplace_back(Properties());

	m_ModelProps[0].scale[0] = m_ModelProps[0].scale[1] = m_ModelProps[0].scale[2] = 5.f;
	m_ModelProps[0].translation[1] = m_ModelProps[0].translation[0] = 2.5f;
	m_ModelProps[0].rotation[0] = -90.f;
	m_ModelProps[1].scale[0] = m_ModelProps[1].scale[1] = m_ModelProps[1].scale[2] = SPONZA_SCALE;
	m_ModelProps[1].rotation[1] = 90.f;
	m_Models[1]->GetUseCombinedTextures() = false;

	m_ModelProps[2].scale[0] = m_ModelProps[2].scale[1] = m_ModelProps[2].scale[2] = 0.016f;

	// for sponza
	m_LightProps.lightPos[0] = { 0.f, 21.f, 0.f };
	m_LightProps.lightPos[1] = { 30.f, 4.8f, -2.95f };
	m_LightProps.lightPos[2] = { -30.f, 3.6f, 19.5f };


	InitDXStuff();

	FramebufferToTextureInit();


	D3D11_INPUT_ELEMENT_DESC defaultShaderInputLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D3D11_INPUT_ELEMENT_DESC skeletalAnimationInputLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONEIDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};


	m_DefaultVertexShader = std::make_shared<DX11VertexShader>("src/shaders/DefaultVertexShader.hlsl", skeletalAnimationInputLayout, ARRAYSIZE(skeletalAnimationInputLayout));

	//m_SpotLightDepthVertexShader = std::make_shared<DX11VertexShader>("src/shaders/SpotLightDepthVertexShader.hlsl", defaultShaderInputLayout, ARRAYSIZE(defaultShaderInputLayout));
	//m_SpotLightDepthPixelShader = std::make_shared<DX11PixelShader>("src/SpotLightDepthPixelShader.hlsl");

	m_SpotLightVertexShader = std::make_shared<DX11VertexShader>("src/shaders/SpotLightVertexShader.hlsl", defaultShaderInputLayout, ARRAYSIZE(defaultShaderInputLayout));
	m_SpotLightPixelShader = std::make_shared<DX11PixelShader>("src/shaders/SpotLightPixelShader.hlsl");

	m_PointLightDepthVertexShader = std::make_shared<DX11VertexShader>("src/shaders/PointLightDepthVertexShader.hlsl", skeletalAnimationInputLayout, ARRAYSIZE(skeletalAnimationInputLayout));
	m_PointLightDepthPixelShader = std::make_shared<DX11PixelShader>("src/shaders/PointLightDepthPixelShader.hlsl");
	m_PointLightDepthGeometryShader = std::make_shared<DX11GeometryShader>("src/shaders/PointLightDepthGeometryShader.hlsl");

	m_CubePixelShader = std::make_shared<DX11PixelShader>("src/shaders/CubePixelShader.hlsl");

	m_DeferredVertexShader = std::make_shared<DX11VertexShader>("src/shaders/DeferredVertexShader.hlsl", skeletalAnimationInputLayout, ARRAYSIZE(skeletalAnimationInputLayout));
	m_DeferredPixelShader = std::make_shared<DX11PixelShader>("src/shaders/DeferredPixelShader.hlsl");

	m_DeferredCompositingVertexShader = std::make_shared<DX11VertexShader>("src/shaders/DeferredCompositingVertexShader.hlsl", defaultShaderInputLayout, ARRAYSIZE(defaultShaderInputLayout));
	m_DeferredCompositingPixelShader = std::make_shared<DX11PixelShader>("src/shaders/DeferredCompositingPixelShader.hlsl");

	m_VoxelizationPixelShader = std::make_shared<DX11PixelShader>("src/shaders/VoxelizationPixelShader.hlsl");

	// for sky cubemapping
	m_RectToCubeVertexShader = std::make_shared<DX11VertexShader>("src/shaders/RectToCubeShaders.hlsl", skeletalAnimationInputLayout, ARRAYSIZE(skeletalAnimationInputLayout));
	m_RectToCubePixelShader = std::make_shared<DX11PixelShader>("src/shaders/RectToCubeShaders.hlsl");
	m_CubemapVertexShader = std::make_shared<DX11VertexShader>("src/shaders/CubemapShaders.hlsl", skeletalAnimationInputLayout, ARRAYSIZE(skeletalAnimationInputLayout));
	m_CubemapPixelShader = std::make_shared<DX11PixelShader>("src/shaders/CubemapShaders.hlsl");

	// postprocessing (FXAA)
	m_PostprocessingPixelShader = std::make_shared<DX11PixelShader>("src/shaders/PostprocessingPixelShader.hlsl");


	//SpotLightDepthToTextureInit();
	for (int i = 0; i < NUM_POINT_LIGHTS; i++)
	{
		m_PointLightDepthSRVs[i] = nullptr;
		m_PointLightDepthTextures[i] = nullptr;
		m_PointLightDepthRenderTextures[i] = nullptr;
	}
	PointLightDepthToTextureInit();


	m_Cameras.emplace_back(Camera()); // main camera 
	// light cameras and prev positions
	for (int i = 0; i < NUM_POINT_LIGHTS; i++)
	{
		m_Cameras.emplace_back(Camera());
		lightPositionsPrev.emplace_back(glm::vec3(0.f));
	}
	m_Cameras.emplace_back(Camera()); // for spotlight


	//gBuffer = new DX11GBuffer();
	//gBuffer->Initialize(texWidth, texHeight);

	NvidiaVXGIInit();
	RenderCubeInit();
	RenderQuadInit();

	// for sky cubemapping
	RenderCubeFromTheInsideInit();
	InitHDRCubemap("../resources/hdrs/kloofendal_48d_partly_cloudy_puresky_4k.hdr");

	initPhysics(true);

	m_BoxAlbedo = Texture2D::Create("..\\resources\\wood_planks_4k\\wood_planks_diff_4k.png");
	m_BoxNormal = Texture2D::Create("..\\resources\\wood_planks_4k\\wood_planks_nor_dx_4k.png");
	m_BoxRoughness = Texture2D::Create("..\\resources\\wood_planks_4k\\wood_planks_rough_4k.png");

	FMODInit();

	//m_GamePad = std::make_unique<GamePad>();

	PostprocessingToTextureInit();

	return true;
}

void TestApplication::InitDXStuff()
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(MatricesCB);
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

	bd.ByteWidth = sizeof(SkeletalAnimationCB);
	hr = m_DX11Device->CreateBuffer(&bd, NULL, &g_pCBSkeletalAnimation);
	if (FAILED(hr)) std::cout << "Can't create g_pCBSkeletalAnimation\n";

	bd.ByteWidth = sizeof(PostprocessingCB);
	hr = m_DX11Device->CreateBuffer(&bd, NULL, &g_pCBPostprocessing);
	if (FAILED(hr)) std::cout << "Can't create g_pCBPostprocessing\n";

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

	// for cubemapping
	D3D11_RASTERIZER_DESC rdesc;
	ZeroMemory(&rdesc, sizeof(rdesc));
	rdesc.CullMode = D3D11_CULL_NONE;
	rdesc.FillMode = D3D11_FILL_SOLID;
	hr = m_DX11Device->CreateRasterizerState(&rdesc, &m_CubemapRasterizerState);
	if (FAILED(hr)) std::cout << "Can't create RasterizerState!\n";

	D3D11_DEPTH_STENCIL_DESC dsdesc;
	ZeroMemory(&dsdesc, sizeof(dsdesc));
	dsdesc.DepthEnable = true;
	dsdesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = m_DX11Device->CreateDepthStencilState(&dsdesc, &m_CubemapDepthStencilState);
	if (FAILED(hr)) std::cout << "Can't create DepthStencilState!\n";

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
}

void TestApplication::RenderLoop()
{
	while (m_Running)
	{
		m_Window->OnUpdate();

		float currentFrame = static_cast<float>(glfwGetTime());
		m_DeltaTime = currentFrame - m_LastFrame;
		m_LastFrame = currentFrame;

		FMODUpdate();

		HandleMouseInput();

		if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_W) == GLFW_PRESS)
			m_Cameras[SelectedCamera].ProcessKeyboard(CameraMovement::FORWARD, m_DeltaTime);
		if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_S) == GLFW_PRESS)
			m_Cameras[SelectedCamera].ProcessKeyboard(CameraMovement::BACKWARD, m_DeltaTime);
		if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_A) == GLFW_PRESS)
			m_Cameras[SelectedCamera].ProcessKeyboard(CameraMovement::LEFT, m_DeltaTime);
		if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_D) == GLFW_PRESS)
			m_Cameras[SelectedCamera].ProcessKeyboard(CameraMovement::RIGHT, m_DeltaTime);

		if (viewPortActive)
		{
			glfwSetInputMode(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else
		{
			glfwSetInputMode(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}


		//if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed)
		//{
		//	auto glmPos = m_Cameras[0].GetPosition();
		//	auto glmDir = m_Cameras[0].GetFrontVector();

		//	PxTransform t;
		//	t.p = PxVec3(glmPos.x, glmPos.y, glmPos.z);
		//	t.q = PxQuat(PxIDENTITY::PxIdentity);

		//	createDynamic(t, PxBoxGeometry(m_boxHalfExtent, m_boxHalfExtent, m_boxHalfExtent), PxVec3(glmDir.x, glmDir.y, glmDir.z) * 30);

		//	FMOD::Channel* channel;
		//	m_FmodSystem->playSound(m_WhooshSound, nullptr, false, &channel);

		//	spacePressed = true;
		//}
		//if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_SPACE) == GLFW_RELEASE)
		//	spacePressed = false;

		//if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_V) == GLFW_PRESS)
		//{
		//	m_coltPhysicsActor->addTorque(PxVec3(300.f, 500.f, 0.f));
		//}

		// setting deafult camera properties
		m_CameraViewMatrix = m_Cameras[SelectedCamera].GetViewMatrix();
		m_CameraPos = m_Cameras[SelectedCamera].GetPosition();
		m_CameraFrontVector = m_Cameras[SelectedCamera].GetFrontVector();

		if (!m_bFreeCameraView)
			MoveCharacter(m_CameraViewMatrix, m_CameraPos, m_CameraFrontVector);

		ImguiFrame();

		m_Renderer->BeginScene();	

		ChangeDepthBiasParameters(depthBias, slopeBias, depthBiasClamp);

		// rendering depth to render texture for point light
		bool lightPosChanged = false;
		for (int i = 0; i < NUM_POINT_LIGHTS; i++)
		{
			lightPosChanged = lightPositionsPrev[i] != m_Cameras[1 + i].GetPosition();
			if (lightPosChanged)
				break;
		}
		if (lightPosChanged || true)
		{
			for (int i = 0; i < NUM_POINT_LIGHTS; i++)
			{
				PreparePointLightViewMatrixes(m_Cameras[i + 1].GetPosition());
				//static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->SetCullFront();
				m_PointLightDepthVertexShader->Bind();
				m_PointLightDepthGeometryShader->Bind();
				m_PointLightDepthPixelShader->Bind();
				PointLightDepthToTextureBegin(i);
				RenderScene(/*m_Cameras[0].GetViewMatrix()*/m_CameraViewMatrix, lightProjMat, m_PointLightDepthVertexShader, m_PointLightDepthPixelShader, m_PointLightDepthGeometryShader, false, nullptr, 0, false);
				PointLightDepthToTextureEnd(i);
				m_PointLightDepthVertexShader->Unbind();
				m_PointLightDepthGeometryShader->Unbind();
				m_PointLightDepthPixelShader->Unbind();
				//static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->SetCullBack();
			}
			lightPositionsPrev[0] = m_Cameras[1].GetPosition();
			lightPositionsPrev[1] = m_Cameras[2].GetPosition();
			lightPositionsPrev[2] = m_Cameras[3].GetPosition();
		}

		ResetRenderState();

		// rendering depth to render texture for spot light	
		//m_DX11DeviceContext->OMSetDepthStencilState(m_pGBufferReverseZDepthStencilState, 0); // for reverse z buffer
	/*	SpotLightDepthToTextureBegin();
		auto reverseProjMat = reverseZ(spotlightProjMat);
		RenderScene(m_Cameras[NUM_POINT_LIGHTS + 1].GetViewMatrix(), spotlightProjMat, m_SpotLightDepthVertexShader, m_SpotLightDepthPixelShader, m_NullGeometryShader, false, nullptr, 0, false);
		SpotLightDepthToTextureEnd();
		m_DX11DeviceContext->OMSetDepthStencilState(nullptr, 0);*/
		//ResetRenderState();

		if (m_bEnableGI)
		{
			// Scene voxelization
			NvidiaVXGIVoxelization();
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
		if (m_DrawInfo.meshesDrawn > 0)
		{
			for (int i = 0; i < NUM_POINT_LIGHTS; i++)
			{
				auto modelMat = glm::mat4(1.f);
				auto translate = m_Cameras[i + 1].GetPosition();
				auto scale = 0.1f;
				modelMat = glm::translate(modelMat, glm::vec3(translate[0], translate[1], translate[2]));
				modelMat = glm::scale(modelMat, glm::vec3(scale, scale, scale));
				RenderCube(modelMat, true);
			}
			renderPhysics(m_DeltaTime);
			//RenderModelBounds(0);
		}
		RenderHDRCubemap();
		RenderToGBufferEnd();

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
		if (m_DrawInfo.meshesDrawn > 0)
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
					glm::mat4 viewMatrix = /*m_Cameras[SelectedCamera].GetViewMatrix()*/m_CameraViewMatrix;
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

			PostprocessingToTextureBegin();
			m_DX11DeviceContext->PSSetShaderResources(0, 1, &frameSRV);
			m_DeferredCompositingVertexShader->Bind();
			m_PostprocessingPixelShader->Bind();
			m_DX11DeviceContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
			PostprocessingCB pCb;
			pCb.enableFXAA = m_bEnableFXAA ? 1 : 0;
			m_DX11DeviceContext->UpdateSubresource(g_pCBPostprocessing, 0, NULL, &pCb, 0, 0);
			m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBPostprocessing);
			RenderQuad();
			m_DeferredCompositingVertexShader->Unbind();
			m_PostprocessingPixelShader->Unbind();
			PostprocessingToTextureEnd();
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
	for (auto& model : m_Models)
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

	cleanupPhysics();

	FMODCleanup();

	SAFE_DELETE(m_Animator);
	SAFE_DELETE(m_RunAnimation);
	SAFE_DELETE(m_IdleAnimation);
	SAFE_DELETE(m_ShootingAnimation);


	SAFE_RELEASE(m_HDRShaderResourceView);
	SAFE_RELEASE(m_CubemapDepthStencilState);
	SAFE_RELEASE(m_CubemapRasterizerState);
	SAFE_RELEASE(m_CubemapShaderResourceView);
	SAFE_RELEASE(m_CubemapTexture);

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

		if (windowResizeEvent.width > 0 && windowResizeEvent.height > 0)
		{
			m_Window->OnResize(windowResizeEvent.width, windowResizeEvent.height);
			m_Renderer->OnWindowResize(windowResizeEvent.width, windowResizeEvent.height);
		}
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
	}
	else if (e.GetType() == MouseEvents::MouseButtonDown)
	{
		auto mouseButtonDownEvent = e.ToType<MouseButtonDownEvent>();
		//OV_INFO("Mouse Button Down Event! Mouse Button Down: {}", mouseButtonDownEvent.button);
		//if (mouseButtonDownEvent.button == 0)
		//{
		//	auto glmDir = glm::normalize(glm::quat(coltMat) * glm::vec3(0.f, 1.f, 0.f));
		//	auto glmPos = props[2].translation + glmDir * 1.2f + glm::vec3(0.f, 2.f, 0.f);

		//	PxTransform t;
		//	t.p = PxVec3(glmPos.x, glmPos.y, glmPos.z);
		//	t.q = PxQuat(PxIDENTITY::PxIdentity);

		//	auto sphereRadius = 0.15f;
		//	createDynamic(t, PxSphereGeometry(sphereRadius), 10000.f, PxVec3(glmDir.x, glmDir.y, glmDir.z) * 100);

		//	//FMOD::Channel* channel;
		//	//m_FmodSystem->playSound(m_WhooshSound, nullptr, false, &channel);
		//}
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
	SetupImguiDockspace();

	int windowFlags = 0;

	if (viewPortActive)
		windowFlags |= ImGuiWindowFlags_NoInputs;
	else
		windowFlags &= ~ImGuiWindowFlags_NoInputs;

	ImGui::Begin("Properties", nullptr, windowFlags);
	ImGui::Checkbox("VSync", &m_bEnableVsync);
	const char* items[]{ "Main Camera", "Light1", "Light2", "Light3", "SpotLight" };
	if (ImGui::Combo("Camera", &SelectedCamera, items, 5))
	{
	}
	ImGui::SliderFloat("Cam Speed", &m_Cameras[SelectedCamera].GetMovementSpeed(), 1.f, 150.f);
	ImGui::SliderFloat3("Tr", (float*)&m_ModelProps[currentModelId].translation, -10.f, 10.f);
	ImGui::SliderFloat3("Rt", (float*)&m_ModelProps[currentModelId].rotation, -180.f, 180.f);
	ImGui::SliderFloat3("Sc", (float*)&m_ModelProps[currentModelId].scale, 0.f, 5.f);
	if (ImGui::Button("Change Mesh"))
	{
		OpenFile(modelPath);
		if (!modelPath.empty())
		{
			delete m_Models[currentModelId];
			m_Models[currentModelId] = new Model(modelPath, "", "", "", "");
			modelPath = "";
		}
	}
	ImGui::Text("Textures");
	ImGui::Checkbox("Combined", &m_Models[currentModelId]->GetUseCombinedTextures());
	if (ImGui::Button("A"))
	{
		OpenFile(m_ModelProps[currentModelId].albedoPath);
		if (!m_ModelProps[currentModelId].albedoPath.empty())
		{
			m_Models[currentModelId]->ChangeTexture(m_ModelProps[currentModelId].albedoPath, TextureType::ALBEDO);
		}
	}
	ImGui::SameLine();
	ImGui::InputText("Albedo", &m_ModelProps[currentModelId].albedoPath);
	if (ImGui::Button("N"))
	{
		OpenFile(m_ModelProps[currentModelId].normalPath);
		if (!m_ModelProps[currentModelId].normalPath.empty())
		{
			m_Models[currentModelId]->ChangeTexture(m_ModelProps[currentModelId].normalPath, TextureType::NORMAL);
		}
	}
	ImGui::SameLine();
	ImGui::InputText("Normal", &m_ModelProps[currentModelId].normalPath);
	if (ImGui::Button("R"))
	{
		OpenFile(m_ModelProps[currentModelId].roughnessPath);
		if (!m_ModelProps[currentModelId].roughnessPath.empty())
		{
			m_Models[currentModelId]->ChangeTexture(m_ModelProps[currentModelId].roughnessPath, TextureType::ROUGHNESS);
		}
	}
	ImGui::SameLine();
	ImGui::InputText("Roughness", &m_ModelProps[currentModelId].roughnessPath);
	if (ImGui::Button("M"))
	{
		OpenFile(m_ModelProps[currentModelId].metallicPath);
		if (!m_ModelProps[currentModelId].metallicPath.empty())
		{
			m_Models[currentModelId]->ChangeTexture(m_ModelProps[currentModelId].metallicPath, TextureType::METALLIC);
		}
	}
	ImGui::SameLine();
	ImGui::InputText("Metallic", &m_ModelProps[currentModelId].metallicPath);
	ImGui::SliderFloat("NormalStrength", &normalStrength, 0.001f, 10.f);

	ImGui::Text("Light");
	const char* lightItems[]{ "1", "2", "3" };
	if (ImGui::Combo("Light", &CurrentLightIndex, lightItems, 3))
	{
	}
	ImGui::SliderFloat3("Position", (float*)&m_LightProps.lightPos[CurrentLightIndex], -30.f, 30.f);
	ImGui::SliderFloat("Constant", &m_LightProps.constant, 0.001f, 10.f);
	ImGui::SliderFloat("Linear", &m_LightProps.linear, 0.001f, 1.f);
	ImGui::SliderFloat("Quadratic", &m_LightProps.quadratic, 0.001f, 1.f);
	ImGui::SliderFloat3("Color", m_LightProps.color, 0.001f, 1.f);
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
	ImGui::SliderFloat("VXAOScale", &m_fVxaoScale, 0.001f, 50.f);
	ImGui::Text("GI Debug View");
	ImGui::Checkbox("Show diffuse only", &m_bShowDiffuseTexture);
	if (m_bShowDiffuseTexture)
		ImGui::Checkbox("Render debug", &m_bVXGIRenderDebug);
	ImGui::Checkbox("Enable SSAO", &g_bEnableSSAO);
	ImGui::Checkbox("Enable FXAA", &m_bEnableFXAA);

	ImGui::SliderFloat("AnimBlendFactor", &m_IdleRunBlendFactor, 0.f, 1.f);
	ImGui::SliderFloat("CharacterCameraZoom", &m_FollowCharacterCameraZoom, 1.f, 5.f);
	ImGui::Checkbox("FreeCameraView", &m_bFreeCameraView);

	ImGui::End();

	m_Cameras[1].GetPosition() = m_LightProps.lightPos[0];
	m_Cameras[2].GetPosition() = m_LightProps.lightPos[1];
	m_Cameras[3].GetPosition() = m_LightProps.lightPos[2];


	ImGui::Begin("Scene", nullptr, windowFlags);
	if (ImGui::Button("New"))
	{
		OpenFile(modelPath);
		if (!modelPath.empty())
		{
			m_Models.emplace_back(new Model(modelPath, "", "", "", ""));
			m_ModelProps.emplace_back(Properties());
			modelPath = "";
		}
	}

	for (int i = 0; i < m_Models.size(); i++)
	{
		char nodeName[16];
		snprintf(nodeName, 16, "Model_%d", i);
		ImGuiTreeNodeFlags flags = ((currentModelId == i) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		bool opened = ImGui::TreeNodeEx(nodeName, flags);
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete"))
			{
				delete m_Models[i];
				m_Models.erase(m_Models.begin() + i);
				m_ModelProps.erase(m_ModelProps.begin() + i);
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

	ImGui::Begin("Status", nullptr, windowFlags);
	ImGui::Text("FPS: %f\nMeshes drawn: %d/%d", 1.f / m_DeltaTime, m_DrawInfo.meshesDrawn, m_DrawInfo.totalMeshes);
	ImGui::End();

	ImGui::Begin("Viewport", nullptr, windowFlags);
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
		PostprocessingToTextureInit();
		//gBuffer->Shutdown();
		//if (!gBuffer->Initialize(texWidth, texHeight))
		//	std::cout << "Failed to init gBuffer!\n";
		PrepareGbufferRenderTargets(texWidth, texHeight);
		texWidthPrev = texWidth;
		texHeightPrev = texHeight;
	}
	ImGui::Image((ImTextureID)(intptr_t)postprocessingSRV, ImVec2(texWidth, texHeight));
	ImGui::End();
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
	// Rest style by AaronBeardless from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.5f;
	style.WindowPadding = ImVec2(13.0f, 10.0f);
	style.WindowRounding = 0.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(32.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.ChildRounding = 3.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 5.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(20.0f, 8.100000381469727f);
	style.FrameRounding = 2.0f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(3.0f, 3.0f);
	style.ItemInnerSpacing = ImVec2(3.0f, 8.0f);
	style.CellPadding = ImVec2(6.0f, 14.10000038146973f);
	style.IndentSpacing = 0.0f;
	style.ColumnsMinSpacing = 10.0f;
	style.ScrollbarSize = 10.0f;
	style.ScrollbarRounding = 2.0f;
	style.GrabMinSize = 12.10000038146973f;
	style.GrabRounding = 1.0f;
	style.TabRounding = 2.0f;
	style.TabBorderSize = 0.0f;
	style.TabMinWidthForCloseButton = 5.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Colors[ImGuiCol_Text] = ImVec4(0.9803921580314636f, 0.9803921580314636f, 0.9803921580314636f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09411764889955521f, 0.09411764889955521f, 0.09411764889955521f, 1.0f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.09411764889955521f, 0.09411764889955521f, 0.09411764889955521f, 1.0f);
	style.Colors[ImGuiCol_Border] = ImVec4(1.0f, 1.0f, 1.0f, 0.09803921729326248f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.09803921729326248f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1568627506494522f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0470588244497776f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1176470592617989f, 0.1176470592617989f, 0.1176470592617989f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.1176470592617989f, 0.1176470592617989f, 0.1176470592617989f, 1.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.1098039224743843f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(1.0f, 1.0f, 1.0f, 0.3921568691730499f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.4705882370471954f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.09803921729326248f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 1.0f, 1.0f, 0.3921568691730499f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.3137255012989044f);
	style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 1.0f, 1.0f, 0.09803921729326248f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1568627506494522f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0470588244497776f);
	style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 1.0f, 1.0f, 0.09803921729326248f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1568627506494522f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0470588244497776f);
	style.Colors[ImGuiCol_Separator] = ImVec4(1.0f, 1.0f, 1.0f, 0.1568627506494522f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.2352941185235977f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2352941185235977f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.0f, 1.0f, 1.0f, 0.1568627506494522f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.2352941185235977f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2352941185235977f);
	style.Colors[ImGuiCol_Tab] = ImVec4(1.0f, 1.0f, 1.0f, 0.09803921729326248f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1568627506494522f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.3137255012989044f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0f, 0.0f, 0.0f, 0.1568627506494522f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2352941185235977f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(1.0f, 1.0f, 1.0f, 0.3529411852359772f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(1.0f, 1.0f, 1.0f, 0.3529411852359772f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(1.0f, 1.0f, 1.0f, 0.3137255012989044f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(1.0f, 1.0f, 1.0f, 0.196078434586525f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.01960784383118153f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.168627455830574f, 0.2313725501298904f, 0.5372549295425415f, 1.0f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5647059082984924f);
}

void TestApplication::SetupImguiDockspace()
{
	int windowFlags = ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoNavFocus;

	if (viewPortActive)
		windowFlags |= ImGuiWindowFlags_NoInputs;
	else
		windowFlags &= ~ImGuiWindowFlags_NoInputs;

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

	//delete renderTexture;
	static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->ResetRenderTargetView();
}

bool TestApplication::OpenFile(std::string& pathRef)
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
		{ glm::vec3(-1.0f, 1.0f, -1.0f),  glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f },
		{ glm::vec3(1.0f, 1.0f, -1.0f),   glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
		{ glm::vec3(1.0f, 1.0f, 1.0f),    glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
		{ glm::vec3(-1.0f, 1.0f, 1.0f),   glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},

		{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 101, 0.f},
		{ glm::vec3(1.0f, -1.0f, -1.0f),  glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 101, 0.f},
		{ glm::vec3(1.0f, -1.0f, 1.0f),   glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), 101, 0.f},
		{ glm::vec3(-1.0f, -1.0f, 1.0f),  glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), 101, 0.f},

		{ glm::vec3(-1.0f, -1.0f, 1.0f),  glm::vec2(0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), 101, 0.f},
		{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), 101, 0.f},
		{ glm::vec3(-1.0f, 1.0f, -1.0f),  glm::vec2(1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), 101, 0.f},
		{ glm::vec3(-1.0f, 1.0f, 1.0f),   glm::vec2(0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), 101, 0.f},

		{ glm::vec3(1.0f, -1.0f, 1.0f),	glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 101, 0.f},
		{ glm::vec3(1.0f, -1.0f, -1.0f),  glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 101, 0.f},
		{ glm::vec3(1.0f, 1.0f, -1.0f),	glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), 101, 0.f},
		{ glm::vec3(1.0f, 1.0f, 1.0f),	glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), 101, 0.f},

		{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), 101, 0.f},
		{ glm::vec3(1.0f, -1.0f, -1.0f),  glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), 101, 0.f},
		{ glm::vec3(1.0f, 1.0f, -1.0f),	glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), 101, 0.f},
		{ glm::vec3(-1.0f, 1.0f, -1.0f),  glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), 101, 0.f},

		{ glm::vec3(-1.0f, -1.0f, 1.0f),  glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 101, 0.f},
		{ glm::vec3(1.0f, -1.0f, 1.0f),   glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 101, 0.f},
		{ glm::vec3(1.0f, 1.0f, 1.0f),    glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), 101, 0.f},
		{ glm::vec3(-1.0f, 1.0f, 1.0f),   glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), 101, 0.f},
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

void TestApplication::RenderCube(const glm::mat4& modelMat, bool lightCube)
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	m_DX11DeviceContext->IASetVertexBuffers(0, 1, &pCubeVertexBuffer, &stride, &offset);
	m_DX11DeviceContext->IASetIndexBuffer(pCubeIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_DX11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	MatricesCB cb;
	cb.modelMat = modelMat;
	cb.viewMat = /*m_Cameras[SelectedCamera].GetViewMatrix()*/m_CameraViewMatrix;
	cb.projMat = glm::perspectiveFovLH(XM_PIDIV4, (FLOAT)texWidth, (FLOAT)texHeight, NEAR_PLANE, FAR_PLANE);
	cb.normalMat = glm::transpose(glm::inverse(modelMat));

	m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, nullptr, &cb, 0, 0);
	m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);

	MaterialCB matCb;
	matCb.sponza = 0.f;
	matCb.normalStrength = normalStrength;
	matCb.renderLightCube = (int)lightCube;

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
		{ glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), },
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

	MatricesCB cb;
	cb.modelMat = glm::scale(glm::mat4(1.f), glm::vec3((float)texWidth / texHeight * 1.f, 1.f, 1.f));
	cb.viewMat = glm::lookAtLH(glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
	//cb.projMat = glm::perspectiveFovLH(XM_PIDIV4, (FLOAT)texWidth, (FLOAT)texHeight, 0.01f, 100.f);
	auto projMatXm = XMMatrixOrthographicLH((float)texWidth / texHeight * 2.f, 2.f, 0.1f, 100.f);
	cb.projMat = *(glm::mat4*)&projMatXm;
	cb.normalMat = glm::mat4(1.f);
	glm::mat4 viewProjInv = glm::inverse(m_Cameras[SelectedCamera].GetViewMatrix() * glm::perspectiveFovLH(XM_PIDIV4, (FLOAT)texWidth, (FLOAT)texHeight, 0.01f, 100.f));
	cb.viewProjMatInv = viewProjInv;

	m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, NULL, &cb, 0, 0);
	m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);
	m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);

	LightCB lightCb;
	lightCb.lightPos[0] = glm::vec4(m_Cameras[1].GetPosition().x, m_Cameras[1].GetPosition().y, m_Cameras[1].GetPosition().z, 1.f);
	lightCb.lightPos[1] = glm::vec4(m_Cameras[2].GetPosition().x, m_Cameras[2].GetPosition().y, m_Cameras[2].GetPosition().z, 1.f);
	lightCb.lightPos[2] = glm::vec4(m_Cameras[3].GetPosition().x, m_Cameras[3].GetPosition().y, m_Cameras[3].GetPosition().z, 1.f);
	lightCb.lightPos[3] = glm::vec4(m_Cameras[4].GetPosition().x, m_Cameras[4].GetPosition().y, m_Cameras[4].GetPosition().z, 1.f);
	lightCb.conLinQuad[0] = m_LightProps.constant;
	lightCb.conLinQuad[1] = m_LightProps.linear;
	lightCb.conLinQuad[2] = m_LightProps.quadratic;
	lightCb.lightColor = { m_LightProps.color[0], m_LightProps.color[1], m_LightProps.color[2] };
	lightCb.bias = m_ShadowMapBias;
	lightCb.camPos = /*m_Cameras[SelectedCamera].GetPosition()*/m_CameraPos;
	lightCb.showDiffuseTexture = m_bShowDiffuseTexture;
	lightCb.numLights = NUM_POINT_LIGHTS;
	lightCb.enableGI = m_bEnableGI;
	lightCb.cubeProj = lightProjMat;
	lightCb.lightViewProjMat = spotlightProjMat * m_Cameras[NUM_POINT_LIGHTS + 1].GetViewMatrix();
	lightCb.rcpFrame = glm::vec4(1.0f / texWidth, 1.0f / texHeight, 0.0f, 0.0f);


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
	std::ifstream t("src/shaders/VoxelizationPixelShader.hlsl");
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
	glm::vec3 camPos = /*m_Cameras[SelectedCamera].GetPosition()*/m_CameraPos;
	glm::vec3 frontVec = /*m_Cameras[SelectedCamera].GetFrontVector()*/ m_CameraFrontVector;
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
			RenderScene(voxelizationViewMatrix, voxelizationProjMatrix, m_DefaultVertexShader, nullPixelShader, m_NullGeometryShader, true, regions, numRegions, false);
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
	glm::mat4 gBufferViewMat = /*m_Cameras[SelectedCamera].GetViewMatrix()*/m_CameraViewMatrix;
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
	RenderScene(/*m_Cameras[SelectedCamera].GetViewMatrix()*/m_CameraViewMatrix, perspectiveMat, m_DeferredVertexShader, m_DeferredPixelShader, m_NullGeometryShader, false, nullptr, 0, true);
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

void TestApplication::RenderScene(glm::mat4 viewMatrix, glm::mat4 projMatrix, std::shared_ptr<DX11VertexShader>& vertexShader, std::shared_ptr<DX11PixelShader>& pixelShader, std::shared_ptr<DX11GeometryShader>& geometryShader, bool voxelizing, const VXGI::Box3f* clippingBoxes, uint32_t numBoxes, bool frustumCulling)
{
	if (vertexShader)
		vertexShader->Bind();
	if (pixelShader)
		pixelShader->Bind();
	if (geometryShader)
		geometryShader->Bind();

	LightCB lightCb;
	lightCb.lightPos[0] = glm::vec4(m_Cameras[1].GetPosition().x, m_Cameras[1].GetPosition().y, m_Cameras[1].GetPosition().z, 1.f);
	lightCb.lightPos[1] = glm::vec4(m_Cameras[2].GetPosition().x, m_Cameras[2].GetPosition().y, m_Cameras[2].GetPosition().z, 1.f);
	lightCb.lightPos[2] = glm::vec4(m_Cameras[3].GetPosition().x, m_Cameras[3].GetPosition().y, m_Cameras[3].GetPosition().z, 1.f);
	lightCb.lightPos[3] = glm::vec4(m_Cameras[4].GetPosition().x, m_Cameras[4].GetPosition().y, m_Cameras[4].GetPosition().z, 1.f);
	lightCb.conLinQuad[0] = m_LightProps.constant;
	lightCb.conLinQuad[1] = m_LightProps.linear;
	lightCb.conLinQuad[2] = m_LightProps.quadratic;
	lightCb.lightColor = { m_LightProps.color[0], m_LightProps.color[1], m_LightProps.color[2] };
	lightCb.lightViewProjMat = spotlightProjMat * m_Cameras[NUM_POINT_LIGHTS + 1].GetViewMatrix();
	lightCb.bias = m_ShadowMapBias;
	lightCb.numLights = NUM_POINT_LIGHTS;
	for (int i = 0; i < 6; i++)
	{
		lightCb.cubeView[i] = m_PointLightDepthCaptureViews[i];
	}
	lightCb.cubeProj = lightProjMat;
	lightCb.camPos = /*m_Cameras[SelectedCamera].GetPosition()*/m_CameraPos;
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

	m_DrawInfo = {};
	for (int i = 0; i < m_Models.size(); i++)
	{
		auto translate = m_ModelProps[i].translation;
		auto rotate = m_ModelProps[i].rotation;
		auto scale = m_ModelProps[i].scale;

		MatricesCB cb;
		auto modelMat = glm::mat4(1.f);
		modelMat = glm::translate(modelMat, glm::vec3(translate[0], translate[1], translate[2]));
		modelMat = glm::rotate(modelMat, glm::radians(rotate[0]), glm::vec3(1.f, 0.f, 0.f));
		modelMat = glm::rotate(modelMat, glm::radians(rotate[1]), glm::vec3(0.f, 1.f, 0.f));
		modelMat = glm::rotate(modelMat, glm::radians(rotate[2]), glm::vec3(0.f, 0.f, 1.f));
		modelMat = glm::scale(modelMat, glm::vec3(scale[0], scale[1], scale[2]));
		//if (i == 0)
		//	modelMat = coltMat;   // games with physics
		cb.modelMat = modelMat;
		if (i == 0)           /// attaching pistol to the right hand
		{
			auto boneInfoMap = m_Models[2]->GetBoneInfoMap();
			auto it = boneInfoMap.find("mixamorig:RightHand");
			if (it == boneInfoMap.end()) {
				std::cout << "Bone not found!\n";
				m_Running = false;
			}
			else {

				auto translate = m_ModelProps[2].translation;
				auto rotate = m_ModelProps[2].rotation;
				auto scale = m_ModelProps[2].scale;
				auto characterMat = glm::mat4(1.f);
				characterMat = glm::translate(characterMat, glm::vec3(translate[0], translate[1], translate[2]));
				characterMat = glm::rotate(characterMat, glm::radians(rotate[0]), glm::vec3(1.f, 0.f, 0.f));
				characterMat = glm::rotate(characterMat, glm::radians(rotate[1]), glm::vec3(0.f, 1.f, 0.f));
				characterMat = glm::rotate(characterMat, glm::radians(rotate[2]), glm::vec3(0.f, 0.f, 1.f));
				characterMat = glm::scale(characterMat, glm::vec3(scale[0], scale[1], scale[2]));

				auto gunPos = glm::vec3(4.5f, 10.5f, 1.225f);
				translate = gunPos;
				rotate = glm::vec3(0.f, 90.f, 0.f);
				scale = m_ModelProps[0].scale;
				auto modelMat = glm::mat4(1.f);
				modelMat = glm::translate(modelMat, glm::vec3(translate[0], translate[1], translate[2]));
				modelMat = glm::translate(modelMat, glm::vec3(0.f, 0.f, 0.f));
				modelMat = glm::rotate(modelMat, glm::radians(rotate[0]), glm::vec3(1.f, 0.f, 0.f));
				modelMat = glm::rotate(modelMat, glm::radians(rotate[1]), glm::vec3(0.f, 1.f, 0.f));
				modelMat = glm::rotate(modelMat, glm::radians(rotate[2]), glm::vec3(0.f, 0.f, 1.f));
				modelMat = glm::scale(modelMat, glm::vec3(scale[0], scale[1], scale[2]));
				modelMat = glm::scale(modelMat, glm::vec3(15.f, 15.f, 15.f));

				BoneInfo value = it->second;
				glm::mat4 boneMat = value.offset;

				auto childFinalBoneMatrix = m_Animator->GetFinalBoneMatrices()[value.id];

				glm::mat4 finalBoneMatrix = childFinalBoneMatrix * glm::inverse(boneMat);

				cb.modelMat = characterMat * finalBoneMatrix * modelMat;
				coltMat = cb.modelMat;
			}
		}

		cb.viewMat = viewMatrix;
		cb.projMat = projMatrix;
		cb.normalMat = glm::transpose(glm::inverse(modelMat));

		MaterialCB matCb;
		if (m_Models[i]->GetFilePath().find("Sponza.gltf") != std::string::npos)
			matCb.sponza = 1.0f;
		else
			matCb.sponza = 0.f;
		matCb.normalStrength = normalStrength;
		matCb.renderLightCube = 0;

		SkeletalAnimationCB saCb;
		if (m_Animator)
		{
			auto finalBoneMatrices = m_Animator->GetFinalBoneMatrices();
			for (int i = 0; i < finalBoneMatrices.size(); i++)
			{
				saCb.finalBonesMatrices[i] = finalBoneMatrices[i];
			}
		}
		else
		{
			for (int i = 0; i < 100; i++)
			{
				saCb.finalBonesMatrices[i] = glm::mat4(1.f);
			}
		}


		m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, NULL, &cb, 0, 0);
		m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, NULL, &matCb, 0, 0);
		m_DX11DeviceContext->UpdateSubresource(g_pCBSkeletalAnimation, 0, NULL, &saCb, 0, 0);

		m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);
		if (!voxelizing)
		{
			m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);
			m_DX11DeviceContext->VSSetConstantBuffers(2, 1, &g_pCBMaterial);
			m_DX11DeviceContext->PSSetConstantBuffers(2, 1, &g_pCBMaterial);
		}
		m_DX11DeviceContext->VSSetConstantBuffers(3, 1, &g_pCBSkeletalAnimation);

		m_DX11DeviceContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
		if (!voxelizing)
		{
			m_DX11DeviceContext->PSSetSamplers(1, 1, &g_pSamplerClamp);
			m_DX11DeviceContext->PSSetSamplers(2, 1, &g_pSamplerComparison);
		}
		else
			m_DX11DeviceContext->PSSetSamplers(1, 1, &g_pSamplerComparison);

		DrawInfo drawInfo;
		if (frustumCulling && i != 2)
		{
			Frustum frustum;
			frustum.ConstructFrustum(viewMatrix, projMatrix, FAR_PLANE);
			drawInfo = m_Models[i]->Draw(clippingBoxes, numBoxes, cb.modelMat, &frustum);
		}
		else
		{
			drawInfo = m_Models[i]->Draw(clippingBoxes, numBoxes, cb.modelMat, nullptr);
		}
		m_DrawInfo.meshesDrawn += drawInfo.meshesDrawn;
		m_DrawInfo.totalMeshes += drawInfo.totalMeshes;
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
	m_physXEventCallback = new PhysXEventCallback(this);
	//m_physXFilterCallback = new PhysXFilterCallback();
	sceneDesc.filterShader = CollisionFilterShader;
	//sceneDesc.filterCallback = (PxSimulationFilterCallback*)m_physXFilterCallback;
	sceneDesc.simulationEventCallback = (PxSimulationEventCallback*)m_physXEventCallback;

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
		createDynamic(PxTransform(PxVec3(0, 40, 100)), PxBoxGeometry(m_boxHalfExtent, m_boxHalfExtent, m_boxHalfExtent), 100.f, PxVec3(0, -50, -100));

	// create pystol actor
	PxTransform localTm(0, 5.f, 0);
	m_coltPhysicsActor = gPhysics->createRigidDynamic(localTm);
	auto coltMeshes = m_Models[0]->GetMeshes();
	for (Mesh& mesh : coltMeshes)
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

	m_CharacterCapsuleCollider = gPhysics->createRigidDynamic(localTm);
	PxShape* feetShape = gPhysics->createShape(PxCapsuleGeometry(0.2f, 1.5f), *gMaterial);
	m_CharacterCapsuleCollider->attachShape(*feetShape);
	PxRigidBodyExt::updateMassAndInertia(*m_CharacterCapsuleCollider, 100.0f);
	gScene->addActor(*m_CharacterCapsuleCollider);

	PxRigidBodyExt::updateMassAndInertia(*m_coltPhysicsActor, 10.0f);
	gScene->addActor(*m_coltPhysicsActor);
}

void TestApplication::stepPhysics(float elapsedTime)
{
	gScene->simulate(elapsedTime);
	gScene->fetchResults(true);
}

void TestApplication::cleanupPhysics()
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
	delete m_physXEventCallback;
	delete m_physXFilterCallback;
}

void TestApplication::renderPhysics(float deltaTime)
{
	if (deltaTime > 0.f && deltaTime < 0.1f)
		stepPhysics(deltaTime);
	else
		stepPhysics(1.f / 60.f);

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

				if (actor != m_coltPhysicsActor && actor != m_CharacterCapsuleCollider)
					RenderCube(boxModelMat, false);
			}
		}
		PxShape* shapes[128];
		const PxU32 nbShapes = m_coltPhysicsActor->getNbShapes();
		m_coltPhysicsActor->getShapes(shapes, nbShapes);
		PxMat44 shapePose(PxShapeExt::getGlobalPose(*shapes[0], *m_coltPhysicsActor));

		//shapePose.scale(PxVec4(5.f));
		//coltMat = *(glm::mat4*)&shapePose;
		//coltMat = glm::scale(coltMat, glm::vec3(10.f));

		auto capsuleRotation = PxQuat(glm::radians(90.f), PxVec3(0.f, 0.f, 1.f));
		m_CharacterCapsuleCollider->setGlobalPose(PxTransform(PxVec3(m_ModelProps[2].translation.x, m_ModelProps[2].translation.y, m_ModelProps[2].translation.z), capsuleRotation));
	}
}

PxRigidDynamic* TestApplication::createDynamic(const PxTransform& t, const PxGeometry& geometry, float density, const PxVec3& velocity)
{
	PxRigidDynamic* dynamic = PxCreateDynamic(*gPhysics, t, geometry, *gMaterial, density);
	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);
	gScene->addActor(*dynamic);
	return dynamic;
}

void TestApplication::createStack(const PxTransform& t, PxU32 size, PxReal halfExtent)
{
	PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *gMaterial);

	setupFiltering(shape, FilterGroup::eOBJECT1, FilterGroup::eFLOOR);

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
	auto meshes = m_Models[modelIndex]->GetMeshes();
	for (auto& mesh : meshes)
	{
		auto bound = mesh.GetBounds();

		auto width = abs(bound.upper.x - bound.lower.x);
		auto height = abs(bound.upper.y - bound.lower.y);
		auto depth = abs(bound.upper.z - bound.lower.z);

		printf("w: %f, h: %f, d: %f\n", width, height, depth);

		auto lowerDimension = std::min({ width, height, depth });

		ID3D11Buffer* pBoundVertexBuffer, * pBoundIndexBuffer;

		Vertex vertices[] =
		{
			{ glm::vec3(-width, height, depth),  glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
			{ glm::vec3(width, height, depth),   glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
			{ glm::vec3(width, height, -depth),  glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
			{ glm::vec3(-width, height, -depth), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},


			{ glm::vec3(-width, -height, -depth), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 101, 0.f},
			{ glm::vec3(width, -height, -depth),  glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 101, 0.f},
			{ glm::vec3(width, -height, depth),   glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), 101, 0.f},
			{ glm::vec3(-width, -height, depth),  glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), 101, 0.f},


			{ glm::vec3(-width, height, depth),   glm::vec2(0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), 101, 0.f},
			{ glm::vec3(-width, height, -depth),  glm::vec2(1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), 101, 0.f},
			{ glm::vec3(-width, -height, -depth), glm::vec2(1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), 101, 0.f},
			{ glm::vec3(-width, -height, depth),  glm::vec2(0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), 101, 0.f},


			{ glm::vec3(width, height, -depth),	 glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 101, 0.f},
			{ glm::vec3(width, height, depth),	 glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 101, 0.f},
			{ glm::vec3(width, -height, depth),	 glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), 101, 0.f},
			{ glm::vec3(width, -height, -depth), glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), 101, 0.f},


			{ glm::vec3(-width, height, -depth),  glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), 101, 0.f},
			{ glm::vec3(width, height, -depth),   glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), 101, 0.f},
			{ glm::vec3(width, -height, -depth),  glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), 101, 0.f},
			{ glm::vec3(-width, -height, -depth), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), 101, 0.f},


			{ glm::vec3(width, height, depth),   glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 101, 0.f},
			{ glm::vec3(-width, height, depth),  glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 101, 0.f},
			{ glm::vec3(-width, -height, depth), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), 101, 0.f},
			{ glm::vec3(width, -height, depth),  glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), 101, 0.f},
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

		HRESULT hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pBoundVertexBuffer);
		if (FAILED(hr)) return;

		UINT indices[] =
		{
			0, 1, 2, 3, 0,
			4, 5, 6, 7, 4,

			8, 9, 10, 11, 8,
			12, 13, 14, 15, 12,

			16, 17, 18, 19, 16,
			20, 21, 22, 23, 20
		};


		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(UINT) * 30;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;

		InitData.pSysMem = indices;

		hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pBoundIndexBuffer);
		if (FAILED(hr)) return;


		auto modelMat = glm::mat4(1.f);

		auto translate = m_ModelProps[modelIndex].translation;
		auto rotate = m_ModelProps[modelIndex].rotation;
		auto scale = m_ModelProps[modelIndex].scale;

		modelMat = glm::translate(modelMat, glm::vec3(translate[0], translate[1], translate[2]));
		modelMat = glm::rotate(modelMat, glm::radians(rotate[0]), glm::vec3(1.f, 0.f, 0.f));
		modelMat = glm::rotate(modelMat, glm::radians(rotate[1]), glm::vec3(0.f, 1.f, 0.f));
		modelMat = glm::rotate(modelMat, glm::radians(rotate[2]), glm::vec3(0.f, 0.f, 1.f));
		modelMat = glm::scale(modelMat, glm::vec3(scale.x, scale.y, scale.z));

		if (modelIndex == 0)
			modelMat = coltMat;

		UINT stride = sizeof(Vertex);
		UINT offset = 0;

		m_DX11DeviceContext->IASetVertexBuffers(0, 1, &pBoundVertexBuffer, &stride, &offset);
		m_DX11DeviceContext->IASetIndexBuffer(pBoundIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		m_DX11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);


		MatricesCB cb;
		cb.modelMat = modelMat;
		cb.viewMat = m_CameraViewMatrix;
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

		m_DX11DeviceContext->DrawIndexed(30, 0, 0);

		pBoundVertexBuffer->Release();
		pBoundIndexBuffer->Release();
	}
}

void TestApplication::FMODInit()
{
	FMOD_RESULT result;
	result = FMOD::System_Create(&m_FmodSystem);      // Create the main system object.
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		exit(-1);
	}
	result = m_FmodSystem->init(512, FMOD_INIT_NORMAL, 0);    // Initialize FMOD.
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		exit(-1);
	}

	result = m_FmodSystem->createSound("../resources/audio/530448__mellau__whoosh-short-5.wav", FMOD_CREATESAMPLE, nullptr, &m_WhooshSound);
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		exit(-1);
	}

	result = m_FmodSystem->createSound("../resources/audio/Wooden Box Drop_1.wav", FMOD_CREATESAMPLE, nullptr, &m_BoxDrop1);
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		exit(-1);
	}

	result = m_FmodSystem->createSound("../resources/audio/Wooden Box Drop_2.wav", FMOD_CREATESAMPLE, nullptr, &m_BoxDrop2);
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		exit(-1);
	}

	result = m_FmodSystem->createSound("../resources/audio/Wooden Box Drop_3.wav", FMOD_CREATESAMPLE, nullptr, &m_BoxDrop3);
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		exit(-1);
	}

	result = m_FmodSystem->createSound("../resources/audio/Wooden Box Drop_4.wav", FMOD_CREATESAMPLE, nullptr, &m_BoxDrop4);
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		exit(-1);
	}
	m_BoxDrop1->setLoopCount(0);
	m_BoxDrop2->setLoopCount(0);
	m_BoxDrop3->setLoopCount(0);
	m_BoxDrop4->setLoopCount(0);
}

void TestApplication::FMODUpdate()
{
	m_FmodSystem->update();
}

void TestApplication::FMODCleanup()
{
	m_FmodSystem->release();
	m_WhooshSound->release();
	m_BoxDrop1->release();
	m_BoxDrop2->release();
	m_BoxDrop3->release();
	m_BoxDrop4->release();
}

const float kEpsilon = 0.000001f;

bool IsEqualUsingDot(float dot)
{
	// Returns false in the presence of NaN values.
	return dot > 1.0f - kEpsilon;
}

float QuaternionAngle(glm::quat a, glm::quat b)
{
	float dot = glm::min(glm::abs(glm::dot(a, b)), 1.0f);
	return IsEqualUsingDot(dot) ? 0.0f : glm::acos(dot) * 2.0f;
}

glm::quat TestApplication::RotateTowards(const glm::quat& from, const glm::quat& to, float maxAngleDegrees)
{
	glm::vec3 fromVector = from * glm::vec3(0.f, 0.f, 1.f);
	glm::vec3 toVector = to * glm::vec3(0.f, 0.f, 1.f);

	glm::vec3 fromDirection = glm::normalize(fromVector);
	glm::vec3 toDirection = glm::normalize(toVector);

	//float angleRadians = glm::acos(glm::dot(fromDirection, toDirection));
	float angleRadians = QuaternionAngle(from, to);

	if (glm::degrees(angleRadians) == 0.f)
		return to;

	if (glm::degrees(angleRadians) <= 90.f)
	{
		maxAngleDegrees *= glm::clamp(glm::degrees(angleRadians) / 180.f);
		/*if (maxAngleDegrees <= 0.1f || maxAngleDegrees >= 360.f)
			maxAngleDegrees = 0.1f;*/
	}

	//float minAngleDegrees = glm::min(glm::degrees(angleRadians), maxAngleDegrees);

	return glm::slerp(from, to, glm::min(1.0f, maxAngleDegrees / glm::degrees(angleRadians)));


	/*glm::vec3 axis = glm::cross(fromDirection, toDirection);*/
	////if (axis.length() <= 0.001)
	////	return from;

	//glm::quat rotationIncrement = glm::angleAxis(glm::radians(minAngleDegrees), axis);
	//if (glm::any(glm::isnan(rotationIncrement)))
	//{
	//	printf("rotationIncrement is NaN!\n");
	//	return from;
	//}
	////	

	/*glm::vec3 result = rotationIncrement * fromDirection;*/

	//////printf("result length: %f\n", glm::length(result));

	//float angle = glm::atan(result.x, result.z); // Note: I expected atan2(z,x) but OP reported success with atan2(x,z) instead! Switch around if you see 90° off.
	//float qx = 0;
	//float qy = 1 * glm::sin(angle / 2);
	//float qz = 0;
	//float qw = glm::cos(angle / 2);
	//return glm::quat(qw, qx, qy, qz);
}

void TestApplication::MoveCharacter(glm::mat4& viewMatrix, glm::vec3& cameraPos, glm::vec3& cameraFrontVector)
{
	// camera follows model
	glm::vec3 eye = glm::vec3(0.f, 0.f, 1.f);
	glm::mat4 eyeMat = glm::mat4(1.f);

	static float yaw = 0.f;
	static float pitch = 0.f;

	//auto pad = m_GamePad->GetState(0);
	//if (pad.IsConnected())
	//{
	//	if (pad.IsViewPressed())
	//	{
	//		m_Running = false;
	//	}

	//	if (pad.IsRightStickPressed())
	//	{
	//		yaw = pitch = 0.f;
	//	}
	//	else
	//	{
	//		float ROTATION_GAIN = 4.f;
	//		yaw += pad.thumbSticks.rightX * ROTATION_GAIN;
	//		pitch -= pad.thumbSticks.rightY * ROTATION_GAIN;
	//	}
	//}
	if (mouseMoving)
	{
		yaw += Xoffset * m_DeltaTime * 5.f;
		pitch -= Yoffset * m_DeltaTime * 5.f;
	}

	if (pitch >= 60.f)
		pitch = 60.f;
	else if (pitch <= -20.f)
		pitch = -20.f;
	if (yaw <= -360.f || yaw >= 360.f)
		yaw = 0.f;
	//yaw = glm::mod(yaw, 360.f);
	//std::cout << "Yaw: " << yaw << '\n';
	//std::cout << "Pitch: " << pitch << '\n';



	auto quatPitch = glm::quat(glm::vec3(glm::radians(pitch), 0.f, 0.f));
	auto quat = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), 0.f));
	auto quatYaw = glm::quat(glm::vec3(0.f, glm::radians(yaw), 0.f));
	auto dir = quat * glm::vec3(0.f, 0.f, -5.f / m_FollowCharacterCameraZoom);
	auto dirYaw = quatYaw * glm::vec3(0.f, 0.f, 5.f);
	auto up = glm::vec3(0.f, 1.f, 0.f);

	auto headPos = m_ModelProps[2].translation + glm::vec3(0.f, 1.75f, 0.f);
	eye = dir + headPos - glm::normalize(glm::cross(dirYaw, up)) * 0.75f;

	//transform.rotation = quatYaw;
	transform.position = m_ModelProps[2].translation;

	// set default camera properties
	viewMatrix = glm::lookAtLH(glm::vec3(eye), headPos - glm::normalize(glm::cross(dirYaw, up)) * 0.75f, glm::vec3(0.f, 1.f, 0.f));
	cameraPos = glm::vec3(eye);
	cameraFrontVector = glm::normalize(headPos - glm::normalize(glm::cross(dirYaw, up)) * 0.75f - cameraPos);

	float moveSpeed = 5.f;
	float rotSpeed = 1000.f * m_DeltaTime;

	glm::vec3 velocityVector = glm::vec3(0.f);
	float verticalAxis = 0.f;
	float horizontalAxis = 0.f;

	if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_W) == GLFW_RELEASE &&
		glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_S) == GLFW_RELEASE &&
		glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_A) == GLFW_RELEASE &&
		glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_D) == GLFW_RELEASE)
		bWalking = false;
	if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_W) == GLFW_PRESS)
	{
		verticalAxis = 1.f;
		bWalking = true;
	}
	if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_S) == GLFW_PRESS)
	{
		verticalAxis = -1.f;
		bWalking = true;
	}
	if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_A) == GLFW_PRESS)
	{
		horizontalAxis = -1.f;
		bWalking = true;
	}
	if (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_D) == GLFW_PRESS)
	{
		horizontalAxis = 1.f;
		bWalking = true;
	}
	if (glfwGetMouseButton(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
		rightMouseButtonPressing = true;
	else if (glfwGetMouseButton(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_MOUSE_BUTTON_2) == GLFW_RELEASE)
		rightMouseButtonPressing = false;


	float movementSpeed = 5.f;

	float v = verticalAxis;
	float h = horizontalAxis;

	float moveAmount = glm::clamp(glm::abs(h) + glm::abs(v));

	glm::quat targetRotation = transform.rotation;

	glm::vec3 moveInput = glm::vec3(h, 0.f, v);

	glm::vec3 velocity = glm::vec3(0.f);

	auto planarDirection = glm::vec3(0.f, glm::radians(yaw), 0.f);
	if (glm::length(moveInput) > 0.f)
	{
		moveInput = glm::normalize(moveInput);
		auto moveDir = glm::normalize(glm::quat(planarDirection) * moveInput);

		//velocity = moveDir;

		if (moveAmount > 0.f)
			targetRotation = glm::quatLookAtLH(moveDir, glm::vec3(0.f, 1.f, 0.f));
	}

	if (rightMouseButtonPressing)
	{
		auto direction = glm::vec3(0.f, glm::radians(yaw), 0.f);
		targetRotation = glm::quat(direction);
	}

	if (targetRotation != transform.rotation)
	{
		transform.rotation = RotateTowards(transform.rotation, targetRotation, rotSpeed);
	}
	if (glm::length(moveInput) > 0.f)
		velocity = glm::normalize(transform.rotation * glm::vec3(0.f, 0.f, 1.f));


	// limit character to bounds of sponza
	//transform.position.x += velocity.x * m_DeltaTime * moveSpeed;
	//if (transform.position.z > 22.364f || transform.position.z < -20.271f || transform.position.x > 8.761f || transform.position.x < -9.821f)
	//	transform.position.x -= velocity.x * m_DeltaTime * moveSpeed;

	//transform.position.z += velocity.z * m_DeltaTime * moveSpeed;
	//if (transform.position.z > 22.364f || transform.position.z < -20.271f || transform.position.x > 8.761f || transform.position.x < -9.821f)
	//	transform.position.z -= velocity.z * m_DeltaTime * moveSpeed;

	transform.position += velocity * m_DeltaTime * moveSpeed;



	//if (pad.thumbSticks.leftY != 0.f)
	//{
	//	transform.position += pad.thumbSticks.leftY * glm::normalize(dirYaw) * m_DeltaTime * moveSpeed;
	//	bWalking = true;
	//}
	//if (pad.thumbSticks.leftX != 0.f)
	//{
	//	transform.position += -pad.thumbSticks.leftX * glm::normalize(glm::cross(dirYaw, up)) * m_DeltaTime * moveSpeed;
	//	transform.rotation = glm::quat(glm::vec3(0.f, glm::radians(yaw + 90.f * pad.thumbSticks.leftX), 0.f));

	//	bWalking = true;
	//}

	auto eulerRotation = glm::eulerAngles(transform.rotation);
	m_ModelProps[2].rotation.x = glm::degrees(eulerRotation.x);
	m_ModelProps[2].rotation.y = glm::degrees(eulerRotation.y);
	m_ModelProps[2].rotation.z = glm::degrees(eulerRotation.z);

	m_ModelProps[2].translation = transform.position;

	if (m_Animator)
	{
		if (!bWalking)
		{
			if (m_IdleRunBlendFactor > 0.f)
				m_IdleRunBlendFactor -= m_DeltaTime * 8.f;
			if (m_IdleRunBlendFactor < 0.f)
				m_IdleRunBlendFactor = 0.f;
		}
		else if (bWalking)
		{
			if (m_IdleRunBlendFactor < 1.f)
				m_IdleRunBlendFactor += m_DeltaTime * 8.f;
			if (m_IdleRunBlendFactor > 1.f)
				m_IdleRunBlendFactor = 1.f;
		}

		aimPitch = glm::degrees(2 * XM_PI - glm::eulerAngles(quatPitch).x);

		glm::quat targetPitchAimRotation = aimPitchRotation;

		targetPitchAimRotation = quatPitch;

		aimPitchRotation = RotateTowards(aimPitchRotation, targetPitchAimRotation, rotSpeed);


		m_Animator->BlendTwoAnimations(m_IdleAnimation, m_RunAnimation, m_IdleRunBlendFactor, m_DeltaTime);
		//else if (rightMouseButtonPressing)
		//{
		//	m_Animator->BlendTwoAnimationsPerBone(m_IdleAnimation, m_IdleAnimation, "mixamorig:Spine1", m_DeltaTime, yaw, glm::degrees(glm::eulerAngles(aimPitchRotation).x), glm::axis(aimPitchRotation) * glm::quat(glm::vec3(0.f, glm::radians(-30.f), 0.f)));
		//}
	}
}

void TestApplication::HandleMouseInput()
{
	double xpos;
	double ypos;
	glfwGetCursorPos(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), &xpos, &ypos);

	static bool firstMouse = true;
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	if (viewPortActive)
	{
		m_Cameras[SelectedCamera].ProcessMouseMovement(xoffset, yoffset);

		Xoffset = xoffset;
		Yoffset = yoffset;

		if (xpos == lastX && ypos == lastY)
			mouseMoving = false;
		else
			mouseMoving = true;
	}

	lastX = xpos;
	lastY = ypos;
}

void TestApplication::RenderCubeFromTheInsideInit()
{
	Vertex vertices[] =
	{
		{ glm::vec3(-1.0f, 1.0f, -1.0f),  glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
		{ glm::vec3(1.0f, 1.0f, -1.0f),   glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
		{ glm::vec3(1.0f, 1.0f, 1.0f),    glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
		{ glm::vec3(-1.0f, 1.0f, 1.0f),   glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},

		{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 101, 0.f},
		{ glm::vec3(1.0f, -1.0f, -1.0f),  glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 101, 0.f},
		{ glm::vec3(1.0f, -1.0f, 1.0f),   glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), 101, 0.f},
		{ glm::vec3(-1.0f, -1.0f, 1.0f),  glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), 101, 0.f},

		{ glm::vec3(-1.0f, -1.0f, 1.0f),  glm::vec2(0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), 101, 0.f},
		{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), 101, 0.f},
		{ glm::vec3(-1.0f, 1.0f, -1.0f),  glm::vec2(1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), 101, 0.f},
		{ glm::vec3(-1.0f, 1.0f, 1.0f),   glm::vec2(0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), 101, 0.f},

		{ glm::vec3(1.0f, -1.0f, 1.0f),	glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 101, 0.f},
		{ glm::vec3(1.0f, -1.0f, -1.0f),  glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 101, 0.f},
		{ glm::vec3(1.0f, 1.0f, -1.0f),	glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), 101, 0.f},
		{ glm::vec3(1.0f, 1.0f, 1.0f),	glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), 101, 0.f},

		{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), 101, 0.f},
		{ glm::vec3(1.0f, -1.0f, -1.0f),  glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), 101, 0.f},
		{ glm::vec3(1.0f, 1.0f, -1.0f),	glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), 101, 0.f},
		{ glm::vec3(-1.0f, 1.0f, -1.0f),  glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), 101, 0.f},

		{ glm::vec3(-1.0f, -1.0f, 1.0f),  glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 101, 0.f},
		{ glm::vec3(1.0f, -1.0f, 1.0f),   glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 101, 0.f},
		{ glm::vec3(1.0f, 1.0f, 1.0f),    glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), 101, 0.f},
		{ glm::vec3(-1.0f, 1.0f, 1.0f),   glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), 101, 0.f},
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

	HRESULT hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pCubemapVertexBuffer);
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

	hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pCubemapIndexBuffer);
	if (FAILED(hr)) return;
}

void TestApplication::RenderCubeFromTheInside()
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	m_DX11DeviceContext->IASetVertexBuffers(0, 1, &pCubemapVertexBuffer, &stride, &offset);
	m_DX11DeviceContext->IASetIndexBuffer(pCubemapIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_DX11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_DX11DeviceContext->DrawIndexed(36, 0, 0);
}

void TestApplication::CreateCubemapTexture(bool upsideDown, int width, int height)
{
	glm::vec3 vectors[] =
	{
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f,  -1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  -1.0f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f, 1.0f,  0.0f), glm::vec3(0.0f,  0.0f, 1.0f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)
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

	m_DX11DeviceContext->OMSetDepthStencilState(m_CubemapDepthStencilState, 1);
	m_DX11DeviceContext->RSSetState(m_CubemapRasterizerState);

	std::vector<RenderTexture*> cubeFaces;

	// Create faces
	for (int i = 0; i < 6; ++i)
	{
		RenderTexture* renderTexture = new RenderTexture;
		renderTexture->Initialize(width, height, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R16G16B16A16_FLOAT, false);
		cubeFaces.push_back(renderTexture);
	}

	for (int i = 0; i < 6; i++)
	{
		RenderTexture* texture = cubeFaces[i];
		// clear		
		m_Renderer->OnWindowResize(width, height);
		texture->SetRenderTarget(false);
		texture->ClearRenderTarget(0.0f, 0.0f, 0.0f, 1.0f, 1.0f);

		// prepare
		MatricesCB matricesCb;
		matricesCb.viewMat = captureViews[i];
		matricesCb.projMat = glm::perspectiveFovLH(glm::radians(90.f), (float)width, (float)height, NEAR_PLANE, FAR_PLANE);
		matricesCb.modelMat = glm::mat4(1.f);

		m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, nullptr, &matricesCb, 0, 0);

		m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);
		m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);
		m_DX11DeviceContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

		// draw cube
		RenderCubeFromTheInside();

		m_Renderer->EndScene(true);
	}

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

	HRESULT result = m_DX11Device->CreateTexture2D(&texDesc, nullptr, &m_CubemapTexture);
	if (FAILED(result)) return;

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

		m_DX11DeviceContext->CopySubresourceRegion(m_CubemapTexture, D3D11CalcSubresource(0, i, 1), 0, 0, 0, texture->GetTexture(),
			0, &sourceRegion);
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = texDesc.MipLevels;
	srvDesc.TextureCube.MostDetailedMip = 0;

	result = m_DX11Device->CreateShaderResourceView(m_CubemapTexture, &srvDesc, &m_CubemapShaderResourceView);
	if (FAILED(result)) return;

	for (RenderTexture* cubeFace : cubeFaces)
		delete cubeFace;

	m_DX11DeviceContext->OMSetDepthStencilState(nullptr, 0);
	m_DX11DeviceContext->RSSetState(nullptr);
	((DX11RendererAPI*)RendererAPI::GetInstance().get())->ResetRenderTargetView();
	RECT rect;
	GetClientRect(((WindowsWindow*)m_Window.get())->GetHWND(), &rect);
	RendererAPI::GetInstance()->OnWindowResize(rect.right - rect.left, rect.bottom - rect.top);
}

void TestApplication::InitHDRCubemap(std::string path)
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

	hr = CreateShaderResourceView(m_DX11Device, (*image).GetImages(), (*image).GetImageCount(),
		(*image).GetMetadata(), &m_HDRShaderResourceView);
	if (FAILED(hr))
	{
		std::cout << "Can't create SRV from HDR texture\n";
		return;
	}

	m_RectToCubeVertexShader->Bind();
	m_RectToCubePixelShader->Bind();
	m_DX11DeviceContext->PSSetShaderResources(0, 1, &m_HDRShaderResourceView);
	CreateCubemapTexture(false, 1024, 1024);
}

void TestApplication::RenderHDRCubemap()
{
	MatricesCB cb;
	cb.modelMat = glm::mat4(1.f);
	cb.viewMat = m_CameraViewMatrix;
	cb.projMat = glm::perspectiveFovLH(XM_PIDIV4, (float)texWidth, (float)texHeight, NEAR_PLANE, FAR_PLANE);

	m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, NULL, &cb, 0, 0);
	m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);
	m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);
	m_DX11DeviceContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	m_CubemapVertexShader->Bind();
	m_CubemapPixelShader->Bind();
	m_DX11DeviceContext->PSSetShaderResources(0, 1, &m_CubemapShaderResourceView);

	m_DX11DeviceContext->OMSetDepthStencilState(m_CubemapDepthStencilState, 0);
	m_DX11DeviceContext->RSSetState(m_CubemapRasterizerState);

	RenderCubeFromTheInside();

	m_DX11DeviceContext->OMSetDepthStencilState(0, 0);
	m_DX11DeviceContext->RSSetState(nullptr);
}

void TestApplication::PostprocessingToTextureInit()
{
	SAFE_RELEASE(postprocessingSRV);
	SAFE_RELEASE(postprocessingTexture);

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

	HRESULT result = m_DX11Device->CreateTexture2D(&texDesc, nullptr, &postprocessingTexture);
	if (FAILED(result)) return;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;

	result = m_DX11Device->CreateShaderResourceView(postprocessingTexture, &srvDesc, &postprocessingSRV);
	if (FAILED(result)) return;

	SAFE_DELETE(postprocessingRenderTexture);
	postprocessingRenderTexture = new RenderTexture;
	postprocessingRenderTexture->Initialize(texWidth, texHeight, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R32G32B32A32_FLOAT, false);
}

void TestApplication::PostprocessingToTextureBegin()
{
	DX11RendererAPI::GetInstance()->SetViewport(vRegionMinX, vRegionMinY, texWidth, texHeight);
	postprocessingRenderTexture->SetRenderTarget(false);
	postprocessingRenderTexture->ClearRenderTarget(0.01f, 0.01f, 0.01f, 1.f, 1.f);
}

void TestApplication::PostprocessingToTextureEnd()
{
	D3D11_BOX sourceRegion;
	sourceRegion.left = 0;
	sourceRegion.right = texWidth;
	sourceRegion.top = 0;
	sourceRegion.bottom = texHeight;
	sourceRegion.front = 0;
	sourceRegion.back = 1;

	m_DX11DeviceContext->CopySubresourceRegion(postprocessingTexture, D3D11CalcSubresource(0, 0, 1), 0, 0, 0, postprocessingRenderTexture->GetTexture(),
		0, &sourceRegion);

	static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->ResetRenderTargetView();
}

void TestApplication::PhysXEventCallback::onConstraintBreak(PxConstraintInfo* constraints, PxU32 count)
{
}

void TestApplication::PhysXEventCallback::onWake(PxActor** actors, PxU32 count)
{
}

void TestApplication::PhysXEventCallback::onSleep(PxActor** actors, PxU32 count)
{
}

void TestApplication::PhysXEventCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
	/*for (physx::PxU32 i = 0; i < nbPairs; i++)
	{
		const physx::PxContactPair& cp = pairs[i];

		if (cp.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			std::mt19937 generator(123);
			std::uniform_int_distribution<int> dis(1, 4);

			int soundIndex = dis(generator);
			FMOD::Channel* channel;
			switch (soundIndex)
			{
			case 1:
				m_App->m_FmodSystem->playSound(m_App->m_BoxDrop1, nullptr, false, &channel);
				break;
			case 2:
				m_App->m_FmodSystem->playSound(m_App->m_BoxDrop2, nullptr, false, &channel);
				break;
			case 3:
				m_App->m_FmodSystem->playSound(m_App->m_BoxDrop3, nullptr, false, &channel);
				break;
			case 4:
				m_App->m_FmodSystem->playSound(m_App->m_BoxDrop4, nullptr, false, &channel);
				break;
			}
		}
	}*/
}

void TestApplication::PhysXEventCallback::onTrigger(PxTriggerPair* pairs, PxU32 count)
{
}

void TestApplication::PhysXEventCallback::onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count)
{
}

PxFilterFlags TestApplication::PhysXFilterCallback::pairFound(PxU64 pairID, PxFilterObjectAttributes attributes0, PxFilterData filterData0, const PxActor* a0, const PxShape* s0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, const PxActor* a1, const PxShape* s1, PxPairFlags& pairFlags)
{
	return PxFilterFlags();
}

void TestApplication::PhysXFilterCallback::pairLost(PxU64 pairID, PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, bool objectRemoved)
{
}

bool TestApplication::PhysXFilterCallback::statusChange(PxU64& pairID, PxPairFlags& pairFlags, PxFilterFlags& filterFlags)
{
	return false;
}
