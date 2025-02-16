#pragma pack_matrix( row_major )

struct Input
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    int4 boneIds : BONEIDS;
    float4 boneWeights : BONEWEIGHTS;
};

struct Output
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 fragPos : TEXCOORD1;
};

cbuffer ObjectBuffer : register(b0)
{
    matrix modelMat;
    matrix viewMat;
    matrix projMat;
    matrix normalMat;
    matrix viewProjMatInv;
}

#include "LightConstantBuffer.hlsli"

#include "SkeletalAnimationConstantBuffer.hlsli"

bool VectorEqual(float4 a, float4 b)
{
    float4 diff = a - b;
    return !any(diff);
}

Output VSMain(Input input)
{
    Output output;
    
    float4 totalPosition = float4(0.f, 0.f, 0.f, 0.f);
    float3 totalNormal = float3(0.f, 0.f, 0.f);
    
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        if (input.boneIds[i] == -1) 
            continue;
        if (input.boneIds[i] >= MAX_BONES)
        {
            totalPosition = float4(input.position, 1.0f);
            break;
        }
        float4 localPosition = mul(float4(input.position, 1.0f), finalBonesMatrices[input.boneIds[i]]);
        totalPosition += localPosition * input.boneWeights[i];
        float3 localNormal = mul(input.normal, finalBonesMatrices[input.boneIds[i]]);
        totalNormal += localNormal * input.boneWeights[i];
    }
    
    if (VectorEqual(totalPosition, float4(0.f, 0.f, 0.f, 0.f)))
        totalPosition = float4(input.position, 1.0f);
    if (VectorEqual(float4(totalNormal, 0.f), float4(0.f, 0.f, 0.f, 0.f)))
        totalNormal = input.normal;
    
    output.position = mul(totalPosition, modelMat);
    output.fragPos = output.position.xyz;
    output.position = mul(output.position, viewMat);
    output.position = mul(output.position, projMat);
    
    output.normal = normalize(mul(float4(totalNormal, 1.f), normalMat));
    output.uv = input.uv;
    
    return output;
}
