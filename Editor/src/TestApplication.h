#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "Oeuvre/Core/Application.h"
#include "Oeuvre/Core/Log.h"
#include "Oeuvre/Events/EventHandler.h"
#include <iostream>

#include "Oeuvre/Renderer/Camera.h"
#include "Platform/DirectX11/RenderTexture.h"

#include "Oeuvre/Renderer/Model.h"
#include "Platform/DirectX11/DX11ComputeShader.h"
#include "Platform/DirectX11/DX11DomainShader.h"
#include "Platform/DirectX11/DX11GeometryShader.h"
#include "Platform/DirectX11/DX11HullShader.h"
#include "Platform/DirectX11/DX11PixelShader.h"
#include "Platform/DirectX11/DX11VertexShader.h"

#include "Platform/DirectX11/DX11Shader.h"

#include "Platform/DirectX11/DX11GBuffer.h"

#include "fmod.hpp"
#include "fmod_errors.h"

#include "GamePad.h"
#include "Oeuvre/Renderer/Animator.h"

#include <random>

#include "Panels/ContentBrowserPanel.h"
#include "Panels/SceneHierarchyPanel.h"

#include "Oeuvre/Terrain/Terrain.h"

#include "GFSDK_SSAO.h"

#include "Platform/DirectX12/DX12RendererAPI.h"
#include "Platform/DirectX12/GbufferD3D12.h"
#include "Platform/DirectX12/RenderTextureD3D12.h"

#include "ImGuizmo.h"

#include <vulkan/vulkan.h>

#define SPONZA_SCALE 0.01f

constexpr float CAM_NEAR_PLANE = 0.1f;
constexpr float CAM_FAR_PLANE = 1000.f;

#define CSM_FIRST 0

#if CSM_FIRST
#define SHADOW_MAP_CASCADE_COUNT 5
#else
#define SHADOW_MAP_CASCADE_COUNT 4
#endif

using namespace Oeuvre;

#include <Oeuvre/Renderer/IBL.h>
#include <Oeuvre/Physics/Physics.h>

#include "Oeuvre/Renderer/ConstantBuffers.h"
#include <Platform/OpenGL/RenderTextureOGL.h>
#include <Platform/OpenGL/OGLShader.h>

#include "Oeuvre/Utils/LightCulling.h"

#include "imgui_node_editor.h"

namespace ed = ax::NodeEditor;

struct FrameResource
{
	FrameResource(ID3D12Device* device, UINT objectCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource();

	static const int MaxConstantBuffers = 128;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	std::unique_ptr<UploadBuffer<MatricesCB>> _MatricesCB;
	std::unique_ptr<UploadBuffer<SkeletalAnimationCB>> _SkeletalAnimationCB;
	std::unique_ptr<UploadBuffer<MaterialCB>> _MaterialCB;
	std::unique_ptr<UploadBuffer<LightCB>> _LightCB;
	std::unique_ptr<UploadBuffer<PointLightShadowGenCB>> _PointLightShadowGenCB;

	D3D12_CPU_DESCRIPTOR_HANDLE _MatricesCBcpuHandles[MaxConstantBuffers]; // b0
	D3D12_GPU_DESCRIPTOR_HANDLE _MatricesCBgpuHandles[MaxConstantBuffers];

	D3D12_CPU_DESCRIPTOR_HANDLE _MaterialCBcpuHandles[MaxConstantBuffers]; // b1
	D3D12_GPU_DESCRIPTOR_HANDLE _MaterialCBgpuHandles[MaxConstantBuffers];

	D3D12_CPU_DESCRIPTOR_HANDLE _SkeletalAnimationCBcpuHandles[MaxConstantBuffers]; // b2
	D3D12_GPU_DESCRIPTOR_HANDLE _SkeletalAnimationCBgpuHandles[MaxConstantBuffers];

	D3D12_CPU_DESCRIPTOR_HANDLE _LightCBcpuHandle; // b3
	D3D12_GPU_DESCRIPTOR_HANDLE _LightCBgpuHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE _PointLightShadowGenCBcpuHandles[MAX_POINT_LIGHTS]; // b4
	D3D12_GPU_DESCRIPTOR_HANDLE _PointLightShadowGenCBgpuHandles[MAX_POINT_LIGHTS];

	// Fence value to mark commands up to this fence point.  This lets us
	// check if these frame resources are still in use by the GPU.
	UINT64 Fence = 0;
};

class TestApplication : public Application
{
public:
	TestApplication();
protected:
	void OnWindowEvent(const Event<WindowEvents>& e);
	void OnMouseEvent(const Event<MouseEvents>& e);
	void OnKeyEvent(const Event<KeyEvents>& e);

	virtual bool Init() override;
	virtual void MainLoop() override;
	virtual void Cleanup() override;
	void InitLightEntities(const Oeuvre::Entity& baseEntitiy);
	void InitMeshEntities(const Oeuvre::Entity& baseEntitiy);

	// ImGui
	void ImguiInit();
	void ImguiFrame();
	void ImguiRender();
	void ImguiCleanup();
	void SetupImGuiStyle();
	void SetupImguiDockspace(bool disableInputs);

	// D3D11
	void D3D11Init();
	void InitDX11Stuff();
	void D3D11Frame();
	void BloomPass();
	void GBufferPass();
	void DirectionalLightDepthPass();
	void PointLightDepthPass();
	void NvidiaHBAOPass(ID3D11ShaderResourceView* depthSRV);
	void ResetRenderState();
	template<typename T>
	void UpdateAndSetConstantBuffer(ID3D11Buffer* buffer, T object, int slot);

	ID3D11Device* m_DX11Device = nullptr;
	ID3D11DeviceContext* m_DX11DeviceContext = nullptr;

	ID3D11Texture2D* frameTexture = nullptr;
	ID3D11ShaderResourceView* frameSRV = nullptr;
	int vRegionMinX = 0, vRegionMinY = 0;
	int texWidth = 1024, texHeight = 1024;
	int texWidthPrev = 1024, texHeightPrev = 1024;
	RenderTexture* renderTexture = nullptr;
	void FramebufferToTextureInit();
	void FramebufferToTextureBegin();
	void FramebufferToTextureEnd();

	std::string sSelectedFile;
	std::string sFilePath;

	std::string modelPath;

	void SaveTextureToFile(ID3D11Texture2D* d3dTexture, const std::string fileName);	

	std::vector<Camera> m_Cameras;

	float lastX = 0.f, lastY = 0.f;
	bool m_PlayModeActive = false;
	bool m_PlayModeActivePrev = true;
	bool keyPressed = false;

	float m_DeltaTime = 0.f, m_LastFrame = 0.f;

	struct LightProps
	{
		float constant = 0.5f;
		float linear = 0.001f;
		float quadratic = 0.001f;
		bool dynamicShadows = true;
	};
	LightProps m_LightProps;
	int CurrentLightIndex = 0;

	int currentModelId = 0;

	ID3D11Buffer* g_pCBMatrixes = nullptr;
	ID3D11Buffer* g_pCBLight = nullptr;
	ID3D11Buffer* g_pCBPointLightShadowGen = nullptr;
	ID3D11Buffer* g_pCBMaterial = nullptr;
	ID3D11Buffer* g_pCBSkeletalAnimation = nullptr;

	ID3D11SamplerState* g_pSamplerLinearWrap = nullptr;
	ID3D11SamplerState* g_pSamplerPointWrap = nullptr;
	ID3D11SamplerState* g_pSamplerPointClamp = nullptr;

	std::shared_ptr<DX11VertexShader> m_DefaultVertexShader = nullptr;
	std::shared_ptr<DX11VertexShader> m_FullScreenQuadVertexShader = nullptr;

	std::shared_ptr<DX11VertexShader> m_NullVertexShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_NullPixelShader = nullptr;
	std::shared_ptr<DX11GeometryShader> m_NullGeometryShader = nullptr;

	// shadow mapping
	std::shared_ptr<DX11VertexShader> m_PointLightDepthVertexShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_PointLightDepthPixelShader = nullptr;
	std::shared_ptr<DX11GeometryShader> m_PointLightDepthGeometryShader = nullptr;

	RenderTexture* m_SpotLightDepthRenderTexture = nullptr;
	ID3D11Texture2D* m_SpotLightDepthTexture = nullptr;
	ID3D11ShaderResourceView* m_SpotLightDepthSRV = nullptr;

	RenderTexture* m_PointLightDepthRenderTextures[MAX_POINT_LIGHTS];
	ID3D11Texture2D* m_PointLightDepthTextures[MAX_POINT_LIGHTS];
	ID3D11ShaderResourceView* m_PointLightDepthSRVs[MAX_POINT_LIGHTS];
	ID3D11SamplerState* m_SamplerLinearClamp = nullptr;
	ID3D11SamplerState* m_SamplerLinearClampComparison = nullptr;

	float m_ShadowMapBias = 0.00010f;

	int SelectedCamera = 0;

	glm::mat4 m_PointLightProjMat = glm::perspectiveLH(glm::radians(90.f), 1.f, SHADOWGEN_NEAR, SHADOWGEN_FAR);

	// rendering cube
	ID3D11Buffer* pCubeVertexBuffer, * pCubeIndexBuffer;
	void RenderCubeInit();
	void RenderCube(const glm::mat4& modelMat);

	// rendering quad
	ID3D11Buffer* pQuadVertexBuffer, * pQuadIndexBuffer;
	void RenderQuadInit();
	void RenderQuad(const glm::mat4& modelMat, const glm::mat4& viewMat, const glm::mat4& projMat, bool albedoOnlyMode, uint32_t objectId);
	void RenderFullScreenQuad();

	void PrepareLightConstantBuffer();

	// lights
	void SpotLightDepthToTextureInit();
	void SpotLightDepthToTextureBegin();
	void SpotLightDepthToTextureEnd();


	// Point light
	glm::vec3 GammaToLinear(const glm::vec3 color);

	void PointLightDepthToTextureInit();
	void PointLightDepthToTextureBegin(int i);
	void PointLightDepthToTextureEnd(int i);

	glm::vec3 m_cubeVectors[18] =
	{
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f)
	};
	std::vector<glm::mat4> m_PointLightDepthCaptureViews;
	void PreparePointLightViewMatrixes(const glm::vec3& lightPos);

	// Directional light
	std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& projview);
	std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
	glm::mat4 TestApplication::GetLightSpaceMatrix(const float nearPlane, const float farPlane);
	std::vector<glm::mat4> GetLightSpaceMatrices();

	//std::vector<float> shadowCascadeLevels{ CAM_FAR_PLANE / 50.0f, CAM_FAR_PLANE / 25.0f, CAM_FAR_PLANE / 10.0f, CAM_FAR_PLANE / 2.0f };
	std::vector<float> shadowCascadeLevels{ CAM_FAR_PLANE / 50.0f, CAM_FAR_PLANE / 20.0f, CAM_FAR_PLANE / 7.5f, CAM_FAR_PLANE / 2.0f };

	glm::vec3 m_sunLightColor = glm::vec3(1.f);
	float m_SunLightPower = 1.f;

	void CascadedShadowmapsInit();
	void CascadedShadowmapsBegin();
	void CascadedShadowmapsEnd();

	ID3D11Texture2D* m_SunLightDepthTexture = nullptr;
	ID3D11ShaderResourceView* m_SunLightDepthSRV = nullptr;
	RenderTexture* m_SunLightDepthRenderTexture = nullptr;
	std::shared_ptr<DX11GeometryShader> m_SunLightGeometryShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_SunLightPixelShader = nullptr;

	void RenderScene(glm::mat4 viewMatrix, glm::mat4 projMatrix, std::shared_ptr<DX11VertexShader>& vertexShader, std::shared_ptr<DX11PixelShader>& pixelShader, std::shared_ptr<DX11GeometryShader>& geometryShader, bool frustumCulling, bool pointLightCulling, const PointLightCullingData& pointLightCullingData, const std::shared_ptr<OGLShader>& oglShader);

	ID3D11RasterizerState* m_DefaultRasterizerState = nullptr;
	float depthBias = 450.f;
	float slopeBias = 4.f;
	float depthBiasClamp = 1.f;

	std::unique_ptr<DX11GBuffer> m_GBuffer = nullptr;
	std::shared_ptr<DX11VertexShader> m_DeferredVertexShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_DeferredPixelShader = nullptr;
	std::shared_ptr<DX11VertexShader> m_DeferredCompositingVertexShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_DeferredCompositingPixelShader = nullptr;

	void PrepareGbufferRenderTargets(int width, int height);

	void RenderToGBufferBegin();
	void RenderToGBufferEnd();

	glm::mat4 reverseZ(const glm::mat4& perspeciveMat);
	ID3D11DepthStencilState* m_pGBufferReverseZDepthStencilState = nullptr;

	std::vector<glm::vec3> lightPositionsPrev;

	bool m_bEnableVsync = true;

	DrawInfo m_DrawInfo = {};
	int m_MeshesDrawn = 0;
	int m_TotalMeshes = 0;

	glm::vec3 shapePos = glm::vec3(0.f);
	glm::mat4 shapeMat = glm::mat4(1.f);

	bool spacePressed = false;

	std::shared_ptr<ITexture> m_BoxAlbedo;
	std::shared_ptr<ITexture> m_BoxNormal;
	std::shared_ptr<ITexture> m_BoxRoughness;
	//std::shared_ptr<ITexture> m_BoxMetallic;

	glm::mat4 coltMat = glm::mat4(1.f);

	float m_boxHalfExtent = 1.f;

	// fmod
	FMOD::System* m_FmodSystem = nullptr;

	FMOD::Sound* m_RevolverShot = nullptr;
	FMOD::Sound* m_RevolverShot1 = nullptr;
	FMOD::Sound* m_RevolverShot2 = nullptr;
	FMOD::Sound* m_RevolverShot3 = nullptr;

	FMOD::Sound* m_HorroAmbience = nullptr;
	FMOD::Sound* m_RE4SaveRoomTrapRemix = nullptr;

	FMOD::Channel* m_PlayingChannel = nullptr;

	void FMODInit();
	void FMODUpdate();
	void FMODCleanup();


	// deafult camera properties //////////////////
	glm::mat4 m_CameraViewMatrix = glm::mat4(1.f);
	glm::mat4 m_CameraProjMatrix = glm::mat4(1.f);
	glm::vec3 m_CameraPos = glm::vec3(1.f);
	glm::vec3 m_CameraFrontVector = glm::vec3(1.f);
	///////////////////////////////////////////////

	glm::vec3 m_CharacterCameraPos = glm::vec3();
	glm::vec3 m_CharacterCameraLookAt = glm::vec3();

	Animator* m_Animator = nullptr;
	Animation* m_RunAnimation = nullptr;
	Animation* m_IdleAnimation = nullptr;
	Animation* m_PistolIdleAnimation = nullptr;
	Animation* m_LeftTurnAnimation = nullptr;
	Animation* m_RightTurnAnimation = nullptr;
	Animation* m_ShootingAnimation = nullptr;
	Animation* m_JumpingAnimation = nullptr;

	Animator* m_ZombieAnimator = nullptr;
	Animation* m_ZombieIdleAnimation = nullptr;
	Animation* m_ZombieWalkAnimation = nullptr;
	Animation* m_ZombieAttackAnimation = nullptr;
	Animation* m_ZombieHitAnimation = nullptr;
	Animation* m_ZombieDeathAnimation = nullptr;

	static const unsigned m_NumOfDudes = 10;

	Animator* m_DudeAnimators[m_NumOfDudes] = { nullptr };

	float m_IdleRunBlendFactor = 0.f;
	float m_IdleJumpBlendFactor = 0.f;

	void MoveCharacter();
	float Xoffset = 0.f, Yoffset = 0.f;
	bool mouseMoving = false;

	bool rightMouseButtonPressing = false;
	bool leftMouseButtonPressing = false;
	bool leftMouseButtonPressed = false;
	float m_TimeElapsedAfterShot = 0.f;

	bool rightMouseButtonPressed = false;

	struct PlayerTransform
	{
		glm::quat rotation;
		glm::vec3 position;
	} transform;

	bool bWalking = true;
	bool bPrevWalking = true;
	bool bJumping = false;

	bool bOnTheGround = true;
	bool bIsMovingUp = false;

	bool bZombieOnTheGround = true;
	bool bIsZombieMovingUp = false;


	float m_FollowCharacterCameraZoom = 1.f;
	float m_PlayerHeight = 2.f;
	float m_PlayerMoveSpeed = 5.f;
	float m_CameraBoomLength = 5.f;
	float m_CameraXOffset = 0.75f;

	float m_CharacterCapsuleColliderRadius = 0.2f;
	float m_CharacterCapsuleColliderHalfHeight = 1.5f;

	float m_ZombieHeight = 1.75f;

	//std::unique_ptr<DirectX::GamePad> m_GamePad;

	// Physics
	void InitPhysics();
	void AddTerrainBody();
	void StepPhysics(float dt);
	void RenderPhysics();
	void CleanupPhysics();

	void HandleMouseInput();

	// Postprocessing 
	int m_ToneMapPreset = 0;
	float m_Exposure = 1.f;

	std::shared_ptr<DX11PixelShader> m_PostprocessingPixelShader = nullptr;

	RenderTexture* postprocessingRenderTexture = nullptr;
	ID3D11ShaderResourceView* postprocessingSRV = nullptr;
	ID3D11Texture2D* postprocessingTexture = nullptr;

	void PostprocessingToTextureInit();
	void PostprocessingToTextureBegin();
	void PostprocessingToTextureEnd(ID3D11Texture2D* texture);

	void PostprocessingPass();

	void SetDefaultRenderTargetAndViewport();

	bool m_FreeCameraViewModeActive = false;

	float aimPitch = 0.f;
	glm::quat aimPitchRotation = glm::quat();

	ID3D11Buffer* g_pCBPostprocessing = nullptr;

	bool m_bEnableFXAA = true;

	glm::mat4 sponzaModelMat;
	glm::mat4 playerModelMat;

	// Muzzle flash
	std::shared_ptr<DX11PixelShader> m_MuzzleFlashPixelShader = nullptr;
	std::shared_ptr<ITexture> m_MuzzleFlashSpriteSheet = nullptr;

	// random
	std::random_device rd{};
	std::mt19937 engine{ rd() };

	std::uniform_int_distribution<> dist{ 0, 3 };

	// shooting ray
	void DrawShootingRay();
	bool RaySphereIntersection(const glm::vec3& rayStart, const glm::vec3& rayDir, float rayLen, const glm::vec3& sphereCenter, float sphereRadius);
	void MoveZombie();

	bool b_ZombieWalking = false;
	bool b_ZombieAttacking = false;
	bool b_ZombieDying = false;
	bool b_ZombieGettingHit = false;

	enum class ZombieState
	{
		IDLE = 0,
		WALK,
		HIT,
		ATTACK,
		DEATH
	} m_ZombieCurrentState;

	float m_ZombieIdleWalkBlendFactor = 0.f;
	float m_ZombieIdleAttackBlendFactor = 0.f;
	float m_ZombieIdleDeathBlendFactor = 0.f;
	float m_ZombieIdleHitBlendFactor = 0.f;

	glm::quat m_ZombieRotation = glm::quat();

	float m_ZombieMoveSpeed = 2.5f;
	bool zombieDeathStarted = false;
	int m_ZombieHealth = 100;
	float m_ZombieDeathTime = 0.f;

	std::shared_ptr<DX11PixelShader> m_SolidColorPixelShader = nullptr;

	//std::shared_ptr<CPostFX> m_CPostFX = std::make_shared< CPostFX>();
	float g_fMiddleGrey = 0.863f;
	float g_fWhite = 1.53f;
	float g_fAdaptation = 1.f;

	bool s_bFirstTime = true;

	std::shared_ptr<Scene> m_Scene;
	std::shared_ptr<SceneHierarchyPanel> m_SceneHierarchyPanel;
	std::shared_ptr<ContentBrowserPanel> m_ContentBrowserPanel;

	// Terrain
	std::shared_ptr<Terrain> m_Terrain = nullptr;
	std::shared_ptr<ITexture> m_TerrainAlbedoTexture = nullptr;
	std::shared_ptr<ITexture> m_TerrainNormalTexture = nullptr;
	std::shared_ptr<ITexture> m_TerrainRoughnessTexture = nullptr;

	float m_TerrainUVScale = 0.025f;

	void DrawTerrain();

	ID3D11RasterizerState* m_WireframeRasterizerState = nullptr;

	Camera* m_PrimaryCamera = nullptr;
	Model* m_PlayerModel = nullptr;

	glm::vec3 ambientDown = glm::vec3(0.5f, 0.5f, 0.5f);
	glm::vec3 ambientUp = glm::vec3(1.f, 1.f, 1.f);

	// Gizmos
	void ImguiDrawGizmo();
	ImGuizmo::OPERATION m_GizmoType{ ImGuizmo::OPERATION::TRANSLATE };
	ImGuizmo::MODE m_GizmoMode{ ImGuizmo::LOCAL };

	// Billboarding
	std::shared_ptr<ITexture> m_PointLightIcon = nullptr;
	std::shared_ptr<DX11VertexShader> m_BillboardVertexShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_BillboardPixelShader = nullptr;
	glm::vec3 m_BillboardCenter = glm::vec3(0.f);


	std::uniform_real_distribution<float> randomFloats{ 0.0f, 1.0f };
	std::default_random_engine generator;

	void PrepareAnimations();

	void OpenScene();
	void SaveScene();
	void SaveSceneAs();
	std::string m_EditorScenePath = "";

	// NVIDIA HBAO Plus
	GFSDK_SSAO_CustomHeap CustomHeap;
	GFSDK_SSAO_Context_D3D11* pAOContext;

	void NvidiaHBAOPlusInit();
	void NvidiaHBAOPlusRender(ID3D11ShaderResourceView* pDepthStencilTextureSRV, const glm::mat4& pProjectionMatrix, float SceneScale, ID3D11RenderTargetView* pOutputColorRTV);
	void NvidiaHBAOPlusCleanup();

	bool m_bEnableHBAO = true;

	bool hemisphericAmbient = false;

	// Bloom
	std::shared_ptr<DX11PixelShader> m_BloomPrefilterPixelShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_DownsamplePixelShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_UpsamplePixelShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_BlurPixelShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_BloomUpsampleAndCombinePixelShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_BloomCombinePixelShader = nullptr;

	std::vector<RenderTexture*> m_BloomRTs;
	std::vector<ID3D11Texture2D*> m_BloomMips;
	std::vector<ID3D11ShaderResourceView*> m_BloomMipSRVs;

	void BloomInit();
	void BloomBegin(int i);
	void BloomEnd(int i, ID3D11Texture2D* texture);

	bool m_BloomEnabled = false;
	float g_fBloomScale = 0.74f;
	float g_fBloomThreshold = 1.f;
	float g_fBloomKnee = 0.5f;
	float g_fBloomClamp = 65472.f;

	int g_iBlurPasses = 1;

	// IBL
	std::shared_ptr<IBL> m_IBL;
	void ChangeHDRCubemap();
	void RenderHDRCubemap();

	// D3D12
	void D3D12Init();
	void D3D12BeforeBeginFrame();
	void D3D12Frame();
	void D3D12AfterEndFrame();
	// D3D12 Descriptor Heaps
	//ComPtr<ID3D12DescriptorHeap> _cbvSrvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> _samplersHeap = nullptr;

	//ExampleDescriptorHeapAllocator _cbvSrvHeapAllocator;
	ExampleDescriptorHeapAllocator _samplersHeapAllocator;

	void BuildD3D12DescriptorHeaps();

	// D3D12 Constant Buffers
	const int _NumFrameResources = 3;
	std::vector<std::unique_ptr<FrameResource>> _FrameResources;
	FrameResource* _CurrFrameResource = nullptr;
	int _CurrFrameResourceIndex = 0;
	void BuildD3D12FrameResources();

	// D3D12 Samplers
	D3D12_CPU_DESCRIPTOR_HANDLE _SamplerLinearWrapCpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE _SamplerLinearWrapGpuHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE _SamplerPointClampCpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE _SamplerPointClampGpuHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE _SamplerComparisonLinearClampCpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE _SamplerComparisonLinearClampGpuHandle;


	void BuildD3D12Samplers();

	// D3D12 Shaders
	ComPtr<ID3DBlob> _vsDeferredByteCode = nullptr;
	ComPtr<ID3DBlob> _psDeferredByteCode = nullptr;

	ComPtr<ID3DBlob> _vsCompositionByteCode = nullptr;
	ComPtr<ID3DBlob> _psCompositionByteCode = nullptr;

	ComPtr<ID3DBlob> _vsPointLightDepthByteCode = nullptr;
	ComPtr<ID3DBlob> _psPointLightDepthByteCode = nullptr;
	ComPtr<ID3DBlob> _gsPointLightDepthByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> _InputLayout;

	void BuildD3D12ShadersAndInputLayout();

	// D3D12 Root Signature
	ComPtr<ID3D12RootSignature> _RootSignature = nullptr;

	void BuildD3D12RootSignature();

	// D3D12 PSO
	ComPtr<ID3D12PipelineState> _PSOGfuffer = nullptr;
	ComPtr<ID3D12PipelineState> _PSOComposition = nullptr;
	ComPtr<ID3D12PipelineState> _PSOPointLightDepth = nullptr;
	void BuildD3D12PSOs();

	// D3D12 Gbuffer
	std::unique_ptr<GBufferD3D12> _Gbuffer = nullptr;

	void InitD3D12Gbuffer();

	// D3D12 RenderTexture
	std::unique_ptr<RenderTextureD3D12> _RenderTexture = nullptr;

	// D3D12 Pointlight Shadows
	std::unique_ptr<RenderTextureD3D12> _PointLightRenderTextures[MAX_POINT_LIGHTS];

	void PointLightDepthToTextureInitD3D12();
	void PointLightDepthToTextureBeginD3D12(int i, ID3D12GraphicsCommandList* commandList);
	void PointLightDepthToTextureEndD3D12(int i, ID3D12GraphicsCommandList* commandList);

	void PointLightDepthPass(ID3D12GraphicsCommandList* commandList);

	void PointLightDepthToTextureSetVieportAndScissorsD3D12(ID3D12GraphicsCommandList* commandList);

	// D3D12 Primitives
	std::unique_ptr<MeshGeometry> m_CubeGeometryD3D12 = nullptr;
	void RenderCubeInitD3D12();
	void RenderCubeD3D12(const glm::mat4& modelMat, uint32_t objectIndex);


	// Mouse picking
	uint32_t _SelectedObjectId = 0;
	ImVec2 _ViewportRelativeCursorPos;

	ComPtr<ID3D11Buffer> m_MousePickingBuffer = nullptr;
	ComPtr<ID3D11UnorderedAccessView> m_MousePickingUAV = nullptr;
	ComPtr<ID3D11ShaderResourceView> m_MousePickingSRV = nullptr;

	ComPtr<ID3D11Buffer> m_MousePickingStagingBuffer = nullptr;

	struct MousePickData
	{
		uint hoveringObjectId;
		glm::vec3 debug;
	};

	void MousePickingWithPixelShaderInit();
	void MousePickingWithPixelShader();

	const unsigned INVALID_ENTITY_ID = 0xFFFFFFFF;

	uint32_t m_MouseHoveringObjectId = INVALID_ENTITY_ID;

	ComPtr<ID3D11RasterizerState> m_CullFrontRasterizerState = nullptr;

	// Vulkan
	VkDescriptorSet _imguiDS;
	VkSampler _imguiSampler;

	AllocatedBuffer _saBuffers[128];
	VkDescriptorSet _saAndMatBufferSets[128];

	AllocatedBuffer _matBuffers[128];

	void VulkanInit();
	void VulkanFrame();
	void VulkanBetween();
	void VulkanBeforeImguiFrame();

	bool m_FreezeRendering = false;

	bool m_EditorDisabled = false;

	// Compute shaders test DirectX11
	void ComputeTestInit();
	void ComputeTestRun();

	std::shared_ptr<DX11ComputeShader> _computeShader = nullptr;
	std::shared_ptr<DX11PixelShader> _pixelShader = nullptr;
	ID3D11Texture2D* _computeTex = nullptr;
	ID3D11UnorderedAccessView* _computeUAV = nullptr;
	ID3D11ShaderResourceView* _computeSRV = nullptr;

	// sphere
	void RenderSphere(int segments, const glm::mat4& modelMat, const glm::vec3& color, float metallic, float roughness, bool albedoOnly);
	float _sphereRoughness = 0.1f;
	float _sphereMetallness = 1.f;
	glm::vec3 _sphereColor = glm::vec3(0.5f, 0.5f, 0.5f);

	// Physics
	Physics m_Physics = Physics();

	// Editor state
	enum class EditorState
	{
		NAVIGATION = 0,
		CAMERA_VIEW,
		PLAYING
	} m_EditorState = EditorState::NAVIGATION;

	bool m_IsViewportWindowFocused = false;

	inline bool IsCursorInsideViewport() {
		return m_IsViewportWindowFocused && _ViewportRelativeCursorPos.x > 0.f && _ViewportRelativeCursorPos.y > 0.f
			&& _ViewportRelativeCursorPos.x < (float)texWidth && _ViewportRelativeCursorPos.y < (float)texHeight;
	}

	// Rendering grid
	const int m_GridLines = 100.f;

	ID3D11Buffer* pGridVertexBuffer, * pGridIndexBuffer;
	void RenderGridInit();
	void RenderGrid();

	bool m_ShowGrid = true;

	float m_CameraFovDegrees = 60.f;

	bool m_bFrustumCulling = true;
	float m_CubemapLod = 1.f;

	bool m_SecondWay = true;

	bool m_bShowDebug = false;

	glm::vec3 m_SunLightDir = glm::normalize(glm::vec3(1.f));

	float m_LightWorldSize = 3.75f;

	void DrawSelectedModelOutline();
	ComPtr<ID3D11DepthStencilState> m_OutlineStencilWrite = nullptr;
	ComPtr<ID3D11DepthStencilState> m_OutlineStencilMask = nullptr;
	ComPtr<ID3D11DepthStencilState> m_OutlineStencilNormal = nullptr;

	float m_JumpPower = 0.f;

	// OpenGL
	std::unique_ptr<RenderTextureOGL> m_RenderTextureOGL = nullptr;

	std::shared_ptr<OGLShader> pbrShader = nullptr;
	std::shared_ptr<OGLShader> equirectangularToCubemapShader = nullptr;
	std::shared_ptr<OGLShader> irradianceShader = nullptr;
	std::shared_ptr<OGLShader> prefilterShader = nullptr;
	std::shared_ptr<OGLShader> brdfShader = nullptr;
	std::shared_ptr<OGLShader> backgroundShader = nullptr;

	unsigned int cubeVAO = 0;
	unsigned int cubeVBO = 0;
	void RenderCubeOGL();

	unsigned int quadVAO = 0;
	unsigned int quadVBO = 0;
	void RenderQuadOGL();

	unsigned int sphereVAO = 0;
	GLsizei sphereIndexCount = 0;
	void RenderSphereOGL();

	unsigned int envCubemap;
	unsigned int irradianceMap;
	unsigned int prefilterMap;
	unsigned int brdfLUTTexture;

	// rusted iron
	std::shared_ptr<ITexture> ironAlbedoMap;
	std::shared_ptr<ITexture> ironNormalMap;
	std::shared_ptr<ITexture> ironMetallicMap;
	std::shared_ptr<ITexture> ironRoughnessMap;
	std::shared_ptr<ITexture> ironAOMap;

	// gold
	std::shared_ptr<ITexture> goldAlbedoMap;
	std::shared_ptr<ITexture> goldNormalMap;
	std::shared_ptr<ITexture> goldMetallicMap;
	std::shared_ptr<ITexture> goldRoughnessMap;
	std::shared_ptr<ITexture> goldAOMap;

	// grass
	std::shared_ptr<ITexture> grassAlbedoMap;
	std::shared_ptr<ITexture> grassNormalMap;
	std::shared_ptr<ITexture> grassMetallicMap;
	std::shared_ptr<ITexture> grassRoughnessMap;
	std::shared_ptr<ITexture> grassAOMap;

	// plastic
	std::shared_ptr<ITexture> plasticAlbedoMap;
	std::shared_ptr<ITexture> plasticNormalMap;
	std::shared_ptr<ITexture> plasticMetallicMap;
	std::shared_ptr<ITexture> plasticRoughnessMap;
	std::shared_ptr<ITexture> plasticAOMap;

	// wall
	std::shared_ptr<ITexture> wallAlbedoMap;
	std::shared_ptr<ITexture> wallNormalMap;
	std::shared_ptr<ITexture> wallMetallicMap;
	std::shared_ptr<ITexture> wallRoughnessMap;
	std::shared_ptr<ITexture> wallAOMap;

	// lights
	glm::vec3 lightPositionsOGL[4] = {
		glm::vec3(-10.0f,  10.0f, 10.0f),
		glm::vec3(10.0f,  10.0f, 10.0f),
		glm::vec3(-10.0f, -10.0f, 10.0f),
		glm::vec3(10.0f, -10.0f, 10.0f),
	};
	glm::vec3 lightColors[4] = {
		glm::vec3(300.0f, 300.0f, 300.0f),
		glm::vec3(300.0f, 300.0f, 300.0f),
		glm::vec3(300.0f, 300.0f, 300.0f),
		glm::vec3(300.0f, 300.0f, 300.0f)
	};

	// Shadow mapping
	std::shared_ptr<OGLShader> simpleDepthShader = nullptr;

	const unsigned int depthMapResolution = 4096;
	unsigned lightFBO, lightDepthMaps, matricesUBO;
	void OpenGLSetupCSM();
	void OpenGLCSMPass();


	void OpenGLSetupIBL(const char* hdriMapPath);
	void OpenGLChangeIBL(const char* hdriMapPath);

	void OpenGLInit();
	void OpenGLFrame();

	float m_SkyboxIntensity = 1.f;

	bool m_ShowCasades = false;

	// Profiling
	float m_D3D11DirLightDepthPassTime = 0.f;
	float m_D3D11PointLightDepthPassTime = 0.f;
	float m_D3D11GbufferPassTime = 0.f;
	float m_D3D11FrameCompoistingPassTime = 0.f;

	float m_D3D11FullFrameTime = 0.f;

	float m_DudesAnimationsUpdateTime = 0.f;
	float m_MousePickWithRaycastTime = 0.f;

	float m_ImguiFrameTime = 0.f;
	float m_ImguiRenderTime = 0.f;

	bool m_Fullscreen = false;

	// Infinite Grid
	std::shared_ptr<DX11VertexShader> m_InfiniteGridVertexShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_InfiniteGridPixelShader = nullptr;

	void RenderInfiniteGrid();
	ComPtr<ID3D11BlendState> m_InfiniteGridBlendState = nullptr;

	// FPS
	float m_FPSUpdateDelay = 0.f;
	float m_FPS = 0.f;

	void RenderModelBounds(const std::string& tag);

	bool Raycast(const glm::vec3& start, const glm::vec3& dir, glm::vec3& outHit, glm::vec3& outNormal);

	struct RaycastResult
	{
		float tNear;
		glm::vec3 hitPos;
		glm::vec3 hitNormal;
	};

	std::vector<RaycastResult> m_BulletHits;
	void AddBulletHit();

	std::shared_ptr<ITexture> m_BulletHoleImage = nullptr;

	bool m_ShowBounds = false;

	// CSM new approach
	struct Cascade {
		float splitDepth;
		glm::mat4 viewProjMatrix;
	};

	std::array<Cascade, SHADOW_MAP_CASCADE_COUNT> cascades;

	void UpdateCascades();
	float CascadeSplitLambda = 0.95f;

	float CascadeFarPlaneOffset = 50.0f, CascadeNearPlaneOffset = -50.0f;
	float posViewSpaceZ = 0.f;

	float BiasModifier = 0.05f;

	void RenderCircle(const glm::mat4& modelMat, bool albedoOnly);

	bool RayIntersectsMeshTriangles(const std::string& modelName, const glm::vec3& orig, const glm::vec3& dir, glm::vec3& hitPoint, float& distance);

	void DrawRaycast();

	struct Ray
	{
		glm::vec3 start, end;
	};

	std::vector<Ray> rays;

	// Node Editor
	struct ed::EditorContext* m_EdContext = nullptr;

	// Render outline
	std::unique_ptr<RenderTexture> m_OutlineRenderTexture = nullptr;
	void OutlinePrepassInit();
	void OutlinePrepassBegin();
	void OutlinePrepassEnd();
};

