#include "ovpch.h"
#include "TestApplication.h"

#include "Oeuvre/Events/EventHandler.h"
#include <iostream>

#include "Oeuvre/Core/EntryPoint.h"

#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"

#include "GLFW/glfw3.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include "Oeuvre/Renderer/RHI/ITexture.h"
#include "Platform/DirectX11/DX11RendererAPI.h"

#include <DirectXTex.h>

#include "DirectXMath.h"

#include <wrl/client.h>

#include <fstream>

#include "Oeuvre/Renderer/Frustum.h"
#include <Platform/DirectX11/DX11Texture2D.h>

#include <glm/gtx/matrix_decompose.hpp>
#include <limits>

#include "Oeuvre/Scene/SceneSerializer.h"
#include "Oeuvre/Scene/ScriptableEntity.h"
#include "Oeuvre/Scripting/ScriptEngine.h"
#include "Oeuvre/Utils/Math.h"

#include "Oeuvre/Utils/PlatformUtils.h"

#include <d3dx12.h>

#include <Oeuvre/Core/Input.h>
#include <thread>

#include <glm/gtx/intersect.hpp>

#include <Platform/Vulkan/VkImages.h>
#include <Platform/Vulkan/VkInitializers.h>
#include <Platform/Vulkan/VulkanRendererAPI.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Platform/OpenGL/OGLRendererAPI.h>
#include <stb_image.h>

#include "Oeuvre/Renderer/ResourceManager.h"
#include <Platform/DirectX12/DX12Texture2D.h>

#include "Oeuvre/Physics/Intersections.h"

#define PLAYER_MODEL 0

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

std::unique_ptr<Application> Oeuvre::CreateApplication()
{
	return std::make_unique<TestApplication>();
}

#define CHECK_LINE() { std::string text = "Point reached in file: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__) + "\n"; std::cout << text; }

bool TestApplication::Init()
{
	m_Fullscreen = ((WindowsWindow*)m_Window.get())->IsFullscreen();
	Log::Initialize();
	m_Renderer->Init();

	if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
	{
		D3D11Init();
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
	{
		D3D12Init();
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
	{
		VulkanInit();
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
	{
		OpenGLInit();
	}

	ResourceManager::InitErrorTextures();

	m_Scene = std::make_shared<Scene>();
	m_SceneHierarchyPanel = std::make_shared<SceneHierarchyPanel>(m_Scene);
	//if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11) 
		//m_ContentBrowserPanel = std::make_shared<ContentBrowserPanel>();

	auto baseEntitiy = m_Scene->CreateEntity("Scene");

	InitMeshEntities(baseEntitiy);

	if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
	{
		auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
		dx12Api->CloseAndFlushCommandlist();
	}

	InitLightEntities(baseEntitiy);

	float scale = SPONZA_SCALE;

	m_PlayerHeight = scale * 125.f;
	m_PlayerMoveSpeed = scale * 400.f;
	m_CameraXOffset = scale * 46.875f;
	m_CameraBoomLength = scale * 312.5f;

	m_CharacterCapsuleColliderRadius = scale * 12.5;
	m_CharacterCapsuleColliderHalfHeight = scale * 47.5f;

	m_ZombieHeight = m_PlayerHeight * 0.75f;

	ImguiInit();

	// Animations
	PrepareAnimations();

	// Audio
	FMODInit();

	// Gamepad
	//m_GamePad = std::make_unique<GamePad>();

	// Physics
	InitPhysics();

	lightPositionsPrev.resize(MAX_POINT_LIGHTS);

	// scripting
	ScriptEngine::Init();

	std::cout << "Textures in pool: " << ResourceManager::GetTextures().size() << "\n";

	ed::Config config;
	config.SettingsFile = "Simple.json";
	m_EdContext = ed::CreateEditor(&config);

	return true;
}

void TestApplication::InitLightEntities(const Oeuvre::Entity& baseEntitiy)
{
	// for sponza
	auto lightEnt = m_Scene->CreateEntity("PointLight");
	auto lightEnt1 = m_Scene->CreateEntity("PointLight1");
	auto lightEnt2 = m_Scene->CreateEntity("PointLight2");

	float pointLightRange = 500.f * SPONZA_SCALE;

	lightEnt.AddComponent<PointLightComponent>().Range = pointLightRange;
	lightEnt1.AddComponent<PointLightComponent>().Range = pointLightRange;
	lightEnt2.AddComponent<PointLightComponent>().Range = pointLightRange;

	lightEnt.GetComponent<TransformComponent>().Translation = { 0.f, 210.f * SPONZA_SCALE, 0.f };
	lightEnt1.GetComponent<TransformComponent>().Translation = { 300.f * SPONZA_SCALE, 48.f * SPONZA_SCALE, -29.5f * SPONZA_SCALE };
	lightEnt2.GetComponent<TransformComponent>().Translation = { -300.f * SPONZA_SCALE, 36.f * SPONZA_SCALE, 195.f * SPONZA_SCALE };

	lightEnt.AddParent(baseEntitiy);
	lightEnt1.AddParent(baseEntitiy);
	lightEnt2.AddParent(baseEntitiy);

	auto cameraEnt = m_Scene->CreateEntity("Camera");
	cameraEnt.AddComponent<CameraComponent>();

	class CameraController : public ScriptableEntity
	{
	public:
		void OnCreate()
		{
			//std::cout << "CameraController::OnCreate! \n";
		}

		void OnDestroy()
		{

		}

		void OnUpdate(float dt)
		{
			//std::cout << "Timestep: " << dt << "\n";
		}
	};

	cameraEnt.AddComponent<NativeScriptComponent>().Bind<CameraController>();
	cameraEnt.AddParent(baseEntitiy);

	auto sunLightEnt = m_Scene->CreateEntity("SunLight");
	sunLightEnt.GetComponent<TransformComponent>().Translation = { 2000.f * SPONZA_SCALE, 0.f, 500.f * SPONZA_SCALE };
	sunLightEnt.GetComponent<TransformComponent>().Rotation = { 60.f, 45.f, 0.f };
	sunLightEnt.AddParent(baseEntitiy);
}

void TestApplication::InitMeshEntities(const Oeuvre::Entity& baseEntitiy)
{
	std::string prefix = "../resources/models/colt_python/";
	auto coltEnt = m_Scene->CreateEntity("Colt");
	coltEnt.GetComponent<TransformComponent>().Scale = glm::vec3(5.f);
	coltEnt.AddComponent<MeshComponent>(prefix + "source/revolver_game.fbx", prefix + "textures/M_WP_Revolver_albedo.jpg", prefix + "textures/M_WP_Revolver_normal.png", prefix + "textures/M_WP_Revolver_roughness.jpg", prefix + "textures/M_WP_Revolver_metallic.jpg", true);
	coltEnt.AddParent(baseEntitiy);

	auto sponzaEnt = m_Scene->CreateEntity("Sponza");
	sponzaEnt.AddComponent<MeshComponent>("../resources/models/sponza/glTF/Sponza.gltf", "", "", "", "", true);
	sponzaEnt.GetComponent<MeshComponent>().CombinedTextures = false;
	sponzaEnt.GetComponent<MeshComponent>().m_Model->GetUseCombinedTextures() = false;
	sponzaEnt.GetComponent<TransformComponent>().Scale = glm::vec3(SPONZA_SCALE);
	sponzaEnt.GetComponent<TransformComponent>().Rotation.y = 90.f;
	sponzaEnt.AddParent(baseEntitiy);

	//auto bistroEnt = m_Scene->CreateEntity("Bistro");
	//bistroEnt.AddComponent<MeshComponent>("..\\resources\\models\\Bistro_v5_2\\BistroExterior.fbx", "", "", "", "", true);
	//bistroEnt.GetComponent<MeshComponent>().CombinedTextures = false;
	//bistroEnt.GetComponent<MeshComponent>().m_Model->GetUseCombinedTextures() = false;
	//bistroEnt.GetComponent<TransformComponent>().Scale = glm::vec3(SPONZA_SCALE);
	//bistroEnt.GetComponent<TransformComponent>().Rotation.x = -90.f;
	//bistroEnt.GetComponent<TransformComponent>().Translation.y = -2.f;
	//bistroEnt.AddParent(baseEntitiy);

	auto playerEnt = m_Scene->CreateEntity("Player");
#if PLAYER_MODEL == 0	
	playerEnt.AddComponent<MeshComponent>("../resources/animations/mixamo/Walking_fixed.fbx", "../resources/animations/mixamo/eve/SpacePirate_M_diffuse.png", "../resources/animations/mixamo/eve/SpacePirate_M_normal.png", "../resources/textures/white.png", "../resources/textures/black.png", true);
	playerEnt.GetComponent<TransformComponent>().Scale = glm::vec3(SPONZA_SCALE * 1.25f);
#else
	playerEnt.AddComponent<MeshComponent>("../resources/animations/mixamo/nathan/BreathingIdle_withSkin.fbx", "../resources/models/nathan/tex/rp_nathan_animated_003_dif.jpg", "../resources/models/nathan/tex/rp_nathan_animated_003_norm.jpg", "../resources/textures/white.png", "../resources/textures/black.png", true);
	playerEnt.GetComponent<TransformComponent>().Scale = glm::vec3(SPONZA_SCALE);
#endif
	playerEnt.AddParent(baseEntitiy);

	auto zombieEnt = m_Scene->CreateEntity("Zombie");
	zombieEnt.AddComponent<MeshComponent>("../resources/animations/mixamo/Cop_Zombie_model_fixed.fbx", "../resources/animations/mixamo/cop_zombie/FuzZombie_diffuse.png", "../resources/animations/mixamo/cop_zombie/FuzZombie_normal.png", "", "", true);
	zombieEnt.GetComponent<TransformComponent>().Scale = glm::vec3(SPONZA_SCALE);
	zombieEnt.AddParent(baseEntitiy);

	// Dude
	for (int i = 0; i < m_NumOfDudes; i++)
	{
		auto dudeEnt = m_Scene->CreateEntity("Dude" + std::to_string(i));
#if PLAYER_MODEL == 0
		dudeEnt.AddComponent<MeshComponent>("../resources/animations/mixamo/Walking_fixed.fbx", "../resources/animations/mixamo/eve/SpacePirate_M_diffuse.png", "../resources/animations/mixamo/eve/SpacePirate_M_normal.png", "../resources/textures/white.png", "../resources/textures/black.png", true);
#else
		dudeEnt.AddComponent<MeshComponent>("../resources/animations/mixamo/nathan/BreathingIdle_withSkin.fbx", "../resources/models/nathan/tex/rp_nathan_animated_003_dif.jpg", "../resources/models/nathan/tex/rp_nathan_animated_003_norm.jpg", "../resources/textures/white.png", "../resources/textures/black.png", true);
#endif
		dudeEnt.GetComponent<TransformComponent>().Scale = glm::vec3(SPONZA_SCALE * 1.25f);
		dudeEnt.GetComponent<TransformComponent>().Translation = glm::vec3(1500.f * SPONZA_SCALE + 200.f * SPONZA_SCALE * (i / 10), 0.f, 400.f * SPONZA_SCALE + 200.f * SPONZA_SCALE * (i % 10));
		dudeEnt.AddParent(baseEntitiy);
	}

	for (int i = 0; i < 6; i++)
	{
		auto platformEnt = m_Scene->CreateEntity("Platform" + std::to_string(i));
		platformEnt.AddComponent<MeshComponent>("../resources/models/platform.fbx", "../resources/textures/wood_planks_4k/wood_planks_diff_4k.png", "../resources/textures/wood_planks_4k/wood_planks_nor_dx_4k.png", "../resources/textures/wood_planks_4k/wood_planks_rough_4k.png", "", true);
		platformEnt.GetComponent<TransformComponent>().Translation = glm::vec3(1100.f * SPONZA_SCALE, 200.f * SPONZA_SCALE + 200.f * SPONZA_SCALE * i, 400.f * SPONZA_SCALE + 200.f * SPONZA_SCALE * i);
		platformEnt.GetComponent<TransformComponent>().Rotation = glm::vec3(-90.f, 0.f, 0.f);
		platformEnt.GetComponent<TransformComponent>().Scale = glm::vec3(10.f * SPONZA_SCALE);
		platformEnt.AddParent(baseEntitiy);
	}
}

void TestApplication::MainLoop()
{
	while (m_Running)
	{
		m_Window->OnUpdate();
		if (m_FreezeRendering) {
			//throttle the speed to avoid the endless spinning
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}
		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			auto vulkanApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();

			if (vulkanApi->_resizeRequested) {
				vulkanApi->ResizeSwapchain();
				OV_INFO("Vulkan swapchain resized");
			}
			vulkanApi->_editorDisabled = m_EditorDisabled;
		}

		float currentFrame = static_cast<float>(glfwGetTime());
		m_DeltaTime = currentFrame - m_LastFrame;
		m_LastFrame = currentFrame;

		m_FPSUpdateDelay += m_DeltaTime;
		if (m_FPSUpdateDelay >= 0.2f)
		{
			m_FPS = 1.f / m_DeltaTime;
			m_FPSUpdateDelay = 0.f;
		}

		m_TimeElapsedAfterShot += m_DeltaTime;
		if (m_TimeElapsedAfterShot >= 0.21f)
			leftMouseButtonPressed = false;

		FMODUpdate();
		HandleMouseInput();

		if (m_DeltaTime > 0.f && m_DeltaTime < 0.1f)
			StepPhysics(m_DeltaTime);
		else
			StepPhysics(1.f / 60.f);

		// Set primary camera
		const auto& cameraEnts = m_Scene->GetAllEntitiesWith<CameraComponent>();
		entt::entity camEnt = { entt::null };
		for (const auto& cameraEnt : cameraEnts)
		{
			if (m_Scene->GetRegistry().get<CameraComponent>(cameraEnt).Primary)
			{
				m_PrimaryCamera = &m_Scene->GetRegistry().get<CameraComponent>(cameraEnt).m_Camera;
				camEnt = cameraEnt;
			}
		}

		// Switch to CAMERA_VIEW mode and back
		if (IsCursorInsideViewport() && m_EditorState == EditorState::NAVIGATION && Input::IsMouseButtonPressed(Mouse::ButtonRight))
		{
			m_EditorState = EditorState::CAMERA_VIEW;
		}
		else if (m_EditorState == EditorState::CAMERA_VIEW && !Input::IsMouseButtonPressed(Mouse::ButtonRight))
		{
			m_EditorState = EditorState::NAVIGATION;
		}

		// Switch editor states
		switch (m_EditorState)
		{
		case EditorState::NAVIGATION:
		{
			m_PlayModeActive = false;
			glfwSetInputMode(m_Window->GetGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			break;
		}
		case EditorState::CAMERA_VIEW:
		{
			glfwSetInputMode(m_Window->GetGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			if (Input::IsKeyPressed(Key::W))
				m_PrimaryCamera->ProcessKeyboard(CameraMovement::FORWARD, m_DeltaTime);
			if (Input::IsKeyPressed(Key::S))
				m_PrimaryCamera->ProcessKeyboard(CameraMovement::BACKWARD, m_DeltaTime);
			if (Input::IsKeyPressed(Key::A))
				m_PrimaryCamera->ProcessKeyboard(CameraMovement::LEFT, m_DeltaTime);
			if (Input::IsKeyPressed(Key::D))
				m_PrimaryCamera->ProcessKeyboard(CameraMovement::RIGHT, m_DeltaTime);

			m_PrimaryCamera->ProcessMouseMovement(Xoffset, Yoffset, true);

			if (camEnt != entt::null)
			{
				auto& transformComp = m_Scene->GetRegistry().get<TransformComponent>(camEnt);
				transformComp.Translation = m_PrimaryCamera->GetPosition();
			}
			break;
		}
		case EditorState::PLAYING:
			m_PlayModeActive = true;
			break;
		}

		if (m_PlayModeActive)
		{
			glfwSetInputMode(m_Window->GetGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			if (m_PlayModeActive != m_PlayModeActivePrev)
				m_Scene->OnRuntimeStart();

			m_Scene->OnRuntimeUpdate(m_DeltaTime);

			MoveCharacter();
			MoveZombie();

			float profilerBeginTime = (float)glfwGetTime();

			for (int i = 0; i < m_NumOfDudes; i++)
				m_DudeAnimators[i]->UpdateAnimation(m_DeltaTime);

			m_DudesAnimationsUpdateTime = (float)glfwGetTime() - profilerBeginTime;
		}
		else
		{
			if (m_PlayModeActive != m_PlayModeActivePrev)
				m_Scene->OnRuntimeStop();
		}
		m_PlayModeActivePrev = m_PlayModeActive;

		// setting deafult camera properties
		m_CameraViewMatrix = m_PrimaryCamera->GetViewMatrix();
		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			m_CameraProjMatrix = glm::perspectiveFovLH_NO(glm::radians(m_CameraFovDegrees), (float)texWidth, (float)texHeight, CAM_NEAR_PLANE, CAM_FAR_PLANE);
			//m_CameraProjMatrix[1][1] *= -1;
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			m_CameraProjMatrix = glm::perspectiveFovRH_NO(glm::radians(m_CameraFovDegrees), (float)texWidth, (float)texHeight, CAM_NEAR_PLANE, CAM_FAR_PLANE);
			//m_CameraProjMatrix[1][1] *= -1;
		}
		else
			m_CameraProjMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_CameraFovDegrees), (float)texWidth, (float)texHeight, CAM_NEAR_PLANE, CAM_FAR_PLANE);
		m_CameraPos = m_PrimaryCamera->GetPosition();
		m_CameraFrontVector = m_PrimaryCamera->GetFrontVector();


		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			VulkanBeforeImguiFrame();
		}

		float profilerBeginTime = (float)glfwGetTime();

		if (!m_EditorDisabled)
		{
			ImguiFrame();
		}

		m_ImguiFrameTime = (float)glfwGetTime() - profilerBeginTime;

		if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
		{
			D3D12BeforeBeginFrame();
		}

		ResourceManager::LoadPendingEntityModels(m_Scene.get());

		m_Renderer->BeginFrame();

		if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
		{
			float profilerBeginTime = (float)glfwGetTime();

			D3D11Frame();

			m_D3D11FullFrameTime = (float)glfwGetTime() - profilerBeginTime;
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
		{
			D3D12Frame();
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			VulkanFrame();
			VulkanBetween();
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			OpenGLFrame();
		}

		if (!m_EditorDisabled)
		{
			float profilerBeginTime = (float)glfwGetTime();

			ImguiRender();

			m_ImguiRenderTime = (float)glfwGetTime() - profilerBeginTime;
		}
		m_Renderer->EndFrame(m_bEnableVsync);
		if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
		{
			D3D12AfterEndFrame();
		}

		m_Window->OnDraw();
	}
	Cleanup();
}

void TestApplication::Cleanup()
{
	if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
	{
		SAFE_RELEASE(frameSRV);
		SAFE_RELEASE(frameTexture);
		SAFE_DELETE(renderTexture);

		SAFE_RELEASE(g_pCBMatrixes);
		SAFE_RELEASE(g_pCBLight);
		SAFE_RELEASE(g_pCBPointLightShadowGen);
		SAFE_RELEASE(g_pCBMaterial);
		SAFE_RELEASE(g_pCBSkeletalAnimation);

		SAFE_RELEASE(g_pSamplerLinearWrap);
		SAFE_RELEASE(g_pSamplerPointWrap);
		SAFE_RELEASE(g_pSamplerPointClamp);

		SAFE_RELEASE(m_SpotLightDepthSRV);
		SAFE_RELEASE(m_SpotLightDepthTexture);
		SAFE_DELETE(m_SpotLightDepthRenderTexture);

		SAFE_RELEASE(m_SamplerLinearClampComparison);
		SAFE_RELEASE(m_SamplerLinearClamp);

		for (int i = 0; i < MAX_POINT_LIGHTS; i++)
		{
			SAFE_RELEASE(m_PointLightDepthSRVs[i]);
			SAFE_RELEASE(m_PointLightDepthTextures[i]);
			SAFE_DELETE(m_PointLightDepthRenderTextures[i]);
		}

		SAFE_RELEASE(m_DefaultRasterizerState);

		SAFE_RELEASE(pCubeVertexBuffer);
		SAFE_RELEASE(pCubeIndexBuffer);
		SAFE_RELEASE(pQuadVertexBuffer);
		SAFE_RELEASE(pQuadIndexBuffer);

		SAFE_DELETE(m_Animator);
		SAFE_DELETE(m_RunAnimation);
		SAFE_DELETE(m_PistolIdleAnimation);
		SAFE_DELETE(m_ShootingAnimation);
		SAFE_DELETE(m_JumpingAnimation);

		SAFE_RELEASE(m_SunLightDepthTexture);
		SAFE_RELEASE(m_SunLightDepthSRV);
		SAFE_DELETE(m_SunLightDepthRenderTexture);

		m_Terrain->Shutdown();
		NvidiaHBAOPlusCleanup();
	}

	CleanupPhysics();
	FMODCleanup();

	ImguiCleanup();

	ScriptEngine::Shutdown();

	m_Scene->Shutdown();
	m_Renderer->Shutdown();
	m_Window->OnClose();
}

void TestApplication::OnWindowEvent(const Event<WindowEvents>& e)
{
	if (e.GetType() == WindowEvents::WindowResize)
	{
		auto windowResizeEvent = e.ToType<WindowResizeEvent>();
		//OV_INFO("Window Resize Event! New Dimensions: {}x{}", windowResizeEvent.width, windowResizeEvent.height);

		if (windowResizeEvent.width > 0 && windowResizeEvent.height > 0)
		{
			m_Renderer->OnWindowResize(windowResizeEvent.width, windowResizeEvent.height);
			m_Window->OnResize(windowResizeEvent.width, windowResizeEvent.height);
		}
		if (windowResizeEvent.width <= 0 && windowResizeEvent.height <= 0)
			m_FreezeRendering = true;
		else
			m_FreezeRendering = false;
	}
	else if (e.GetType() == WindowEvents::WindowClose)
	{
		//OV_INFO("Window Close Event!");
		m_Running = false;
	}
}

void TestApplication::OnMouseEvent(const Event<MouseEvents>& e)
{
	if (e.GetType() == MouseEvents::MouseMoved)
	{
		auto mouseMovedEvent = e.ToType<MouseMovedEvent>();
		//OV_INFO("Mouse Moved Event! Mouse Position: ({}, {})", mouseMovedEvent.x, mouseMovedEvent.y);
		auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
		dx12Api->OnMouseMove(mouseMovedEvent.x, mouseMovedEvent.y);
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

		//if (mouseButtonDownEvent.button == Mouse::ButtonLeft && m_EditorState == EditorState::CAMERA_VIEW)
		//{
		//	glm::vec3 hitPoint;
		//	glm::vec3 origin = m_CameraPos;
		//	bool hit = RayIntersectsMeshTriangles("Sponza", origin, glm::normalize(m_CameraFrontVector), hitPoint);
		//	if (hit)
		//	{
		//		rays.push_back({ origin, hitPoint });
		//	}
		//}

		if (m_PlayModeActive && mouseButtonDownEvent.button == Mouse::ButtonLeft && rightMouseButtonPressing)
		{
			//AddBulletHit();
			leftMouseButtonPressed = true;
			FMOD::Channel* channel;
			int x = dist(engine);
			switch (x)
			{
			case 0:
				m_FmodSystem->playSound(m_RevolverShot, nullptr, false, &channel);
				break;
			case 1:
				m_FmodSystem->playSound(m_RevolverShot1, nullptr, false, &channel);
				break;
			case 2:
				m_FmodSystem->playSound(m_RevolverShot2, nullptr, false, &channel);
				break;
			case 3:
				m_FmodSystem->playSound(m_RevolverShot3, nullptr, false, &channel);
				break;
			default:
				m_FmodSystem->playSound(m_RevolverShot, nullptr, false, &channel);
				break;
			}

			m_TimeElapsedAfterShot = 0.f;
		}
	}
	else if (e.GetType() == MouseEvents::MouseButtonUp)
	{
		//auto mouseButtonUpEvent = e.ToType<MouseButtonUpEvent>();
		//OV_INFO("Mouse Button Up Event! Mouse Button Up: {}", mouseButtonUpEvent.button);
	}
	else if (e.GetType() == MouseEvents::MouseScroll)
	{
		//auto mouseScrollEvent = e.ToType<MouseScrollEvent>();
		//OV_INFO("Mouse Scroll Event! xoffset: {}, yoffset: {}", mouseScrollEvent.xoffset, mouseScrollEvent.yoffset);
	}
}

void TestApplication::OnKeyEvent(const Event<KeyEvents>& e)
{
	if (e.GetType() == KeyEvents::KeyDown)
	{
		auto keyDownEvent = e.ToType<KeyDownEvent>();
		//OV_INFO("Key Down Event! Key Down: {}", keyDownEvent.keycode);
		if (keyDownEvent.keycode == GLFW_KEY_C)
		{
			if (m_EditorState == EditorState::NAVIGATION && IsCursorInsideViewport())
				m_EditorState = EditorState::PLAYING;
			else if (m_EditorState == EditorState::PLAYING)
				m_EditorState = EditorState::NAVIGATION;
		}
		if (keyDownEvent.keycode == GLFW_KEY_SPACE && m_Physics.m_Character->IsSupported())
		{
			bJumping = true;
			m_JumpPower = 10.f;
		}
		if (keyDownEvent.keycode == GLFW_KEY_I)
		{
			m_EditorDisabled = !m_EditorDisabled;
		}
		if (keyDownEvent.keycode == GLFW_KEY_F && (RendererAPI::GetAPI() == RendererAPI::API::DirectX11 || RendererAPI::GetAPI() == RendererAPI::API::OpenGL))
		{
			m_Physics.ThrowCube(Vec3(m_CameraPos.x, m_CameraPos.y, m_CameraPos.z), Vec3(m_CameraFrontVector.x, m_CameraFrontVector.y, m_CameraFrontVector.z));
		}
		if (keyDownEvent.keycode == GLFW_KEY_F11)
		{
			m_Fullscreen = !m_Fullscreen;
			m_Window->SetFullscreen(m_Fullscreen);
			RECT clientRect;
			GetClientRect(WindowsWindow::GetInstance()->GetHWND(), &clientRect);
			m_Renderer->OnWindowResize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
		}
		if (keyDownEvent.keycode == GLFW_KEY_E)
		{
			glm::vec3 camFront = m_CameraFrontVector;
			JPH::Vec3 impulse = JPH::Vec3(camFront.x, camFront.y, camFront.z) * 50.f;
			m_Physics.physics_system->GetBodyInterface().AddImpulse(m_Physics.sphere_id, impulse);
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
	if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
	{
		ImGui_ImplGlfw_InitForVulkan(m_Window->GetGLFWwindow(), true);
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
	{
		ImGui_ImplGlfw_InitForOpenGL(m_Window->GetGLFWwindow(), true);
	}
	else
	{
		ImGui_ImplGlfw_InitForOther(m_Window->GetGLFWwindow(), true);
	}


	if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
	{
		auto device = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDevice();
		auto deviceContext = static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->GetDeviceContext();
		ImGui_ImplDX11_Init(device, deviceContext);
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
	{
		auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();

		// Setup Platform/Renderer backends
		ImGui_ImplDX12_InitInfo init_info = {};
		init_info.Device = dx12Api->GetDevice().Get();
		init_info.CommandQueue = dx12Api->GetCommandQueue().Get();
		init_info.NumFramesInFlight = dx12Api->GetSwapchainBufferCount() * _NumFrameResources;
		init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM; // Or your render target format.

		// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
		// The example_win32_directx12/main.cpp application include a simple free-list based allocator.
		init_info.SrvDescriptorHeap = dx12Api->GetSRVDescriptorHeap();
		init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return DX12RendererAPI::g_pd3dSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
		init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return DX12RendererAPI::g_pd3dSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };

		ImGui_ImplDX12_Init(&init_info);
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
	{
		auto vulkanApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = vulkanApi->_instance;
		init_info.PhysicalDevice = vulkanApi->_chosenGPU;
		init_info.Device = vulkanApi->_device;
		init_info.Queue = vulkanApi->_graphicsQueue;
		init_info.DescriptorPool = vulkanApi->_imguiPool;
		init_info.MinImageCount = 3;
		init_info.ImageCount = 3;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.UseDynamicRendering = true;

		//dynamic rendering parameters for imgui to use
		init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanApi->_swapchainImageFormat;

		ImGui_ImplVulkan_Init(&init_info);

		VkSamplerCreateInfo sampler_info{};
		sampler_info.pNext = nullptr;
		sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_info.magFilter = VK_FILTER_NEAREST;
		sampler_info.minFilter = VK_FILTER_NEAREST;
		sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.minLod = -1000.f;
		sampler_info.maxLod = 1000.f;
		sampler_info.maxAnisotropy = 1.0f;
		sampler_info.unnormalizedCoordinates = VK_FALSE;

		vkCreateSampler(vulkanApi->_device, &sampler_info, nullptr, &_imguiSampler);

		_imguiDS = ImGui_ImplVulkan_AddTexture(_imguiSampler, vulkanApi->_imguiImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
	{
		ImGui_ImplOpenGL3_Init();
	}

	SetupImGuiStyle();
}

template<typename UIFunction>
static void DrawSection(const std::string& name, UIFunction uiFunction)
{
	const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

	ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImGui::Separator();
	bool open = ImGui::TreeNodeEx((void*)std::hash<std::string>{}(name), treeNodeFlags, name.c_str());
	ImGui::PopStyleVar();

	if (open)
	{
		uiFunction();
		ImGui::TreePop();
	}
}

void TestApplication::ImguiFrame()
{
	// (Your code process and dispatch Win32 messages)
	// Start the Dear ImGui frame
	if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
		ImGui_ImplDX11_NewFrame();
	else if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
		ImGui_ImplDX12_NewFrame();
	else if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		ImGui_ImplVulkan_NewFrame();
	else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		ImGui_ImplOpenGL3_NewFrame();

	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGuizmo::BeginFrame();

	int windowFlags = 0;

	bool disableInputsCondition = m_EditorState == EditorState::CAMERA_VIEW || m_EditorState == EditorState::PLAYING;

	if (disableInputsCondition)
		windowFlags |= ImGuiWindowFlags_NoInputs;
	else
		windowFlags &= ~ImGuiWindowFlags_NoInputs;

	SetupImguiDockspace(disableInputsCondition);

	ImGui::Begin("Options", nullptr, windowFlags);
	ImGui::Checkbox("VSync", &m_bEnableVsync);

	DrawSection("Lighting", [this]() {
		ImGui::Checkbox("Dynamic shadows", &m_LightProps.dynamicShadows);
		ImGui::Checkbox("HemisphericAmbient", &hemisphericAmbient);
		ImGui::ColorEdit3("Ambient down", (float*)&ambientDown);
		ImGui::ColorEdit3("Ambient up", (float*)&ambientUp);
		ImGui::SliderFloat("PointLight Bias", &m_ShadowMapBias, 0.00001f, 0.0006f);
		});

	DrawSection("Postprocessing", [this]() {
		ImGui::Checkbox("Bloom Enabled", &m_BloomEnabled);
		ImGui::SliderFloat("Bloom Scale", &g_fBloomScale, 0.1f, 2.0f);
		ImGui::SliderFloat("Bloom Threshold", &g_fBloomThreshold, 0.1f, 2.5f);
		ImGui::SliderFloat("Bloom Knee", &g_fBloomKnee, 0.f, 1.f);
		ImGui::SliderFloat("Bloom Clamp", &g_fBloomClamp, 0.f, 65472.f);
		ImGui::SliderInt("Blur Passes", &g_iBlurPasses, 1, 10);
		static const char* items[]{ "Uncharted2","ACES","Reinhard", "ReinhardLuma" };
		ImGui::Combo("ToneMap", &m_ToneMapPreset, items, SIZE_OF_ARRAY(items));
		ImGui::SliderFloat("Exposure", &m_Exposure, 0.f, 10.f);
		ImGui::Checkbox("Enable HBAO+", &m_bEnableHBAO);
		ImGui::Checkbox("Enable FXAA", &m_bEnableFXAA);
		ImGui::SliderFloat("AnimBlendFactor", &m_IdleRunBlendFactor, 0.f, 1.f);
		ImGui::SliderFloat("CharacterCameraZoom", &m_FollowCharacterCameraZoom, 0.01f, 1.f);
		ImGui::Checkbox("FreeCameraView", &m_FreeCameraViewModeActive);
		ImGui::Checkbox("Show Debug", &m_bShowDebug);
		ImGui::SliderFloat("TerrainUVScale", &m_TerrainUVScale, 0.01f, 1.f);
		});

	DrawSection("Directional Light", [this]() {
		ImGui::ColorEdit3("Sunlight Color", (float*)&m_sunLightColor);
		ImGui::SliderFloat("Sunlight Power", &m_SunLightPower, 0.1f, 10.f);
		ImGui::Checkbox("Show cascades", &m_ShowCasades);
		ImGui::Text("IBL");
		ImGui::SliderFloat("Sphere roughness", &_sphereRoughness, 0.01f, 1.f);
		ImGui::SliderFloat("Sphere metallness", &_sphereMetallness, 0.01f, 1.f);
		ImGui::ColorEdit3("Sphere color", (float*)&_sphereColor);
		if (ImGui::Button("Change IBL texture"))
		{
			ChangeHDRCubemap();
		}
		ImGui::SliderFloat("Cubemap LOD", &m_CubemapLod, 0.f, 4.f);
		ImGui::SliderFloat("Skybox intensity", &m_SkyboxIntensity, 0.f, 5.f);
		ImGui::SliderFloat("Cascade Split Lambda", &CascadeSplitLambda, 0.1f, 1.f);
		ImGui::SliderFloat("BiasModifier", &BiasModifier, 0.0f, 0.5f, "%.4f");
		});

	DrawSection("Misc", [this]() {
		ImGui::DragFloat("Camera FOV", &m_CameraFovDegrees, 1.f, 1.f, 180.f);
		ImGui::Checkbox("Show Grid", &m_ShowGrid);
		ImGui::Checkbox("Show Bounds", &m_ShowBounds);
		ImGui::Checkbox("Frustum Culling", &m_bFrustumCulling);
		});

	ImGui::End();

	m_SceneHierarchyPanel->OnImGuiRender(disableInputsCondition);
	//if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
		//m_ContentBrowserPanel->OnImGuiRender(m_ViewportActive);

	ImGui::Begin("Statisitcs", nullptr, windowFlags);
	ImGui::Text(
		"FPS: %.4f\nFrame time: %.4f ms\nD3D11PointLightDepthPassTime: %.4f ms\nD3D11DirLightDepthPassTime: %.4f ms\nD3D11GbufferPassTime: %.4f ms\nD3D11FrameCompositingPassTime: %.4f ms\n"
		"DudesAnimationsUpdateTime: %.4f ms\nMousePickWithRaycastTime: %.4f ms\n"
		"ImguiFrameTime: %.4f ms\nImguiRenderTime: %.4f ms\n"
		"D3D11FullFrameTime: %.4f ms\n"
		"Viewport Size: %dx%d\nMeshes drawn: %d/%d",
		m_FPS, m_DeltaTime * 1000.f,
		m_D3D11PointLightDepthPassTime * 1000.f, m_D3D11DirLightDepthPassTime * 1000.f, m_D3D11GbufferPassTime * 1000.f, m_D3D11FrameCompoistingPassTime * 1000.f,
		m_DudesAnimationsUpdateTime * 1000.f, m_MousePickWithRaycastTime * 1000.f,
		m_ImguiFrameTime * 1000.f, m_ImguiRenderTime * 1000.f,
		m_D3D11FullFrameTime * 1000.f,
		texWidth, texHeight, m_DrawInfo.meshesDrawn, m_DrawInfo.totalMeshes);
	ImGui::Text("PosViewSpaceZ: %.2f", posViewSpaceZ);
	ImGui::End();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("Viewport", nullptr, windowFlags);

	ImVec2 vMin = ImGui::GetWindowContentRegionMin();
	ImVec2 vMax = ImGui::GetWindowContentRegionMax();

	vMin.x += ImGui::GetWindowPos().x;
	vMin.y += ImGui::GetWindowPos().y;
	vMax.x += ImGui::GetWindowPos().x;
	vMax.y += ImGui::GetWindowPos().y;

	vRegionMinX = vMin.x;
	vRegionMinY = vMin.y;

	//_CursorPos = ImGui::GetCursorPos();
	_ViewportRelativeCursorPos.x = ImGui::GetMousePos().x - vRegionMinX;
	_ViewportRelativeCursorPos.y = ImGui::GetMousePos().y - vRegionMinY;

	texWidth = vMax.x - vMin.x;
	texHeight = vMax.y - vMin.y;
	if (texWidth != texWidthPrev || texHeight != texHeightPrev)
	{
		if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
		{
			if (texWidth > 0 && texHeight > 0)
			{
				PrepareGbufferRenderTargets(texWidth, texHeight);
				FramebufferToTextureInit();
				PostprocessingToTextureInit();
				//NvidiaShadowLibOnResizeWindow();
				BloomInit();
				OutlinePrepassInit();
			}
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
		{
			auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
			if (texWidth > 0 && texHeight > 0)
			{
				dx12Api->_ImageWidth = texWidth;
				dx12Api->_ImageHeight = texHeight;
				dx12Api->_ImageSizeChanged = true;
			}
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			m_RenderTextureOGL->Resize(texWidth, texHeight);
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			if (texWidth > 0 && texHeight > 0)
			{
				auto vkApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();
				vkApi->ResizeDrawImage(texWidth, texHeight);
				ImGui_ImplVulkan_RemoveTexture(_imguiDS);
				_imguiDS = ImGui_ImplVulkan_AddTexture(_imguiSampler, vkApi->_imguiImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}
		texWidthPrev = texWidth;
		texHeightPrev = texHeight;
	}
	if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
	{
		ImGui::Image((ImTextureID)(intptr_t)postprocessingSRV, ImVec2(texWidth, texHeight));
		//ImGui::Image((ImTextureID)(intptr_t)frameSRV, ImVec2(texWidth, texHeight));
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
	{
		//auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
		//ImGui::Image((ImTextureID)dx12Api->_renderToTextureSRVgpuHandle.ptr, ImVec2(texWidth, texHeight));
		//auto descriptor = _Gbuffer->GetGPUDescriptor(0);
		auto descriptor = _RenderTexture->GetGPUDescriptor();
		ImGui::Image((ImTextureID)descriptor.ptr, ImVec2(texWidth, texHeight));
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
	{
		ImGui::Image((ImTextureID)_imguiDS, ImVec2(texWidth, texHeight), ImVec2(0, 0), ImVec2(1, -1));
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
	{
		ImGui::Image((ImTextureID)m_RenderTextureOGL->GetTexture(), ImVec2(texWidth, texHeight), ImVec2(0, 0), ImVec2(1, -1));
	}

	if (ImGui::BeginDragDropTarget())
	{
		auto payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM");
		if (payload)
		{
			std::string fileStr = (char*)payload->Data;
			auto dotPos = fileStr.find_last_of('.');
			if (dotPos != std::string::npos)
			{
				std::string fileExtension = fileStr.substr(dotPos + 1);
				//std::cout << "EXTENSION: " << fileExtension << "\n";
				if (fileExtension == "obj" || fileExtension == "fbx" || fileExtension == "gltf" || fileExtension == "glb")
				{
					auto newModelEntitiy = m_Scene->CreateEntity("NewModel");
					newModelEntitiy.AddComponent<MeshComponent>(fileStr.c_str(), "", "", "", "", false);
					auto& meshComp = newModelEntitiy.GetComponent<MeshComponent>();
					//OV_INFO("Model loading started: {}", fileStr.c_str());
					std::thread newModelThread([&]() {
						meshComp.Load();
						});
					newModelThread.detach();
				}
				else
					MessageBoxA(((WindowsWindow*)m_Window.get())->GetHWND(), "Unsupported model format!", "Mesh loading error", MB_OK);
			}
		}

		ImGui::EndDragDropTarget();
	}

	ImguiDrawGizmo();

	m_IsViewportWindowFocused = ImGui::IsWindowFocused();

	ImGui::End();
	ImGui::PopStyleVar();

	ImGui::Begin("Node Editor");
	ed::SetCurrentEditor(m_EdContext);
	ed::Begin("Node Editor", ImVec2(0.0, 0.0f));
	int uniqueId = 1;
	// Start drawing nodes.
	ed::BeginNode(uniqueId++);
	ImGui::Text("Node A");
	ed::BeginPin(uniqueId++, ed::PinKind::Input);
	ImGui::Text("-> In");
	ed::EndPin();
	ImGui::SameLine();
	ed::BeginPin(uniqueId++, ed::PinKind::Output);
	ImGui::Text("Out ->");
	ed::EndPin();
	ed::EndNode();
	ed::End();
	ed::SetCurrentEditor(nullptr);
	ImGui::End();
}

void TestApplication::ImguiRender()
{
	// Rendering
	// (Your code clears your framebuffer, renders your other stuff etc.)
	ImGui::Render();
	if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
	{
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
	{
		auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dx12Api->GetCommandList().Get());
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
	{
		auto vulkanApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vulkanApi->GetCurrentFrame()._mainCommandBuffer);
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
	{
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	// (Your code calls swapchain's Present() function)
}

void TestApplication::ImguiCleanup()
{
	if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
		ImGui_ImplDX11_Shutdown();
	else if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
		ImGui_ImplDX12_Shutdown();
	else if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
	{
		auto vulkanApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();
		vkDeviceWaitIdle(vulkanApi->_device);
		ImGui_ImplVulkan_RemoveTexture(_imguiDS);
		vkDestroySampler(vulkanApi->_device, _imguiSampler, nullptr);
		ImGui_ImplVulkan_Shutdown();
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
	{
		ImGui_ImplOpenGL3_Shutdown();
	}
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

static ImVec4 ConvertImguiColorToGrayscale(ImVec4 color)
{
	float Y = 0.299f * color.x + 0.587f * color.y + 0.114f * color.z;
	return ImVec4(Y, Y, Y, color.w);
}

void TestApplication::SetupImGuiStyle()
{
	// Unreal style by dev0-1 from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.6000000238418579f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.WindowRounding = 0.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(32.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Left;
	style.ChildRounding = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 0.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(4.0f, 3.0f);
	style.FrameRounding = 0.0f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(8.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 14.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 0.0f;
	style.TabRounding = 4.0f;
	style.TabBorderSize = 0.0f;
	style.TabCloseButtonMinWidthSelected = 0.0f;
	style.TabCloseButtonMinWidthUnselected = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05882352963089943f, 0.05882352963089943f, 0.05882352963089943f, 0.9399999976158142f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.9399999976158142f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2000000029802322f, 0.2078431397676468f, 0.2196078449487686f, 0.5400000214576721f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4000000059604645f, 0.4000000059604645f, 0.4000000059604645f, 0.4000000059604645f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.1764705926179886f, 0.1764705926179886f, 0.1764705926179886f, 0.6700000166893005f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.03921568766236305f, 0.03921568766236305f, 0.03921568766236305f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2862745225429535f, 0.2862745225429535f, 0.2862745225429535f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.5099999904632568f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1372549086809158f, 0.1372549086809158f, 0.1372549086809158f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.5299999713897705f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.9372549057006836f, 0.9372549057006836f, 0.9372549057006836f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.8588235378265381f, 0.8588235378265381f, 0.8588235378265381f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.4392156898975372f, 0.4392156898975372f, 0.4392156898975372f, 0.4000000059604645f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4588235318660736f, 0.4666666686534882f, 0.47843137383461f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.4196078479290009f, 0.4196078479290009f, 0.4196078479290009f, 1.0f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.6980392336845398f, 0.6980392336845398f, 0.6980392336845398f, 0.3100000023841858f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.6980392336845398f, 0.6980392336845398f, 0.6980392336845398f, 0.800000011920929f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.47843137383461f, 0.4980392158031464f, 0.5176470875740051f, 1.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.7176470756530762f, 0.7176470756530762f, 0.7176470756530762f, 0.7799999713897705f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.9098039269447327f, 0.9098039269447327f, 0.9098039269447327f, 0.25f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.8078431487083435f, 0.8078431487083435f, 0.8078431487083435f, 0.6700000166893005f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.4588235318660736f, 0.4588235318660736f, 0.4588235318660736f, 0.949999988079071f);

	//style.Colors[ImGuiCol_Tab] = ImVec4(0.1764705926179886f, 0.3490196168422699f, 0.5764706134796143f, 0.8619999885559082f);
	//style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
	//style.Colors[ImGuiCol_TabActive] = ImVec4(0.196078434586525f, 0.407843142747879f, 0.6784313917160034f, 1.0f);
	//style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428f, 0.1019607856869698f, 0.1450980454683304f, 0.9724000096321106f);
	//style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 1.0f);

	style.Colors[ImGuiCol_Tab] = ConvertImguiColorToGrayscale(ImVec4(0.1764705926179886f, 0.3490196168422699f, 0.5764706134796143f, 0.8619999885559082f));
	style.Colors[ImGuiCol_TabHovered] = ConvertImguiColorToGrayscale(ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f));
	style.Colors[ImGuiCol_TabActive] = ConvertImguiColorToGrayscale(ImVec4(0.196078434586525f, 0.407843142747879f, 0.6784313917160034f, 1.0f));
	style.Colors[ImGuiCol_TabUnfocused] = ConvertImguiColorToGrayscale(ImVec4(0.06666667014360428f, 0.1019607856869698f, 0.1450980454683304f, 0.9724000096321106f));
	style.Colors[ImGuiCol_TabUnfocusedActive] = ConvertImguiColorToGrayscale(ImVec4(0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 1.0f));

	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.729411780834198f, 0.6000000238418579f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.8666666746139526f, 0.8666666746139526f, 0.8666666746139526f, 0.3499999940395355f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.6000000238418579f, 0.6000000238418579f, 0.6000000238418579f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
}

void TestApplication::SetupImguiDockspace(bool disableInputs)
{
	int windowFlags = ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoNavFocus;

	if (disableInputs)
		windowFlags |= ImGuiWindowFlags_NoInputs;
	else
		windowFlags &= ~ImGuiWindowFlags_NoInputs;

	ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_::ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(m_Window->GetWidth(), m_Window->GetHeight()), ImGuiCond_::ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_WindowRounding, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_WindowBorderSize, 0.f);

	bool dockspace_open = true;
	ImGui::Begin("Dockspace", &dockspace_open, windowFlags);

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Create"))
			{
			}
			if (ImGui::MenuItem("Open", "Ctrl+O"))
			{
				OpenScene();
			}

			if (ImGui::MenuItem("Save", "Ctrl+S", nullptr, !m_EditorScenePath.empty()))
			{
				SaveScene();
			}

			if (ImGui::MenuItem("Save as.."))
			{
				SaveSceneAs();
			}

			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	ImGui::PopStyleVar(2);

	ImGui::DockSpace(ImGui::GetID("Dockspace"));

	ImGui::End();
}

void TestApplication::RenderScene(glm::mat4 viewMatrix, glm::mat4 projMatrix, std::shared_ptr<DX11VertexShader>& vertexShader,
	std::shared_ptr<DX11PixelShader>& pixelShader, std::shared_ptr<DX11GeometryShader>& geometryShader, bool frustumCulling, bool pointLightCulling, const PointLightCullingData& pointLightCullingData, const std::shared_ptr<OGLShader>& oglShader)
{
	if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
	{
		if (vertexShader)
			vertexShader->Bind();
		if (pixelShader)
			pixelShader->Bind();
		if (geometryShader)
			geometryShader->Bind();
	}

	int i = 0;

	auto meshEnts = m_Scene->GetAllEntitiesWith<MeshComponent>();
	m_DrawInfo = {};

	const glm::mat4& VP = projMatrix * viewMatrix;

	auto& playerModel = m_Scene->TryGetEntityByTag("Player").GetComponent<MeshComponent>().m_Model;

	for (auto meshEnt : meshEnts)
	{
		Entity entity = { meshEnt, m_Scene.get() };
		auto& transformComponent = m_Scene->GetWorldSpaceTransform(entity);
		auto& meshTransform = m_Scene->GetWorldSpaceTransformMatrix(entity);
		auto& meshComponent = m_Scene->GetRegistry().get<MeshComponent>(meshEnt);
		auto& model = meshComponent.m_Model;
		if (!model || !model->IsLoaded())
			continue;

		MatricesCB mtxCb = {};

		mtxCb.modelMat = meshTransform;

		auto& tag = m_Scene->GetRegistry().get<TagComponent>(meshEnt).Tag;

		if (tag == "Sponza")
			sponzaModelMat = mtxCb.modelMat;
		else if (tag == "Player")
		{
			playerModelMat = mtxCb.modelMat;
		}
		else if (tag == "Colt")           /// attaching pistol to the right hand
		{
			if (playerModel && PLAYER_MODEL == 0)
			{
				auto& boneInfoMap = playerModel->GetBoneInfoMap();
				auto it = boneInfoMap.find("mixamorig:RightHand");
				if (it == boneInfoMap.end()) {
					std::cout << "Bone not found!\n";
					m_Running = false;
				}
				else {
					auto characterMat = playerModelMat;

					auto gunPos = glm::vec3(4.5f, 10.5f, 1.225f);
					auto translate = gunPos;
					auto rotate = glm::vec3(0.f, 90.f, 0.f);
					auto scale = transformComponent.Scale;
					auto modelMat = glm::mat4(1.f);
					modelMat = glm::translate(modelMat, glm::vec3(translate[0], translate[1], translate[2]));
					modelMat = glm::rotate(modelMat, glm::radians(rotate[0]), glm::vec3(1.f, 0.f, 0.f));
					modelMat = glm::rotate(modelMat, glm::radians(rotate[1]), glm::vec3(0.f, 1.f, 0.f));
					modelMat = glm::rotate(modelMat, glm::radians(rotate[2]), glm::vec3(0.f, 0.f, 1.f));
					modelMat = glm::scale(modelMat, glm::vec3(scale[0], scale[1], scale[2]));
					modelMat = glm::scale(modelMat, glm::vec3(15.f, 15.f, 15.f));

					BoneInfo value = it->second;
					glm::mat4 boneMat = value.offset;

					auto childFinalBoneMatrix = m_Animator->GetFinalBoneMatrices()[value.id];

					glm::mat4 finalBoneMatrix = childFinalBoneMatrix * glm::inverse(boneMat);

					mtxCb.modelMat = characterMat * finalBoneMatrix * modelMat;
					coltMat = mtxCb.modelMat;
				}
			}
			else
			{
				coltMat = mtxCb.modelMat;
			}
		}

		mtxCb.viewMat = viewMatrix;
		mtxCb.projMat = projMatrix;
		mtxCb.normalMat = glm::transpose(glm::inverse(mtxCb.modelMat));
		mtxCb.viewMatInv = glm::inverse(mtxCb.viewMat);
		mtxCb.projMatInv = glm::inverse(mtxCb.projMat);

		MaterialCB matCb = {};
		matCb.albedoOnly = 0;
		if (tag == "Sponza")
			matCb.sponza = 1;
		else
			matCb.sponza = 0;
		matCb.objectId = (uint32_t)meshEnt;
		matCb.emission = meshComponent.Emission;
		matCb.color = meshComponent.Color;
		matCb.roughness = meshComponent.Roughness;
		matCb.metallic = meshComponent.Metallic;
		matCb.terrain = 0;
		matCb.cubemapLod = 0.f;
		matCb.objectOutlinePass = 0;
		matCb.useNormalMap = meshComponent.UseNormalMap ? 1 : 0;
		matCb.invertNormalG = meshComponent.InvertNormalG ? 1 : 0;

		SkeletalAnimationCB saCb = {};
		std::vector<glm::mat4> finalBoneMatrices;
		if (tag == "Player" && m_Animator)
		{
			finalBoneMatrices = m_Animator->GetFinalBoneMatrices();
		}
		else if (tag == "Zombie" && m_ZombieAnimator)
		{
			finalBoneMatrices = m_ZombieAnimator->GetFinalBoneMatrices();
		}
		else if (tag.compare(0, 4, "Dude") == 0)
		{
			finalBoneMatrices = m_DudeAnimators[0]->GetFinalBoneMatrices();
		}
		else
		{
			finalBoneMatrices.resize(MAX_BONES);
			for (int j = 0; j < MAX_BONES; j++)
			{
				finalBoneMatrices[j] = glm::mat4(1.f);
			}
		}
		unsigned fbmSize = finalBoneMatrices.size();
		for (int j = 0; j < fbmSize; j++)
		{
			saCb.finalBonesMatrices[j] = finalBoneMatrices[j];
		}

		if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
		{
			m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, NULL, &mtxCb, 0, 0);
			m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, NULL, &matCb, 0, 0);
			m_DX11DeviceContext->UpdateSubresource(g_pCBSkeletalAnimation, 0, NULL, &saCb, 0, 0);

			m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);
			m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);

			m_DX11DeviceContext->VSSetConstantBuffers(1, 1, &g_pCBMaterial);
			m_DX11DeviceContext->PSSetConstantBuffers(1, 1, &g_pCBMaterial);

			m_DX11DeviceContext->VSSetConstantBuffers(2, 1, &g_pCBSkeletalAnimation);
			m_DX11DeviceContext->PSSetConstantBuffers(2, 1, &g_pCBSkeletalAnimation);

			m_DX11DeviceContext->VSSetConstantBuffers(3, 1, &g_pCBLight);
			m_DX11DeviceContext->PSSetConstantBuffers(3, 1, &g_pCBLight);

			m_DX11DeviceContext->PSSetSamplers(0, 1, &g_pSamplerLinearWrap);
			m_DX11DeviceContext->PSSetSamplers(1, 1, &g_pSamplerPointClamp);
			m_DX11DeviceContext->PSSetSamplers(2, 1, &m_SamplerLinearClampComparison);
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
		{
			auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();

			auto albedoGpuHandle = ((DX12Texture2D*)model->m_pTextureAlbedo.get())->GetGPUHandle();
			matCb.texturesBindlessIndexStart = dx12Api->g_pd3dSrvDescHeapAlloc.GetDescriptorIndex(albedoGpuHandle);

			_CurrFrameResource->_MatricesCB->CopyData(i, mtxCb);
			_CurrFrameResource->_MaterialCB->CopyData(i, matCb);
			_CurrFrameResource->_SkeletalAnimationCB->CopyData(i, saCb);


			auto& commandList = dx12Api->GetCommandList();

			commandList->SetGraphicsRootDescriptorTable(0, _CurrFrameResource->_MatricesCBgpuHandles[i]);
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			auto vkApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();

			VkCommandBuffer cmd = vkApi->GetCurrentFrame()._mainCommandBuffer;

			SkeletalAnimationBuffer saBuffer{};
			for (int j = 0; j < fbmSize; j++)
			{
				saBuffer.finalBonesMatrices[j] = saCb.finalBonesMatrices[j];
			}
			memcpy(_saBuffers[i].info.pMappedData, &saBuffer, sizeof(saBuffer));

			MaterialBuffer matBuffer{};
			matBuffer.color = matCb.color;
			matBuffer.metalRough = glm::vec2(matCb.metallic, matCb.roughness);
			matBuffer.sponza = matCb.sponza;
			matBuffer.notTextured = matCb.notTextured;
			matBuffer.useNormalMap = matCb.useNormalMap;
			memcpy(_matBuffers[i].info.pMappedData, &matBuffer, sizeof(matBuffer));

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkApi->_meshPipelineLayout, 0, 1, &_saAndMatBufferSets[i], 0, nullptr);
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			oglShader->SetMat4("model", mtxCb.modelMat);
			oglShader->SetMat3("normalMatrix", mtxCb.normalMat);
			for (int j = 0; j < fbmSize; j++)
			{
				oglShader->SetMat4("finalBonesMatrices[" + std::to_string(j) + "]", saCb.finalBonesMatrices[j]);
			}
			oglShader->SetInt("sponza", matCb.sponza);
			oglShader->SetFloat("skyboxIntensity", m_SkyboxIntensity);
			oglShader->SetFloat3("matColor", matCb.color);
			oglShader->SetFloat2("matMetalRough", glm::vec2(matCb.metallic, matCb.roughness));
			oglShader->SetInt("matUseNormalMap", matCb.useNormalMap);
		}

		glm::mat4 proj = mtxCb.projMat;
		//proj[1][1] *= -1;
		glm::mat4 transform = proj * mtxCb.viewMat * mtxCb.modelMat;

		//if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
		//{
		//	if (m_SceneHierarchyPanel->GetSelectedEntity() == entity)
		//		m_DX11DeviceContext->OMSetDepthStencilState(m_OutlineStencilWrite.Get(), 1);
		//}

		DrawInfo drawInfo;
		if (frustumCulling && tag != "Player")
		{
			drawInfo = model->Draw(VP * mtxCb.modelMat, true, mtxCb.modelMat, pointLightCulling, pointLightCullingData);
		}
		else
		{
			drawInfo = model->Draw(VP * mtxCb.modelMat, false, mtxCb.modelMat, pointLightCulling, pointLightCullingData);
		}

		//if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
		//{
		//	m_DX11DeviceContext->OMSetDepthStencilState(m_OutlineStencilNormal.Get(), 1);
		//}


		m_DrawInfo.meshesDrawn += drawInfo.meshesDrawn;
		m_DrawInfo.totalMeshes += drawInfo.totalMeshes;

		i++;
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

	result = m_FmodSystem->createSound("../resources/audio/Tokarev_Fire0.wav", FMOD_CREATESAMPLE, nullptr, &m_RevolverShot);
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		exit(-1);
	}

	result = m_FmodSystem->createSound("../resources/audio/Tokarev_Fire1.wav", FMOD_CREATESAMPLE, nullptr, &m_RevolverShot1);
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		exit(-1);
	}

	result = m_FmodSystem->createSound("../resources/audio/Tokarev_Fire2.wav", FMOD_CREATESAMPLE, nullptr, &m_RevolverShot2);
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		exit(-1);
	}

	result = m_FmodSystem->createSound("../resources/audio/Tokarev_Fire3.wav", FMOD_CREATESAMPLE, nullptr, &m_RevolverShot3);
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		exit(-1);
	}

	result = m_FmodSystem->createSound("../resources/audio/terror_ambience.mp3", FMOD_CREATESAMPLE | FMOD_LOOP_NORMAL, nullptr, &m_HorroAmbience);
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		exit(-1);
	}

	result = m_FmodSystem->createSound("../resources/audio/Resident Evil 4 Save Room Theme Song (Trap Remix).mp3", FMOD_CREATESAMPLE | FMOD_LOOP_NORMAL, nullptr, &m_RE4SaveRoomTrapRemix);
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		exit(-1);
	}
	//m_FmodSystem->playSound(m_HorroAmbience, nullptr, false, &channel);
	//m_RE4SaveRoomTrapRemix->setLoopCount(-1);
	//m_FmodSystem->playSound(m_RE4SaveRoomTrapRemix, nullptr, false, &m_PlayingChannel);
	//channel->setLoopCount(-1);
	//m_FmodSystem->playSound(m_ZombieGrowl, nullptr, false, &m_PlayingChannel);
}

void TestApplication::FMODUpdate()
{
	m_FmodSystem->update();
}

void TestApplication::FMODCleanup()
{
	m_RevolverShot->release();
	m_RevolverShot1->release();
	m_RevolverShot2->release();
	m_RevolverShot3->release();
	m_HorroAmbience->release();
	m_FmodSystem->release();
}

bool TestApplication::RayIntersectsMeshTriangles(const std::string& modelName, const glm::vec3& orig, const glm::vec3& dir, glm::vec3& hitPoint, float& distance)
{
	std::set<float> distances;

	Entity meshEntity = m_Scene->TryGetEntityByTag(modelName);
	if (!meshEntity)
		return false;
	auto model = meshEntity.GetComponent<MeshComponent>().m_Model.get();
	const auto& transform = m_Scene->GetWorldSpaceTransformMatrix(meshEntity);
	for (auto& mesh : model->GetMeshes())
	{
		const auto& vertices = mesh.GetVertices();
		const auto& indices = mesh.GetIndices();

		// 3. Iterate through the indices to access triangles
		for (size_t i = 0; i < indices.size(); i += 3) {
			// Get the indices for the current triangle
			unsigned int index0 = indices[i];
			unsigned int index1 = indices[i + 1];
			unsigned int index2 = indices[i + 2];

			// Get the actual vertex data using these indices
			const Vertex& v0 = vertices[index0];
			const Vertex& v1 = vertices[index1];
			const Vertex& v2 = vertices[index2];

			glm::vec3 p0 = glm::vec3(transform * glm::vec4(v0.Pos, 1.f));
			glm::vec3 p1 = glm::vec3(transform * glm::vec4(v1.Pos, 1.f));
			glm::vec3 p2 = glm::vec3(transform * glm::vec4(v2.Pos, 1.f));

			glm::vec3 hitPoint; // Stores u, v, w barycentric coordinates of the hit point
			float distance;
			//bool hit = glm::intersectRayTriangle(orig, dir, p0, p1, p2, barycentricCoords, distance);
			bool hit = Intersections::intersectRayTriangle(orig, dir, p0, p1, p2, distance, hitPoint);
			if (hit)
			{
				distances.insert(distance);
			}
		}
	}

	if (distances.empty())
		return false;

	float closestDistance = *distances.begin();
	hitPoint = orig + dir * closestDistance;
	distance = closestDistance;

	//std::cout << "Hitpoint found: " << hitPoint.x << ", " << hitPoint.y << ", " << hitPoint.z << "\n";

	return true;
}

void TestApplication::DrawRaycast()
{
	for (const auto& ray : rays)
	{
		glm::vec3 rayStart = ray.start;
		glm::vec3 rayEnd = ray.end;

		ID3D11Buffer* pRayVertexBuffer, * pRayIndexBuffer;

		Vertex vertices[] =
		{
			{ rayStart, glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
			{ rayEnd, glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		};

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(Vertex) * 2;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = vertices;

		HRESULT hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pRayVertexBuffer);
		if (FAILED(hr)) return;

		UINT indices[] =
		{
			0, 1
		};

		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(UINT) * 2;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;

		InitData.pSysMem = indices;

		hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pRayIndexBuffer);
		if (FAILED(hr)) return;

		MatricesCB cb{};
		cb.modelMat = glm::mat4(1.f);
		cb.viewMat = m_CameraViewMatrix;
		cb.projMat = m_CameraProjMatrix;
		cb.normalMat = glm::transpose(glm::inverse(cb.modelMat));
		cb.viewMatInv = glm::inverse(cb.viewMat);
		cb.projMatInv = glm::inverse(cb.projMat);

		m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, nullptr, &cb, 0, 0);
		m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);

		MaterialCB matCb{};
		matCb.sponza = 0;
		matCb.objectId = INVALID_ENTITY_ID;
		matCb.terrain = 0;
		matCb.albedoOnly = 1;
		matCb.notTextured = 1;
		matCb.color = glm::vec3(1.f, 0.f, 0.f);
		matCb.roughness = 1.f;
		matCb.metallic = 0.f;
		matCb.objectOutlinePass = 0;
		matCb.emission = 0.0f;

		m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, nullptr, &matCb, 0, 0);
		m_DX11DeviceContext->PSSetConstantBuffers(1, 1, &g_pCBMaterial);

		UINT stride = sizeof(Vertex);
		UINT offset = 0;

		m_DX11DeviceContext->IASetVertexBuffers(0, 1, &pRayVertexBuffer, &stride, &offset);
		m_DX11DeviceContext->IASetIndexBuffer(pRayIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		m_DX11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

		m_DX11DeviceContext->DrawIndexed(2, 0, 0);

		pRayVertexBuffer->Release();
		pRayIndexBuffer->Release();
	}
}

void TestApplication::OutlinePrepassInit()
{
	if (m_OutlineRenderTexture)
		m_OutlineRenderTexture.reset();

	m_OutlineRenderTexture = std::make_unique<RenderTexture>();
	m_OutlineRenderTexture->Initialize(texWidth, texHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R16G16B16A16_FLOAT, false, false, 0);
}

void TestApplication::OutlinePrepassBegin()
{
	DX11RendererAPI::GetInstance()->SetViewport(0, 0, texWidth, texHeight);
	m_OutlineRenderTexture->SetRenderTarget(false, true, false, nullptr);
	m_OutlineRenderTexture->ClearRenderTarget(0.0f, 0.0f, 0.0f, 1.0f, 1.f);

	m_DefaultVertexShader->Bind();
	m_SolidColorPixelShader->Bind();
	DrawSelectedModelOutline();
}

void TestApplication::OutlinePrepassEnd()
{
	//SetDefaultRenderTargetAndViewport();
}

void TestApplication::MoveCharacter()
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
		yaw += Xoffset * 0.1f;
		pitch -= Yoffset * 0.1f;
	}

	if (pitch >= 60.f)
		pitch = 60.f;
	else if (pitch <= -30.f)
		pitch = -30.f;
	if (yaw <= -360.f || yaw >= 360.f)
		yaw = 0.f;
	//yaw = glm::mod(yaw, 360.f);
	//std::cout << "Yaw: " << yaw << '\n';
	//std::cout << "Pitch: " << pitch << '\n';

	auto& playerEntity = m_Scene->TryGetEntityByTag("Player");
	auto& playerGlobalTransform = m_Scene->GetWorldSpaceTransform(playerEntity);
	auto& playerGlobalTransforMatrix = m_Scene->GetWorldSpaceTransformMatrix(playerEntity);
	auto& playerTransform = playerEntity.GetComponent<TransformComponent>();
	auto& model = playerEntity.GetComponent<MeshComponent>().m_Model;

	auto quatPitch = glm::quat(glm::vec3(glm::radians(pitch), 0.f, 0.f));
	auto quat = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), 0.f));
	auto quatYaw = glm::quat(glm::vec3(0.f, glm::radians(yaw), 0.f));
	auto dirYaw = quatYaw * glm::vec3(0.f, 0.f, 5.f);
	auto up = glm::vec3(0.f, 1.f, 0.f);

	auto headPos = playerGlobalTransform.Translation + glm::vec3(0.f, m_PlayerHeight, 0.f);
	auto viewTarget = headPos - glm::normalize(glm::cross(dirYaw, up)) * m_CameraXOffset;

	auto dir = quat * glm::vec3(0.f, 0.f, -m_CameraBoomLength * m_FollowCharacterCameraZoom);

	eye = viewTarget + dir;

	//glm::vec3 hitPoint;
	//float distance;
	//if (RayIntersectsMeshTriangles("Sponza", viewTarget, glm::normalize(quat * glm::vec3(0.f, 0.f, -1.f)), hitPoint, distance))
	//{
	//	//rays.push_back({ viewTarget, hitPoint });
	//	if (distance <= glm::length(dir))
	//		eye = viewTarget + quat * glm::vec3(0.f, 0.f, -distance * 0.9f);
	//}

	//transform.rotation = quatYaw;

	float moveSpeed = 5.f;
	float rotSpeed = 1000.f * m_DeltaTime;

	glm::vec3 velocityVector = glm::vec3(0.f);
	float verticalAxis = 0.f;
	float horizontalAxis = 0.f;

	if (glfwGetKey(m_Window->GetGLFWwindow(), GLFW_KEY_W) == GLFW_RELEASE &&
		glfwGetKey(m_Window->GetGLFWwindow(), GLFW_KEY_S) == GLFW_RELEASE &&
		glfwGetKey(m_Window->GetGLFWwindow(), GLFW_KEY_A) == GLFW_RELEASE &&
		glfwGetKey(m_Window->GetGLFWwindow(), GLFW_KEY_D) == GLFW_RELEASE)
		bWalking = false;
	if (glfwGetKey(m_Window->GetGLFWwindow(), GLFW_KEY_W) == GLFW_PRESS)
	{
		verticalAxis = 1.f;
		bWalking = true;
	}
	if (glfwGetKey(m_Window->GetGLFWwindow(), GLFW_KEY_S) == GLFW_PRESS)
	{
		verticalAxis = -1.f;
		bWalking = true;
	}
	if (glfwGetKey(m_Window->GetGLFWwindow(), GLFW_KEY_A) == GLFW_PRESS)
	{
		if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
			horizontalAxis = 1.f;
		else
			horizontalAxis = -1.f;
		bWalking = true;
	}
	if (glfwGetKey(m_Window->GetGLFWwindow(), GLFW_KEY_D) == GLFW_PRESS)
	{
		if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
			horizontalAxis = -1.f;
		else
			horizontalAxis = 1.f;
		bWalking = true;
	}
	if (glfwGetMouseButton(m_Window->GetGLFWwindow(), GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
		rightMouseButtonPressing = true;
	else if (glfwGetMouseButton(m_Window->GetGLFWwindow(), GLFW_MOUSE_BUTTON_2) == GLFW_RELEASE)
		rightMouseButtonPressing = false;

	if (glfwGetMouseButton(m_Window->GetGLFWwindow(), GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
		leftMouseButtonPressing = true;
	else if (glfwGetMouseButton(m_Window->GetGLFWwindow(), GLFW_MOUSE_BUTTON_1) == GLFW_RELEASE)
		leftMouseButtonPressing = false;


	float v = verticalAxis;
	float h = horizontalAxis;

	float moveAmount = glm::clamp(glm::abs(h) + glm::abs(v));

	glm::quat targetRotation = transform.rotation;

	glm::vec3 moveInput = glm::vec3(h, 0.f, v);

	glm::vec3 direction = glm::vec3(0.f);

	auto planarDirection = glm::vec3(0.f, glm::radians(yaw), 0.f);
	if (glm::length(moveInput) > 0.f)
	{
		moveInput = glm::normalize(moveInput);
		auto moveDir = glm::normalize(glm::quat(planarDirection) * moveInput);

		if (moveAmount > 0.f)
			targetRotation = glm::quatLookAtLH(moveDir, glm::vec3(0.f, 1.f, 0.f));
	}

	if (rightMouseButtonPressing)
	{
		if (m_FollowCharacterCameraZoom > 0.2f)
			m_FollowCharacterCameraZoom -= m_DeltaTime * 4.f;
		auto direction = glm::vec3(0.f, glm::radians(yaw), 0.f);
		targetRotation = glm::quat(direction);
	}
	else
	{
		if (m_FollowCharacterCameraZoom < 0.7f)
			m_FollowCharacterCameraZoom += m_DeltaTime * 4.f;
	}

	if (targetRotation != transform.rotation)
	{
		transform.rotation = Math::QuaternionRotateTowards(transform.rotation, targetRotation, rotSpeed);
	}
	if (glm::length(moveInput) > 0.f && !rightMouseButtonPressing)
		direction = glm::normalize(transform.rotation * glm::vec3(0.f, 0.f, 1.f));




	const float m_ControlMovementInAir = true;

	if (m_Physics.m_Character->IsSupported())// || m_ControlMovementInAir)
	{
		m_Physics.m_CharacterDisplacement += m_PlayerMoveSpeed * Vec3(direction.x, direction.y, direction.z) * m_DeltaTime;
	}

	m_Physics.m_AllowCharacterSliding = !m_Physics.m_Character->IsSupported() || !m_Physics.m_CharacterDisplacement.IsNearZero();


	JPH::Vec3 m_DesiredVelocity = m_Physics.m_CharacterDisplacement / m_DeltaTime;

	JPH::Vec3 currentVerticalVelocity = JPH::Vec3(0, m_Physics.m_Character->GetLinearVelocity().GetY(), 0);
	JPH::Vec3 groundVelocity = m_Physics.m_Character->GetGroundVelocity();

	bool jumping = (currentVerticalVelocity.GetY() - groundVelocity.GetY()) >= 0.1f;

	JPH::Vec3 newVelocity;

	if (m_Physics.m_Character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround && !m_Physics.m_Character->IsSlopeTooSteep(m_Physics.m_Character->GetGroundNormal()))
	{
		// When grounded, acquire velocity of ground
		newVelocity = groundVelocity;

		// Jump
		if (m_JumpPower > 0.0f && !jumping)
		{
			newVelocity += JPH::Vec3(0, m_JumpPower, 0);
			m_JumpPower = 0.0f;
		}
	}
	else
	{
		newVelocity = currentVerticalVelocity;
	}

	// Add gravity
	newVelocity += m_Physics.m_Gravity * m_DeltaTime;

	if (m_Physics.m_Character->IsSupported())// || m_ControlMovementInAir)
	{
		newVelocity += m_DesiredVelocity;
	}
	else
	{
		// preserve current horizontal velocity
		JPH::Vec3 currentHorizontalVelocity = m_Physics.m_Character->GetLinearVelocity() - currentVerticalVelocity;
		newVelocity += currentHorizontalVelocity;
	}

	// Update the velocity
	m_Physics.m_Character->SetLinearVelocity(newVelocity);







	auto charPos = m_Physics.m_Character->GetPosition() + Vec3(0.f, -m_Physics.m_CharacterHeight, 0.f);

	transform.position = glm::vec3(charPos.GetX(), charPos.GetY(), charPos.GetZ());

	auto eulerRotation = glm::eulerAngles(transform.rotation);
	playerTransform.Rotation.x = glm::degrees(eulerRotation.x);
	playerTransform.Rotation.y = glm::degrees(eulerRotation.y);
	playerTransform.Rotation.z = glm::degrees(eulerRotation.z);
	playerTransform.Translation = transform.position;

	if (m_Animator)
	{
		float blendFactorMultiplier = 8.f;

		if (!bWalking)
		{
			if (bIsMovingUp)
			{
				if (m_IdleJumpBlendFactor < 1.f)
					m_IdleJumpBlendFactor += m_DeltaTime * blendFactorMultiplier;
				if (m_IdleJumpBlendFactor > 1.f)
					m_IdleJumpBlendFactor = 1.f;
			}
			else
			{
				if (m_IdleJumpBlendFactor > 0.f)
					m_IdleJumpBlendFactor -= m_DeltaTime * blendFactorMultiplier;
				if (m_IdleJumpBlendFactor < 0.f)
					m_IdleJumpBlendFactor = 0.f;

				if (m_IdleRunBlendFactor > 0.f)
					m_IdleRunBlendFactor -= m_DeltaTime * blendFactorMultiplier;
				if (m_IdleRunBlendFactor < 0.f)
					m_IdleRunBlendFactor = 0.f;
			}
		}
		else if (bWalking)
		{
			if (bIsMovingUp)
			{
				if (m_IdleJumpBlendFactor < 1.f)
					m_IdleJumpBlendFactor += m_DeltaTime * blendFactorMultiplier;
				if (m_IdleJumpBlendFactor > 1.f)
					m_IdleJumpBlendFactor = 1.f;
			}
			else
			{
				if (m_IdleJumpBlendFactor > 0.f)
					m_IdleJumpBlendFactor -= m_DeltaTime * blendFactorMultiplier;
				if (m_IdleJumpBlendFactor < 0.f)
					m_IdleJumpBlendFactor = 0.f;

				if (m_IdleRunBlendFactor < 1.f)
					m_IdleRunBlendFactor += m_DeltaTime * blendFactorMultiplier;
				if (m_IdleRunBlendFactor > 1.f)
					m_IdleRunBlendFactor = 1.f;
			}
		}

		aimPitch = glm::degrees(2 * XM_PI - glm::eulerAngles(quatPitch).x);

		glm::quat targetPitchAimRotation = aimPitchRotation;

		targetPitchAimRotation = quatPitch;

		aimPitchRotation = Math::QuaternionRotateTowards(aimPitchRotation, targetPitchAimRotation, rotSpeed);

		glm::mat4 boneTransform = glm::mat4(1.f);

#if PLAYER_MODEL == 0
		if (!rightMouseButtonPressing)
		{
			if (bIsMovingUp)
				m_Animator->BlendTwoAnimations(m_IdleAnimation, m_JumpingAnimation, m_IdleJumpBlendFactor, m_DeltaTime);
			else
				m_Animator->BlendTwoAnimations(m_IdleAnimation, m_RunAnimation, m_IdleRunBlendFactor, m_DeltaTime);
		}
		else
		{
			if (!leftMouseButtonPressed)
				m_Animator->BlendTwoAnimationsPerBone(m_PistolIdleAnimation, m_PistolIdleAnimation, "mixamorig:Spine1", m_DeltaTime, yaw, glm::degrees(glm::eulerAngles(aimPitchRotation).x), glm::axis(aimPitchRotation) * glm::quat(glm::vec3(0.f, glm::radians(-30.f), 0.f)), 0.f, boneTransform);
			else
				m_Animator->BlendTwoAnimationsPerBone(m_PistolIdleAnimation, m_ShootingAnimation, "mixamorig:Spine1", m_DeltaTime, yaw - 60.f, glm::degrees(glm::eulerAngles(aimPitchRotation).x), glm::axis(aimPitchRotation) * glm::quat(glm::vec3(0.f, glm::radians(-30.f), 0.f)), m_TimeElapsedAfterShot, boneTransform);
		}
#else
		if (!rightMouseButtonPressing)
		{
			m_Animator->BlendTwoAnimations(m_IdleAnimation, m_RunAnimation, m_IdleRunBlendFactor, m_DeltaTime);
		}

#endif

		if (model)
		{
			glm::mat4 finalCamMat = glm::mat4(1.f);
			glm::vec3 translation = glm::vec3(0.f);
			auto boneInfoMap = model->GetBoneInfoMap();
			auto it = boneInfoMap.find("mixamorig:Neck");
			if (it != boneInfoMap.end())
			{
				auto camPos = glm::vec3(0.f, 8.f, 0.f);
				//auto camPos = glm::vec3(0.f, -12.f, 0.f); second way
				auto translate = camPos;
				auto rotate = glm::vec3(0.f, 0.f, 0.f);
				auto modelMat = glm::mat4(1.f);
				modelMat = glm::translate(modelMat, glm::vec3(translate[0], translate[1], translate[2]));
				//modelMat = glm::translate(modelMat, glm::vec3(0.f, 0.f, 0.f));
				modelMat = glm::rotate(modelMat, glm::radians(rotate[0]), glm::vec3(1.f, 0.f, 0.f));
				modelMat = glm::rotate(modelMat, glm::radians(rotate[1]), glm::vec3(0.f, 1.f, 0.f));
				modelMat = glm::rotate(modelMat, glm::radians(rotate[2]), glm::vec3(0.f, 0.f, 1.f));

				BoneInfo value = it->second;
				glm::mat4 boneMat = value.offset;

				auto childFinalBoneMatrix = m_Animator->GetFinalBoneMatrices()[value.id];

				glm::mat4 finalBoneMatrix = childFinalBoneMatrix * glm::inverse(boneMat);

				finalCamMat = playerGlobalTransforMatrix * finalBoneMatrix * modelMat;

				glm::vec3 scale;
				glm::quat rotation;
				//glm::vec3 translation;
				glm::vec3 skew;
				glm::vec4 perspective;

				glm::decompose(finalCamMat, scale, rotation, translation, skew, perspective);
			}

			auto cameraEntity = m_Scene->TryGetEntityByTag("Camera");
			auto& camera = cameraEntity.GetComponent<CameraComponent>().m_Camera;

			if (!rightMouseButtonPressing)
			{
				camera.FollowCamera(eye, viewTarget);
			}
			else
			{
				camera.FollowCamera(translation + dir - glm::normalize(glm::cross(dirYaw, up)) * 31.5f * SPONZA_SCALE, translation + quat * glm::vec3(0.f, 0.f, 300.f));
			}
		}
	}
}

void TestApplication::MoveZombie()
{
	auto& zombieEntity = m_Scene->TryGetEntityByTag("Zombie");
	bool zombieEnitityValid = zombieEntity.IsValid();
	if (!zombieEnitityValid)
	{
		//if (pxZombieController)
		//{
		//	pxZombieController->release();
		//	pxZombieController = nullptr;
		//}
		SAFE_DELETE(m_ZombieIdleAnimation);
		SAFE_DELETE(m_ZombieWalkAnimation);
		SAFE_DELETE(m_ZombieAttackAnimation);
		SAFE_DELETE(m_ZombieHitAnimation);
		SAFE_DELETE(m_ZombieDeathAnimation);
		SAFE_DELETE(m_ZombieAnimator);
	}
	else if (zombieEnitityValid && m_ZombieAnimator)
	{
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;

		glm::decompose(coltMat, scale, rotation, translation, skew, perspective);

		glm::vec3 rayStart = translation;
		glm::vec3 rayDir = glm::normalize(translation + rotation * glm::vec3(0.f, 10.f, 0.f));

		auto& playerEntity = m_Scene->TryGetEntityByTag("Player");
		auto& zombieGlobalTransform = m_Scene->GetWorldSpaceTransform(zombieEntity);
		auto& zombieTransform = zombieEntity.GetComponent<TransformComponent>();
		auto& playerGlobalTransform = m_Scene->GetWorldSpaceTransform(playerEntity);

		auto sphereRadius = 4.f;
		auto sphereCenter = zombieGlobalTransform.Translation + glm::vec3(0.f, m_ZombieHeight, 0.f);

		auto intersects = RaySphereIntersection(rayStart, rayDir, 500.f, sphereCenter, sphereRadius);

		float distanceBetweenZombieAndPlayer = glm::length(playerGlobalTransform.Translation - zombieGlobalTransform.Translation);

		if (!b_ZombieWalking)
		{
			if (m_ZombieIdleWalkBlendFactor > 0.f)
				m_ZombieIdleWalkBlendFactor -= m_DeltaTime * 8.f;
			if (m_ZombieIdleWalkBlendFactor < 0.f)
				m_ZombieIdleWalkBlendFactor = 0.f;
		}
		else if (b_ZombieWalking)
		{
			if (m_ZombieIdleWalkBlendFactor < 1.f)
				m_ZombieIdleWalkBlendFactor += m_DeltaTime * 8.f;
			if (m_ZombieIdleWalkBlendFactor > 1.f)
				m_ZombieIdleWalkBlendFactor = 1.f;
		}

		if (!b_ZombieGettingHit)
		{
			if (m_ZombieIdleHitBlendFactor > 0.f)
				m_ZombieIdleHitBlendFactor -= m_DeltaTime * 8.f;
			if (m_ZombieIdleHitBlendFactor < 0.f)
			{
				m_ZombieIdleHitBlendFactor = 0.f;
			}

		}
		else if (b_ZombieGettingHit)
		{
			if (m_ZombieIdleHitBlendFactor < 1.f)
				m_ZombieIdleHitBlendFactor += m_DeltaTime * 8.f;
			if (m_ZombieIdleHitBlendFactor > 1.f)
			{
				m_ZombieIdleHitBlendFactor = 1.f;
			}
		}

		if (!b_ZombieAttacking)
		{
			if (m_ZombieIdleAttackBlendFactor > 0.f)
				m_ZombieIdleAttackBlendFactor -= m_DeltaTime * 8.f;
			if (m_ZombieIdleAttackBlendFactor < 0.f)
			{
				m_ZombieIdleAttackBlendFactor = 0.f;
			}
		}
		else if (b_ZombieAttacking)
		{
			if (m_ZombieIdleAttackBlendFactor < 1.f)
				m_ZombieIdleAttackBlendFactor += m_DeltaTime * 8.f;
			if (m_ZombieIdleAttackBlendFactor > 1.f)
			{
				m_ZombieIdleAttackBlendFactor = 1.f;
			}
		}

		if (!b_ZombieDying)
		{
			if (m_ZombieIdleDeathBlendFactor > 0.f)
				m_ZombieIdleDeathBlendFactor -= m_DeltaTime * 8.f;
			if (m_ZombieIdleDeathBlendFactor < 0.f)
			{
				m_ZombieIdleDeathBlendFactor = 0.f;
			}

		}
		else if (b_ZombieDying)
		{
			if (m_ZombieIdleDeathBlendFactor < 1.f)
				m_ZombieIdleDeathBlendFactor += m_DeltaTime * 8.f;
			if (m_ZombieIdleDeathBlendFactor > 1.f)
			{
				m_ZombieIdleDeathBlendFactor = 1.f;
			}
		}

		static bool playerShot = false;

		if (intersects && m_TimeElapsedAfterShot <= 0.1f && !playerShot)
		{
			m_ZombieCurrentState = ZombieState::HIT;
			b_ZombieGettingHit = true;
			b_ZombieWalking = false;
			b_ZombieAttacking = false;
			b_ZombieDying = false;
			m_ZombieHealth -= 20.f;
			playerShot = true;
		}
		else if (!playerShot && m_ZombieHealth > 0 && distanceBetweenZombieAndPlayer >= 8.f)
		{
			m_ZombieCurrentState = ZombieState::WALK;
			b_ZombieGettingHit = false;
			b_ZombieWalking = true;
			b_ZombieAttacking = false;
			b_ZombieDying = false;
		}
		else if (!playerShot && m_ZombieHealth > 0 && distanceBetweenZombieAndPlayer < 8.f)
		{
			m_ZombieCurrentState = ZombieState::ATTACK;
			b_ZombieGettingHit = false;
			b_ZombieWalking = false;
			b_ZombieAttacking = true;
			b_ZombieDying = false;
		}
		else if (!playerShot && m_ZombieHealth <= 0)
		{
			m_ZombieCurrentState = ZombieState::DEATH;
			b_ZombieGettingHit = false;
			b_ZombieWalking = false;
			b_ZombieDying = true;
			zombieDeathStarted = true;
		}
		else if (!playerShot)
		{
			m_ZombieCurrentState = ZombieState::IDLE;
			b_ZombieGettingHit = false;
			b_ZombieWalking = false;
			b_ZombieDying = false;
		}

		if (m_TimeElapsedAfterShot >= 1.5f)
			playerShot = false;

		switch (m_ZombieCurrentState)
		{
		case ZombieState::IDLE:
			m_ZombieAnimator->BlendTwoAnimations(m_ZombieIdleAnimation, m_ZombieWalkAnimation, m_ZombieIdleWalkBlendFactor, m_DeltaTime);
			break;
		case ZombieState::WALK:
			m_ZombieAnimator->BlendTwoAnimations(m_ZombieIdleAnimation, m_ZombieWalkAnimation, m_ZombieIdleWalkBlendFactor, m_DeltaTime);
			break;
		case ZombieState::HIT:
			m_ZombieAnimator->BlendTwoAnimations(m_ZombieIdleAnimation, m_ZombieHitAnimation, m_ZombieIdleHitBlendFactor, m_DeltaTime);
			break;
		case ZombieState::ATTACK:
			m_ZombieAnimator->BlendTwoAnimations(m_ZombieIdleAnimation, m_ZombieAttackAnimation, m_ZombieIdleAttackBlendFactor, m_DeltaTime);
			break;
		case ZombieState::DEATH:
			m_ZombieAnimator->BlendTwoAnimations(m_ZombieIdleAnimation, m_ZombieDeathAnimation, m_ZombieIdleDeathBlendFactor, m_DeltaTime);
			break;
		}

		float rotSpeed = 1000.f * m_DeltaTime;
		glm::vec3 velocity = glm::normalize(playerGlobalTransform.Translation - zombieGlobalTransform.Translation);

		auto currentRotation = glm::quat(glm::vec3(glm::radians(zombieGlobalTransform.Rotation.x), glm::radians(zombieGlobalTransform.Rotation.y), glm::radians(zombieGlobalTransform.Rotation.z)));

		m_ZombieRotation = glm::quatLookAtLH(velocity, glm::vec3(0.f, 1.f, 0.f));

		auto eulerAngles = glm::eulerAngles(m_ZombieRotation);
		zombieTransform.Rotation.x = glm::degrees(eulerAngles.x);
		zombieTransform.Rotation.y = glm::degrees(eulerAngles.y);
		zombieTransform.Rotation.z = glm::degrees(eulerAngles.z);

		if (b_ZombieWalking && distanceBetweenZombieAndPlayer >= 8.f && !zombieDeathStarted)
		{
			/*PxVec3 horizontalVelocity = PxVec3(velocity.x, 0.f, velocity.z) * 2.5f;
			static PxVec3 verticalVelocity = PxVec3(0.f, 0.f, 0.f);
			const PxVec3 gravity = PxVec3(0.f, -3.f, 0.f);

			PxControllerState state;
			pxZombieController->getState(state);

			bZombieOnTheGround = state.touchedActor && state.collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN ? true : false;
			bIsZombieMovingUp = state.isMovingUp;

			if (bZombieOnTheGround)
				verticalVelocity = PxVec3(0.f, 0.f, 0.f);

			verticalVelocity += gravity;

			auto vel = (horizontalVelocity + verticalVelocity) * m_DeltaTime;

			PxControllerFilters filters;
			filters.mFilterFlags = PxQueryFlag::eSTATIC | PxQueryFlag::ePREFILTER;
			PxControllerCollisionFlags collisionFlags = pxZombieController->move(vel, 0.00001f, m_DeltaTime, filters);

			auto& pxPos = pxZombieController->getFootPosition();*/

			//zombieTransform.Translation = glm::vec3(pxPos.x, pxPos.y, pxPos.z);
		}


		if (zombieDeathStarted)
			m_ZombieDeathTime += m_DeltaTime;

		if (m_ZombieDeathTime > 2.f)
		{
			bool playing;
			m_PlayingChannel->isPlaying(&playing);
			if (playing)
			{
				m_PlayingChannel->stop();
			}
			m_Scene->DestroyEntity(zombieEntity);
		}
	}
}

// D3D11
void TestApplication::D3D11Init()
{
	m_DX11Device = ((DX11RendererAPI*)RendererAPI::GetInstance().get())->GetDevice();
	m_DX11DeviceContext = ((DX11RendererAPI*)RendererAPI::GetInstance().get())->GetDeviceContext();

	InitDX11Stuff();
	FramebufferToTextureInit();
	m_GBuffer = std::make_unique<DX11GBuffer>();
	m_GBuffer->Initialize(texWidth, texHeight);

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


	m_DefaultVertexShader = std::make_shared<DX11VertexShader>("src/Shaders/DefaultVertexShader.hlsl", skeletalAnimationInputLayout, ARRAYSIZE(skeletalAnimationInputLayout));
	m_FullScreenQuadVertexShader = std::make_shared<DX11VertexShader>("src/Shaders/FullScreenQuadVertexShader.hlsl", defaultShaderInputLayout, ARRAYSIZE(defaultShaderInputLayout));

	// Point light
	m_PointLightDepthVertexShader = std::make_shared<DX11VertexShader>("src/Shaders/PointLightDepthVertexShader.hlsl", skeletalAnimationInputLayout, ARRAYSIZE(skeletalAnimationInputLayout));
	m_PointLightDepthPixelShader = std::make_shared<DX11PixelShader>("src/Shaders/PointLightDepthPixelShader.hlsl");
	m_PointLightDepthGeometryShader = std::make_shared<DX11GeometryShader>("src/Shaders/PointLightDepthGeometryShader.hlsl");

	// Directional light
#if CSM_FIRST
	m_SunLightGeometryShader = std::make_shared<DX11GeometryShader>("src/Shaders/DirectionalLightDepthGeometryShader.hlsl");
#else
	m_SunLightGeometryShader = std::make_shared<DX11GeometryShader>("src/Shaders/DirectionalLightDepthGeometryShader2.hlsl");
#endif
	m_SunLightPixelShader = std::make_shared<DX11PixelShader>("src/Shaders/DirectionalLightDepthPixelShader.hlsl");

	// Deferred shading
	m_DeferredVertexShader = std::make_shared<DX11VertexShader>("src/Shaders/DeferredVertexShader.hlsl", skeletalAnimationInputLayout, ARRAYSIZE(skeletalAnimationInputLayout));
	m_DeferredPixelShader = std::make_shared<DX11PixelShader>("src/Shaders/DeferredPixelShader.hlsl");

	m_DeferredCompositingVertexShader = std::make_shared<DX11VertexShader>("src/Shaders/DeferredCompositingVertexShader.hlsl", defaultShaderInputLayout, ARRAYSIZE(defaultShaderInputLayout));
	m_DeferredCompositingPixelShader = std::make_shared<DX11PixelShader>("src/Shaders/DeferredCompositingPixelShader.hlsl");

	// Postprocessing (FXAA)
	m_PostprocessingPixelShader = std::make_shared<DX11PixelShader>("src/Shaders/PostprocessingPixelShader.hlsl");

	// Muzzle flash
	m_MuzzleFlashPixelShader = std::make_shared<DX11PixelShader>("src/Shaders/MuzzleFlashPixelShader.hlsl");
	m_MuzzleFlashSpriteSheet = ResourceManager::GetOrCreateTexture("../resources/textures/MuzzleFlash_ALB.png");

	// Solid color pixel shader (white)
	m_SolidColorPixelShader = std::make_shared<DX11PixelShader>("src/Shaders/SolidColorPixelShader.hlsl");

	// Bloom
	m_BloomPrefilterPixelShader = std::make_shared<DX11PixelShader>("src/Shaders/Bloom/BloomPrefilterPixelShader.hlsl");
	m_DownsamplePixelShader = std::make_shared<DX11PixelShader>("src/Shaders/Bloom/DownsampleBox13TapPixelShader.hlsl");
	m_UpsamplePixelShader = std::make_shared<DX11PixelShader>("src/Shaders/Bloom/UpsampleTentPixelShader.hlsl");
	m_BlurPixelShader = std::make_shared<DX11PixelShader>("src/Shaders/Bloom/BlurPixelShader.hlsl");
	m_BloomUpsampleAndCombinePixelShader = std::make_shared<DX11PixelShader>("src/Shaders/Bloom/BloomUpsampleAndCombinePixelShader.hlsl");
	m_BloomCombinePixelShader = std::make_shared<DX11PixelShader>("src/Shaders/Bloom/BloomCombinePixelShader.hlsl");

	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		m_PointLightDepthSRVs[i] = nullptr;
		m_PointLightDepthTextures[i] = nullptr;
		m_PointLightDepthRenderTextures[i] = nullptr;
	}
	PointLightDepthToTextureInit();

	CascadedShadowmapsInit();

	RenderCubeInit();
	RenderQuadInit();
	RenderGridInit();

	m_IBL = std::make_shared<IBL>();
	m_IBL->Init(m_DX11Device, m_DX11DeviceContext, "../resources/hdris/venice_dawn_1_4k.hdr");

	PostprocessingToTextureInit();

	// Terrain
	m_Terrain = std::make_shared<Terrain>();
	m_Terrain->Initialize(m_DX11Device, "../resources/terrain/setup.txt");
	//m_Terrain->Initialize(m_DX11Device, "../resources/terrain/setup_raw.txt");
	m_TerrainAlbedoTexture = ResourceManager::GetOrCreateTexture("../resources/textures/brown_mud_leaves_01_diff_4k.png");
	m_TerrainNormalTexture = ResourceManager::GetOrCreateTexture("../resources/textures/brown_mud_leaves_01_nor_dx_4k.png");
	m_TerrainRoughnessTexture = ResourceManager::GetOrCreateTexture("../resources/textures/brown_mud_leaves_01_rough_4k.png");

	m_BoxAlbedo = ResourceManager::GetOrCreateTexture("../resources/textures/wood_planks_4k/wood_planks_diff_4k.png");
	m_BoxNormal = ResourceManager::GetOrCreateTexture("../resources/textures/wood_planks_4k/wood_planks_nor_dx_4k.png");
	m_BoxRoughness = ResourceManager::GetOrCreateTexture("../resources/textures/wood_planks_4k/wood_planks_rough_4k.png");

	// Billboarding
	m_PointLightIcon = ResourceManager::GetOrCreateTexture("../resources/icons/lamp_icon.png");
	m_BillboardVertexShader = std::make_shared<DX11VertexShader>("src/Shaders/BillboardVertexShader.hlsl", defaultShaderInputLayout, ARRAYSIZE(defaultShaderInputLayout));
	m_BillboardPixelShader = std::make_shared<DX11PixelShader>("src/Shaders/BillboardPixelShader.hlsl");

	// Nvidia HBAO Plus
	NvidiaHBAOPlusInit();
	// Nvidia ShadowLib
	//NvidiaShadowLibOnInit();
	MousePickingWithPixelShaderInit();

	// Infinite grid
	m_InfiniteGridVertexShader = std::make_shared<DX11VertexShader>("src/Shaders/InfiniteGridVertexShader.hlsl", defaultShaderInputLayout, ARRAYSIZE(defaultShaderInputLayout));
	m_InfiniteGridPixelShader = std::make_shared<DX11PixelShader>("src/Shaders/InfiniteGridPixelShader.hlsl");

	// Bullet holes
	m_BulletHoleImage = ResourceManager::GetOrCreateTexture("../resources/icons/bullethole.png");

	//Object outline
	OutlinePrepassInit();
}

void TestApplication::InitDX11Stuff()
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

	bd.ByteWidth = sizeof(PointLightShadowGenCB);
	hr = m_DX11Device->CreateBuffer(&bd, NULL, &g_pCBPointLightShadowGen);
	if (FAILED(hr)) std::cout << "Can't create g_pCBPointLightShadowGen\n";

	bd.ByteWidth = sizeof(MaterialCB);
	hr = m_DX11Device->CreateBuffer(&bd, NULL, &g_pCBMaterial);
	if (FAILED(hr)) std::cout << "Can't create g_pCBMaterial\n";

	bd.ByteWidth = sizeof(SkeletalAnimationCB);
	hr = m_DX11Device->CreateBuffer(&bd, NULL, &g_pCBSkeletalAnimation);
	if (FAILED(hr)) std::cout << "Can't create g_pCBSkeletalAnimation\n";

	bd.ByteWidth = sizeof(PostprocessingCB);
	hr = m_DX11Device->CreateBuffer(&bd, NULL, &g_pCBPostprocessing);
	if (FAILED(hr)) std::cout << "Can't create g_pCBPostprocessing\n";

	D3D11_SAMPLER_DESC sampDesc = CD3D11_SAMPLER_DESC(D3D11_DEFAULT);
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	hr = m_DX11Device->CreateSamplerState(&sampDesc, &g_pSamplerLinearWrap);
	if (FAILED(hr)) std::cout << "Can't create g_pSamplerLinearWrap!\n";

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	hr = m_DX11Device->CreateSamplerState(&sampDesc, &g_pSamplerPointWrap);
	if (FAILED(hr)) std::cout << "Can't create g_pSamplerPointWrap!\n";

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

	hr = m_DX11Device->CreateSamplerState(&sampDesc, &g_pSamplerPointClamp);
	if (FAILED(hr)) std::cout << "Can't create g_pSamplerPointClamp!\n";

	D3D11_RASTERIZER_DESC rdesc = CD3D11_RASTERIZER_DESC(D3D11_DEFAULT);
	rdesc.CullMode = D3D11_CULL_NONE;
	rdesc.FillMode = D3D11_FILL_WIREFRAME;
	hr = m_DX11Device->CreateRasterizerState(&rdesc, &m_WireframeRasterizerState);
	if (FAILED(hr)) std::cout << "Can't create m_WireframeRasterizerState!\n";

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

	hr = m_DX11Device->CreateSamplerState(&sampDesc, &m_SamplerLinearClamp);
	if (FAILED(hr)) std::cout << "Can't create m_SamplerLinearClamp!\n";

	sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;

	hr = m_DX11Device->CreateSamplerState(&sampDesc, &m_SamplerLinearClampComparison);
	if (FAILED(hr)) std::cout << "Can't create g_pSamplerComparison!\n";

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
	if (FAILED(hr)) std::cout << "Can't create m_pGBufferReverseZDepthStencilState!\n";

	CD3D11_DEPTH_STENCIL_DESC dsDesc{ D3D11_DEFAULT };
	dsDesc.DepthEnable = TRUE;
	dsDesc.StencilEnable = TRUE;
	dsDesc.StencilWriteMask = 0xFF;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.BackFace.StencilFailOp = dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilPassOp = dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.BackFace.StencilFunc = dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	hr = m_DX11Device->CreateDepthStencilState(&dsDesc, m_OutlineStencilWrite.GetAddressOf());
	if (FAILED(hr)) std::cout << "Can't create m_OutlineStencilWrite!\n";

	dsDesc.DepthEnable = TRUE;
	dsDesc.StencilEnable = TRUE;
	dsDesc.StencilWriteMask = 0x0;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.BackFace.StencilPassOp = dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;

	hr = m_DX11Device->CreateDepthStencilState(&dsDesc, m_OutlineStencilMask.GetAddressOf());
	if (FAILED(hr)) std::cout << "Can't create m_OutlineStencilMask!\n";

	dsDesc.DepthEnable = TRUE;
	dsDesc.StencilEnable = FALSE;

	hr = m_DX11Device->CreateDepthStencilState(&dsDesc, m_OutlineStencilNormal.GetAddressOf());
	if (FAILED(hr)) std::cout << "Can't create m_OutlineStencilNormal!\n";

	D3D11_RASTERIZER_DESC rasterDesc = CD3D11_RASTERIZER_DESC(D3D11_DEFAULT);
	hr = m_DX11Device->CreateRasterizerState(&rasterDesc, &m_DefaultRasterizerState);
	if (FAILED(hr)) std::cout << "Can't create m_DefaultRasterizerState!\n";

	rasterDesc.CullMode = D3D11_CULL_FRONT;
	hr = m_DX11Device->CreateRasterizerState(&rasterDesc, m_CullFrontRasterizerState.GetAddressOf());
	if (FAILED(hr)) std::cout << "Can't create m_CullFrontRasterizerState!\n";

	D3D11_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = m_DX11Device->CreateBlendState(&blendDesc, m_InfiniteGridBlendState.GetAddressOf());
	if (FAILED(hr)) std::cout << "Can't create m_InfiniteGridBlendState!\n";
}

void TestApplication::D3D11Frame()
{
	PrepareLightConstantBuffer();

	float profilerBeginTime = (float)glfwGetTime();
	PointLightDepthPass();
	m_D3D11PointLightDepthPassTime = (float)glfwGetTime() - profilerBeginTime;

	profilerBeginTime = (float)glfwGetTime();
	DirectionalLightDepthPass();
	m_D3D11DirLightDepthPassTime = (float)glfwGetTime() - profilerBeginTime;

	//ResetRenderState();

	profilerBeginTime = (float)glfwGetTime();
	GBufferPass();
	m_D3D11GbufferPassTime = (float)glfwGetTime() - profilerBeginTime;

	//ResetRenderState();

	OutlinePrepassBegin();
	OutlinePrepassEnd();

	//ResetRenderState();

	auto albedoMetallicSRV = m_GBuffer->GetShaderResourceView(0);
	auto normalRoughnessSRV = m_GBuffer->GetShaderResourceView(1);
	auto albedoOnlyModeSRV = m_GBuffer->GetShaderResourceView(2);
	auto selectedObjectShapeSRV = m_OutlineRenderTexture->GetSRV();
	auto depthSRV = m_GBuffer->GetDepthShaderResourceView();


	//NvidiaShadowLibOnRender(depthSRV);

	NvidiaHBAOPass(depthSRV);

	// Main deferred render
	if (m_DrawInfo.meshesDrawn > 0)
	{
		profilerBeginTime = (float)glfwGetTime();

		FramebufferToTextureBegin();
		m_DeferredCompositingPixelShader->Bind();
		m_DX11DeviceContext->PSSetShaderResources(0, 1, &albedoMetallicSRV);
		m_DX11DeviceContext->PSSetShaderResources(1, 1, &normalRoughnessSRV);
		m_DX11DeviceContext->PSSetShaderResources(2, 1, &albedoOnlyModeSRV);
		m_DX11DeviceContext->PSSetShaderResources(3, 1, &depthSRV);
		m_DX11DeviceContext->PSSetShaderResources(4, 1, &selectedObjectShapeSRV);

		if (postprocessingSRV)
			m_DX11DeviceContext->PSSetShaderResources(5, 1, &postprocessingSRV);

		// IBL
		m_DX11DeviceContext->PSSetShaderResources(6, 1, m_IBL->m_IrradianceShaderResourceView.GetAddressOf());
		m_DX11DeviceContext->PSSetShaderResources(7, 1, m_IBL->m_PrefilterShaderResourceView.GetAddressOf());
		m_DX11DeviceContext->PSSetShaderResources(8, 1, m_IBL->m_d3dBRDFTextureSRV.GetAddressOf());
		// Depth textures
		auto sunlightSRV = m_SunLightDepthRenderTexture->GetDepthStencilSRV();
		m_DX11DeviceContext->PSSetShaderResources(9, 1, &sunlightSRV);
		m_DX11DeviceContext->PSSetShaderResources(20, MAX_POINT_LIGHTS, m_PointLightDepthSRVs);

		m_DX11DeviceContext->PSSetSamplers(0, 1, &g_pSamplerLinearWrap);
		m_DX11DeviceContext->PSSetSamplers(1, 1, &g_pSamplerPointClamp);
		m_DX11DeviceContext->PSSetSamplers(2, 1, &m_SamplerLinearClampComparison);
		m_DX11DeviceContext->PSSetConstantBuffers(3, 1, &g_pCBLight);

		PointLightShadowGenCB plsgCb{};
		plsgCb.cubeProj = m_PointLightProjMat;
		for (int i = 0; i < 6; i++)
		{
			plsgCb.cubeView[i] = m_PointLightDepthCaptureViews[i];
		}
		m_DX11DeviceContext->UpdateSubresource(g_pCBPointLightShadowGen, 0, NULL, &plsgCb, 0, 0);
		m_DX11DeviceContext->PSSetConstantBuffers(4, 1, &g_pCBPointLightShadowGen);

		PostprocessingCB pCb{};
		pCb.enableHBAO = m_bEnableHBAO ? 1 : 0;
		m_DX11DeviceContext->UpdateSubresource(g_pCBPostprocessing, 0, NULL, &pCb, 0, 0);
		m_DX11DeviceContext->PSSetConstantBuffers(2, 1, &g_pCBPostprocessing);

		MatricesCB mtxCb{};
		mtxCb.modelMat = glm::mat4(1.f);
		mtxCb.viewMat = m_CameraViewMatrix;
		mtxCb.projMat = m_CameraProjMatrix;
		mtxCb.normalMat = glm::transpose(glm::inverse(mtxCb.modelMat));
		mtxCb.viewMatInv = glm::inverse(mtxCb.viewMat);
		mtxCb.projMatInv = glm::inverse(mtxCb.projMat);
		m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, NULL, &mtxCb, 0, 0);
		m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);

		RenderFullScreenQuad();
		m_DeferredCompositingPixelShader->Unbind();
		FramebufferToTextureEnd();

		m_D3D11FrameCompoistingPassTime = (float)glfwGetTime() - profilerBeginTime;

		if (m_BloomEnabled)
		{
			BloomPass();
		}

		PostprocessingPass();
	}
	SetDefaultRenderTargetAndViewport();
	MousePickingWithPixelShader();
}

void TestApplication::BloomPass()
{
	//double time = glfwGetTime();

	// Prefilter
	PostprocessingToTextureBegin();
	m_DX11DeviceContext->PSSetShaderResources(0, 1, &frameSRV);
	m_DX11DeviceContext->PSSetSamplers(0, 1, &m_SamplerLinearClamp);
	m_BloomPrefilterPixelShader->Bind();
	PostprocessingCB pCb;
	pCb.threshold = glm::vec4(g_fBloomThreshold, g_fBloomThreshold - g_fBloomKnee, g_fBloomKnee * 2.f, 0.25f / g_fBloomKnee);
	pCb.params = glm::vec4(g_fBloomClamp, 0.f, 0.f, 0.f);
	m_DX11DeviceContext->UpdateSubresource(g_pCBPostprocessing, 0, NULL, &pCb, 0, 0);
	m_DX11DeviceContext->PSSetConstantBuffers(2, 1, &g_pCBPostprocessing);
	RenderFullScreenQuad();
	PostprocessingToTextureEnd(postprocessingTexture);

	// Downsample and bloom
	for (int i = 0; i < 6; i++)
	{
		BloomBegin(i);
		m_DX11DeviceContext->PSSetShaderResources(0, 1, i == 0 ? &postprocessingSRV : &m_BloomMipSRVs[i - 1]);
		m_DX11DeviceContext->PSSetSamplers(0, 1, &m_SamplerLinearClamp);
		m_DownsamplePixelShader->Bind();
		RenderFullScreenQuad();
		BloomEnd(i, m_BloomMips[i]);
		for (int j = 0; j < g_iBlurPasses; j++)
		{
			// Blur
			// Horizontal
			BloomBegin(i);
			m_DX11DeviceContext->PSSetShaderResources(0, 1, &m_BloomMipSRVs[i]);
			m_DX11DeviceContext->PSSetSamplers(0, 1, &m_SamplerLinearClamp);
			m_BlurPixelShader->Bind();
			pCb.horizontalBlur = 1;
			//m_DX11DeviceContext->UpdateSubresource(g_pCBPostprocessing, 0, NULL, &pCb, 0, 0);
			//m_DX11DeviceContext->PSSetConstantBuffers(2, 1, &g_pCBPostprocessing);
			UpdateAndSetConstantBuffer(g_pCBPostprocessing, pCb, 2);
			RenderFullScreenQuad();
			BloomEnd(i, m_BloomMips[i]);
			// Vertical
			BloomBegin(i);
			m_DX11DeviceContext->PSSetShaderResources(0, 1, &m_BloomMipSRVs[i]);
			m_DX11DeviceContext->PSSetSamplers(0, 1, &m_SamplerLinearClamp);
			m_BlurPixelShader->Bind();
			pCb.horizontalBlur = 0;
			m_DX11DeviceContext->UpdateSubresource(g_pCBPostprocessing, 0, NULL, &pCb, 0, 0);
			m_DX11DeviceContext->PSSetConstantBuffers(2, 1, &g_pCBPostprocessing);
			RenderFullScreenQuad();
			BloomEnd(i, m_BloomMips[i]);
		}
	}

	for (int i = 4; i >= 0; i--)
	{
		BloomBegin(i);
		m_DX11DeviceContext->PSSetShaderResources(0, 1, &m_BloomMipSRVs[i + 1]);
		m_DX11DeviceContext->PSSetShaderResources(1, 1, &m_BloomMipSRVs[i]);
		m_DX11DeviceContext->PSSetSamplers(0, 1, &m_SamplerLinearClamp);
		m_BloomUpsampleAndCombinePixelShader->Bind();
		pCb.sampleScale = 2.f;
		m_DX11DeviceContext->UpdateSubresource(g_pCBPostprocessing, 0, NULL, &pCb, 0, 0);
		m_DX11DeviceContext->PSSetConstantBuffers(2, 1, &g_pCBPostprocessing);
		RenderFullScreenQuad();
		BloomEnd(i, m_BloomMips[i]);
	}

	// Final combine
	PostprocessingToTextureBegin();
	m_DX11DeviceContext->PSSetShaderResources(0, 1, &m_BloomMipSRVs[0]);
	m_DX11DeviceContext->PSSetShaderResources(1, 1, &frameSRV);
	m_DX11DeviceContext->PSSetSamplers(0, 1, &m_SamplerLinearClamp);
	m_BloomUpsampleAndCombinePixelShader->Bind();
	pCb.sampleScale = 2.f;
	m_DX11DeviceContext->UpdateSubresource(g_pCBPostprocessing, 0, NULL, &pCb, 0, 0);
	m_DX11DeviceContext->PSSetConstantBuffers(2, 1, &g_pCBPostprocessing);
	RenderFullScreenQuad();
	PostprocessingToTextureEnd(postprocessingTexture);

	//double result = glfwGetTime() - time;
	//OV_INFO("Bloom pass time: {}", result);
}

void TestApplication::GBufferPass()
{
	RenderToGBufferBegin();
	if (m_DrawInfo.meshesDrawn > 0)
	{
		auto lightEnts = m_Scene->GetAllEntitiesWith<PointLightComponent>();
		int i = 0;
		for (auto lightEnt : lightEnts)
		{
			Entity lightEntity = { lightEnt, m_Scene.get() };
			auto lightPos = m_Scene->GetWorldSpaceTransform(lightEntity).Translation;
			auto& pointLightComponent = lightEntity.GetComponent<PointLightComponent>();

			if (m_EditorState != EditorState::PLAYING)
			{
				Entity selectedEntity = m_SceneHierarchyPanel->GetSelectedEntity();
				if (selectedEntity && selectedEntity == lightEntity)
				{
					m_DeferredVertexShader->Bind();
					m_DeferredPixelShader->Bind();
					auto worldMat = glm::mat4(1.f);
					worldMat = glm::translate(worldMat, lightPos);
					worldMat = glm::scale(worldMat, glm::vec3(pointLightComponent.Range * 2.f));
					RenderCircle(worldMat, true);
					worldMat = glm::translate(glm::mat4(1.f), lightPos);
					worldMat = glm::rotate(worldMat, glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
					worldMat = glm::scale(worldMat, glm::vec3(pointLightComponent.Range * 2.f));
					RenderCircle(worldMat, true);
					worldMat = glm::translate(glm::mat4(1.f), lightPos);
					worldMat = glm::rotate(worldMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
					worldMat = glm::scale(worldMat, glm::vec3(pointLightComponent.Range * 2.f));
					RenderCircle(worldMat, true);
				}

				m_PointLightIcon->Bind(0);
				m_BillboardVertexShader->Bind();
				m_BillboardPixelShader->Bind();
				auto modelMat = glm::mat4(1.f);
				modelMat = glm::translate(modelMat, lightPos);
				modelMat = glm::scale(modelMat, glm::vec3(0.15f));
				RenderQuad(modelMat, m_CameraViewMatrix, m_CameraProjMatrix, true, (uint32_t)lightEnt);
				m_DeferredVertexShader->Bind();
				m_DeferredPixelShader->Bind();
			}

			i++;
		}

		//for (const auto& hit : m_BulletHits)
		//{
		//	m_BulletHoleImage->Bind(0);
		//	m_DeferredVertexShader->Bind();
		//	m_DeferredPixelShader->Bind();
		//	auto modelMat = glm::mat4(1.f);
		//	glm::mat4 rotation = glm::toMat4(glm::quatLookAtLH(glm::vec3(-1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f)));
		//	modelMat = glm::translate(modelMat, hit.hitPos);
		//	modelMat = modelMat * rotation;
		//	modelMat = glm::scale(modelMat, glm::vec3(0.05f));
		//	RenderQuad(modelMat, m_CameraViewMatrix, m_CameraProjMatrix, true);
		//}

		if (rightMouseButtonPressing)
		{
			DrawShootingRay();
		}

		DrawRaycast();

		DrawTerrain();

		if (m_ShowGrid)
		{
			RenderGrid();
			//RenderInfiniteGrid();
		}

		if (m_ShowBounds)
		{
			RenderModelBounds("Sponza");
		}

		RenderPhysics();

		auto sphereMat = *(glm::mat4*)&m_Physics.physics_system->GetBodyInterface().GetWorldTransform(m_Physics.sphere_id);
		RenderSphere(64, sphereMat, _sphereColor, _sphereMetallness, _sphereRoughness, false);

		if (m_TimeElapsedAfterShot <= 0.025f && rightMouseButtonPressing)
		{
			m_MuzzleFlashSpriteSheet->Bind(0);
			m_DefaultVertexShader->Bind();
			m_MuzzleFlashPixelShader->Bind();
			glm::mat4 modelMat = glm::mat4(1.f);
			modelMat = glm::scale(modelMat, glm::vec3(0.333f, 0.333f, 0.333f));
			modelMat = glm::rotate(modelMat, glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f));
			modelMat = glm::translate(modelMat, glm::vec3(0.f, 0.f, -0.4f));
			modelMat = coltMat * modelMat;

			MaterialCB matCb{};
			matCb.sponza = 0;
			matCb.objectId = 0;
			matCb.terrain = 0;
			matCb.notTextured = 1;
			matCb.roughness = 1.f;
			matCb.metallic = 0.f;
			matCb.objectOutlinePass = 0;
			matCb.emission = 5.f;
			matCb.albedoOnly = 0;

			m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, nullptr, &matCb, 0, 0);
			m_DX11DeviceContext->PSSetConstantBuffers(1, 1, &g_pCBMaterial);

			RenderQuad(modelMat, m_CameraViewMatrix, m_CameraProjMatrix, true, INVALID_ENTITY_ID);
			m_DeferredPixelShader->Bind();
		}
	}

	RenderHDRCubemap();

	RenderToGBufferEnd();
}

void TestApplication::DirectionalLightDepthPass()
{
	// rendering depth to render texture for directional light
	m_PointLightDepthVertexShader->Bind();
	m_SunLightGeometryShader->Bind();
	m_SunLightPixelShader->Bind();
	CascadedShadowmapsBegin();
	//PrepareLightConstantBuffer();
	m_DX11DeviceContext->PSSetConstantBuffers(3, 1, &g_pCBLight);
	m_DX11DeviceContext->GSSetConstantBuffers(3, 1, &g_pCBLight);
	RenderScene(m_CameraViewMatrix, m_CameraProjMatrix, m_PointLightDepthVertexShader, m_SunLightPixelShader, m_SunLightGeometryShader, false, false, {}, nullptr);
	//DrawTerrain();
	CascadedShadowmapsEnd();
	m_PointLightDepthVertexShader->Unbind();
	m_SunLightGeometryShader->Unbind();
	m_SunLightPixelShader->Unbind();
}

void TestApplication::PointLightDepthPass()
{
	// rendering depth to render texture for point light
	auto lightEnts = m_Scene->GetAllEntitiesWith<PointLightComponent>();
	bool lightPosChanged = false;
	int i = 0;
	for (auto lightEnt : lightEnts)
	{
		Entity entity = { lightEnt, m_Scene.get() };
		auto& lightPos = m_Scene->GetWorldSpaceTransform(entity).Translation;
		lightPosChanged = lightPositionsPrev[i] != lightPos;
		if (lightPosChanged)
			break;
		i++;
	}
	Frustum frustum;
	frustum.ConstructFrustum(m_CameraViewMatrix, m_CameraProjMatrix, CAM_FAR_PLANE);
	if (lightPosChanged || m_LightProps.dynamicShadows)
	{
		int i = 0;
		for (auto lightEnt : lightEnts)
		{
			Entity entity = { lightEnt, m_Scene.get() };
			auto& tc = m_Scene->GetWorldSpaceTransform(entity);

			auto lightPos = tc.Translation;
			auto playerPos = m_Scene->GetWorldSpaceTransform(m_Scene->TryGetEntityByTag("Player")).Translation;
			auto lightRange = m_Scene->GetRegistry().get<PointLightComponent>(lightEnt).Range;

			bool zombieWithinLightRange = false;
			auto zombieEntity = m_Scene->TryGetEntityByTag("Zombie");
			if (zombieEntity.IsValid())
			{
				auto zombiePos = m_Scene->GetWorldSpaceTransform(zombieEntity).Translation;
				zombieWithinLightRange = glm::length(lightPos - zombiePos) <= lightRange;
			}
			bool playerWithinLightRange = glm::length(lightPos - playerPos) <= lightRange;

			if (lightPosChanged || frustum.CheckSphere(lightPos, lightRange) && (playerWithinLightRange || zombieWithinLightRange))
			{
				PreparePointLightViewMatrixes(lightPos);
				PointLightShadowGenCB plsgCb{};
				plsgCb.cubeProj = m_PointLightProjMat;
				for (int j = 0; j < 6; j++)
				{
					plsgCb.cubeView[j] = m_PointLightDepthCaptureViews[j];
				}
				m_DX11DeviceContext->UpdateSubresource(g_pCBPointLightShadowGen, 0, NULL, &plsgCb, 0, 0);
				m_DX11DeviceContext->GSSetConstantBuffers(4, 1, &g_pCBPointLightShadowGen);
				m_PointLightDepthVertexShader->Bind();
				m_PointLightDepthGeometryShader->Bind();
				m_PointLightDepthPixelShader->Bind();
				PointLightDepthToTextureBegin(i);
				PointLightCullingData plcData{};
				plcData.position = lightPos;
				plcData.range = lightRange;
				RenderScene(m_CameraViewMatrix, m_PointLightProjMat, m_PointLightDepthVertexShader, m_PointLightDepthPixelShader, m_PointLightDepthGeometryShader, false, true, plcData, nullptr);
				PointLightDepthToTextureEnd(i);
				m_PointLightDepthVertexShader->Unbind();
				m_PointLightDepthGeometryShader->Unbind();
				m_PointLightDepthPixelShader->Unbind();
			}

			lightPositionsPrev[i] = lightPos;
			i++;
		}
	}

	ResetRenderState();
}

void TestApplication::NvidiaHBAOPass(ID3D11ShaderResourceView* depthSRV)
{
	if (m_bEnableHBAO)
	{
		// Nvidia HBAO Plus pass 
		PostprocessingToTextureBegin();
		NvidiaHBAOPlusRender(depthSRV, m_CameraProjMatrix, 1.f, postprocessingRenderTexture->GetRTV());
		PostprocessingToTextureEnd(postprocessingTexture);
	}
}

void TestApplication::PostprocessingPass()
{
	// Final postprocessing
	PostprocessingToTextureBegin();
	m_DX11DeviceContext->PSSetShaderResources(0, 1, m_BloomEnabled ? &postprocessingSRV : &frameSRV);
	m_PostprocessingPixelShader->Bind();
	m_DX11DeviceContext->PSSetSamplers(0, 1, &m_SamplerLinearClamp);
	PostprocessingCB pCb;
	pCb.enableFXAA = m_bEnableFXAA ? 1 : 0;
	pCb.tonemapPreset = m_ToneMapPreset;
	pCb.exposure = m_Exposure;
	m_DX11DeviceContext->UpdateSubresource(g_pCBPostprocessing, 0, NULL, &pCb, 0, 0);
	m_DX11DeviceContext->PSSetConstantBuffers(2, 1, &g_pCBPostprocessing);
	RenderFullScreenQuad();
	m_PostprocessingPixelShader->Unbind();
	PostprocessingToTextureEnd(postprocessingTexture);
}

void TestApplication::ResetRenderState()
{
	m_DX11DeviceContext->RSSetState(m_DefaultRasterizerState);
	m_DX11DeviceContext->IASetInputLayout(m_DefaultVertexShader->GetRawInputLayout());
	m_DX11DeviceContext->OMSetBlendState(nullptr, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
	m_DX11DeviceContext->OMSetDepthStencilState(nullptr, 0);
}

template<typename T>
void TestApplication::UpdateAndSetConstantBuffer(ID3D11Buffer* buffer, T object, int slot)
{
	m_DX11DeviceContext->UpdateSubresource(buffer, 0, NULL, &object, 0, 0);
	m_DX11DeviceContext->PSSetConstantBuffers(slot, 1, &buffer);
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
	texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
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
	renderTexture->Initialize(texWidth, texHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R16G16B16A16_FLOAT, false, false, 0);
}

void TestApplication::FramebufferToTextureBegin()
{
	DX11RendererAPI::GetInstance()->SetViewport(0, 0, texWidth, texHeight);
	ID3D11UnorderedAccessView* uavs[]{ m_MousePickingUAV.Get() };
	renderTexture->SetRenderTarget(false, true, true, uavs);
	renderTexture->ClearRenderTarget(0.0f, 0.0f, 0.0f, 1.0f, 1.f);
}

void TestApplication::FramebufferToTextureEnd()
{
	m_DX11DeviceContext->CopyResource(frameTexture, renderTexture->GetTexture());
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
		{ glm::vec3(-0.5f, 0.5f, -0.5f),  glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.5f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(0.5f, 0.5f, -0.5f),   glm::vec2(0.5f, 0.0f), glm::vec3(0.0f, 0.5f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(0.5f, 0.5f, 0.5f),    glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, 0.5f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(-0.5f, 0.5f, 0.5f),   glm::vec2(0.0f, 0.5f), glm::vec3(0.0f, 0.5f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},

		{ glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, -0.5f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(0.5f, -0.5f, -0.5f),  glm::vec2(0.5f, 0.0f), glm::vec3(0.0f, -0.5f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(0.5f, -0.5f, 0.5f),   glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, -0.5f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(-0.5f, -0.5f, 0.5f),  glm::vec2(0.0f, 0.5f), glm::vec3(0.0f, -0.5f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},

		{ glm::vec3(-0.5f, -0.5f, 0.5f),  glm::vec2(0.0f, 0.0f), glm::vec3(-0.5f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.5f, 0.0f), glm::vec3(-0.5f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(-0.5f, 0.5f, -0.5f),  glm::vec2(0.5f, 0.5f), glm::vec3(-0.5f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(-0.5f, 0.5f, 0.5f),   glm::vec2(0.0f, 0.5f), glm::vec3(-0.5f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},

		{ glm::vec3(0.5f, -0.5f, 0.5f),	  glm::vec2(0.0f, 0.0f), glm::vec3(0.5f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(0.5f, -0.5f, -0.5f),  glm::vec2(0.5f, 0.0f), glm::vec3(0.5f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(0.5f, 0.5f, -0.5f),	  glm::vec2(0.5f, 0.5f), glm::vec3(0.5f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(0.5f, 0.5f, 0.5f),	  glm::vec2(0.0f, 0.5f), glm::vec3(0.5f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},

		{ glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -0.5f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(0.5f, -0.5f, -0.5f),  glm::vec2(0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -0.5f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(0.5f, 0.5f, -0.5f),	  glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, 0.0f, -0.5f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(-0.5f, 0.5f, -0.5f),  glm::vec2(0.0f, 0.5f), glm::vec3(0.0f, 0.0f, -0.5f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},

		{ glm::vec3(-0.5f, -0.5f, 0.5f),  glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.5f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(0.5f, -0.5f, 0.5f),   glm::vec2(0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 0.5f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(0.5f, 0.5f, 0.5f),    glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 0.5f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ glm::vec3(-0.5f, 0.5f, 0.5f),   glm::vec2(0.0f, 0.5f), glm::vec3(0.0f, 0.0f, 0.5f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
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

	MatricesCB cb = {};
	cb.modelMat = modelMat;
	cb.viewMat = m_CameraViewMatrix;
	cb.projMat = m_CameraProjMatrix;
	cb.normalMat = glm::transpose(glm::inverse(cb.modelMat));

	m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, nullptr, &cb, 0, 0);
	m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);
	m_DX11DeviceContext->PSSetConstantBuffers(3, 1, &g_pCBLight);

	MaterialCB matCb;
	matCb.sponza = 0.f;

	matCb.notTextured = 1;
	matCb.color = glm::vec3(1.f);
	matCb.metallic = 1.f;
	matCb.roughness = 0.2f;

	matCb.terrain = 0;
	matCb.albedoOnly = 0;
	matCb.objectId = 0;
	matCb.objectOutlinePass = 0;
	matCb.emission = 0.f;

	m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, nullptr, &matCb, 0, 0);
	m_DX11DeviceContext->PSSetConstantBuffers(1, 1, &g_pCBMaterial);

	m_DX11DeviceContext->DrawIndexed(36, 0, 0);
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

void TestApplication::RenderQuad(const glm::mat4& modelMat, const glm::mat4& viewMat, const glm::mat4& projMat, bool albedoOnlyMode, uint32_t objectId)
{
	MatricesCB cb = {};
	cb.modelMat = modelMat;
	cb.viewMat = viewMat;
	cb.projMat = projMat;
	cb.normalMat = glm::transpose(glm::inverse(cb.modelMat));
	cb.viewMatInv = glm::inverse(cb.viewMat);
	cb.projMatInv = glm::inverse(cb.projMat);

	m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, nullptr, &cb, 0, 0);

	MaterialCB matCb = {};
	matCb.albedoOnly = albedoOnlyMode ? 1 : 0;
	matCb.sponza = 0.f;
	matCb.objectId = objectId;
	matCb.notTextured = 0;
	matCb.color = glm::vec3(1.f);
	matCb.roughness = 1.f;
	matCb.metallic = 0.f;
	matCb.terrain = 0;
	matCb.emission = 0.f;
	matCb.useNormalMap = 0;
	matCb.objectOutlinePass = 0;

	m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, nullptr, &matCb, 0, 0);

	m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);
	m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);

	m_DX11DeviceContext->VSSetConstantBuffers(1, 1, &g_pCBMaterial);
	m_DX11DeviceContext->PSSetConstantBuffers(1, 1, &g_pCBMaterial);

	//PrepareLightConstantBuffer();
	m_DX11DeviceContext->VSSetConstantBuffers(3, 1, &g_pCBLight);
	m_DX11DeviceContext->PSSetConstantBuffers(3, 1, &g_pCBLight);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	m_DX11DeviceContext->IASetVertexBuffers(0, 1, &pQuadVertexBuffer, &stride, &offset);
	m_DX11DeviceContext->IASetIndexBuffer(pQuadIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_DX11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_DX11DeviceContext->DrawIndexed(6, 0, 0);
}

void TestApplication::RenderFullScreenQuad()
{
	m_DX11DeviceContext->IASetInputLayout(NULL);
	m_DX11DeviceContext->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
	m_DX11DeviceContext->IASetIndexBuffer(NULL, DXGI_FORMAT_UNKNOWN, 0);
	m_DX11DeviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	m_FullScreenQuadVertexShader->Bind();

	m_DX11DeviceContext->Draw(4, 0);
}

void TestApplication::PrepareLightConstantBuffer()
{
	LightCB lightCb{};
	auto lightEnts = m_Scene->GetAllEntitiesWith<PointLightComponent>();
	int i = 0;
	for (auto lightEnt : lightEnts)
	{
		Entity entity = { lightEnt, m_Scene.get() };
		auto& component = m_Scene->GetRegistry().get<PointLightComponent>(lightEnt);

		TransformComponent transform = m_Scene->GetWorldSpaceTransform(entity);

		auto lightPos = transform.Translation;
		lightCb.lightPos[i] = glm::vec4(lightPos, 1.f);
		//lightCb.lightColor[i] = glm::vec4(GammaToLinear(lightColor), 1.f);
		lightCb.lightColor[i] = glm::vec4(component.Color, 1.f);
		lightCb.rangeRcpAndIntensity[i].x = 1.f / component.Range;
		lightCb.rangeRcpAndIntensity[i].y = component.Intensity;
		i++;
	}

	auto sunLightEntity = m_Scene->TryGetEntityByTag("SunLight");
	if (sunLightEntity)
	{
		m_SunLightDir = glm::normalize(sunLightEntity.GetComponent<TransformComponent>().Rotation);
		lightCb.sunLightDir = glm::vec4(m_SunLightDir, 1.f);
	}
	lightCb.bias = m_ShadowMapBias;
	lightCb.camPos = glm::vec4(m_CameraPos, 0.f);
	lightCb.showDiffuseTexture = m_bShowDebug;
	lightCb.numLights = lightEnts.size();
	lightCb.rcpFrame = glm::vec4(1.0f / texWidth, 1.0f / texHeight, 0.0f, 0.0f);

	// for directional light
#if CSM_FIRST
	const std::vector<glm::mat4>& lsm = GetLightSpaceMatrices();
	for (int j = 0; j < shadowCascadeLevels.size() + 1; j++)
	{
		lightCb.lightSpaceMatrices[j] = lsm[j];
	}
	for (int j = 0; j < shadowCascadeLevels.size(); j++)
	{
		lightCb.cascadePlaneDistances[j] = glm::vec4(shadowCascadeLevels[j], 0.f, 0.f, 0.f);
	}
	lightCb.cascadeCount = shadowCascadeLevels.size();
#else
	UpdateCascades();
	for (int j = 0; j < SHADOW_MAP_CASCADE_COUNT; j++)
	{
		lightCb.lightSpaceMatrices[j] = cascades[j].viewProjMatrix;
	}
	for (int j = 0; j < SHADOW_MAP_CASCADE_COUNT; j++)
	{
		lightCb.cascadePlaneDistances[j] = glm::vec4(cascades[j].splitDepth, 0.f, 0.f, 0.f);
		//std::cout << "Cascade split number " << j << ": " << lightCb.cascadePlaneDistances[j].x << "\n";
	}
	lightCb.cascadeCount = SHADOW_MAP_CASCADE_COUNT;
#endif
	lightCb.cascadePlaneDistances[9] = glm::vec4(BiasModifier, 0.f, 0.f, 0.f);

	lightCb.farPlane = CAM_FAR_PLANE;
	lightCb.showCascades = m_ShowCasades;

	lightCb.hemisphericAmbient = hemisphericAmbient ? 1 : 0;
	//lightCb.ambientDown = glm::vec4(GammaToLinear(ambientDown), 1.f);
	lightCb.ambientDown = glm::vec4(ambientDown, 1.f);
	//lightCb.ambientRange = glm::vec4(GammaToLinear(ambientUp) - GammaToLinear(ambientDown), 1.f);
	lightCb.ambientRange = glm::vec4(ambientUp - ambientDown, 1.f);

	lightCb.billboardCenterWS = glm::vec4(m_BillboardCenter, 1.f);

	lightCb.sunLightColor = glm::vec4(m_sunLightColor, 1.f);
	lightCb.sunLightPower = m_SunLightPower;
	lightCb.skyboxIntensity = m_SkyboxIntensity;

	uint32_t selectedObjectId = (uint32_t)m_SceneHierarchyPanel->GetSelectedEntity();

	if (IsCursorInsideViewport())
		lightCb.padding = glm::vec3(glm::vec2(_ViewportRelativeCursorPos.x / texWidth, _ViewportRelativeCursorPos.y / texHeight), (float)selectedObjectId);
	else
		lightCb.padding = glm::vec3(glm::vec2(-1.f), (float)selectedObjectId);

	if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
	{
		m_DX11DeviceContext->UpdateSubresource(g_pCBLight, 0, NULL, &lightCb, 0, 0);
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
	{
		auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
		auto& commandList = dx12Api->GetCommandList();

		_CurrFrameResource->_LightCB->CopyData(0, lightCb);

		commandList->SetGraphicsRootDescriptorTable(7, _CurrFrameResource->_LightCBgpuHandle);
	}
	else if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
	{
		auto vkApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();

		VkCommandBuffer cmd = vkApi->GetCurrentFrame()._mainCommandBuffer;

		LightBuffer lb{};
		lb.camPos = lightCb.camPos;
		lb.farPlane = lightCb.farPlane;
		lb.lightCount = lightCb.numLights;
		lb.lightDir = lightCb.sunLightDir;
		lb.sunlightPower = lightCb.sunLightPower;
		lb.view = m_CameraViewMatrix;
		for (int j = 0; j < lightCb.numLights; j++)
		{
			lb.lightColorsAndRanges[j] = glm::vec4(glm::vec3(lightCb.lightColor[j]), lightCb.rangeRcpAndIntensity[j].x);
			lb.lightPositionsAndIntensities[j] = glm::vec4(glm::vec3(lightCb.lightPos[j]), lightCb.rangeRcpAndIntensity[j].y);
		}

		memcpy(vkApi->_lightBuffer.info.pMappedData, &lb, sizeof(lb));
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkApi->_meshPipelineLayout, 2, 1, &vkApi->_lightBufferDescriptorSet, 0, nullptr);
	}
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
	srvDesc.Format = DXGI_FORMAT_R16_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.MostDetailedMip = 0;

	result = m_DX11Device->CreateShaderResourceView(m_SpotLightDepthTexture, &srvDesc, &m_SpotLightDepthSRV);
	if (FAILED(result)) return;

	m_SpotLightDepthRenderTexture = new RenderTexture;
	m_SpotLightDepthRenderTexture->Initialize(SHADOWMAP_WIDTH, SHADOWMAP_WIDTH, 1, DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_D16_UNORM, DXGI_FORMAT_R16_FLOAT, false, false, 0);
}

void TestApplication::SpotLightDepthToTextureBegin()
{
	DX11RendererAPI::GetInstance()->SetViewport(0, 0, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT);

	m_SpotLightDepthRenderTexture->SetRenderTarget(true, false, false, nullptr);
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

	DX11RendererAPI::GetInstance()->SetViewport(0, 0, texWidth, texHeight);

	//delete m_SpotLightDepthRenderTexture;
}

void TestApplication::PointLightDepthToTextureInit()
{
	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
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
		m_PointLightDepthRenderTextures[i]->Initialize(SHADOWMAP_WIDTH, SHADOWMAP_WIDTH, 1, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, true, false, 0);
	}

}

void TestApplication::PointLightDepthToTextureBegin(int i)
{
	float m_iShadowMapSize = SHADOWMAP_WIDTH;

	D3D11_VIEWPORT vp[6];
	for (int i = 0; i < 6; i++)
	{
		vp[i] = { 0, 0, m_iShadowMapSize, m_iShadowMapSize, 0.0f, 1.0f };
	}
	m_DX11DeviceContext->RSSetViewports(6, vp);

	m_PointLightDepthRenderTextures[i]->SetRenderTarget(true, false, false, nullptr);
	m_PointLightDepthRenderTextures[i]->ClearRenderTarget(0.f, 0.f, 0.f, 1.f, 1.f);
}

void TestApplication::PointLightDepthToTextureEnd(int i)
{
	static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->ResetRenderTargetView();

	ID3D11Texture2D* dsTex = m_PointLightDepthRenderTextures[i]->GetDepthStencilTexture();

	m_DX11DeviceContext->CopyResource(m_PointLightDepthTextures[i], dsTex);

	RECT rect;
	GetClientRect(((WindowsWindow*)m_Window.get())->GetHWND(), &rect);
	DX11RendererAPI::GetInstance()->SetViewport(0, 0, rect.right - rect.left, rect.bottom - rect.top);
}

void TestApplication::PreparePointLightViewMatrixes(const glm::vec3& lightPos)
{
	m_PointLightDepthCaptureViews =
	{
		glm::lookAtLH(lightPos, lightPos + m_cubeVectors[1], m_cubeVectors[2]),
		glm::lookAtLH(lightPos, lightPos + m_cubeVectors[4], m_cubeVectors[5]),
		glm::lookAtLH(lightPos, lightPos + m_cubeVectors[7], m_cubeVectors[8]),
		glm::lookAtLH(lightPos, lightPos + m_cubeVectors[10], m_cubeVectors[11]),
		glm::lookAtLH(lightPos, lightPos + m_cubeVectors[13], m_cubeVectors[14]),
		glm::lookAtLH(lightPos, lightPos + m_cubeVectors[16], m_cubeVectors[17])
	};
}

std::vector<glm::vec4> TestApplication::GetFrustumCornersWorldSpace(const glm::mat4& projview)
{
	const auto& inv = glm::inverse(projview);

	std::vector<glm::vec4> frustumCorners;
	for (unsigned int x = 0; x < 2; ++x)
	{
		for (unsigned int y = 0; y < 2; ++y)
		{
			for (unsigned int z = 0; z < 2; ++z)
			{
				const glm::vec4& pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, RendererAPI::GetAPI() == RendererAPI::API::OpenGL ? 2.0f * z - 1.0f : z, 1.0f);
				frustumCorners.push_back(pt / pt.w);
			}
		}
	}

	//glm::vec4 corners[8] = {
	//	glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f),
	//	glm::vec4(1.0f,  1.0f, 0.0f, 1.0f),
	//	glm::vec4(1.0f, -1.0f, 0.0f, 1.0f),
	//	glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
	//	glm::vec4(-1.0f,  1.0f, 1.0f, 1.0f),
	//	glm::vec4(1.0f,  1.0f, 1.0f, 1.0f),
	//	glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
	//	glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
	//};

	//for (int i = 0; i < 8; i++)
	//{
	//	const glm::vec4& pt = inv * corners[i];
	//	frustumCorners.push_back(pt / pt.w);
	//}

	return frustumCorners;
}

std::vector<glm::vec4> TestApplication::GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
{
	return GetFrustumCornersWorldSpace(proj * view);
}

glm::mat4 TestApplication::GetLightSpaceMatrix(const float nearPlane, const float farPlane)
{
	const auto& proj = RendererAPI::GetAPI() == RendererAPI::API::OpenGL ?
		glm::perspectiveFovRH_NO(glm::radians(m_CameraFovDegrees), (float)texWidth, (float)texHeight, nearPlane, farPlane) :
		glm::perspectiveFovLH_ZO(glm::radians(m_CameraFovDegrees), (float)texWidth, (float)texHeight, nearPlane, farPlane);
	const auto& corners = GetFrustumCornersWorldSpace(proj, m_CameraViewMatrix);

	glm::vec3 center = glm::vec3(0, 0, 0);
	for (const auto& v : corners)
	{
		center += glm::vec3(v);
	}
	center /= corners.size();

	const auto& lightView = RendererAPI::GetAPI() == RendererAPI::API::OpenGL ?
		glm::lookAtRH(center + m_SunLightDir, center, glm::vec3(0.0f, 1.0f, 0.0f)) :
		glm::lookAtLH(center + m_SunLightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest();
	float minY = std::numeric_limits<float>::max();
	float maxY = std::numeric_limits<float>::lowest();
	float minZ = std::numeric_limits<float>::max();
	float maxZ = std::numeric_limits<float>::lowest();
	for (const auto& v : corners)
	{
		const auto trf = lightView * v;
		minX = std::min(minX, trf.x);
		maxX = std::max(maxX, trf.x);
		minY = std::min(minY, trf.y);
		maxY = std::max(maxY, trf.y);
		minZ = std::min(minZ, trf.z);
		maxZ = std::max(maxZ, trf.z);
	}

	// Tune this parameter according to the scene
	constexpr float zMult = 10.0f;
	if (minZ < 0)
	{
		minZ *= zMult;
	}
	else
	{
		minZ /= zMult;
	}
	if (maxZ < 0)
	{
		maxZ /= zMult;
	}
	else
	{
		maxZ *= zMult;
	}

	const glm::mat4& lightProjection = RendererAPI::GetAPI() == RendererAPI::API::OpenGL ?
		glm::orthoRH_NO(minX, maxX, minY, maxY, minZ, maxZ) :
		glm::orthoLH_ZO(minX, maxX, minY, maxY, minZ, maxZ);
	return lightProjection * lightView;
}

std::vector<glm::mat4> TestApplication::GetLightSpaceMatrices()
{
	std::vector<glm::mat4> ret;
	for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i)
	{
		if (i == 0)
		{
			ret.push_back(GetLightSpaceMatrix(CAM_NEAR_PLANE, shadowCascadeLevels[i]));
		}
		else if (i < shadowCascadeLevels.size())
		{
			ret.push_back(GetLightSpaceMatrix(shadowCascadeLevels[i - 1], shadowCascadeLevels[i]));
		}
		else
		{
			ret.push_back(GetLightSpaceMatrix(shadowCascadeLevels[i - 1], CAM_FAR_PLANE));
		}
	}
	return ret;
}

void TestApplication::CascadedShadowmapsInit()
{
	const int arraySize = SHADOW_MAP_CASCADE_COUNT;

	if (m_SunLightDepthTexture) m_SunLightDepthTexture->Release();
	if (m_SunLightDepthSRV) m_SunLightDepthSRV->Release();

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = CASCADED_SHADOW_SIZE;
	texDesc.Height = CASCADED_SHADOW_SIZE;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = arraySize;
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	HRESULT result = m_DX11Device->CreateTexture2D(&texDesc, nullptr, &m_SunLightDepthTexture);
	if (FAILED(result)) return;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = arraySize;
	srvDesc.Texture2DArray.MipLevels = 1;
	srvDesc.Texture2DArray.MostDetailedMip = 0;

	result = m_DX11Device->CreateShaderResourceView(m_SunLightDepthTexture, &srvDesc, &m_SunLightDepthSRV);
	if (FAILED(result)) return;

	m_SunLightDepthRenderTexture = new RenderTexture;
	m_SunLightDepthRenderTexture->Initialize(CASCADED_SHADOW_SIZE, CASCADED_SHADOW_SIZE, 1,
		DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, false, true, arraySize);
}

void TestApplication::CascadedShadowmapsBegin()
{
	const int numViewports = SHADOW_MAP_CASCADE_COUNT;

	float m_iShadowMapSize = CASCADED_SHADOW_SIZE;

	D3D11_VIEWPORT vp[numViewports];
	for (int i = 0; i < numViewports; i++)
	{
		vp[i] = { 0, 0, m_iShadowMapSize, m_iShadowMapSize, 0.0f, 1.0f };
	}
	m_DX11DeviceContext->RSSetViewports(numViewports, vp);

	m_SunLightDepthRenderTexture->SetRenderTarget(true, false, false, nullptr);
	m_SunLightDepthRenderTexture->ClearRenderTarget(0.f, 0.f, 0.f, 1.f, 1.f);
}

void TestApplication::CascadedShadowmapsEnd()
{
	static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->ResetRenderTargetView();

	//ID3D11Texture2D* dsTex = m_SunLightDepthRenderTexture->GetDepthStencilTexture();

	//m_DX11DeviceContext->CopyResource(m_SunLightDepthTexture, dsTex);

	DX11RendererAPI::GetInstance()->SetViewport(0, 0, texWidth, texHeight);
}

void TestApplication::PrepareGbufferRenderTargets(int width, int height)
{
	m_GBuffer->Shutdown();
	m_GBuffer->Initialize(width, height);
}

void TestApplication::RenderToGBufferBegin()
{
	m_GBuffer->SetRenderTargets();
	m_GBuffer->ClearRenderTargets(0.f, 0.f, 0.f, 1.f);

	m_DX11DeviceContext->VSSetConstantBuffers(3, 1, &g_pCBLight);
	m_DX11DeviceContext->PSSetConstantBuffers(3, 1, &g_pCBLight);

	m_DeferredVertexShader->Bind();
	m_DeferredPixelShader->Bind();	

	RenderScene(m_CameraViewMatrix, m_CameraProjMatrix, m_DeferredVertexShader, m_DeferredPixelShader, m_NullGeometryShader, m_bFrustumCulling, false, {}, nullptr);
}

void TestApplication::RenderToGBufferEnd()
{
	SetDefaultRenderTargetAndViewport();
}

glm::mat4 TestApplication::reverseZ(const glm::mat4& perspeciveMat)
{
	constexpr glm::mat4 reverseZ = { 1.0f, 0.0f, 0.0f, 0.0f,
									0.0f, 1.0f,  0.0f, 0.0f,
									0.0f, 0.0f, -1.0f, 0.0f,
									0.0f, 0.0f,  1.0f, 1.0f };
	return reverseZ * perspeciveMat;
}

void TestApplication::DrawShootingRay()
{
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;

	glm::decompose(coltMat, scale, rotation, translation, skew, perspective);

	glm::vec3 rayStart = translation;
	glm::vec3 rayEnd = translation + rotation * glm::vec3(0.f, 500.f, 0.f);

	ID3D11Buffer* pRayVertexBuffer, * pRayIndexBuffer;

	Vertex vertices[] =
	{
		{ rayStart, glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
		{ rayEnd, glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f}},
	};

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Vertex) * 2;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;

	HRESULT hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pRayVertexBuffer);
	if (FAILED(hr)) return;

	UINT indices[] =
	{
		0, 1
	};

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(UINT) * 2;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	InitData.pSysMem = indices;

	hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pRayIndexBuffer);
	if (FAILED(hr)) return;

	MatricesCB cb{};
	cb.modelMat = glm::mat4(1.f);
	cb.viewMat = m_CameraViewMatrix;
	cb.projMat = m_CameraProjMatrix;
	cb.normalMat = glm::transpose(glm::inverse(cb.modelMat));
	cb.viewMatInv = glm::inverse(cb.viewMat);
	cb.projMatInv = glm::inverse(cb.projMat);

	m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, nullptr, &cb, 0, 0);
	m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);

	MaterialCB matCb{};
	matCb.sponza = 0;
	matCb.objectId = INVALID_ENTITY_ID;
	matCb.terrain = 0;
	matCb.albedoOnly = 1;
	matCb.notTextured = 1;
	matCb.color = glm::vec3(1.f, 0.f, 0.f);
	matCb.roughness = 1.f;
	matCb.metallic = 0.f;
	matCb.objectOutlinePass = 0;
	matCb.emission = 0.0f;

	m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, nullptr, &matCb, 0, 0);
	m_DX11DeviceContext->PSSetConstantBuffers(1, 1, &g_pCBMaterial);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	m_DX11DeviceContext->IASetVertexBuffers(0, 1, &pRayVertexBuffer, &stride, &offset);
	m_DX11DeviceContext->IASetIndexBuffer(pRayIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_DX11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

	m_DX11DeviceContext->DrawIndexed(2, 0, 0);

	pRayVertexBuffer->Release();
	pRayIndexBuffer->Release();
}

bool TestApplication::RaySphereIntersection(const glm::vec3& rayStart, const glm::vec3& rayDir, float rayLen, const glm::vec3& sphereCenter, float sphereRadius)
{
	// Find the distance along the ray closest to the center
	float t = glm::dot(rayDir, sphereCenter - rayStart);
	// Find closest point on ray to the circle center
	glm::vec3 p = rayStart + t * rayDir;
	return glm::length(p - sphereCenter) <= sphereRadius;
}

void TestApplication::DrawTerrain()
{
	MatricesCB cb = {};
	cb.modelMat = glm::translate(glm::mat4(1.f), glm::vec3(1250.f * SPONZA_SCALE, 0.f, 1250.f * SPONZA_SCALE));
	//cb.modelMat = glm::mat4(1.f);
	cb.viewMat = m_CameraViewMatrix;
	cb.projMat = m_CameraProjMatrix;
	cb.normalMat = glm::transpose(glm::inverse(cb.modelMat));

	m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, nullptr, &cb, 0, 0);
	m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);

	MaterialCB matCb = {};
	matCb.sponza = 0;
	matCb.objectId = 0;
	matCb.notTextured = 0;
	matCb.albedoOnly = 0;
	matCb.terrain = 1;
	matCb.terrainUVScale = m_TerrainUVScale;
	matCb.objectOutlinePass = 0;
	matCb.emission = 0.f;
	matCb.useNormalMap = 1;
	matCb.color = glm::vec3(1.f);

	m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, nullptr, &matCb, 0, 0);
	m_DX11DeviceContext->PSSetConstantBuffers(1, 1, &g_pCBMaterial);
	m_DX11DeviceContext->VSSetConstantBuffers(1, 1, &g_pCBMaterial);

	m_DX11DeviceContext->PSSetSamplers(0, 1, &g_pSamplerLinearWrap);

	m_TerrainAlbedoTexture->Bind(0);
	m_TerrainNormalTexture->Bind(1);
	m_TerrainRoughnessTexture->Bind(2);

	m_Terrain->Render(m_DX11DeviceContext);
}

void TestApplication::ImguiDrawGizmo()
{
	// Gizmos
	if (m_EditorState == EditorState::NAVIGATION && ImGui::IsMouseClicked(0) && !ImGuizmo::IsOver() && IsCursorInsideViewport() && m_MouseHoveringObjectId != INVALID_ENTITY_ID)
	{
		std::cout << "Mouse clicked on entitity with ID: " << m_MouseHoveringObjectId << "\n";
		m_SceneHierarchyPanel->SetSelectedEntity(Entity((entt::entity)m_MouseHoveringObjectId, m_Scene.get()));
	}
	Entity selectedEntity = m_SceneHierarchyPanel->GetSelectedEntity();
	if (selectedEntity && m_GizmoType != -1 && m_EditorState != EditorState::PLAYING)
	{
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();

		ImGuizmo::SetRect(vRegionMinX, vRegionMinY, texWidth, texHeight);

		if (ImGui::IsKeyPressed(ImGuiKey_W))
			m_GizmoType = ImGuizmo::TRANSLATE;
		if (ImGui::IsKeyPressed(ImGuiKey_E))
			m_GizmoType = ImGuizmo::ROTATE;
		if (ImGui::IsKeyPressed(ImGuiKey_R))
			m_GizmoType = ImGuizmo::SCALE;
		if (ImGui::IsKeyPressed(ImGuiKey_Q))
		{
			if (m_GizmoMode == ImGuizmo::LOCAL)
				m_GizmoMode = ImGuizmo::WORLD;
			else
				m_GizmoMode = ImGuizmo::LOCAL;
		}


		// Editor camera
		const glm::mat4& cameraProjection = m_CameraProjMatrix;
		const glm::mat4& cameraView = m_CameraViewMatrix;

		// Entity transform
		auto& tc = selectedEntity.GetComponent<TransformComponent>();
		glm::mat4 transform = m_Scene->GetWorldSpaceTransformMatrix(selectedEntity);

		// Snapping
		bool snap = (glfwGetKey(((WindowsWindow*)m_Window.get())->GetGLFWwindow(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);
		float snapValue = 0.5f; // Snap to 0.5m for translation/scale
		// Snap to 45 degrees for rotation
		if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
			snapValue = 45.0f;

		float snapValues[3] = { snapValue, snapValue, snapValue };

		if (ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
			(ImGuizmo::OPERATION)m_GizmoType, m_GizmoMode, glm::value_ptr(transform),
			nullptr, snap ? snapValues : nullptr))
		{
			Entity parent = { selectedEntity.GetComponent<RelationshipComponent>().Parent, m_Scene.get() };
			if (parent)
			{
				glm::mat4 parentTransform = m_Scene->GetWorldSpaceTransformMatrix(parent);
				transform = glm::inverse(parentTransform) * transform;
			}

			glm::vec3 scale;
			glm::quat rotation;
			glm::vec3 translation;
			glm::vec3 skew;
			glm::vec4 perspective;

			glm::decompose(transform, scale, rotation, translation, skew, perspective);

			switch (m_GizmoType)
			{
			case ImGuizmo::TRANSLATE:
			{
				tc.Translation = translation;
				break;
			}
			case ImGuizmo::ROTATE:
			{
				glm::vec3 rotationVec = glm::eulerAngles(rotation);
				glm::vec3 deltaRotation = glm::vec3(glm::degrees(rotationVec.x), glm::degrees(rotationVec.y), glm::degrees(rotationVec.z)) - tc.Rotation;
				tc.Rotation += deltaRotation;
				break;
			}
			case ImGuizmo::SCALE:
			{
				if (glm::isnan(scale.x) || glm::isnan(scale.y) || glm::isnan(scale.z))
				{
					scale = glm::vec3(0.001f);
				}
				tc.Scale = glm::max(glm::vec3(0.001f), scale);
				break;
			}
			}
		}
	}
}

void TestApplication::InitPhysics()
{
	m_Physics.Init();
	//if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
	//{
	//	AddTerrainBody();
	//}
}

void TestApplication::AddTerrainBody()
{
	// Terrain
	const unsigned ts = m_Terrain->m_terrainWidth;

	float* hfSamples = new float[ts * ts];

	uint32_t index = 0;
	for (uint32_t col = 0; col < ts; col++)
	{
		for (uint32_t row = 0; row < ts; row++)
		{
			int coL = ts - 1 - col;
			int roW = ts - 1 - row;

			//float height = m_Terrain->m_heightMap[(coL * ts) + row].y;
			//float height = m_Terrain->m_heightMap[(coL * ts) + roW].y;
			//float height = m_Terrain->m_heightMap[(col * ts) + roW].y;
			//float height = m_Terrain->m_heightMap[(col * ts) + row].y;

			//float height = m_Terrain->m_heightMap[(row * ts) + col].y;
			float height = m_Terrain->m_heightMap[(roW * ts) + col].y;
			//float height = m_Terrain->m_heightMap[(row * ts) + coL].y;
			//float height = m_Terrain->m_heightMap[(roW * ts) + coL].y;



			hfSamples[(row * ts) + col] = height;
		}
	}

	HeightFieldShapeSettings settings(hfSamples, Vec3(0.f, 0.f, 0.f), Vec3(1.f, 1.f, 1.f), ts);
	settings.SetEmbedded();
	auto result = settings.Create();
	auto& shape = result.Get();
	BodyCreationSettings terrain_settings(shape, RVec3(0.0_r, 0.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);
	terrain_settings.mFriction = 0.9f;

	m_Physics.physics_system->GetBodyInterface().CreateAndAddBody(terrain_settings, EActivation::DontActivate);

	delete[] hfSamples;
}

void TestApplication::StepPhysics(float dt)
{
	m_Physics.Update(dt);
}

void TestApplication::RenderPhysics()
{
	BodyIDVector bodies;
	m_Physics.physics_system->GetBodies(bodies);
	for (const auto& body : bodies)
	{
		if (body == m_Physics.sphere_id || body == m_Physics.floor->GetID())
			continue;
		RMat44 worldTransform = m_Physics.physics_system->GetBodyInterface().GetWorldTransform(body);
		glm::mat4 worldMatrix = *reinterpret_cast<glm::mat4*>(&worldTransform);
		RenderCube(worldMatrix);
	}
}

void TestApplication::CleanupPhysics()
{
	m_Physics.Shutdown();
}

void TestApplication::HandleMouseInput()
{
	double xpos;
	double ypos;
	glfwGetCursorPos(m_Window->GetGLFWwindow(), &xpos, &ypos);

	static bool firstMouse = true;
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset;
	if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		xoffset = lastX - xpos;
	else
		xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	Xoffset = xoffset;
	Yoffset = yoffset;

	if (xpos == lastX && ypos == lastY)
		mouseMoving = false;
	else
		mouseMoving = true;

	lastX = xpos;
	lastY = ypos;
}

void TestApplication::PrepareAnimations()
{
	auto& playerEnt = m_Scene->TryGetEntityByTag("Player");
	auto& zombieEnt = m_Scene->TryGetEntityByTag("Zombie");
	auto playerModel = playerEnt.GetComponent<MeshComponent>().m_Model.get();
	auto zombieModel = zombieEnt.GetComponent<MeshComponent>().m_Model.get();


	SAFE_DELETE(m_RunAnimation);
	SAFE_DELETE(m_IdleAnimation);
	SAFE_DELETE(m_PistolIdleAnimation);
	SAFE_DELETE(m_LeftTurnAnimation);
	SAFE_DELETE(m_RightTurnAnimation);
	SAFE_DELETE(m_ShootingAnimation);
	SAFE_DELETE(m_Animator);

#if PLAYER_MODEL == 0
	m_RunAnimation = new Animation("../resources/animations/mixamo/Running_fixed.fbx", playerModel);
	m_IdleAnimation = new Animation("../resources/animations/mixamo/Idle_fixed.fbx", playerModel);
	m_PistolIdleAnimation = new Animation("../resources/animations/mixamo/Pistol_Idle_fixed.fbx", playerModel);
	m_LeftTurnAnimation = new Animation("../resources/animations/mixamo/Left_Turn_fixed.fbx", playerModel);
	m_RightTurnAnimation = new Animation("../resources/animations/mixamo/Right_Turn_fixed.fbx", playerModel);
	m_ShootingAnimation = new Animation("../resources/animations/mixamo/Shooting_fixed.fbx", playerModel);
	m_JumpingAnimation = new Animation("../resources/animations/mixamo/Jumping_fixed.fbx", playerModel);

	m_Animator = new Animator(m_PistolIdleAnimation);
#else
	m_RunAnimation = new Animation("../resources/animations/mixamo/nathan/Running.fbx", playerModel);
	m_IdleAnimation = new Animation("../resources/animations/mixamo/nathan/BreathingIdle.fbx", playerModel);
	//m_JumpingAnimation = new Animation("../resources/animations/mixamo/guy/Guy_Jump0_OneFrame.fbx", playerModel);

	m_Animator = new Animator(m_RunAnimation);
#endif

	for (int i = 0; i < m_NumOfDudes; i++)
		m_DudeAnimators[i] = new Animator(m_RunAnimation);

	SAFE_DELETE(m_ZombieIdleAnimation);
	SAFE_DELETE(m_ZombieWalkAnimation);
	SAFE_DELETE(m_ZombieAttackAnimation);
	SAFE_DELETE(m_ZombieHitAnimation);
	SAFE_DELETE(m_ZombieDeathAnimation);
	SAFE_DELETE(m_ZombieAnimator);

	if (zombieEnt.IsValid())
	{
		m_ZombieIdleAnimation = new Animation("../resources/animations/mixamo/zombie_idle_fixed.fbx", zombieModel);
		m_ZombieWalkAnimation = new Animation("../resources/animations/mixamo/zombie_walking_fixed.fbx", zombieModel);
		m_ZombieAttackAnimation = new Animation("../resources/animations/mixamo/zombie_attack_fixed.fbx", zombieModel);
		m_ZombieHitAnimation = new Animation("../resources/animations/mixamo/zombie_hit_fixed.fbx", zombieModel);
		m_ZombieDeathAnimation = new Animation("../resources/animations/mixamo/zombie_death_fixed.fbx", zombieModel);

		m_ZombieAnimator = new Animator(m_ZombieIdleAnimation);
	}
}

void TestApplication::OpenScene()
{
	std::string& filepath = FileDialogs::OpenFile("Oeuvre Scene(*.ovscene)\0 * .ovscene\0");
	m_EditorScenePath = filepath;
	SceneSerializer serializer(m_Scene);
	serializer.Deserialize(filepath);
	m_SceneHierarchyPanel->SetSelectedEntity(Entity());
	PrepareAnimations();
	std::cout << "Scene opened: " << filepath << "\n";
}

void TestApplication::SaveScene()
{
	if (m_EditorScenePath.empty())
		return;
	SceneSerializer serializer(m_Scene);
	serializer.Serialize(m_EditorScenePath);
}

void TestApplication::SaveSceneAs()
{
	std::string& filepath = FileDialogs::SaveFile("Oeuvre Scene(*.ovscene)\0 * .ovscene\0");
	m_EditorScenePath = filepath;
	SceneSerializer serializer(m_Scene);
	serializer.Serialize(filepath);
}

void TestApplication::NvidiaHBAOPlusInit()
{
	CustomHeap.new_ = ::operator new;
	CustomHeap.delete_ = ::operator delete;

	GFSDK_SSAO_Status status;
	status = GFSDK_SSAO_CreateContext_D3D11(m_DX11Device, &pAOContext, &CustomHeap);
	assert(status == GFSDK_SSAO_OK); // HBAO+ requires feature level 11_0 or above
}

void TestApplication::NvidiaHBAOPlusRender(ID3D11ShaderResourceView* pDepthStencilTextureSRV, const glm::mat4& pProjectionMatrix, float SceneScale, ID3D11RenderTargetView* pOutputColorRTV)
{
	GFSDK_SSAO_InputData_D3D11 Input;
	Input.DepthData.DepthTextureType = GFSDK_SSAO_HARDWARE_DEPTHS;
	Input.DepthData.pFullResDepthTextureSRV = pDepthStencilTextureSRV;
	Input.DepthData.ProjectionMatrix.Data = *(GFSDK_SSAO_Float4x4*)&pProjectionMatrix;
	Input.DepthData.ProjectionMatrix.Layout = GFSDK_SSAO_ROW_MAJOR_ORDER;
	Input.DepthData.MetersToViewSpaceUnits = SceneScale;

	GFSDK_SSAO_Parameters Params;
	Params.Radius = 2.f;
	Params.Bias = 0.1f;
	Params.PowerExponent = 2.f;
	Params.Blur.Enable = true;
	Params.Blur.Radius = GFSDK_SSAO_BLUR_RADIUS_4;
	Params.Blur.Sharpness = 16.f;

	GFSDK_SSAO_Output_D3D11 Output;
	Output.pRenderTargetView = pOutputColorRTV;
	Output.Blend.Mode = GFSDK_SSAO_OVERWRITE_RGB;

	GFSDK_SSAO_Status status;
	status = pAOContext->RenderAO(m_DX11DeviceContext, Input, Params, Output);
	assert(status == GFSDK_SSAO_OK);
}

void TestApplication::NvidiaHBAOPlusCleanup()
{
	pAOContext->Release();
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
	texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
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
	postprocessingRenderTexture->Initialize(texWidth, texHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R16G16B16A16_FLOAT, false, false, 0);
}

void TestApplication::PostprocessingToTextureBegin()
{
	DX11RendererAPI::GetInstance()->SetViewport(0, 0, texWidth, texHeight);
	postprocessingRenderTexture->SetRenderTarget(false, true, false, nullptr);
	postprocessingRenderTexture->ClearRenderTarget(0.0f, 0.0f, 0.0f, 1.f, 1.f);
}

void TestApplication::PostprocessingToTextureEnd(ID3D11Texture2D* texture)
{
	m_DX11DeviceContext->CopyResource(texture, postprocessingRenderTexture->GetTexture());
}

void TestApplication::SetDefaultRenderTargetAndViewport()
{
	static_cast<DX11RendererAPI*>(RendererAPI::GetInstance().get())->ResetRenderTargetView();
	RECT rect;
	GetClientRect(((WindowsWindow*)m_Window.get())->GetHWND(), &rect);
	DX11RendererAPI::GetInstance()->SetViewport(0, 0, rect.right - rect.left, rect.bottom - rect.top);
}

void TestApplication::BloomInit()
{
	if (texWidth < 128 || texHeight < 128)
		return;

	if (m_BloomMips.size() > 0)
	{
		for (RenderTexture* rt : m_BloomRTs)
			SAFE_DELETE(rt);
		for (ID3D11Texture2D* texture : m_BloomMips)
			SAFE_RELEASE(texture);
		for (ID3D11ShaderResourceView* srv : m_BloomMipSRVs)
			SAFE_RELEASE(srv);

		m_BloomRTs.clear();
		m_BloomMips.clear();
		m_BloomMipSRVs.clear();
	}

	const int TOTAL_MIPS = 6;
	m_BloomRTs.reserve(TOTAL_MIPS);
	m_BloomMips.reserve(TOTAL_MIPS);
	m_BloomMipSRVs.reserve(TOTAL_MIPS);

	for (int i = 0; i < TOTAL_MIPS; i++)
	{
		m_BloomRTs.push_back(nullptr);
		m_BloomMips.push_back(nullptr);
		m_BloomMipSRVs.push_back(nullptr);

		float scale = glm::pow(0.5f, float(i + 1));

		int mipWidth = (int)(texWidth * scale);
		int mipHeight = (int)(texHeight * scale);

		m_BloomRTs[i] = new RenderTexture();
		m_BloomRTs[i]->Initialize(mipWidth, mipHeight, 1,
			DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R16G16B16A16_FLOAT,
			false, false, 0);

		D3D11_TEXTURE2D_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(texDesc));
		texDesc.Width = mipWidth;
		texDesc.Height = mipHeight;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		HRESULT result = m_DX11Device->CreateTexture2D(&texDesc, nullptr, &m_BloomMips[i]);
		if (FAILED(result))
		{
			//OV_ERROR("Can't create bloom mip number {}", i);
			return;
		}
		else
		{
			//OV_INFO("Created bloom mip with size: {}x{}", mipWidth, mipHeight);
		}


		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;

		result = m_DX11Device->CreateShaderResourceView(m_BloomMips[i], &srvDesc, &m_BloomMipSRVs[i]);
		if (FAILED(result))
		{
			//OV_ERROR("Can't create bloom mip SRV number {}", i);
			return;
		}
	}
}

void TestApplication::BloomBegin(int i)
{
	D3D11_TEXTURE2D_DESC desc;
	m_BloomRTs[i]->GetTexture()->GetDesc(&desc);
	DX11RendererAPI::GetInstance()->SetViewport(0, 0, desc.Width, desc.Height);
	m_BloomRTs[i]->SetRenderTarget(false, true, false, nullptr);
	m_BloomRTs[i]->ClearRenderTarget(0.01f, 0.01f, 0.01f, 1.f, 1.f);
}

void TestApplication::BloomEnd(int i, ID3D11Texture2D* texture)
{
	m_DX11DeviceContext->CopyResource(texture, m_BloomRTs[i]->GetTexture());
}

void TestApplication::ChangeHDRCubemap()
{
	auto filePath = FileDialogs::OpenFile("HDR images (*.hdr)\0*.hdr\0\0");
	if (!filePath.empty())
	{
		if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
		{
			m_IBL.reset(new IBL());
			m_IBL->Init(m_DX11Device, m_DX11DeviceContext, filePath);
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			OpenGLChangeIBL(filePath.c_str());
		}
	}
}

void TestApplication::RenderHDRCubemap()
{
	MatricesCB cb{};
	cb.modelMat = glm::mat4(1.f);
	cb.viewMat = m_CameraViewMatrix;
	cb.projMat = m_CameraProjMatrix;

	MaterialCB matCb{};
	matCb.cubemapLod = m_CubemapLod;

	m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, nullptr, &cb, 0, 0);
	m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, nullptr, &matCb, 0, 0);

	m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);
	m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);
	m_DX11DeviceContext->PSSetConstantBuffers(1, 1, &g_pCBMaterial);
	m_DX11DeviceContext->PSSetSamplers(0, 1, &g_pSamplerLinearWrap);

	m_IBL->m_CubemapVertexShader->Bind();
	m_IBL->m_CubemapPixelShader->Bind();
	//m_DX11DeviceContext->PSSetShaderResources(0, 1, m_IBL->m_CubemapShaderResourceView.GetAddressOf());
	//m_DX11DeviceContext->PSSetShaderResources(0, 1, m_IBL.m_IrradianceShaderResourceView.GetAddressOf());
	m_DX11DeviceContext->PSSetShaderResources(0, 1, m_IBL->m_PrefilterShaderResourceView.GetAddressOf());

	m_DX11DeviceContext->OMSetDepthStencilState(m_IBL->m_HDRDepthStencilState.Get(), 0);
	m_DX11DeviceContext->RSSetState(m_IBL->m_HDRrasterizerState.Get());

	m_IBL->RenderCubeFromTheInside(m_DX11DeviceContext);

	m_DX11DeviceContext->OMSetDepthStencilState(nullptr, 0);
	m_DX11DeviceContext->RSSetState(m_DefaultRasterizerState);
}

// D3D12 
void TestApplication::D3D12Init()
{
	BuildD3D12DescriptorHeaps();
	BuildD3D12FrameResources();
	BuildD3D12Samplers();
	BuildD3D12ShadersAndInputLayout();
	BuildD3D12RootSignature();
	BuildD3D12PSOs();

	InitD3D12Gbuffer();

	auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
	auto& device = dx12Api->GetDevice();
	_RenderTexture = std::make_unique<RenderTextureD3D12>(DXGI_FORMAT_R16G16B16A16_FLOAT, false);
	_RenderTexture->SetDevice(device.Get());

	// Pointlight Shadowgen
	PointLightDepthToTextureInitD3D12();
	// Primitives
	RenderCubeInitD3D12();
}

void TestApplication::BuildD3D12DescriptorHeaps()
{
	auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
	auto& device = dx12Api->GetDevice();

	//D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc;
	//cbvSrvHeapDesc.NumDescriptors = 4;
	//cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//cbvSrvHeapDesc.NodeMask = 0;
	//ThrowIfFailed(device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&_cbvSrvHeap)));
	//_cbvSrvHeapAllocator.Create(device.Get(), _cbvSrvHeap.Get());

	D3D12_DESCRIPTOR_HEAP_DESC samplersHeapDesc;
	samplersHeapDesc.NumDescriptors = 3;
	samplersHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplersHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	samplersHeapDesc.NodeMask = 0;
	ThrowIfFailed(device->CreateDescriptorHeap(&samplersHeapDesc, IID_PPV_ARGS(&_samplersHeap)));
	_samplersHeap->SetName(L"My Samplers Heap");
	_samplersHeapAllocator.Create(device.Get(), _samplersHeap.Get());
}

void TestApplication::BuildD3D12FrameResources()
{
	auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
	auto& device = dx12Api->GetDevice();

	for (int i = 0; i < _NumFrameResources; ++i)
	{
		_FrameResources.push_back(std::make_unique<FrameResource>(device.Get(), FrameResource::MaxConstantBuffers));
	}
}

void TestApplication::BuildD3D12Samplers()
{
	auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
	auto& device = dx12Api->GetDevice();

	D3D12_SAMPLER_DESC linearWrapDesc{};
	linearWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	linearWrapDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	linearWrapDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	linearWrapDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	linearWrapDesc.MinLOD = 0.f;
	linearWrapDesc.MaxLOD = D3D12_FLOAT32_MAX;
	linearWrapDesc.MipLODBias = 0.f;

	_samplersHeapAllocator.Alloc(&_SamplerLinearWrapCpuHandle, &_SamplerLinearWrapGpuHandle);
	device->CreateSampler(&linearWrapDesc, _SamplerLinearWrapCpuHandle);

	D3D12_SAMPLER_DESC pointClampDesc{};
	pointClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	pointClampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pointClampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pointClampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pointClampDesc.MinLOD = 0.f;
	pointClampDesc.MaxLOD = D3D12_FLOAT32_MAX;
	pointClampDesc.MipLODBias = 0.f;
	_samplersHeapAllocator.Alloc(&_SamplerPointClampCpuHandle, &_SamplerPointClampGpuHandle);
	device->CreateSampler(&pointClampDesc, _SamplerPointClampCpuHandle);

	D3D12_SAMPLER_DESC comparisonLinearClamp{};
	comparisonLinearClamp.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	comparisonLinearClamp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	comparisonLinearClamp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	comparisonLinearClamp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	comparisonLinearClamp.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	comparisonLinearClamp.MinLOD = 0.f;
	comparisonLinearClamp.MaxLOD = D3D11_FLOAT32_MAX;
	comparisonLinearClamp.MipLODBias = 0.f;
	_samplersHeapAllocator.Alloc(&_SamplerComparisonLinearClampCpuHandle, &_SamplerComparisonLinearClampGpuHandle);
	device->CreateSampler(&comparisonLinearClamp, _SamplerComparisonLinearClampCpuHandle);
}

void TestApplication::BuildD3D12ShadersAndInputLayout()
{
	_vsDeferredByteCode = d3dUtil::CompileShader(L"src/Shaders/DeferredVertexShaderD3D12.hlsl", nullptr, "VSMain", "vs_6_6");
	_psDeferredByteCode = d3dUtil::CompileShader(L"src/Shaders/DeferredPixelShaderD3D12.hlsl", nullptr, "PSMain", "ps_6_6");

	_vsCompositionByteCode = d3dUtil::CompileShader(L"src/Shaders/FullScreenQuadVertexShader.hlsl", nullptr, "VSMain", "vs_6_6");
	_psCompositionByteCode = d3dUtil::CompileShader(L"src/Shaders/DeferredCompositingPixelShaderD3D12.hlsl", nullptr, "PSMain", "ps_6_6");

	_vsPointLightDepthByteCode = d3dUtil::CompileShader(L"src/Shaders/PointLightDepthVertexShader.hlsl", nullptr, "VSMain", "vs_6_6");
	_psPointLightDepthByteCode = d3dUtil::CompileShader(L"src/Shaders/PointLightDepthPixelShader.hlsl", nullptr, "PSMain", "ps_6_6");
	_gsPointLightDepthByteCode = d3dUtil::CompileShader(L"src/Shaders/PointLightDepthGeometryShaderD3D12.hlsl", nullptr, "GSMain", "gs_6_6");

	_InputLayout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEIDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void TestApplication::BuildD3D12PSOs()
{
	auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
	auto& device = dx12Api->GetDevice();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { _InputLayout.data(), (UINT)_InputLayout.size() };
	psoDesc.pRootSignature = _RootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(_vsDeferredByteCode->GetBufferPointer()),
		_vsDeferredByteCode->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(_psDeferredByteCode->GetBufferPointer()),
		_psDeferredByteCode->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 4;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.RTVFormats[2] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.RTVFormats[3] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_PSOGfuffer)));

	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { _InputLayout.data(), (UINT)_InputLayout.size() };
	psoDesc.pRootSignature = _RootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(_vsCompositionByteCode->GetBufferPointer()),
		_vsCompositionByteCode->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(_psCompositionByteCode->GetBufferPointer()),
		_psCompositionByteCode->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_PSOComposition)));

	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { _InputLayout.data(), (UINT)_InputLayout.size() };
	psoDesc.pRootSignature = _RootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(_vsPointLightDepthByteCode->GetBufferPointer()),
		_vsPointLightDepthByteCode->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(_psPointLightDepthByteCode->GetBufferPointer()),
		_psPointLightDepthByteCode->GetBufferSize()
	};
	psoDesc.GS =
	{
		reinterpret_cast<BYTE*>(_gsPointLightDepthByteCode->GetBufferPointer()),
		_gsPointLightDepthByteCode->GetBufferSize()
	};

	D3D12_RASTERIZER_DESC rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterDesc.DepthBias = 200;
	rasterDesc.DepthBiasClamp = 1.f;
	rasterDesc.SlopeScaledDepthBias = 3.f;

	psoDesc.RasterizerState = rasterDesc;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 0;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_PSOPointLightDepth)));
}

void TestApplication::BuildD3D12RootSignature()
{
	auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
	auto& device = dx12Api->GetDevice();

	// Root parameter can be a atable, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER RootParameter[9]{};

	// Create a single descriptor table of CBVs.
	CD3DX12_DESCRIPTOR_RANGE DescRange[9]{};
	DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 3, 0); // b0 - b2
	DescRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 3, 0); // s0 - s2
	DescRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0
	DescRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // t1 
	DescRange[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); // t2
	DescRange[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3); // t3 
	DescRange[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 16, 4); // t4 - t20
	DescRange[7].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 3); // b3
	DescRange[8].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 4); // b4

	RootParameter[0].InitAsDescriptorTable(1, &DescRange[0]); // b0 - b2
	RootParameter[1].InitAsDescriptorTable(1, &DescRange[1]); // s0 - s2
	RootParameter[2].InitAsDescriptorTable(1, &DescRange[2]); // t0 
	RootParameter[3].InitAsDescriptorTable(1, &DescRange[3]); // t1 
	RootParameter[4].InitAsDescriptorTable(1, &DescRange[4]); // t2 
	RootParameter[5].InitAsDescriptorTable(1, &DescRange[5]); // t3 
	RootParameter[6].InitAsDescriptorTable(1, &DescRange[6]); // t4 - t20
	RootParameter[7].InitAsDescriptorTable(1, &DescRange[7]); // b3
	RootParameter[8].InitAsDescriptorTable(1, &DescRange[8]); // b4

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(9, RootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED); // added

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&_RootSignature)));
}

void TestApplication::InitD3D12Gbuffer()
{
	auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
	auto& device = dx12Api->GetDevice();

	DXGI_FORMAT formats[4]
	{
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT
	};
	_Gbuffer = std::make_unique<GBufferD3D12>(formats);
	_Gbuffer->SetDevice(device.Get());
}

void TestApplication::D3D12BeforeBeginFrame()
{
	auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
	auto& device = dx12Api->GetDevice();
	auto& commandList = dx12Api->GetCommandList();

	// Cycle through the circular frame resource array.
	_CurrFrameResourceIndex = (_CurrFrameResourceIndex + 1) % _NumFrameResources;
	_CurrFrameResource = _FrameResources[_CurrFrameResourceIndex].get();

	auto& fence = dx12Api->_Fence;

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (_CurrFrameResource->Fence != 0 && fence->GetCompletedValue() < _CurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(fence->SetEventOnCompletion(_CurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	auto& cmdListAlloc = _CurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(commandList->Reset(cmdListAlloc.Get(), nullptr));
}

void TestApplication::D3D12AfterEndFrame()
{
	auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
	auto& commandQueue = dx12Api->GetCommandQueue();
	auto& fence = dx12Api->_Fence;
	auto& currentFence = dx12Api->_CurrentFence;

	// Advance the fence value to mark commands up to this fence point.
	_CurrFrameResource->Fence = ++currentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	commandQueue->Signal(fence.Get(), currentFence);
}

void TestApplication::D3D12Frame()
{
	auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
	auto& device = dx12Api->GetDevice();
	auto& commandList = dx12Api->GetCommandList();

	auto cbvHeap = dx12Api->GetSRVDescriptorHeap();
	ID3D12DescriptorHeap* descriptorHeaps[] = { cbvHeap, _samplersHeap.Get() };
	commandList->SetDescriptorHeaps(2, descriptorHeaps);

	// Pointlight Depth Pass
	PointLightDepthPass(commandList.Get());

	// Gbuffer
	_Gbuffer->SetClearColor(0.52f, 0.80f, 0.92f, 1.f);
	_Gbuffer->SizeResources(texWidth, texHeight);
	_Gbuffer->BeginScene(commandList.Get());

	//auto cbvHeap = dx12Api->GetSRVDescriptorHeap();
	//ID3D12DescriptorHeap* descriptorHeaps[] = { cbvHeap, _samplersHeap.Get() };
	//commandList->SetDescriptorHeaps(2, descriptorHeaps);
	commandList->SetGraphicsRootSignature(_RootSignature.Get());
	commandList->SetGraphicsRootDescriptorTable(1, _SamplerLinearWrapGpuHandle);
	commandList->SetPipelineState(_PSOGfuffer.Get());

	PrepareLightConstantBuffer();
	RenderScene(m_CameraViewMatrix, m_CameraProjMatrix, m_NullVertexShader, m_NullPixelShader, m_NullGeometryShader, m_bFrustumCulling, false, {}, nullptr);

	//BodyIDVector bodies;
	//m_Physics.physics_system->GetBodies(bodies);
	//uint32_t meshesSize = m_Scene->GetAllEntitiesWith<MeshComponent>()->size();
	//int i = 0;
	//for (const auto& body : bodies)
	//{
	//	RMat44 worldTransform = m_Physics.physics_system->GetBodyInterface().GetWorldTransform(body);
	//	glm::mat4 worldMatrix = *reinterpret_cast<glm::mat4*>(&worldTransform);
	//	RenderCubeD3D12(worldMatrix, meshesSize + i);
	//	i++;
	//}

	_Gbuffer->EndScene(commandList.Get());
	dx12Api->ResetRenderTarget();

	// Fullscreen Composition
	// 
	// Render to texture
	_RenderTexture->SetClearColor(1.f, 1.f, 1.f, 1.f);
	_RenderTexture->SizeResources(texWidth, texHeight);
	_RenderTexture->BeginScene(commandList.Get());
	/////////////////////////

	commandList->SetPipelineState(_PSOComposition.Get());
	commandList->IASetVertexBuffers(0, 0, nullptr);
	commandList->IASetIndexBuffer(nullptr);
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//PointLightShadowGenCB plsgCb;
	//plsgCb.cubeProj = m_PointLightProjMat;
	//_PointLightShadowGenCB->CopyData(0, plsgCb);

	commandList->SetGraphicsRootDescriptorTable(0, _CurrFrameResource->_MatricesCBgpuHandles[0]);
	commandList->SetGraphicsRootDescriptorTable(1, _SamplerLinearWrapGpuHandle);
	commandList->SetGraphicsRootDescriptorTable(2, _Gbuffer->GetGPUDescriptor(0));
	commandList->SetGraphicsRootDescriptorTable(3, _Gbuffer->GetGPUDescriptor(1));
	commandList->SetGraphicsRootDescriptorTable(4, _Gbuffer->GetGPUDescriptor(2));
	commandList->SetGraphicsRootDescriptorTable(5, _Gbuffer->GetGPUDescriptor(3));
	commandList->SetGraphicsRootDescriptorTable(6, _PointLightRenderTextures[0]->GetGPUDescriptor());
	commandList->SetGraphicsRootDescriptorTable(7, _CurrFrameResource->_LightCBgpuHandle);
	commandList->SetGraphicsRootDescriptorTable(8, _CurrFrameResource->_PointLightShadowGenCBgpuHandles[0]);

	commandList->DrawInstanced(4, 1, 0, 0);

	//Render to texture
	_RenderTexture->EndScene(commandList.Get());
	dx12Api->ResetRenderTarget();
	//////////////////////////
}

void TestApplication::PointLightDepthToTextureInitD3D12()
{
	auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
	auto& device = dx12Api->GetDevice();

	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		_PointLightRenderTextures[i] = std::make_unique<RenderTextureD3D12>(DXGI_FORMAT_R32_FLOAT, true, true);
		_PointLightRenderTextures[i]->SetDevice(device.Get());
		_PointLightRenderTextures[i]->SizeResources(SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT);
	}
}

void TestApplication::PointLightDepthToTextureBeginD3D12(int i, ID3D12GraphicsCommandList* commandList)
{
	_PointLightRenderTextures[i]->BeginScene(commandList);
	PointLightDepthToTextureSetVieportAndScissorsD3D12(commandList);
}

void TestApplication::PointLightDepthToTextureEndD3D12(int i, ID3D12GraphicsCommandList* commandList)
{
	_PointLightRenderTextures[i]->EndScene(commandList);
}

void TestApplication::PointLightDepthPass(ID3D12GraphicsCommandList* commandList)
{
	commandList->SetGraphicsRootSignature(_RootSignature.Get());
	commandList->SetGraphicsRootDescriptorTable(1, _SamplerLinearWrapGpuHandle);
	commandList->SetPipelineState(_PSOPointLightDepth.Get());

	auto lightEnts = m_Scene->GetAllEntitiesWith<PointLightComponent>();
	bool lightPosChanged = false;
	int i = 0;
	for (auto lightEnt : lightEnts)
	{
		auto& lightPos = m_Scene->GetRegistry().get<TransformComponent>(lightEnt).Translation;
		lightPosChanged = lightPositionsPrev[i] != lightPos;
		if (lightPosChanged)
			break;
		i++;
	}

	if (lightPosChanged || m_LightProps.dynamicShadows)
	{
		int i = 0;
		for (auto lightEnt : lightEnts)
		{
			Entity entity = { lightEnt, m_Scene.get() };
			auto& tc = m_Scene->GetWorldSpaceTransform(entity);

			auto lightPos = tc.Translation;
			auto playerPos = m_Scene->TryGetEntityByTag("Player").GetComponent<TransformComponent>().Translation;
			auto lightRange = m_Scene->GetRegistry().get<PointLightComponent>(lightEnt).Range;

			PreparePointLightViewMatrixes(lightPos);
			PointLightShadowGenCB plsgCb;
			plsgCb.cubeProj = m_PointLightProjMat;
			for (int j = 0; j < 6; j++)
			{
				plsgCb.cubeView[j] = m_PointLightDepthCaptureViews[j];
			}
			_CurrFrameResource->_PointLightShadowGenCB->CopyData(i, plsgCb);
			commandList->SetGraphicsRootDescriptorTable(8, _CurrFrameResource->_PointLightShadowGenCBgpuHandles[i]);
			PointLightDepthToTextureBeginD3D12(i, commandList);
			RenderScene(m_CameraViewMatrix, m_PointLightProjMat, m_PointLightDepthVertexShader, m_PointLightDepthPixelShader, m_PointLightDepthGeometryShader, false, false, {}, nullptr);
			PointLightDepthToTextureEndD3D12(i, commandList);

			lightPositionsPrev[i] = lightPos;
			i += 1;
		}
	}
}

void TestApplication::PointLightDepthToTextureSetVieportAndScissorsD3D12(ID3D12GraphicsCommandList* commandList)
{
	float fShadowMapSize = SHADOWMAP_WIDTH;

	// Set the viewport and scissor rect.
	D3D12_VIEWPORT viewports[6];
	D3D12_RECT scissorRects[6];
	for (int i = 0; i < 6; i++)
	{
		viewports[i] = { 0.0f, 0.0f, fShadowMapSize, fShadowMapSize , D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
		scissorRects[i] = { 0, 0, (long)fShadowMapSize, (long)fShadowMapSize };
	}

	commandList->RSSetViewports(6, viewports);
	commandList->RSSetScissorRects(6, scissorRects);
}

void TestApplication::RenderCubeInitD3D12()
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

	auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
	auto& device = dx12Api->GetDevice();
	auto& commandList = dx12Api->GetCommandList();

	const UINT vbByteSize = 24 * sizeof(Vertex);
	const UINT ibByteSize = 36 * sizeof(uint32_t);

	m_CubeGeometryD3D12 = std::make_unique<MeshGeometry>();
	m_CubeGeometryD3D12->Name = "cubeMeshGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &m_CubeGeometryD3D12->VertexBufferCPU));
	CopyMemory(m_CubeGeometryD3D12->VertexBufferCPU->GetBufferPointer(), vertices, vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &m_CubeGeometryD3D12->IndexBufferCPU));
	CopyMemory(m_CubeGeometryD3D12->IndexBufferCPU->GetBufferPointer(), indices, ibByteSize);

	m_CubeGeometryD3D12->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
		commandList.Get(), vertices, vbByteSize, m_CubeGeometryD3D12->VertexBufferUploader);

	m_CubeGeometryD3D12->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
		commandList.Get(), indices, ibByteSize, m_CubeGeometryD3D12->IndexBufferUploader);

	m_CubeGeometryD3D12->VertexByteStride = sizeof(Vertex);
	m_CubeGeometryD3D12->VertexBufferByteSize = vbByteSize;
	m_CubeGeometryD3D12->IndexFormat = DXGI_FORMAT_R32_UINT;
	m_CubeGeometryD3D12->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = 36;
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	m_CubeGeometryD3D12->DrawArgs["cubeMesh"] = submesh;
}

void TestApplication::RenderCubeD3D12(const glm::mat4& modelMat, uint32_t drawObjectIndex)
{
	MatricesCB mtxCb{};
	mtxCb.modelMat = modelMat;
	mtxCb.viewMat = m_CameraViewMatrix;
	mtxCb.projMat = m_CameraProjMatrix;
	mtxCb.normalMat = glm::transpose(glm::inverse(mtxCb.modelMat));
	mtxCb.projMatInv = glm::inverse(mtxCb.projMat);

	MaterialCB matCb{};
	matCb.color = glm::vec3(1.f);
	matCb.emission = 0.f;
	matCb.notTextured = 1;
	matCb.roughness = 1.f;
	matCb.metallic = 0.f;
	matCb.sponza = 0;
	matCb.albedoOnly = 0;
	matCb.terrain = 0;
	matCb.useNormalMap = 0;
	matCb.objectId = 0;

	_CurrFrameResource->_MatricesCB->CopyData(drawObjectIndex, mtxCb);
	_CurrFrameResource->_MaterialCB->CopyData(drawObjectIndex, matCb);
	//_CurrFrameResource->_SkeletalAnimationCB->CopyData(drawObjectIndex, saCb);

	auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
	auto& commandList = dx12Api->GetCommandList();

	commandList->SetGraphicsRootDescriptorTable(0, _CurrFrameResource->_MatricesCBgpuHandles[drawObjectIndex]);

	auto vertexBufferView = m_CubeGeometryD3D12->VertexBufferView();
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

	auto indexBufferView = m_CubeGeometryD3D12->IndexBufferView();
	commandList->IASetIndexBuffer(&indexBufferView);

	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->DrawIndexedInstanced(m_CubeGeometryD3D12->DrawArgs["cubeMesh"].IndexCount, 1, 0, 0, 0);
}


static bool intersectRayAABB(const glm::vec3& origin, const glm::vec3& direction, const AABB& aabb, float& t_near)
{
	glm::vec3 dirfrac;
	glm::vec3 lb = aabb.min;
	glm::vec3 rt = aabb.max;

	// r.dir is unit direction vector of ray
	dirfrac.x = 1.0f / direction.x;
	dirfrac.y = 1.0f / direction.y;
	dirfrac.z = 1.0f / direction.z;
	// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
	// r.org is origin of ray
	float t1 = (lb.x - origin.x) * dirfrac.x;
	float t2 = (rt.x - origin.x) * dirfrac.x;
	float t3 = (lb.y - origin.y) * dirfrac.y;
	float t4 = (rt.y - origin.y) * dirfrac.y;
	float t5 = (lb.z - origin.z) * dirfrac.z;
	float t6 = (rt.z - origin.z) * dirfrac.z;

	float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
	float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

	// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
	if (tmax < 0)
	{
		t_near = tmax;
		return false;
	}

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax)
	{
		t_near = tmax;
		return false;
	}

	t_near = tmin;
	return true;
}

void TestApplication::MousePickingWithPixelShaderInit()
{
	D3D11_BUFFER_DESC desc{};
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = sizeof(MousePickData);
	desc.StructureByteStride = sizeof(MousePickData);
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	m_DX11Device->CreateBuffer(&desc, nullptr, m_MousePickingBuffer.GetAddressOf());
	m_DX11Device->CreateUnorderedAccessView(m_MousePickingBuffer.Get(), nullptr, m_MousePickingUAV.GetAddressOf());

	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.MiscFlags = 0;

	m_DX11Device->CreateBuffer(&desc, nullptr, m_MousePickingStagingBuffer.GetAddressOf());
}

void TestApplication::MousePickingWithPixelShader()
{
	m_DX11DeviceContext->CopyResource(m_MousePickingStagingBuffer.Get(), m_MousePickingBuffer.Get());
	D3D11_MAPPED_SUBRESOURCE mappedData;
	m_DX11DeviceContext->Map(m_MousePickingStagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &mappedData);

	MousePickData data = *(MousePickData*)mappedData.pData;
	m_MouseHoveringObjectId = data.hoveringObjectId;
	posViewSpaceZ = data.debug.z;

	m_DX11DeviceContext->Unmap(m_MousePickingStagingBuffer.Get(), 0);
}

// Vulkan
void TestApplication::VulkanInit()
{
	auto vkApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();

	for (int i = 0; i < 128; i++)
	{
		_saBuffers[i] = vkApi->CreateBuffer(sizeof(SkeletalAnimationBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO);
		vkApi->_mainDeletionQueue.push_function([=]() {
			std::cout << "Destroying one of _saBuffers\n";
			vkApi->DestroyBuffer(_saBuffers[i]);
			});

		_matBuffers[i] = vkApi->CreateBuffer(sizeof(MaterialBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO);
		vkApi->_mainDeletionQueue.push_function([=]() {
			std::cout << "Destroying one of _matBuffers\n";
			vkApi->DestroyBuffer(_matBuffers[i]);
			});

		_saAndMatBufferSets[i] = vkApi->_globalDescriptorAllocator.Allocate(vkApi->_device, vkApi->_saAndMatBufferLayout);
		DescriptorWriter writer;
		writer.WriteBuffer(0, _saBuffers[i].buffer, sizeof(SkeletalAnimationBuffer), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writer.WriteBuffer(1, _matBuffers[i].buffer, sizeof(MaterialBuffer), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writer.UpdateSet(vkApi->_device, _saAndMatBufferSets[i]);
	}
}

void TestApplication::VulkanFrame()
{
	PrepareLightConstantBuffer();
	RenderScene(m_CameraViewMatrix, m_CameraProjMatrix, m_NullVertexShader, m_NullPixelShader, m_NullGeometryShader, m_bFrustumCulling, false, {}, nullptr);
}

void TestApplication::VulkanBetween()
{
	auto vulkanApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();

	VkCommandBuffer cmd = vulkanApi->GetCurrentFrame()._mainCommandBuffer;

	vkCmdEndRendering(cmd);

	//transition the draw image and the swapchain image into their correct transfer layouts
	VkUtil::TransitionImage(cmd, vulkanApi->_drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	VkUtil::TransitionImage(cmd, vulkanApi->_swapchainImages[vulkanApi->_swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	if (!m_EditorDisabled)
	{
		VkUtil::TransitionImage(cmd, vulkanApi->_imguiImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		VkUtil::CopyImageToImage(cmd, vulkanApi->_drawImage.image, vulkanApi->_imguiImage.image, vulkanApi->_drawExtent, vulkanApi->_drawExtent);
		VkUtil::TransitionImage(cmd, vulkanApi->_imguiImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// set swapchain image layout to Attachment Optimal so we can draw it
		VkUtil::TransitionImage(cmd, vulkanApi->_swapchainImages[vulkanApi->_swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		//draw imgui into the swapchain image
		VkRenderingAttachmentInfo colorAttachment = VkInit::AttachmentInfo(vulkanApi->_swapchainImageViews[vulkanApi->_swapchainImageIndex], nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingInfo renderInfo = VkInit::RenderingInfo(vulkanApi->_swapchainExtent, &colorAttachment, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);
	}
	else
	{
		VkUtil::CopyImageToImage(cmd, vulkanApi->_drawImage.image, vulkanApi->_swapchainImages[vulkanApi->_swapchainImageIndex], vulkanApi->_drawExtent, vulkanApi->_swapchainExtent);
	}
}

void TestApplication::VulkanBeforeImguiFrame()
{
	auto vkApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();

	// wait until the gpu has finished rendering the last frame. Timeout of 1
	// second
	VK_CHECK(vkWaitForFences(vkApi->_device, 1, &vkApi->GetCurrentFrame()._renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(vkApi->_device, 1, &vkApi->GetCurrentFrame()._renderFence));

	//vkApi->GetCurrentFrame()._deletionQueue.flush();
}

void TestApplication::ComputeTestInit()
{
	_computeShader = std::make_shared<DX11ComputeShader>("src/Shaders/Tests/TestComputeShader.hlsl", "CSMain");
	_pixelShader = std::make_shared<DX11PixelShader>("src/Shaders/Tests/TestPixelShader.hlsl");

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = 1024;
	texDesc.Height = 1024;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

	ThrowIfFailed(m_DX11Device->CreateTexture2D(&texDesc, nullptr, &_computeTex));

	ThrowIfFailed(m_DX11Device->CreateUnorderedAccessView(_computeTex, nullptr, &_computeUAV));
	ThrowIfFailed(m_DX11Device->CreateShaderResourceView(_computeTex, nullptr, &_computeSRV));
}

void TestApplication::ComputeTestRun()
{
	m_DX11DeviceContext->CSSetUnorderedAccessViews(0, 1, &_computeUAV, nullptr);
	_computeShader->Bind();
	m_DX11DeviceContext->Dispatch(64, 64, 1);
	ID3D11UnorderedAccessView* nullUAV[] = { nullptr };
	m_DX11DeviceContext->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);
	_computeShader->Unbind();

	PostprocessingToTextureBegin();
	_pixelShader->Bind();
	m_DX11DeviceContext->PSSetShaderResources(0, 1, &_computeSRV);
	m_DX11DeviceContext->PSSetSamplers(0, 1, &m_SamplerLinearClamp);
	RenderFullScreenQuad();
	_pixelShader->Unbind();
	PostprocessingToTextureEnd(postprocessingTexture);
}

void TestApplication::RenderCircle(const glm::mat4& modelMat, bool albedoOnly)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	float radius = 0.5f;
	int numSegments = 64;

	for (int i = 0; i <= numSegments; ++i)
	{
		float angle = 2.0f * XM_PI * (static_cast<float>(i) / numSegments);
		Vertex vertex = {};
		vertex.Pos = glm::vec3(radius * cos(angle), radius * sin(angle), 0.f);
		vertex.Normal = glm::normalize(glm::vec3(0.f, 0.f, 0.f));
		vertex.Tex = glm::vec2(0.f, 0.f);
		vertex.BoneIDs[0] = 101;
		vertex.BoneIDs[1] = 101;
		vertex.BoneIDs[2] = 101;
		vertex.BoneIDs[3] = 101;
		vertex.Weights[0] = 0;
		vertex.Weights[1] = 0;
		vertex.Weights[2] = 0;
		vertex.Weights[3] = 0;
		vertices.push_back(vertex);
		indices.push_back(i);
	}

	ID3D11Buffer* circleVertexBuffer, * circleIndexBuffer;

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Vertex) * vertices.size();
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices.data();

	HRESULT hr = m_DX11Device->CreateBuffer(&bd, &InitData, &circleVertexBuffer);
	if (FAILED(hr)) return;

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(UINT) * indices.size();
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	InitData.pSysMem = indices.data();

	hr = m_DX11Device->CreateBuffer(&bd, &InitData, &circleIndexBuffer);
	if (FAILED(hr)) return;

	MatricesCB mtxCb = {};
	mtxCb.modelMat = modelMat;
	mtxCb.viewMat = m_CameraViewMatrix;
	mtxCb.projMat = m_CameraProjMatrix;
	mtxCb.normalMat = glm::transpose(glm::inverse(mtxCb.modelMat));
	mtxCb.viewMatInv = glm::inverse(mtxCb.viewMat);
	mtxCb.projMatInv = glm::inverse(mtxCb.projMat);

	MaterialCB matCb = {};
	matCb.albedoOnly = albedoOnly ? 1 : 0;
	matCb.sponza = 0.f;
	matCb.objectId = INVALID_ENTITY_ID;
	matCb.notTextured = 1;
	matCb.color = glm::vec3(1.f);
	matCb.roughness = 1.f;
	matCb.metallic = 0.f;
	matCb.terrain = 0;
	matCb.emission = 0.f;
	matCb.useNormalMap = 0;
	matCb.objectOutlinePass = 0;

	m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, nullptr, &mtxCb, 0, 0);
	m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, nullptr, &matCb, 0, 0);

	m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);
	m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);

	m_DX11DeviceContext->VSSetConstantBuffers(1, 1, &g_pCBMaterial);
	m_DX11DeviceContext->PSSetConstantBuffers(1, 1, &g_pCBMaterial);

	m_DX11DeviceContext->VSSetConstantBuffers(3, 1, &g_pCBLight);
	m_DX11DeviceContext->PSSetConstantBuffers(3, 1, &g_pCBLight);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	m_DX11DeviceContext->IASetVertexBuffers(0, 1, &circleVertexBuffer, &stride, &offset);
	m_DX11DeviceContext->IASetIndexBuffer(circleIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_DX11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	m_DX11DeviceContext->DrawIndexed(indices.size(), 0, 0);

	circleVertexBuffer->Release();
	circleIndexBuffer->Release();
}

void TestApplication::RenderSphere(int segments, const glm::mat4& modelMat, const glm::vec3& color, float metallic, float roughness, bool albedoOnly)
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	const float radius = 0.5f;

	const unsigned int X_SEGMENTS = segments;
	const unsigned int Y_SEGMENTS = segments;
	const float PI = 3.14159265359f;
	for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
	{
		for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = radius * std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
			float yPos = radius * std::cos(ySegment * PI);
			float zPos = radius * std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

			Vertex vertex = {};
			vertex.Pos = glm::vec3(xPos, yPos, zPos);
			vertex.Normal = glm::normalize(glm::vec3(xPos, yPos, zPos));
			vertex.Tex = glm::vec2(xSegment, ySegment);
			vertex.BoneIDs[0] = 101;
			vertex.BoneIDs[1] = 101;
			vertex.BoneIDs[2] = 101;
			vertex.BoneIDs[3] = 101;
			vertex.Weights[0] = 0;
			vertex.Weights[1] = 0;
			vertex.Weights[2] = 0;
			vertex.Weights[3] = 0;
			vertices.push_back(vertex);
		}
	}

	bool oddRow = false;
	for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
	{
		if (!oddRow) // even rows: y == 0, y == 2; and so on
		{
			for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
			{
				indices.push_back(y * (X_SEGMENTS + 1) + x);
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				indices.push_back(y * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
	}

	ID3D11Buffer* sphereVertexBuffer, * sphereIndexBuffer;

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Vertex) * vertices.size();
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices.data();

	HRESULT hr = m_DX11Device->CreateBuffer(&bd, &InitData, &sphereVertexBuffer);
	if (FAILED(hr)) return;

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(UINT) * indices.size();
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	InitData.pSysMem = indices.data();

	hr = m_DX11Device->CreateBuffer(&bd, &InitData, &sphereIndexBuffer);
	if (FAILED(hr)) return;

	MatricesCB mtxCb = {};
	mtxCb.modelMat = modelMat;
	mtxCb.viewMat = m_CameraViewMatrix;
	mtxCb.projMat = m_CameraProjMatrix;
	mtxCb.normalMat = glm::transpose(glm::inverse(mtxCb.modelMat));
	mtxCb.viewMatInv = glm::inverse(mtxCb.viewMat);
	mtxCb.projMatInv = glm::inverse(mtxCb.projMat);

	MaterialCB matCb = {};
	matCb.albedoOnly = albedoOnly ? 1 : 0;
	matCb.sponza = 0.f;
	matCb.objectId = INVALID_ENTITY_ID;
	matCb.notTextured = 1;
	matCb.color = color;
	matCb.roughness = roughness;
	matCb.metallic = metallic;
	matCb.terrain = 0;
	matCb.emission = 0.f;
	matCb.useNormalMap = 0;
	matCb.objectOutlinePass = 0;

	m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, nullptr, &mtxCb, 0, 0);
	m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, nullptr, &matCb, 0, 0);

	m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);
	m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);
	m_DX11DeviceContext->VSSetConstantBuffers(1, 1, &g_pCBMaterial);
	m_DX11DeviceContext->PSSetConstantBuffers(1, 1, &g_pCBMaterial);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	m_DX11DeviceContext->IASetVertexBuffers(0, 1, &sphereVertexBuffer, &stride, &offset);
	m_DX11DeviceContext->IASetIndexBuffer(sphereIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_DX11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_DX11DeviceContext->DrawIndexed(indices.size(), 0, 0);

	sphereVertexBuffer->Release();
	sphereIndexBuffer->Release();
}

void TestApplication::RenderGridInit()
{
	std::vector<Vertex> vertices;

	for (int i = 0; i <= m_GridLines; i++)
	{
		float z = 1.0f - (2.f / m_GridLines) * i;
		Vertex v1 = { glm::vec3(-1.0f, 0.0f, z),  glm::vec2(0.0f, 0.0f), glm::vec3(0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f} };
		Vertex v2 = { glm::vec3(1.0f, 0.0f, z),  glm::vec2(0.0f, 0.0f), glm::vec3(0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f} };

		vertices.push_back(v1);
		vertices.push_back(v2);

		float x = 1.0f - (2.f / m_GridLines) * i;
		Vertex v3 = { glm::vec3(x, 0.0f, -1.f),  glm::vec2(0.0f, 0.0f), glm::vec3(0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f} };
		Vertex v4 = { glm::vec3(x, 0.0f, 1.f),  glm::vec2(0.0f, 0.0f), glm::vec3(0.0f), { 101, 101, 101, 101 }, { 0.f, 0.f, 0.f, 0.f} };

		vertices.push_back(v3);
		vertices.push_back(v4);
	}

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Vertex) * vertices.size();
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices.data();

	HRESULT hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pGridVertexBuffer);
	if (FAILED(hr)) return;

	std::vector<unsigned> indices;

	for (int i = 0; i < m_GridLines * 4 + 4; i++)
	{
		indices.push_back(i);
	}

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(unsigned) * indices.size();
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	InitData.pSysMem = indices.data();

	hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pGridIndexBuffer);
	if (FAILED(hr)) return;
}

void TestApplication::RenderGrid()
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	m_DX11DeviceContext->IASetVertexBuffers(0, 1, &pGridVertexBuffer, &stride, &offset);
	m_DX11DeviceContext->IASetIndexBuffer(pGridIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_DX11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	MatricesCB cb = {};
	cb.modelMat = glm::scale(glm::mat4(1.f), glm::vec3(100.f));
	cb.viewMat = m_CameraViewMatrix;
	cb.projMat = m_CameraProjMatrix;
	cb.normalMat = glm::transpose(glm::inverse(cb.modelMat));

	m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, nullptr, &cb, 0, 0);
	m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);

	MaterialCB matCb = {};
	matCb.sponza = 0.f;

	matCb.notTextured = 1;
	matCb.color = glm::vec3(1.f);
	matCb.albedoOnly = 1;

	matCb.metallic = 0.f;
	matCb.roughness = 1.f;
	matCb.terrain = 0;
	matCb.objectId = INVALID_ENTITY_ID;
	matCb.objectOutlinePass = 0;
	matCb.emission = 0.f;
	matCb.useNormalMap = 0;

	m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, nullptr, &matCb, 0, 0);
	m_DX11DeviceContext->PSSetConstantBuffers(1, 1, &g_pCBMaterial);

	m_DX11DeviceContext->DrawIndexed(m_GridLines * 4 + 4, 0, 0);
}

void TestApplication::DrawSelectedModelOutline()
{
	const bool frustumCulling = true;

	const glm::mat4& VP = m_CameraProjMatrix * m_CameraViewMatrix;

	Entity entity = m_SceneHierarchyPanel->GetSelectedEntity();

	if (!entity || !entity.HasComponent<MeshComponent>())
		return;

	auto& transformComponent = m_Scene->GetWorldSpaceTransform(entity);
	auto& meshTransform = m_Scene->GetWorldSpaceTransformMatrix(entity);
	auto& meshComponent = entity.GetComponent<MeshComponent>();
	auto& model = meshComponent.m_Model;

	MatricesCB mtxCb = {};
	mtxCb.modelMat = meshTransform;

	auto& tag = entity.GetComponent<TagComponent>().Tag;

	if (tag == "Colt")           /// attaching pistol to the right hand
	{
		auto& playerModel = m_Scene->TryGetEntityByTag("Player").GetComponent<MeshComponent>().m_Model;

		if (playerModel && PLAYER_MODEL == 0)
		{
			auto& boneInfoMap = playerModel->GetBoneInfoMap();
			auto it = boneInfoMap.find("mixamorig:RightHand");
			if (it == boneInfoMap.end()) {
				std::cout << "Bone not found!\n";
				m_Running = false;
			}
			else {
				auto characterMat = playerModelMat;

				auto gunPos = glm::vec3(4.5f, 10.5f, 1.225f);
				auto translate = gunPos;
				auto rotate = glm::vec3(0.f, 90.f, 0.f);
				auto scale = transformComponent.Scale;
				auto modelMat = glm::mat4(1.f);
				modelMat = glm::translate(modelMat, glm::vec3(translate[0], translate[1], translate[2]));
				modelMat = glm::rotate(modelMat, glm::radians(rotate[0]), glm::vec3(1.f, 0.f, 0.f));
				modelMat = glm::rotate(modelMat, glm::radians(rotate[1]), glm::vec3(0.f, 1.f, 0.f));
				modelMat = glm::rotate(modelMat, glm::radians(rotate[2]), glm::vec3(0.f, 0.f, 1.f));
				modelMat = glm::scale(modelMat, glm::vec3(scale[0], scale[1], scale[2]));
				modelMat = glm::scale(modelMat, glm::vec3(15.f, 15.f, 15.f));

				BoneInfo value = it->second;
				glm::mat4 boneMat = value.offset;

				auto childFinalBoneMatrix = m_Animator->GetFinalBoneMatrices()[value.id];

				glm::mat4 finalBoneMatrix = childFinalBoneMatrix * glm::inverse(boneMat);

				mtxCb.modelMat = characterMat * finalBoneMatrix * modelMat;
			}
		}
	}

	mtxCb.viewMat = m_CameraViewMatrix;
	mtxCb.projMat = m_CameraProjMatrix;
	mtxCb.normalMat = glm::transpose(glm::inverse(mtxCb.modelMat));
	mtxCb.projMatInv = glm::inverse(mtxCb.projMat);

	MaterialCB matCb = {};
	matCb.albedoOnly = 1;
	matCb.objectId = (uint32_t)entity;
	matCb.emission = 0.f;
	matCb.notTextured = 1;
	matCb.color = glm::vec3(1.f, 0.64705f, 0.f);
	matCb.roughness = 1.f;
	matCb.metallic = 0.f;

	matCb.terrain = 0;
	matCb.cubemapLod = 0.f;
	matCb.objectOutlinePass = 0;

	SkeletalAnimationCB saCb = {};
	if (tag == "Player" && m_Animator)
	{
		auto finalBoneMatrices = m_Animator->GetFinalBoneMatrices();
		for (int i = 0; i < finalBoneMatrices.size(); i++)
		{
			saCb.finalBonesMatrices[i] = finalBoneMatrices[i];
		}
	}
	else if (tag == "Zombie" && m_ZombieAnimator)
	{
		auto finalBoneMatrices = m_ZombieAnimator->GetFinalBoneMatrices();
		for (int i = 0; i < finalBoneMatrices.size(); i++)
		{
			saCb.finalBonesMatrices[i] = finalBoneMatrices[i];
		}
	}
	else if (tag.compare(0, 4, "Dude") == 0)
	{
		auto finalBoneMatrices = m_DudeAnimators[0]->GetFinalBoneMatrices();
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

	if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
	{
		m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, NULL, &mtxCb, 0, 0);
		m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, NULL, &matCb, 0, 0);
		m_DX11DeviceContext->UpdateSubresource(g_pCBSkeletalAnimation, 0, NULL, &saCb, 0, 0);

		m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);
		m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);
		m_DX11DeviceContext->VSSetConstantBuffers(1, 1, &g_pCBMaterial);
		m_DX11DeviceContext->PSSetConstantBuffers(1, 1, &g_pCBMaterial);
		m_DX11DeviceContext->VSSetConstantBuffers(2, 1, &g_pCBSkeletalAnimation);

		m_DX11DeviceContext->PSSetSamplers(1, 1, &m_SamplerLinearClamp);
		m_DX11DeviceContext->PSSetSamplers(2, 1, &m_SamplerLinearClampComparison);
	}

	glm::mat4 proj = mtxCb.projMat;
	proj[1][1] *= -1;
	glm::mat4 transform = proj * mtxCb.viewMat * mtxCb.modelMat;

	//m_DX11DeviceContext->RSSetState(m_CullFrontRasterizerState.Get());

	if (frustumCulling && tag != "Player")
	{
		model->Draw(VP * mtxCb.modelMat, true, transform, false, {});
	}
	else
	{
		model->Draw(glm::mat4(1.f), false, transform, false, {});
	}

	//m_DX11DeviceContext->RSSetState(m_DefaultRasterizerState);
}

// OpenGL
void TestApplication::RenderCubeOGL()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			 // bottom face
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			  1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 // top face
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			  1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			  1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

void TestApplication::RenderQuadOGL()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void TestApplication::RenderSphereOGL()
{
	if (sphereVAO == 0)
	{
		glGenVertexArrays(1, &sphereVAO);

		unsigned int vbo, ebo;
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> uv;
		std::vector<glm::vec3> normals;
		std::vector<unsigned int> indices;

		const unsigned int X_SEGMENTS = 64;
		const unsigned int Y_SEGMENTS = 64;
		const float PI = 3.14159265359f;
		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
			{
				float xSegment = (float)x / (float)X_SEGMENTS;
				float ySegment = (float)y / (float)Y_SEGMENTS;
				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				float yPos = std::cos(ySegment * PI);
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

				positions.push_back(glm::vec3(xPos, yPos, zPos));
				uv.push_back(glm::vec2(xSegment, ySegment));
				normals.push_back(glm::vec3(xPos, yPos, zPos));
			}
		}

		bool oddRow = false;
		for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
		{
			if (!oddRow) // even rows: y == 0, y == 2; and so on
			{
				for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
				{
					indices.push_back(y * (X_SEGMENTS + 1) + x);
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				}
			}
			else
			{
				for (int x = X_SEGMENTS; x >= 0; --x)
				{
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					indices.push_back(y * (X_SEGMENTS + 1) + x);
				}
			}
			oddRow = !oddRow;
		}
		sphereIndexCount = static_cast<GLsizei>(indices.size());

		std::vector<float> data;
		for (unsigned int i = 0; i < positions.size(); ++i)
		{
			data.push_back(positions[i].x);
			data.push_back(positions[i].y);
			data.push_back(positions[i].z);
			if (normals.size() > 0)
			{
				data.push_back(normals[i].x);
				data.push_back(normals[i].y);
				data.push_back(normals[i].z);
			}
			if (uv.size() > 0)
			{
				data.push_back(uv[i].x);
				data.push_back(uv[i].y);
			}
		}
		glBindVertexArray(sphereVAO);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
		unsigned int stride = (3 + 2 + 3) * sizeof(float);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
	}

	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, sphereIndexCount, GL_UNSIGNED_INT, 0);
}

void TestApplication::OpenGLSetupCSM()
{
	// configure light FBO
	// -----------------------
	glGenFramebuffers(1, &lightFBO);

	glGenTextures(1, &lightDepthMaps);
	glBindTexture(GL_TEXTURE_2D_ARRAY, lightDepthMaps);
	glTexImage3D(
		GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, depthMapResolution, depthMapResolution, int(shadowCascadeLevels.size()) + 1,
		0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);

	glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, lightDepthMaps, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!";
		throw 0;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// configure UBO
	// --------------------
	glGenBuffers(1, &matricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4x4) * 16, nullptr, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void TestApplication::OpenGLCSMPass()
{
	const auto& lightMatrices = GetLightSpaceMatrices();
	glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
	for (size_t i = 0; i < lightMatrices.size(); ++i)
	{
		glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4x4), sizeof(glm::mat4x4), &lightMatrices[i]);
	}
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	simpleDepthShader->Bind();
	glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
	glViewport(0, 0, depthMapResolution, depthMapResolution);
	glClear(GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_FRONT);  // peter panning
	RenderScene(m_CameraViewMatrix, m_CameraProjMatrix, m_NullVertexShader, m_NullPixelShader, m_NullGeometryShader, false, false, {}, simpleDepthShader);
	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TestApplication::OpenGLSetupIBL(const char* hdriMapPath)
{
	glDepthFunc(GL_LEQUAL);
	// pbr: setup framebuffer
// ----------------------
	unsigned int captureFBO;
	unsigned int captureRBO;
	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

	// pbr: load the HDR environment map
	// ---------------------------------
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrComponents;
	float* data = stbi_loadf(hdriMapPath, &width, &height, &nrComponents, 0);
	unsigned int hdrTexture;
	if (data)
	{
		glGenTextures(1, &hdrTexture);
		glBindTexture(GL_TEXTURE_2D, hdrTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Failed to load HDR image." << std::endl;
	}

	// pbr: setup cubemap to render to and attach to framebuffer
	// ---------------------------------------------------------

	glGenTextures(1, &envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // enable pre-filter mipmap sampling (combatting visible dots artifact)
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
	// ----------------------------------------------------------------------------------------------
	glm::mat4 captureProjection = glm::perspectiveRH_NO(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
		glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	// pbr: convert HDR equirectangular environment map to cubemap equivalent
	// ----------------------------------------------------------------------
	equirectangularToCubemapShader->Bind();
	equirectangularToCubemapShader->SetInt("equirectangularMap", 0);
	equirectangularToCubemapShader->SetMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrTexture);

	glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		equirectangularToCubemapShader->SetMat4("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RenderCubeOGL();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
	// --------------------------------------------------------------------------------

	glGenTextures(1, &irradianceMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

	// pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
	// -----------------------------------------------------------------------------
	irradianceShader->Bind();
	irradianceShader->SetInt("environmentMap", 0);
	irradianceShader->SetMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		irradianceShader->SetMat4("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RenderCubeOGL();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
	// --------------------------------------------------------------------------------

	glGenTextures(1, &prefilterMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
	// ----------------------------------------------------------------------------------------------------
	prefilterShader->Bind();
	prefilterShader->SetInt("environmentMap", 0);
	prefilterShader->SetMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	unsigned int maxMipLevels = 5;
	for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
	{
		// reisze framebuffer according to mip-level size.
		unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
		unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(maxMipLevels - 1);
		prefilterShader->SetFloat("roughness", roughness);
		for (unsigned int i = 0; i < 6; ++i)
		{
			prefilterShader->SetMat4("view", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			RenderCubeOGL();
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// pbr: generate a 2D LUT from the BRDF equations used.
	// ----------------------------------------------------

	glGenTextures(1, &brdfLUTTexture);

	// pre-allocate enough memory for the LUT texture.
	glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
	// be sure to set wrapping mode to GL_CLAMP_TO_EDGE
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

	glViewport(0, 0, 512, 512);
	brdfShader->Bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	RenderQuadOGL();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &captureFBO);
	glDeleteRenderbuffers(1, &captureRBO);

	glDepthFunc(GL_LESS);
}

void TestApplication::OpenGLChangeIBL(const char* hdriMapPath)
{
	glDeleteTextures(1, &brdfLUTTexture);
	glDeleteTextures(1, &prefilterMap);
	glDeleteTextures(1, &irradianceMap);
	glDeleteTextures(1, &envCubemap);
	glDisable(GL_CULL_FACE);
	OpenGLSetupIBL(hdriMapPath);
	glEnable(GL_CULL_FACE);
}

void TestApplication::OpenGLInit()
{
	m_RenderTextureOGL = std::make_unique<RenderTextureOGL>();
	m_RenderTextureOGL->Initialize(texWidth, texHeight, 0, GL_RGBA16F, GL_RGBA);

	// build and compile shaders
	pbrShader = std::make_shared<OGLShader>("src/Shaders/GLSL/pbr.vert", "src/Shaders/GLSL/pbr.frag");
	equirectangularToCubemapShader = std::make_shared<OGLShader>("src/Shaders/GLSL/cubemap.vert", "src/Shaders/GLSL/equirectangular_to_cubemap.frag");
	irradianceShader = std::make_shared<OGLShader>("src/Shaders/GLSL/cubemap.vert", "src/Shaders/GLSL/irradiance_convolution.frag");
	prefilterShader = std::make_shared<OGLShader>("src/Shaders/GLSL/cubemap.vert", "src/Shaders/GLSL/prefilter.frag");
	brdfShader = std::make_shared<OGLShader>("src/Shaders/GLSL/brdf.vert", "src/Shaders/GLSL/brdf.frag");
	backgroundShader = std::make_shared<OGLShader>("src/Shaders/GLSL/background.vert", "src/Shaders/GLSL/background.frag");

	pbrShader->Bind();
	pbrShader->SetInt("irradianceMap", 0);
	pbrShader->SetInt("prefilterMap", 1);
	pbrShader->SetInt("brdfLUT", 2);
	pbrShader->SetInt("albedoMap", 3);
	pbrShader->SetInt("normalMap", 4);
	pbrShader->SetInt("metallicMap", 5);
	pbrShader->SetInt("roughnessMap", 6);
	pbrShader->SetInt("aoMap", 7);

	backgroundShader->Bind();
	backgroundShader->SetInt("environmentMap", 0);

	ironAlbedoMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/rusted_iron/albedo.png");
	ironNormalMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/rusted_iron/normal.png");
	ironMetallicMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/rusted_iron/metallic.png");
	ironRoughnessMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/rusted_iron/roughness.png");
	ironAOMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/rusted_iron/ao.png");

	goldAlbedoMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/gold/albedo.png");
	goldNormalMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/gold/normal.png");
	goldMetallicMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/gold/metallic.png");
	goldRoughnessMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/gold/roughness.png");
	goldAOMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/gold/ao.png");

	grassAlbedoMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/grass/albedo.png");
	grassNormalMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/grass/normal.png");
	grassMetallicMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/grass/metallic.png");
	grassRoughnessMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/grass/roughness.png");
	grassAOMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/grass/ao.png");

	plasticAlbedoMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/plastic/albedo.png");
	plasticNormalMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/plastic/normal.png");
	plasticMetallicMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/plastic/metallic.png");
	plasticRoughnessMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/plastic/roughness.png");
	plasticAOMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/plastic/ao.png");

	wallAlbedoMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/wall/albedo.png");
	wallNormalMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/wall/normal.png");
	wallMetallicMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/wall/metallic.png");
	wallRoughnessMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/wall/roughness.png");
	wallAOMap = ResourceManager::GetOrCreateTexture("../resources/textures/pbr/wall/ao.png");

	// configure global opengl state
   // -----------------------------
	glEnable(GL_DEPTH_TEST);
	// enable seamless cubemap sampling for lower mip levels in the pre-filter map.
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	OpenGLSetupIBL("../resources/hdris/newport_loft.hdr");
	glEnable(GL_CULL_FACE);

	// Cascaded shadow maps
	simpleDepthShader = std::make_shared<OGLShader>("src/Shaders/GLSL/shadow_mapping_depth.vert", "src/Shaders/GLSL/shadow_mapping_depth.frag", "src/Shaders/GLSL/shadow_mapping_depth.geom");
	OpenGLSetupCSM();
}

void TestApplication::OpenGLFrame()
{
	// render shadows
	OpenGLCSMPass();

	m_RenderTextureOGL->SetRenderTarget(false, false);
	m_RenderTextureOGL->ClearRenderTarget(0.f, 0.f, 0.f, 1.f, 1.f);
	glViewport(0, 0, texWidth, texHeight);
	// render scene, supplying the convoluted irradiance map to the final shader.
	// ------------------------------------------------------------------------------------------
	pbrShader->Bind();
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = m_CameraViewMatrix;
	glm::mat4 proj = m_CameraProjMatrix;

	pbrShader->SetMat4("view", view);
	pbrShader->SetFloat3("camPos", m_CameraPos);
	pbrShader->SetMat4("projection", proj);

	pbrShader->SetInt("shadowMap", 8);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D_ARRAY, lightDepthMaps);

	auto sunLightEntity = m_Scene->TryGetEntityByTag("SunLight");
	if (sunLightEntity)
	{
		m_SunLightDir = glm::normalize(sunLightEntity.GetComponent<TransformComponent>().Rotation);
	}
	pbrShader->SetFloat3("lightDir", m_SunLightDir);
	pbrShader->SetFloat("farPlane", CAM_FAR_PLANE);
	pbrShader->SetInt("cascadeCount", shadowCascadeLevels.size());
	for (size_t i = 0; i < shadowCascadeLevels.size(); ++i)
	{
		pbrShader->SetFloat("cascadePlaneDistances[" + std::to_string(i) + "]", shadowCascadeLevels[i]);
	}
	pbrShader->SetFloat("sunlightPower", m_SunLightPower);
	pbrShader->SetFloat2("matMetalRough", glm::vec2(1.f));
	pbrShader->SetInt("showCascades", m_ShowCasades ? 1 : 0);

	// bind pre-computed IBL data
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);

	// rusted iron
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, ironAlbedoMap->GetRendererID());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, ironNormalMap->GetRendererID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, ironMetallicMap->GetRendererID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, ironRoughnessMap->GetRendererID());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, ironAOMap->GetRendererID());

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-5.0, 0.0, 2.0));
	pbrShader->SetMat4("model", model);
	pbrShader->SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
	RenderSphereOGL();

	// gold
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, goldAlbedoMap->GetRendererID());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, goldNormalMap->GetRendererID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, goldMetallicMap->GetRendererID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, goldRoughnessMap->GetRendererID());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, goldAOMap->GetRendererID());

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-3.0, 0.0, 2.0));
	pbrShader->SetMat4("model", model);
	pbrShader->SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
	RenderSphereOGL();

	// grass
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, grassAlbedoMap->GetRendererID());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, grassNormalMap->GetRendererID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, grassMetallicMap->GetRendererID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, grassRoughnessMap->GetRendererID());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, grassAOMap->GetRendererID());

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-1.0, 0.0, 2.0));
	pbrShader->SetMat4("model", model);
	pbrShader->SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
	RenderSphereOGL();

	// plastic
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, plasticAlbedoMap->GetRendererID());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, plasticNormalMap->GetRendererID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, plasticMetallicMap->GetRendererID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, plasticRoughnessMap->GetRendererID());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, plasticAOMap->GetRendererID());

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(1.0, 0.0, 2.0));
	pbrShader->SetMat4("model", model);
	pbrShader->SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
	RenderSphereOGL();

	// wall
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, wallAlbedoMap->GetRendererID());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, wallNormalMap->GetRendererID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, wallMetallicMap->GetRendererID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, wallRoughnessMap->GetRendererID());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, wallAOMap->GetRendererID());

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(3.0, 0.0, 2.0));
	pbrShader->SetMat4("model", model);
	pbrShader->SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
	RenderSphereOGL();

	const auto& lightEnts = m_Scene->GetAllEntitiesWith<PointLightComponent>();
	pbrShader->SetInt("lightCount", lightEnts.size());
	int i = 0;
	for (const auto& lightEnt : lightEnts)
	{
		Entity entity = { lightEnt, m_Scene.get() };
		const auto& lightComp = entity.GetComponent<PointLightComponent>();
		const auto& trsComp = entity.GetComponent<TransformComponent>();

		pbrShader->SetFloat3("lightPositions[" + std::to_string(i) + "]", trsComp.Translation);
		pbrShader->SetFloat3("lightColors[" + std::to_string(i) + "]", lightComp.Color);
		pbrShader->SetFloat("lightIntensities[" + std::to_string(i) + "]", lightComp.Intensity);
		pbrShader->SetFloat("lightRanges[" + std::to_string(i) + "]", lightComp.Range);
		i++;
	}

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, goldAlbedoMap->GetRendererID());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, goldNormalMap->GetRendererID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, goldMetallicMap->GetRendererID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, goldRoughnessMap->GetRendererID());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, goldAOMap->GetRendererID());
	BodyIDVector bodies;
	m_Physics.physics_system->GetBodies(bodies);
	for (const auto& body : bodies)
	{
		RMat44 worldTransform = m_Physics.physics_system->GetBodyInterface().GetWorldTransform(body);
		glm::mat4 worldMatrix = *reinterpret_cast<glm::mat4*>(&worldTransform);
		pbrShader->SetMat4("model", worldMatrix);
		pbrShader->SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(worldMatrix))));
		RenderCubeOGL();
	}

	RenderScene(m_CameraViewMatrix, m_CameraProjMatrix, m_NullVertexShader, m_NullPixelShader, m_NullGeometryShader, m_bFrustumCulling, false, {}, pbrShader);

	// render skybox (render as last to prevent overdraw)
	backgroundShader->Bind();
	backgroundShader->SetMat4("view", view);
	backgroundShader->SetMat4("projection", proj);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap); // display irradiance map
	//glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap); // display prefilter map
	glDepthFunc(GL_LEQUAL);
	glCullFace(GL_FRONT);
	RenderCubeOGL();
	glDepthFunc(GL_LESS);
	glCullFace(GL_BACK);

	// render BRDF map to screen
	//brdfShader->Bind();
	//RenderQuadOGL();

	((OGLRendererAPI*)RendererAPI::GetInstance().get())->ResetRenderTarget();
}

void TestApplication::RenderInfiniteGrid()
{
	MatricesCB cb = {};
	cb.modelMat = glm::mat4(1.f);
	cb.viewMat = m_CameraViewMatrix;
	cb.projMat = m_CameraProjMatrix;
	cb.normalMat = glm::transpose(glm::inverse(cb.modelMat));

	m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, nullptr, &cb, 0, 0);
	m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);

	m_DX11DeviceContext->VSSetConstantBuffers(3, 1, &g_pCBLight);

	m_DX11DeviceContext->IASetInputLayout(NULL);
	m_DX11DeviceContext->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
	m_DX11DeviceContext->IASetIndexBuffer(NULL, DXGI_FORMAT_UNKNOWN, 0);
	m_DX11DeviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_InfiniteGridVertexShader->Bind();
	m_InfiniteGridPixelShader->Bind();

	m_DX11DeviceContext->OMSetBlendState(m_InfiniteGridBlendState.Get(), nullptr, 0xFFFFFFFF);

	m_DX11DeviceContext->Draw(6, 0);

	m_DX11DeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

	m_DeferredVertexShader->Bind();
	m_DeferredPixelShader->Bind();
}

void TestApplication::RenderModelBounds(const std::string& tag)
{
	auto& entity = m_Scene->TryGetEntityByTag(tag);
	auto& model = m_Scene->GetRegistry().get<MeshComponent>(entity).m_Model;
	auto& modelTransform = m_Scene->GetRegistry().get<TransformComponent>(entity);

	auto& meshes = model->GetMeshes();
	for (auto& mesh : meshes)
	{
		auto bound = mesh.GetBounds();

		auto width = abs(bound.max.x - bound.min.x);
		auto height = abs(bound.max.y - bound.min.y);
		auto depth = abs(bound.max.z - bound.min.z);

		auto lowerDimension = std::min({ width, height, depth });

		ID3D11Buffer* pBoundVertexBuffer, * pBoundIndexBuffer;

		Vertex vertices[] =
		{
			{ glm::vec3(bound.min.x, bound.max.y, bound.max.z), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
			{ glm::vec3(bound.max.x, bound.max.y, bound.max.z), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
			{ glm::vec3(bound.max.x, bound.min.y, bound.max.z), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
			{ glm::vec3(bound.min.x, bound.min.y, bound.max.z), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
			{ glm::vec3(bound.min.x, bound.max.y, bound.min.z), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
			{ glm::vec3(bound.max.x, bound.max.y, bound.min.z), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
			{ glm::vec3(bound.max.x, bound.min.y, bound.min.z), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
			{ glm::vec3(bound.min.x, bound.min.y, bound.min.z), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 101, 0.f},
		};

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(Vertex) * 8;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = vertices;

		HRESULT hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pBoundVertexBuffer);
		if (FAILED(hr)) return;

		UINT indices[] =
		{
			0, 1, 2, 3, 0, 4, 5, 1, 0, 4, 7, 3, 0, 1, 2, 6, 5, 6, 7, 4
		};


		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(UINT) * 20;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;

		InitData.pSysMem = indices;

		hr = m_DX11Device->CreateBuffer(&bd, &InitData, &pBoundIndexBuffer);
		if (FAILED(hr)) return;

		auto modelMat = glm::mat4(1.f);
		modelMat = m_Scene->GetWorldSpaceTransformMatrix(entity);

		if (tag == "Colt")
			modelMat = coltMat;

		UINT stride = sizeof(Vertex);
		UINT offset = 0;

		m_DX11DeviceContext->IASetVertexBuffers(0, 1, &pBoundVertexBuffer, &stride, &offset);
		m_DX11DeviceContext->IASetIndexBuffer(pBoundIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		m_DX11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

		MatricesCB cb{};
		cb.modelMat = modelMat;
		cb.viewMat = m_CameraViewMatrix;
		cb.projMat = m_CameraProjMatrix;
		cb.normalMat = glm::transpose(glm::inverse(cb.modelMat));

		m_DX11DeviceContext->UpdateSubresource(g_pCBMatrixes, 0, nullptr, &cb, 0, 0);
		m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);

		MaterialCB matCb{};
		matCb.sponza = 0.f;
		matCb.terrain = 0;
		matCb.notTextured = 1;
		matCb.color = glm::vec3(0.f, 1.f, 0.f);
		matCb.metallic = 0.f;
		matCb.roughness = 1.f;
		matCb.objectId = 0;
		matCb.useNormalMap = 0;
		matCb.cubemapLod = 0;
		matCb.objectOutlinePass = 0;

		m_DX11DeviceContext->UpdateSubresource(g_pCBMaterial, 0, nullptr, &matCb, 0, 0);
		m_DX11DeviceContext->PSSetConstantBuffers(2, 1, &g_pCBMaterial);

		m_DX11DeviceContext->DrawIndexed(20, 0, 0);

		pBoundVertexBuffer->Release();
		pBoundIndexBuffer->Release();
	}
}



bool TestApplication::Raycast(const glm::vec3& start, const glm::vec3& dir, glm::vec3& outHit, glm::vec3& outNormal)
{
	auto cmp = [](const RaycastResult& a, const RaycastResult& b) { return a.tNear < b.tNear; };
	std::set<RaycastResult, decltype(cmp)> results(cmp);
	auto meshEnts = m_Scene->GetAllEntitiesWith<MeshComponent>();

	for (auto meshEnt : meshEnts)
	{
		Entity entity = { meshEnt, m_Scene.get() };
		auto& meshComponent = m_Scene->GetRegistry().get<MeshComponent>(meshEnt);
		auto& model = meshComponent.m_Model;

		auto aabb = model->GetBounds();

		auto lower = aabb.min;
		auto upper = aabb.max;

		auto modelMatrix = m_Scene->GetWorldSpaceTransformMatrix(entity);

		//aabb.min = glm::vec3(modelMatrix * glm::vec4(lower.x, lower.y, lower.z, 1.f));
		//aabb.max = glm::vec3(modelMatrix * glm::vec4(upper.x, upper.y, upper.z, 1.f));

		glm::mat4 inverse = glm::inverse(modelMatrix);
		glm::vec3 origin = glm::vec3(inverse * glm::vec4(start, 1.0f));
		glm::vec3 direction = glm::normalize(glm::vec3(inverse * glm::vec4(dir, 0.0f)));

		float tNear;
		glm::vec3 hitPos;
		glm::vec3 hitNormal;
		if (intersectRayAABB(origin, direction, aabb, tNear))
		{
			hitPos = origin + direction * tNear;
			hitPos = glm::vec3(modelMatrix * glm::vec4(hitPos, 1.f));
			results.insert({ tNear, hitPos, hitNormal });
		}
	}
	//Entity entity = { (entt::entity)objects.begin()->first, m_Scene.get() };
	outHit = results.begin()->hitPos;
	outNormal = results.begin()->hitNormal;

	return !results.empty();
}

void TestApplication::AddBulletHit()
{
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;

	glm::decompose(coltMat, scale, rotation, translation, skew, perspective);

	glm::vec3 rayStart = translation + rotation * glm::vec3(0.f, 8.f, 0.f);
	glm::vec3 rayDir = glm::normalize(rayStart + rotation * glm::vec3(0.f, 500.f, 0.f));

	glm::vec3 hit;
	glm::vec3 normal;
	if (Raycast(rayStart, rayDir, hit, normal))
	{
		m_BulletHits.push_back({ 0.f, hit, normal });
	}
}

void TestApplication::UpdateCascades()
{
	float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

	float nearClip = CAM_NEAR_PLANE;
	float farClip = CAM_FAR_PLANE;
	float clipRange = farClip - nearClip;

	float minZ = nearClip;
	float maxZ = nearClip + clipRange;

	float range = maxZ - minZ;
	float ratio = maxZ / minZ;

	// Calculate split depths based on view camera frustum
	// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
		float log = minZ * std::pow(ratio, p);
		float uniform = minZ + range * p;
		float d = CascadeSplitLambda * (log - uniform) + uniform;
		cascadeSplits[i] = (d - nearClip) / clipRange;
	}

	cascadeSplits[3] = 0.3f;

	// Calculate orthographic projection matrix for each cascade
	float lastSplitDist = 0.0;
	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
	{
		float splitDist = cascadeSplits[i];

		glm::vec3 frustumCorners[8] = {
			glm::vec3(-1.0f,  1.0f, 0.0f),
			glm::vec3(1.0f,  1.0f, 0.0f),
			glm::vec3(1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f,  1.0f, 1.0f),
			glm::vec3(1.0f,  1.0f, 1.0f),
			glm::vec3(1.0f, -1.0f, 1.0f),
			glm::vec3(-1.0f, -1.0f, 1.0f),
		};

		// Project frustum corners into world space
		glm::mat4 invCam = glm::inverse(m_CameraProjMatrix * m_CameraViewMatrix);
		for (uint32_t j = 0; j < 8; j++)
		{
			glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[j], 1.0f);
			frustumCorners[j] = invCorner / invCorner.w;
		}

		for (uint32_t j = 0; j < 4; j++)
		{
			glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
			frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
			frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
		}

		// Get frustum center
		glm::vec3 frustumCenter = glm::vec3(0.0f);
		for (uint32_t j = 0; j < 8; j++) {
			frustumCenter += frustumCorners[j];
		}
		frustumCenter /= 8.0f;

		float radius = 0.0f;
		for (uint32_t j = 0; j < 8; j++) {
			float distance = glm::length(frustumCorners[j] - frustumCenter);
			radius = glm::max(radius, distance);
		}
		radius = std::ceil(radius * 16.0f) / 16.0f;

		glm::vec3 maxExtents = glm::vec3(radius);
		glm::vec3 minExtents = -maxExtents;

		glm::vec3 lightDir = normalize(-m_SunLightDir);
		glm::mat4 lightViewMatrix = glm::lookAtLH(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightOrthoMatrix = glm::orthoLH_ZO(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + CascadeNearPlaneOffset, maxExtents.z - minExtents.z + CascadeFarPlaneOffset);


		// Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
		glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
		float ShadowMapResolution = CASCADED_SHADOW_SIZE;

		glm::vec4 shadowOrigin = (shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution / 2.0f;
		glm::vec4 roundedOrigin = glm::round(shadowOrigin);
		glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
		roundOffset = roundOffset * 2.0f / ShadowMapResolution;
		roundOffset.z = 0.0f;
		roundOffset.w = 0.0f;

		lightOrthoMatrix[3] += roundOffset;


		// Store split distance and matrix in cascade
		cascades[i].splitDepth = (CAM_NEAR_PLANE + splitDist * clipRange);// *-1.0f;
		cascades[i].viewProjMatrix = lightOrthoMatrix * lightViewMatrix;

		lastSplitDist = cascadeSplits[i];
	}
}

template <typename T>
void CreateD3D12ConstantBuffer(UploadBuffer<T>* buffer, D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle, int offset)
{
	auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
	auto& device = dx12Api->GetDevice();

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = buffer->Resource()->GetGPUVirtualAddress();
	// Offset to the ith object constant buffer in the buffer.
	int CBufIndex = offset;
	cbAddress += CBufIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = objCBByteSize;

	dx12Api->g_pd3dSrvDescHeapAlloc.Alloc(cpuHandle, gpuHandle);
	device->CreateConstantBufferView(&cbvDesc, *cpuHandle);
}

FrameResource::FrameResource(ID3D12Device* device, UINT objectCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

	_MatricesCB = std::make_unique<UploadBuffer<MatricesCB>>(device, objectCount, true);
	_MaterialCB = std::make_unique<UploadBuffer<MaterialCB>>(device, objectCount, true);
	_SkeletalAnimationCB = std::make_unique<UploadBuffer<SkeletalAnimationCB>>(device, objectCount, true);
	_LightCB = std::make_unique<UploadBuffer<LightCB>>(device, 1, true);
	_PointLightShadowGenCB = std::make_unique<UploadBuffer<PointLightShadowGenCB>>(device, MAX_POINT_LIGHTS, true);

	for (int i = 0; i < objectCount; i++)
	{
		CreateD3D12ConstantBuffer<MatricesCB>(_MatricesCB.get(), &_MatricesCBcpuHandles[i], &_MatricesCBgpuHandles[i], i);
		CreateD3D12ConstantBuffer<MaterialCB>(_MaterialCB.get(), &_MaterialCBcpuHandles[i], &_MaterialCBgpuHandles[i], i);
		CreateD3D12ConstantBuffer<SkeletalAnimationCB>(_SkeletalAnimationCB.get(), &_SkeletalAnimationCBcpuHandles[i], &_SkeletalAnimationCBgpuHandles[i], i);
	}

	CreateD3D12ConstantBuffer<LightCB>(_LightCB.get(), &_LightCBcpuHandle, &_LightCBgpuHandle, 0);

	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
		CreateD3D12ConstantBuffer<PointLightShadowGenCB>(_PointLightShadowGenCB.get(), &_PointLightShadowGenCBcpuHandles[i], &_PointLightShadowGenCBgpuHandles[i], i);
}

FrameResource::~FrameResource()
{
}