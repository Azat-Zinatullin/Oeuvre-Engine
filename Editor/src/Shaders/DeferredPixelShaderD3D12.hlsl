#pragma pack_matrix( row_major )

struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 fragPos : TEXCOORD1;
};

struct Output
{
    float4 albedo : SV_Target0;
    float4 normalRoughness : SV_Target1;
    float4 positionMetallic : SV_Target2;
    float4 albedoOnlyMode : SV_Target3;
};

#include "Include/Common.hlsli"

#include "Include/MaterialConstantBuffer.hlsli"

#include "Include/LightConstantBuffer.hlsli"

SamplerState textureSamplerWrap : register(s0);
SamplerState textureSamplerPoint : register(s1);
SamplerState textureSamplerComparison : register(s2);

Texture2D albedoMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D roughnessMap : register(t2);
Texture2D metallicMap : register(t3);

float3 getNormalFromMap(float3 worldPos, float2 uv, float3 normal)
{
    float3 normalFromMap = normalMap.Sample(textureSamplerWrap, uv).rgb;
    
    float3 tangentNormal = normalFromMap * 2.0 - 1.0;

    float3 Q1 = ddx(worldPos);
    float3 Q2 = ddy(worldPos);
    float2 st1 = ddx(uv);
    float2 st2 = ddy(uv);

    float3 N = normalize(normal);
    float3 T = normalize(Q1 * st2.y - Q2 * st1.y);
    float3 B = -normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);

    return normalize(mul(tangentNormal, TBN));
}

Output PSMain(Input input)
{
    Output output;

    if (terrain)
    {
        input.uv = input.fragPos.xz * terrainUVScale;
    }
    
    float4 diffuseColor = albedoMap.Sample(textureSamplerWrap, input.uv) * float4(SRGBToLinear(materialColor), 1.f);
    //Texture2D alb = ResourceDescriptorHeap[texturesBindlessIndexStart];
    //float4 diffuseColor = alb.Sample(textureSamplerWrap, input.uv) * float4(SRGBToLinear(materialColor), 1.f);
    float3 normal = normalize(input.normal);
    float roughness = roughnessMap.Sample(textureSamplerWrap, input.uv).r * materialRoughness;
    float metallic = metallicMap.Sample(textureSamplerWrap, input.uv).r * materialMetallic;
  
    if (useNormalMap)
    {
        normal = getNormalFromMap(input.fragPos, input.uv, input.normal);
    }
    if (sponza)
    {
        roughness = roughnessMap.Sample(textureSamplerWrap, input.uv).g * materialRoughness;
        metallic = metallicMap.Sample(textureSamplerWrap, input.uv).b * materialMetallic;
    }
    
    if (notTextured)
    {
        diffuseColor = float4(SRGBToLinear(materialColor), 1.f);
        normal = normalize(input.normal);
        roughness = materialRoughness;
        metallic = materialMetallic;
    }
    if (terrain)
    {
        metallic = 0.f;
        roughness = 0.3f;
        //normal = input.normal;
    }
        
    if (diffuseColor.a < 0.1)
        discard;
    output.albedo = diffuseColor;
    output.normalRoughness = float4(normal.xyz, roughness);
    output.positionMetallic = float4(input.fragPos, metallic);
    
    if (albedoOnly)
        output.albedoOnlyMode = float4(0.5f, emission, 0.f, 0.f);
    else
        output.albedoOnlyMode = float4(0.f, emission, 0.f, 0.f);
    
    return output;
}