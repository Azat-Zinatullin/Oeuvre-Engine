#ifndef PBR_RESOURCES_HLSLI
#define PBR_RESOURCES_HLSLI

#include "Include/Common.hlsli"

Texture2D<float4> gBufferAlbedoMetallic : register(t0);
Texture2D<float4> gBufferNormalRoughness : register(t1);
Texture2D<float4> gBufferAlbedoOnlyMode : register(t2);
Texture2D<float> gBufferDepth : register(t3);
Texture2D<float4> gBufferSelectedObjectShape : register(t4);

Texture2D t_hbao : register(t5);

TextureCube<float4> irradianceMap : register(t6);
TextureCube<float4> preFilterMap : register(t7);
Texture2D<float2> brdfLUT : register(t8);

Texture2DArray<float> sunLightShadowMap : register(t9);

TextureCube<float> depthMaps[MAX_POINT_LIGHTS] : register(t20);

SamplerState linearTextureSampler : register(s0);
SamplerState pointTextureSampler : register(s1);
SamplerComparisonState comparisonSampler : register(s2);

#endif // PBR_RESOURCES_HLSLI