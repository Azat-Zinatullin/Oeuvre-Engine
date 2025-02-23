#pragma pack_matrix( row_major )

struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 fragPos : TEXCOORD1;
};

#include "LightConstantBuffer.hlsli"

float4 PSMain(Input input) : SV_Target
{
    return float4(lightColor, 1.f);
}