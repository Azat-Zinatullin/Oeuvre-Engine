#pragma pack_matrix( row_major )

cbuffer ObjectBuffer : register(b0)
{
    matrix modelMat;
    matrix viewMat;
    matrix projMat;
    matrix normalMat;
    matrix projMatInv;
};

struct VertexInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    int4 boneIds : BONEIDS;
    float4 boneWeights : BONEWEIGHTS;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float3 localPos : TEXCOORD0;
};

struct Output
{
    float4 albedoMetallic : SV_Target0;
    float4 normalRoughness : SV_Target1;
    float4 albedoOnly : SV_Target2;
};

#include "../Include/MaterialConstantBuffer.hlsli"

SamplerState textureSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};
TextureCube<float4> environmentMap : register(t0);

PixelInput VSMain(VertexInput input)
{
    PixelInput output;
    
    output.localPos = normalize(input.position.xyz);

    float4x4 newView = viewMat;
    newView[3][0] = 0.0;
    newView[3][1] = 0.0;
    newView[3][2] = 0.0;

    output.position.w = 1.f;
    
    output.position = mul(float4(input.position, 1.f), modelMat);
    output.position = mul(output.position, newView);
    output.position = mul(output.position, projMat);
    
    output.position = output.position.xyzw;
    output.position.z = output.position.w * 0.9999;

    return output;
}

Output PSMain(PixelInput input)
{
    //float3 envColor = environmentMap.Sample(textureSampler, input.localPos).rgb;
    float3 envColor = environmentMap.SampleLevel(textureSampler, input.localPos, cubemapLod).rgb;

    envColor = envColor / (envColor + float3(1, 1, 1));
    envColor = pow(envColor, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

    Output output;
    output.albedoMetallic = float4(envColor, 0.f);
    output.normalRoughness = float4(0.f, 0.f, 0.f, 1.f);
    output.albedoOnly = float4(0.5f, 0.f, 0.f, 0.f);
    
    return output;
}