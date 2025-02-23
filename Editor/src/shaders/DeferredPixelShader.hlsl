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
    float4 lightViewPosition : SV_Target3;
};

#include "LightConstantBuffer.hlsli"

cbuffer MaterialBuffer : register(b2)
{
    float sponza;
    float normalStrength;
    int renderLightCube;
}

SamplerState textureSampler : register(s0);

Texture2D albedoMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D roughnessMap : register(t2);
Texture2D metallicMap : register(t3);

Texture2D spoltightDepthMap : register(t4);

float3 getNormalFromMap(float3 worldPos, float2 uv, float3 normal)
{
    float3 tangentNormal = normalMap.Sample(textureSampler, uv).rgb * 2.0 - 1.0;

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

    //if (renderLightCube)
    //    input.uv = input.uv / 2.f;
    
    float4 diffuseColor = albedoMap.Sample(textureSampler, input.uv);
    float3 normal = getNormalFromMap(input.fragPos, input.uv, input.normal);   
    float roughness = roughnessMap.Sample(textureSampler, input.uv).r;
    float metallic = metallicMap.Sample(textureSampler, input.uv).r;
    if (sponza)
    {
        roughness = roughnessMap.Sample(textureSampler, input.uv).g;
        metallic = metallicMap.Sample(textureSampler, input.uv).b;
    }
    
    if (renderLightCube)
    {
        //float4 color = float4(0.5f, 0.5f, 0.5f, 0.f);
        //if (input.uv.r <= 0.05f || input.uv.r >= 0.95f || input.uv.g <= 0.05f || input.uv.g >= 0.95f)
        //    color = float4(0.f, 0.f, 0.f, 1.f);
        float4 color = float4(1.f, 1.f, 1.f, 1.f);
        diffuseColor = color;
        normal = float3(0.f, 0.f, 0.f);
        roughness = 1.f;
        metallic = 0.f;
    }
        
    if (diffuseColor.a < 0.1)
        discard;
    output.albedo = diffuseColor;
    output.normalRoughness = float4(normal.xyz, roughness);   
    output.positionMetallic = float4(input.fragPos, metallic);
    
    output.lightViewPosition = mul(float4(input.fragPos, 1.f), lightViewProjMat);
    
    return output;
}