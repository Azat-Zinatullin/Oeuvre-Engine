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
    float4 albedoMetallic : SV_Target0;
    float4 normalRoughness : SV_Target1;
    float4 albedoOnlyMode : SV_Target2;
};

#include "Include/MaterialConstantBuffer.hlsli"

Texture2D billboardTexture : register(t0);
SamplerState textureSampler : register(s0);

Output PSMain(Input input) : SV_TARGET
{
    float4 tex = billboardTexture.Sample(textureSampler, input.uv);

    if (tex.a < 0.1f)
        discard;
    
    Output output;
    output.albedoMetallic = float4(tex.rgb, 0.0f);
    output.normalRoughness = float4(0.f, 0.f, 0.f, 1.f);
    output.albedoOnlyMode = float4(0.5f, 0.f, (float)objectId, 0.f);
    
    return output;
}