#pragma pack_matrix( row_major )

struct Input
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
};

struct Output
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 fragPos : POSITION;
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

Output VSMain(Input input)
{
    Output output;
    output.position = mul(float4(input.position, 1.f), modelMat);
    output.fragPos = output.position.xyz;
    output.position = mul(output.position, viewMat);
    output.position = mul(output.position, projMat);
    
    output.normal = normalize(mul(float4(input.normal, 0.f), normalMat));
    output.uv = input.uv;
    
    return output;
}
