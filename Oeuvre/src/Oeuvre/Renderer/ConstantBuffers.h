#pragma once
#include <glm/glm.hpp>

constexpr float SHADOWGEN_NEAR = 0.1f;
constexpr float SHADOWGEN_FAR = 500.f;
constexpr int SHADOWMAP_WIDTH = 1024;
constexpr int SHADOWMAP_HEIGHT = 1024;

#define MAX_POINT_LIGHTS 16

constexpr float CASCADED_SHADOW_SIZE = 4096.f;

struct __declspec(align(16)) MatricesCB
{
	glm::mat4 modelMat;
	glm::mat4 viewMat;
	glm::mat4 projMat;
	glm::mat4 normalMat;
	glm::mat4 viewMatInv;
	glm::mat4 projMatInv;
};

struct __declspec(align(16)) MaterialCB
{
	glm::vec3 color;
	int sponza;
	int objectId;
	int notTextured;
	int albedoOnly;
	float roughness;
	float metallic;
	int terrain;
	float terrainUVScale;
	int objectOutlinePass;
	float cubemapLod;
	int useNormalMap;
	int invertNormalG;
	float emission;
	int texturesBindlessIndexStart;
	glm::vec3 padding;
};

struct __declspec(align(16)) SkeletalAnimationCB
{
	glm::mat4 finalBonesMatrices[100];
};

struct __declspec(align(16)) LightCB
{
	glm::mat4 lightSpaceMatrices[16];
	glm::mat4 lightViewProjMat;
	glm::vec4 lightPos[MAX_POINT_LIGHTS];
	glm::vec4 lightColor[MAX_POINT_LIGHTS];
	glm::vec4 rangeRcpAndIntensity[MAX_POINT_LIGHTS];
	glm::vec4 rcpFrame;
	glm::vec4 billboardCenterWS;
	glm::vec4 sunLightDir;
	glm::vec4 cascadePlaneDistances[16];
	glm::vec4 ambientDown;
	glm::vec4 ambientRange;
	glm::vec4 sunLightColor;
	glm::vec4 camPos;
	int hemisphericAmbient;
	float bias;
	float skyboxIntensity;
	int showDiffuseTexture;
	int numLights;
	int showCascades;
	float sunLightPower;
	int cascadeCount;
	float farPlane;
	glm::vec3 padding;
};

struct __declspec(align(16)) PointLightShadowGenCB
{
	glm::mat4 cubeProj;
	glm::mat4 cubeView[6];
};

struct __declspec(align(16)) PostprocessingCB
{
	glm::vec4 kernelOffsets[16];
	int enableFXAA;
	int enableHBAO;
	int horizontalBlur;
	float sampleScale;
	glm::vec4 params;
	glm::vec4 threshold;
	glm::vec2 textureSize;
	int tonemapPreset;
	float exposure;
};
