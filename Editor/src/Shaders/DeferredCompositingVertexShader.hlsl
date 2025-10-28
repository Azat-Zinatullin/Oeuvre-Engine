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
};

#include "Include/MatricesConstantBuffer.hlsli"

Output VSMain(Input input)
{
    Output output;   
    output.position = mul(float4(input.position, 1.f), modelMat);
    output.position = mul(output.position, viewMat);
    output.position = mul(output.position, projMat);
    
    output.uv = input.uv;
    
    return output;
}
