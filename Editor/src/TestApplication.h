#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "Oeuvre/Core/Application.h"
#include "Oeuvre/Events/EventHandler.h"
#include <iostream>
#include "Oeuvre/Core/Log.h"

#include "Oeuvre/Renderer/RenderTexture.h"
#include "Oeuvre/Renderer/Camera.h"

#include "Oeuvre/Renderer/Model.h"
#include "Platform/DirectX11/DX11VertexShader.h"
#include "Platform/DirectX11/DX11PixelShader.h"
#include "Platform/DirectX11/DX11GeometryShader.h"

#include "Platform/DirectX11/DX11Shader.h"

#include "Platform/DirectX11/DX11GBuffer.h"

#include "Platform/Nvidia/GFSDK_VXGI.h"
#include "Platform/Nvidia/GFSDK_NVRHI_D3D11.h"
#include "Platform/Nvidia/BindingHelpers.h"

#include <unordered_set>

#include "PxPhysicsAPI.h"
#include "foundation/PxPreprocessor.h"

#include "fmod.hpp"
#include "fmod_errors.h"

#include "Oeuvre/Renderer/Animator.h"
#include "GamePad.h"

#define PVD_HOST "127.0.0.1"

using namespace physx;

#define SPONZA_SCALE 0.016f

const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.1f;
const int SHADOWMAP_WIDTH = 1024;
const int SHADOWMAP_HEIGHT = 1024;

#define MAX_POINT_LIGHTS 16
constexpr int NUM_POINT_LIGHTS = 3;

using namespace Oeuvre;

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p) = nullptr; } } 
#define SAFE_DELETE(p) { if(p) { delete (p); (p) = nullptr; } } 

class GIErrorCallback : public NVRHI::IErrorCallback
{
	void signalError(const char* file, int line, const char* errorDesc) override;
};

class TestApplication : public Application
{
public:
	TestApplication();
	static std::unique_ptr<TestApplication> Create();

protected:
	void OnWindowEvent(const Event<WindowEvents>& e);
	void OnMouseEvent(const Event<MouseEvents>& e);
	void OnKeyEvent(const Event<KeyEvents>& e);

	virtual bool Init();
	virtual void RenderLoop();
	void ResetRenderState();
	virtual void Cleanup();


	void ImguiInit();
	void ImguiFrame();
	void ImguiRender();
	void ImguiCleanup();

	void SetupImGuiStyle();

	void SetupImguiDockspace();

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

	bool openFile(std::string& pathRef);
	std::string sSelectedFile;
	std::string sFilePath;

	std::string modelPath;

	void SaveTextureToFile(ID3D11Texture2D* d3dTexture, const std::string fileName);

	std::vector<Camera> m_Cameras;

	float lastX = 0.f, lastY = 0.f;
	bool viewPortActive = false;
	bool keyPressed = false;

	float m_DeltaTime = 0.f, m_LastFrame = 0.f;

	struct Properties
	{
		glm::vec3 translation{};
		glm::vec3 rotation{};
		glm::vec3 scale = glm::vec3(1.f);
		std::string albedoPath;
		std::string normalPath;
		std::string roughnessPath;
		std::string metallicPath;
	};
	std::vector<Properties> props;

	struct LightProps
	{
		glm::vec3 lightPos[NUM_POINT_LIGHTS];
		float constant = 1.f;
		float linear = 0.09f;
		float quadratic = 0.032f;
		float color[3] = { 1.f, 1.f, 1.f };
	};
	LightProps lightProps;
	int CurrentLightIndex = 0;

	int currentModelId = 0;

	std::vector<Model*> m_Models;
	ID3D11Buffer* g_pCBMatrixes = nullptr;
	ID3D11Buffer* g_pCBLight = nullptr;
	ID3D11Buffer* g_pCBMaterial = nullptr;
	ID3D11Buffer* g_pCBSkeletalAnimation = nullptr;

	ID3D11SamplerState* g_pSamplerLinear = nullptr;
	std::shared_ptr<DX11VertexShader> m_DefaultVertexShader = nullptr;

	std::shared_ptr<DX11VertexShader> m_NullVertexShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_NullPixelShader = nullptr;
	std::shared_ptr<DX11GeometryShader> m_NullGeometryShader = nullptr;

	// shadow mapping
	std::shared_ptr<DX11VertexShader> m_SpotLightDepthVertexShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_SpotLightDepthPixelShader = nullptr;

	std::shared_ptr<DX11VertexShader> m_SpotLightVertexShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_SpotLightPixelShader = nullptr;

	std::shared_ptr<DX11VertexShader> m_PointLightDepthVertexShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_PointLightDepthPixelShader = nullptr;
	std::shared_ptr<DX11GeometryShader> m_PointLightDepthGeometryShader = nullptr;

	RenderTexture* m_SpotLightDepthRenderTexture = nullptr;
	ID3D11Texture2D* m_SpotLightDepthTexture = nullptr;
	ID3D11ShaderResourceView* m_SpotLightDepthSRV = nullptr;

	RenderTexture* m_PointLightDepthRenderTextures[NUM_POINT_LIGHTS];
	ID3D11Texture2D* m_PointLightDepthTextures[NUM_POINT_LIGHTS];
	ID3D11ShaderResourceView* m_PointLightDepthSRVs[NUM_POINT_LIGHTS];
	ID3D11SamplerState* g_pSamplerClamp = nullptr;
	ID3D11SamplerState* g_pSamplerComparison = nullptr;

	float m_ShadowMapBias = 0.00010f;

	int SelectedCamera = 0;

	glm::mat4 lightProjMat = glm::perspectiveLH(XM_PIDIV2, 1.f, SCREEN_NEAR, SCREEN_DEPTH);
	glm::mat4 spotlightProjMat = glm::perspectiveLH(XM_PIDIV2, 1.f, SCREEN_NEAR, SCREEN_DEPTH);	

	// rendering cube
	ID3D11Buffer* pCubeVertexBuffer, *pCubeIndexBuffer;
	void RenderCubeInit();
	void RenderCube(const glm::mat4& modelMat);

	// rendering quad
	ID3D11Buffer* pQuadVertexBuffer, *pQuadIndexBuffer;
	void RenderQuadInit();
	void RenderQuad();

	// lights
	void SpotLightDepthToTextureInit();
	void SpotLightDepthToTextureBegin();
	void SpotLightDepthToTextureEnd();

	void PointLightDepthToTextureInit();
	void PointLightDepthToTextureBegin(int i);
	void PointLightDepthToTextureEnd(int i);
	std::vector<glm::mat4> m_PointLightDepthCaptureViews;

	std::shared_ptr<DX11PixelShader> m_CubePixelShader;

	void RenderScene(glm::mat4 viewMatrix, glm::mat4 projMatrix, std::shared_ptr<DX11VertexShader>& vertexShader, std::shared_ptr<DX11PixelShader>& pixelShader, std::shared_ptr<DX11GeometryShader>& geometryShader, bool voxelizing, const VXGI::Box3f* clippingBoxes, uint32_t numBoxes, bool frustumCulling);

	void PreparePointLightViewMatrixes(glm::vec3 lightPos);

	ID3D11RasterizerState* m_DepthRasterizerState = nullptr;
	float depthBias = 40.f;
	float slopeBias = 4.f;
	float depthBiasClamp = 1.f;
	void ChangeDepthBiasParameters(float depthBias, float slopeBias, float clamp);

	float normalStrength = 1.f;

	// Nvidia VXGI
	void NvidiaVXGIInit();
	void NvidiaVXGIVoxelization();
	void NvidiaVXGIConeTracing();

	GIErrorCallback giErrorCallback;
	NVRHI::RendererInterfaceD3D11* rendererInterface = nullptr;
	VXGI::IGlobalIllumination* giObject = nullptr;
	VXGI::IShaderCompiler* shaderCompiler = nullptr;
	VXGI::IUserDefinedShaderSet* gsShaderSet = nullptr;
	VXGI::IUserDefinedShaderSet* psShaderSet = nullptr;
	VXGI::IBasicViewTracer* basicViewTracer = nullptr;

	NVRHI::DrawCallState* drawCallState = new NVRHI::DrawCallState();

	DX11GBuffer* gBuffer = nullptr;
	std::shared_ptr<DX11VertexShader> m_DeferredVertexShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_DeferredPixelShader = nullptr;
	std::shared_ptr<DX11VertexShader> m_DeferredCompositingVertexShader = nullptr;
	std::shared_ptr<DX11PixelShader> m_DeferredCompositingPixelShader = nullptr;

	std::shared_ptr<DX11PixelShader> m_VoxelizationPixelShader = nullptr;

	glm::mat4 voxelizationViewMatrix = glm::mat4(1.f);
	glm::mat4 voxelizationProjMatrix = glm::mat4(1.f);

	NVRHI::TextureHandle indirectDiffuse = nullptr;
	NVRHI::TextureHandle indirectConfidence = nullptr;
	NVRHI::TextureHandle indirectSpecular = nullptr;
	ID3D11ShaderResourceView* indirectSRV = nullptr;
	ID3D11ShaderResourceView* confidenceSRV = nullptr;
	ID3D11ShaderResourceView* indirectSpecularSRV = nullptr;

	float g_fVoxelSize = 8.f/(1.f/SPONZA_SCALE);
	bool g_bEnableMultiBounce = true;
	float g_fMultiBounceScale = 1.0f;
	int g_nMapSize = 128;

	float g_fDiffuseScale = 1.0f;
	float g_fSpecularScale = 0.0f;
	float g_fSamplingRate = 1.0f;
	float g_fQuality = 0.1f;
	bool g_bTemporalFiltering = true;

	void PrepareGbufferRenderTargets(int width, int height);

	NVRHI::TextureHandle       m_TargetAlbedo = nullptr;
	NVRHI::TextureHandle       m_TargetNormalRoughness = nullptr;
	NVRHI::TextureHandle       m_TargetNormalRoughnessPrev = nullptr;
	NVRHI::TextureHandle       m_TargetPositionMetallic = nullptr;
	NVRHI::TextureHandle       m_TargetDepth = nullptr;
	NVRHI::TextureHandle       m_TargetDepthPrev = nullptr;
	NVRHI::TextureHandle       m_TargetLightViewPosition = nullptr;

	NVRHI::ConstantBufferHandle m_pGlobalCBufferMatrixes = nullptr;
	NVRHI::ConstantBufferHandle m_pGlobalCBufferLight = nullptr;
	NVRHI::InputLayoutHandle m_pInputLayout = nullptr;
	NVRHI::ShaderHandle m_pDefaultVS = nullptr;
	NVRHI::SamplerHandle m_pDefaultSamplerState = nullptr;
	NVRHI::SamplerHandle m_pComparisonSamplerState = nullptr;
	NVRHI::TextureHandle m_ShadowMap = nullptr;
	NVRHI::TextureHandle m_DiffuseTexture = nullptr;

	NVRHI::BufferHandle m_VertexBuffer = nullptr;
	NVRHI::BufferHandle m_IndexBuffer = nullptr;

	NVRHI::DrawCallState* voxelizationState = new NVRHI::DrawCallState();

	bool m_bEnableGI = true;
	bool m_bEnableIndirectSpecular = true;
	bool m_bShowDiffuseTexture = false;
	bool m_bVXGIRenderDebug = false;

	float m_fVxaoRange = 512.0f;
	float m_fVxaoScale = 0.01f;

	bool g_InputBuffersPrevValid = false;
	VXGI::IBasicViewTracer::InputBuffers g_InputBuffersPrev;

	void RenderToGBufferBegin();
	void RenderToGBufferEnd();
	NVRHI::DrawCallState* gbufferState = new NVRHI::DrawCallState();

	glm::mat4 reverseZ(const glm::mat4& perspeciveMat);
	ID3D11DepthStencilState* m_pGBufferReverseZDepthStencilState = nullptr;

	const float NEAR_PLANE = 0.1f;
	const float FAR_PLANE = 10000.f;

	bool g_bEnableSSAO = false;
	float g_fSsaoRadius = 50.f;
	NVRHI::TextureHandle ssao = nullptr;
	ID3D11ShaderResourceView* ssaoSRV = nullptr;

	std::vector<glm::vec3> lightPositionsPrev;

	// for cone tracing
	NVRHI::TextureHandle m_GBufferDepth = nullptr;
	NVRHI::TextureHandle m_GBufferNormal = nullptr;

	bool m_bEnableVsync = true;

	int m_MeshesDrawn = 0;

	// NVIDIA PhysX
	PxDefaultAllocator		gAllocator;
	PxDefaultErrorCallback	gErrorCallback;
	PxFoundation* gFoundation = NULL;
	PxPhysics* gPhysics = NULL;
	PxDefaultCpuDispatcher* gDispatcher = NULL;
	PxScene* gScene = NULL;
	PxMaterial* gMaterial = NULL;
	PxPvd* gPvd = NULL;

	PxReal stackZ = 10.0f;

	void initPhysics(bool interactive);
	void stepPhysics(float elapsedTime);
	void cleanupPhysics();
	void renderPhysics(float deltaTime);
	PxRigidDynamic* createDynamic(const PxTransform& t, const PxGeometry& geometry, const PxVec3& velocity = PxVec3(0));
	void createStack(const PxTransform& t, PxU32 size, PxReal halfExtent);

	glm::vec3 shapePos = glm::vec3(0.f);
	glm::mat4 shapeMat = glm::mat4(1.f);

	bool spacePressed = false;

	std::shared_ptr<Texture> m_BoxAlbedo;
	std::shared_ptr<Texture> m_BoxNormal;
	std::shared_ptr<Texture> m_BoxRoughness;
	//std::shared_ptr<Texture> m_BoxMetallic;

	PxRigidDynamic* m_coltPhysicsActor = nullptr;
	glm::mat4 coltMat = glm::mat4(1.f);

	void RenderModelBounds(int modelIndex); 

	float m_boxHalfExtent = 0.25f;

	// fmod
	FMOD::System* m_FmodSystem = nullptr;
	FMOD::Sound* m_WhooshSound = nullptr;
	FMOD::Sound* m_BoxDrop1 = nullptr;
	FMOD::Sound* m_BoxDrop2 = nullptr;
	FMOD::Sound* m_BoxDrop3 = nullptr;
	FMOD::Sound* m_BoxDrop4 = nullptr;
	void FMODInit();
	void FMODUpdate();
	void FMODCleanup();

	class PhysXEventCallback : PxSimulationEventCallback
	{
	public:
		void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override;
		void onWake(PxActor** actors, PxU32 count) override;
		void onSleep(PxActor** actors, PxU32 count) override;
		void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;
		void onTrigger(PxTriggerPair* pairs, PxU32 count) override;
		void onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override;
		PhysXEventCallback(TestApplication* app) : m_App(app) {}
	private:
		TestApplication* m_App;
	};

	class PhysXFilterCallback : PxSimulationFilterCallback
	{
		// Унаследовано через PxSimulationFilterCallback
		PxFilterFlags pairFound(PxU64 pairID, PxFilterObjectAttributes attributes0, PxFilterData filterData0, const PxActor* a0, const PxShape* s0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, const PxActor* a1, const PxShape* s1, PxPairFlags& pairFlags) override;
		void pairLost(PxU64 pairID, PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, bool objectRemoved) override;
		bool statusChange(PxU64& pairID, PxPairFlags& pairFlags, PxFilterFlags& filterFlags) override;
	};

	PhysXEventCallback* m_physXEventCallback = nullptr;
	PhysXFilterCallback* m_physXFilterCallback = nullptr;


	// deafult camera properties //////////////////
	glm::mat4 m_CameraViewMatrix = glm::mat4(1.f);
	glm::vec3 m_CameraPos = glm::vec3(1.f);
	glm::vec3 m_CameraFrontVector = glm::vec3(1.f);
	///////////////////////////////////////////////


	Model* m_AnimatedModel = nullptr;
	Animator* m_Animator = nullptr;
	Animation* m_RunAnimation = nullptr;
	Animation* m_IdleAnimation = nullptr;

	float m_IdleRunBlendFactor = 0.f;

	glm::quat RotateTowards(const glm::quat& from, const glm::quat& to, float maxAngleDegrees);
	void MoveCharacter(glm::mat4& viewMatrix, glm::vec3& cameraPos, glm::vec3 & cameraFrontVector);
	float Xoffset = 0.f, Yoffset = 0.f;
	bool mouseMoving = false;

	struct PlayerTransform
	{
		glm::quat rotation;
		glm::vec3 position;
	} transform;

	bool bWalking = true;
	bool bPrevWalking = true;

	float m_FollowCharacterCameraZoom = 1.f;
	PxRigidDynamic* m_CharacterCapsuleCollider = nullptr;

	std::unique_ptr<DirectX::GamePad> m_GamePad;
};

